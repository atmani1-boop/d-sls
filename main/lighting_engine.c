/**
 * @file lighting_engine.c
 * @brief Lighting engine implementation
 */

#include "lighting_engine.h"
#include "logger.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <string.h>

static const char *TAG = "LIGHTING";

// PWM configuration
#define PWM_FREQ_HZ 1000
#define PWM_RESOLUTION LEDC_TIMER_13_BIT  // 13-bit = 8192 levels
#define PWM_MAX_DUTY ((1 << 13) - 1)      // 8191

// GPIO pins for first 16 channels (will be extended for all 224 channels in real hardware)
#define PWM_GPIO_START 8
#define PWM_NUM_CHANNELS 16  // For now, only configure first 16 channels

// Global state
static fixture_rgbw_t g_fixtures[NUM_FIXTURES] = {0};
static uint8_t g_global_intensity = 255;
static uint8_t g_emergency_dim_percent = 100;
static bool g_lighting_running = false;

// Gamma correction table (2.2 gamma, 256 → 8192 levels)
static uint16_t g_gamma_table[256];

/**
 * @brief Build gamma correction table
 */
static void build_gamma_table(void)
{
    for (int i = 0; i < 256; i++) {
        float normalized = i / 255.0f;
        float corrected = powf(normalized, 2.2f);
        g_gamma_table[i] = (uint16_t)(corrected * PWM_MAX_DUTY);
    }
    
    ESP_LOGI(TAG, "Gamma table built: 0→%d, 128→%d, 255→%d",
             g_gamma_table[0], g_gamma_table[128], g_gamma_table[255]);
}

/**
 * @brief Initialize PWM channels
 */
static esp_err_t init_pwm(void)
{
    // Configure timer
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    
    esp_err_t err = ledc_timer_config(&timer_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %s", esp_err_to_name(err));
        return err;
    }
    
    // Configure channels (first 16 only for now)
    for (int i = 0; i < PWM_NUM_CHANNELS; i++) {
        ledc_channel_config_t ch_conf = {
            .gpio_num = PWM_GPIO_START + i,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = i,
            .timer_sel = LEDC_TIMER_0,
            .duty = 0,
            .hpoint = 0,
        };
        
        err = ledc_channel_config(&ch_conf);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to configure channel %d on GPIO %d: %s",
                     i, PWM_GPIO_START + i, esp_err_to_name(err));
        }
    }
    
    ESP_LOGI(TAG, "PWM initialized: %d channels, %d Hz, 13-bit", PWM_NUM_CHANNELS, PWM_FREQ_HZ);
    return ESP_OK;
}

/**
 * @brief Update PWM outputs from fixture data
 */
static void update_pwm_outputs(void)
{
    // Apply global intensity and emergency dimming
    float intensity_factor = (g_global_intensity / 255.0f) * (g_emergency_dim_percent / 100.0f);
    
    // Update first 4 fixtures (16 channels)
    for (int fix = 0; fix < 4 && fix < NUM_FIXTURES; fix++) {
        // Apply intensity
        uint8_t r = (uint8_t)(g_fixtures[fix].red * intensity_factor);
        uint8_t g = (uint8_t)(g_fixtures[fix].green * intensity_factor);
        uint8_t b = (uint8_t)(g_fixtures[fix].blue * intensity_factor);
        uint8_t w = (uint8_t)(g_fixtures[fix].white * intensity_factor);
        
        // Apply gamma correction
        uint16_t r_pwm = g_gamma_table[r];
        uint16_t g_pwm = g_gamma_table[g];
        uint16_t b_pwm = g_gamma_table[b];
        uint16_t w_pwm = g_gamma_table[w];
        
        // Set PWM duty cycles
        int ch_base = fix * 4;
        if (ch_base + 3 < PWM_NUM_CHANNELS) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, ch_base + 0, r_pwm);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, ch_base + 1, g_pwm);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, ch_base + 2, b_pwm);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, ch_base + 3, w_pwm);
            
            ledc_update_duty(LEDC_LOW_SPEED_MODE, ch_base + 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, ch_base + 1);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, ch_base + 2);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, ch_base + 3);
        }
    }
}

/**
 * @brief Lighting engine task
 */
static void task_lighting(void *pvParameters)
{
    ESP_LOGI(TAG, "Lighting engine task started");
    
    const TickType_t update_period = pdMS_TO_TICKS(10);  // 100Hz update rate
    
    while (g_lighting_running) {
        update_pwm_outputs();
        vTaskDelay(update_period);
    }
    
    ESP_LOGI(TAG, "Lighting engine task stopped");
    vTaskDelete(NULL);
}

esp_err_t lighting_engine_init(void)
{
    ESP_LOGI(TAG, "Initializing lighting engine");
    
    // Build gamma table
    build_gamma_table();
    
    // Initialize PWM
    esp_err_t err = init_pwm();
    if (err != ESP_OK) {
        return err;
    }
    
    // Clear fixture data
    memset(g_fixtures, 0, sizeof(g_fixtures));
    
    ESP_LOGI(TAG, "Lighting engine initialized: %d fixtures (%d channels)",
             NUM_FIXTURES, TOTAL_DMX_CHANNELS);
    
    return ESP_OK;
}

esp_err_t lighting_engine_start(void)
{
    if (g_lighting_running) {
        ESP_LOGW(TAG, "Lighting engine already running");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Starting lighting engine");
    g_lighting_running = true;
    
    // Create lighting task
    BaseType_t ret = xTaskCreate(
        task_lighting,
        "lighting",
        3072,
        NULL,
        7,  // Priority 7
        NULL
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create lighting task");
        g_lighting_running = false;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Lighting engine started");
    return ESP_OK;
}

esp_err_t lighting_engine_stop(void)
{
    ESP_LOGI(TAG, "Stopping lighting engine");
    g_lighting_running = false;
    return ESP_OK;
}

esp_err_t lighting_engine_update_from_dmx(const uint8_t *dmx_data, uint16_t start_address)
{
    if (!dmx_data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Validate start address
    if (start_address < 1 || start_address > 512) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Convert to 0-based index
    int dmx_offset = start_address - 1;
    
    // Update fixtures from DMX data
    for (int i = 0; i < NUM_FIXTURES; i++) {
        int ch = dmx_offset + (i * CHANNELS_PER_FIXTURE);
        
        if (ch + 3 < 512) {
            g_fixtures[i].red = dmx_data[ch + 0];
            g_fixtures[i].green = dmx_data[ch + 1];
            g_fixtures[i].blue = dmx_data[ch + 2];
            g_fixtures[i].white = dmx_data[ch + 3];
        }
    }
    
    return ESP_OK;
}

esp_err_t lighting_engine_set_fixture(uint8_t fixture_id, const fixture_rgbw_t *rgbw)
{
    if (fixture_id >= NUM_FIXTURES || !rgbw) {
        return ESP_ERR_INVALID_ARG;
    }
    
    g_fixtures[fixture_id] = *rgbw;
    return ESP_OK;
}

esp_err_t lighting_engine_set_all_fixtures(const fixture_rgbw_t *fixtures)
{
    if (!fixtures) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(g_fixtures, fixtures, sizeof(g_fixtures));
    return ESP_OK;
}

esp_err_t lighting_engine_set_global_intensity(uint8_t intensity)
{
    g_global_intensity = intensity;
    ESP_LOGI(TAG, "Global intensity set to %d", intensity);
    return ESP_OK;
}

uint8_t lighting_engine_get_global_intensity(void)
{
    return g_global_intensity;
}

esp_err_t lighting_engine_set_emergency_dim(uint8_t percent)
{
    if (percent > 100) {
        return ESP_ERR_INVALID_ARG;
    }
    
    g_emergency_dim_percent = percent;
    
    if (percent < 100) {
        ESP_LOGW(TAG, "Emergency dimming activated: %d%%", percent);
    } else {
        ESP_LOGI(TAG, "Emergency dimming deactivated");
    }
    
    return ESP_OK;
}

esp_err_t lighting_engine_blackout(void)
{
    ESP_LOGI(TAG, "Blackout");
    memset(g_fixtures, 0, sizeof(g_fixtures));
    return ESP_OK;
}

uint16_t lighting_engine_gamma_correct(uint8_t value)
{
    return g_gamma_table[value];
}
