/**
 * @file task_led.c
 * @brief DIAMANT v2.1 REV E - LED Task with Night-Saver & Dynamic CCT
 * @version 2.1.0
 * @date 2026-02-17
 * 
 * FreeRTOS task for LED control with adaptive brightness and color temperature
 */

#include "include/profiles.h"
#include <stdint.h>
#include <stdbool.h>

// External function declarations
extern uint8_t night_saver_apply(uint8_t base, float soc, bool enabled);
extern void cct_calculate_mix(uint16_t cct, uint8_t intensity, uint8_t *warm, uint8_t *cool);
extern void* fusion_get_data(void);

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

static const char *TAG = "TASK_LED";

// LEDC configuration
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_WARM        18  // GPIO18 - Warm LED
#define LEDC_OUTPUT_COOL        19  // GPIO19 - Cool LED
#define LEDC_CHANNEL_WARM       LEDC_CHANNEL_0
#define LEDC_CHANNEL_COOL       LEDC_CHANNEL_1
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT  // 13-bit resolution (0-8191)
#define LEDC_FREQUENCY          (5000)  // 5 kHz PWM frequency

// Maximum duty cycle value for 13-bit resolution
#define LEDC_MAX_DUTY           (8191)

/**
 * @brief Initialize LEDC for LED control
 * @return ESP_OK on success
 */
static esp_err_t led_init_pwm(void) {
    // Prepare and set configuration of timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %d", err);
        return err;
    }
    
    // Prepare and set configuration of warm channel
    ledc_channel_config_t ledc_channel_warm = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_WARM,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_WARM,
        .duty           = 0,
        .hpoint         = 0
    };
    err = ledc_channel_config(&ledc_channel_warm);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure warm LED channel: %d", err);
        return err;
    }
    
    // Prepare and set configuration of cool channel
    ledc_channel_config_t ledc_channel_cool = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_COOL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_COOL,
        .duty           = 0,
        .hpoint         = 0
    };
    err = ledc_channel_config(&ledc_channel_cool);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure cool LED channel: %d", err);
        return err;
    }
    
    ESP_LOGI(TAG, "LEDC PWM initialized: Warm=GPIO%d, Cool=GPIO%d, Freq=%dHz",
             LEDC_OUTPUT_WARM, LEDC_OUTPUT_COOL, LEDC_FREQUENCY);
    
    return ESP_OK;
}

/**
 * @brief Set LED PWM outputs
 * @param warm_percent Warm LED percentage (0-100)
 * @param cool_percent Cool LED percentage (0-100)
 */
static void led_set_output(uint8_t warm_percent, uint8_t cool_percent) {
    // Convert percentage to duty cycle (0-8191)
    uint32_t warm_duty = (warm_percent * LEDC_MAX_DUTY) / 100;
    uint32_t cool_duty = (cool_percent * LEDC_MAX_DUTY) / 100;
    
    // Set duty cycles
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_WARM, warm_duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_WARM);
    
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_COOL, cool_duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_COOL);
}

/**
 * @brief LED control task
 * @param pvParameters Task parameters (unused)
 */
void task_led(void *pvParameters) {
    ESP_LOGI(TAG, "Task LED started with Night-Saver + Dynamic CCT");
    
    // Initialize PWM
    if (led_init_pwm() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LED PWM, task exiting");
        vTaskDelete(NULL);
        return;
    }
    
    // Forward declaration of fusion data type
    typedef struct {
        struct {
            float soc_percent;
        } bms;
        led_profile_t active_led_profile;
    } fusion_data_t;
    
    while (1) {
        fusion_data_t *fusion = (fusion_data_t*)fusion_get_data();
        
        if (fusion != NULL) {
            led_profile_t profile = fusion->active_led_profile;
            
            // Ensure profile is within valid range
            if (profile >= LED_PROFILE_COUNT) {
                ESP_LOGW(TAG, "Invalid profile index: %d, using CLEAR_NIGHT", profile);
                profile = LED_CLEAR_NIGHT;
            }
            
            const led_config_t *cfg = &led_configs[profile];
            
            // Apply Night-Saver algorithm
            uint8_t adjusted_intensity = night_saver_apply(
                cfg->intensity,
                fusion->bms.soc_percent,
                cfg->night_saver_enabled
            );
            
            // Calculate CCT mix
            uint8_t warm, cool;
            cct_calculate_mix(cfg->cct_kelvin, adjusted_intensity, &warm, &cool);
            
            // Set PWM outputs
            led_set_output(warm, cool);
            
            ESP_LOGI(TAG, "Profile 0x%02X: %s | Base=%d%% Adjusted=%d%% CCT=%dK (W=%d%% C=%d%%)",
                     cfg->id_hex, cfg->name, cfg->intensity, adjusted_intensity, 
                     cfg->cct_kelvin, warm, cool);
        } else {
            ESP_LOGW(TAG, "Fusion data unavailable, LEDs off");
            led_set_output(0, 0);
        }
        
        // Update every 100ms
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

#else
// Stub implementation for non-ESP platforms
#include <stdio.h>
#include <unistd.h>

void task_led(void *pvParameters) {
    printf("[TASK_LED] LED task started (stub implementation)\n");
    
    // Forward declaration of fusion data type
    typedef struct {
        struct {
            float soc_percent;
        } bms;
        led_profile_t active_led_profile;
    } fusion_data_t;
    
    while (1) {
        fusion_data_t *fusion = (fusion_data_t*)fusion_get_data();
        
        if (fusion != NULL) {
            led_profile_t profile = fusion->active_led_profile;
            
            if (profile < LED_PROFILE_COUNT) {
                const led_config_t *cfg = &led_configs[profile];
                
                uint8_t adjusted_intensity = night_saver_apply(
                    cfg->intensity,
                    fusion->bms.soc_percent,
                    cfg->night_saver_enabled
                );
                
                uint8_t warm, cool;
                cct_calculate_mix(cfg->cct_kelvin, adjusted_intensity, &warm, &cool);
                
                printf("[TASK_LED] Profile 0x%02X: %s | Base=%d%% Adjusted=%d%% CCT=%dK (W=%d%% C=%d%%)\n",
                       cfg->id_hex, cfg->name, cfg->intensity, adjusted_intensity, 
                       cfg->cct_kelvin, warm, cool);
            }
        }
        
        sleep(1);  // Update every second (for testing)
    }
}

#endif
