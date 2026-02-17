/**
 * @file main.c
 * @brief DIAMANT v2.1 REV E - Main Application
 * @version 2.1.0
 * @date 2026-02-17
 * 
 * Entry point for the DIAMANT fusion system
 */

#include "include/profiles.h"
#include <stdio.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MAIN";

// External declarations
extern void task_led(void *pvParameters);
extern void* web_server_start(void);
extern void fusion_update_profiles(void *fusion);
extern void* fusion_get_data(void);

void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  DIAMANT v2.1 REV E - FUSION SYSTEM");
    ESP_LOGI(TAG, "  35-Profile Specification");
    ESP_LOGI(TAG, "  Night-Saver + Dynamic CCT");
    ESP_LOGI(TAG, "========================================");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Print profile counts
    ESP_LOGI(TAG, "Loaded profiles:");
    ESP_LOGI(TAG, "  - LED: %d profiles (0x00-0x09)", LED_PROFILE_COUNT);
    ESP_LOGI(TAG, "  - MPPT: %d profiles (0x10-0x16)", MPPT_PROFILE_COUNT);
    ESP_LOGI(TAG, "  - V2G: %d profiles (0x20-0x26)", V2G_PROFILE_COUNT);
    ESP_LOGI(TAG, "  - Battery: %d profiles (0x30-0x33)", BATTERY_PROFILE_COUNT);
    ESP_LOGI(TAG, "  - Weather: %d profiles (0x40-0x43)", WEATHER_PROFILE_COUNT);
    ESP_LOGI(TAG, "  - Astro: %d profiles (0x50-0x52)", ASTRO_PROFILE_COUNT);
    ESP_LOGI(TAG, "  - Calendar: %d profiles (0x60-0x62)", CALENDAR_PROFILE_COUNT);
    ESP_LOGI(TAG, "Total: 35 profiles");
    
    // Initialize fusion data with test values
    typedef struct {
        struct {
            float soc_percent;
            float voltage;
            float current;
            float temp_min;
            float temp_max;
        } bms;
        struct {
            int state;
            float lux;
            float pv_voltage;
            float pv_current;
            bool pv_drop_60pct_5min;
            float pv_fluctuation;
        } weather;
        struct {
            int season;
            bool is_weekend;
            bool is_holiday;
            uint8_t hour;
            uint8_t minute;
        } calendar;
        struct {
            float temp_board;
            float temp_heatsink;
            float temp_battery;
            bool overheat_flag;
        } thermal;
        int time_period;
        led_profile_t active_led_profile;
        mppt_profile_t active_mppt_profile;
        v2g_profile_t active_v2g_profile;
    } fusion_data_t;
    
    fusion_data_t *fusion = (fusion_data_t*)fusion_get_data();
    if (fusion) {
        // Set test data
        fusion->bms.soc_percent = 65.0f;
        fusion->bms.voltage = 48.2f;
        fusion->bms.current = 5.5f;
        fusion->bms.temp_min = 22.0f;
        fusion->bms.temp_max = 25.0f;
        
        fusion->weather.lux = 150.0f;
        fusion->weather.pv_voltage = 72.0f;
        fusion->weather.pv_current = 8.2f;
        fusion->weather.pv_drop_60pct_5min = false;
        fusion->weather.pv_fluctuation = 0.05f;
        
        fusion->calendar.hour = 20;
        fusion->calendar.minute = 30;
        
        fusion->thermal.temp_board = 35.0f;
        fusion->thermal.temp_heatsink = 42.0f;
        fusion->thermal.temp_battery = 24.0f;
        fusion->thermal.overheat_flag = false;
        
        // Update profiles based on initial conditions
        fusion_update_profiles(fusion);
    }
    
    // Start LED task
    xTaskCreate(task_led, "task_led", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "LED task started");
    
    // Note: Web server and WiFi initialization would go here
    // For now, just a placeholder
    ESP_LOGI(TAG, "System initialized successfully");
    ESP_LOGI(TAG, "Web server would start on http://diamant.local");
}

#else
// Non-ESP platform (for testing)
#include <unistd.h>

int main(void) {
    printf("========================================\n");
    printf("  DIAMANT v2.1 REV E - FUSION SYSTEM\n");
    printf("  35-Profile Specification\n");
    printf("  Night-Saver + Dynamic CCT\n");
    printf("========================================\n");
    
    printf("Loaded profiles:\n");
    printf("  - LED: %zu profiles (0x00-0x09)\n", LED_PROFILE_COUNT);
    printf("  - MPPT: %zu profiles (0x10-0x16)\n", MPPT_PROFILE_COUNT);
    printf("  - V2G: %zu profiles (0x20-0x26)\n", V2G_PROFILE_COUNT);
    printf("  - Battery: %zu profiles (0x30-0x33)\n", BATTERY_PROFILE_COUNT);
    printf("  - Weather: %zu profiles (0x40-0x43)\n", WEATHER_PROFILE_COUNT);
    printf("  - Astro: %zu profiles (0x50-0x52)\n", ASTRO_PROFILE_COUNT);
    printf("  - Calendar: %zu profiles (0x60-0x62)\n", CALENDAR_PROFILE_COUNT);
    printf("Total: 35 profiles\n\n");
    
    // Test Night-Saver
    printf("Testing Night-Saver algorithm:\n");
    extern uint8_t night_saver_apply(uint8_t base, float soc, bool enabled);
    
    uint8_t result;
    result = night_saver_apply(100, 65.0f, true);
    printf("  SOC 65%%, base 100%% → %d%% (expected 100%%)\n", result);
    
    result = night_saver_apply(100, 45.0f, true);
    printf("  SOC 45%%, base 100%% → %d%% (expected 80%%)\n", result);
    
    result = night_saver_apply(100, 25.0f, true);
    printf("  SOC 25%%, base 100%% → %d%% (expected 60%%)\n", result);
    
    result = night_saver_apply(100, 15.0f, true);
    printf("  SOC 15%%, base 100%% → %d%% (expected 30%%)\n", result);
    
    result = night_saver_apply(100, 15.0f, false);
    printf("  SOC 15%%, base 100%%, disabled → %d%% (expected 100%%)\n\n", result);
    
    // Test CCT control
    printf("Testing CCT control:\n");
    extern void cct_calculate_mix(uint16_t cct, uint8_t intensity, uint8_t *warm, uint8_t *cool);
    
    uint8_t warm, cool;
    
    cct_calculate_mix(2700, 100, &warm, &cool);
    printf("  2700K, 100%% → Warm: %d%%, Cool: %d%%\n", warm, cool);
    
    cct_calculate_mix(3850, 100, &warm, &cool);
    printf("  3850K, 100%% → Warm: %d%%, Cool: %d%%\n", warm, cool);
    
    cct_calculate_mix(5000, 100, &warm, &cool);
    printf("  5000K, 100%% → Warm: %d%%, Cool: %d%%\n", warm, cool);
    
    printf("\nTest complete!\n");
    
    return 0;
}

#endif
