/**
 * @file web_server_example.c
 * @brief Example integration of web server in main application
 * 
 * This example demonstrates how to integrate the web server module
 * into the DIAMANT v2.1 ESP32-C6 application.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "include/web_server.h"
#include "include/profiles.h"
#include "include/mram_storage.h"

static const char *TAG = "EXAMPLE";

// Global fusion data - must be defined in your main application
fusion_data_t g_fusion_data = {0};

// Example: Update fusion data periodically
static void update_fusion_data_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting fusion data update task");
    
    while (1) {
        // Get current timestamp
        time(&g_fusion_data.timestamp);
        
        // Update weather data (from weather.c)
        bool is_daytime = (g_fusion_data.astro.current_period == PERIOD_DAY);
        weather_update(&g_fusion_data.weather, is_daytime);
        
        // Update astronomical data (from astro.c)
        astro_calculate(&g_fusion_data.astro, g_fusion_data.timestamp);
        astro_update_period(&g_fusion_data.astro, g_fusion_data.timestamp);
        
        // Update calendar data (from calendar.c)
        calendar_calculate(&g_fusion_data.calendar, g_fusion_data.timestamp);
        
        // Update BMS data (from your BMS driver)
        // Example values - replace with actual BMS readings
        g_fusion_data.bms.soc_percent = 75.5;
        g_fusion_data.bms.voltage = 12.6;
        g_fusion_data.bms.current = 2.3;
        g_fusion_data.bms.temperature = 22.1;
        g_fusion_data.bms.alert_active = false;
        g_fusion_data.bms.low_battery = (g_fusion_data.bms.soc_percent < 20.0);
        g_fusion_data.bms.high_battery = (g_fusion_data.bms.soc_percent > 90.0);
        
        // Update system temperatures (from your temperature sensors)
        g_fusion_data.board_temp = 35.2;
        g_fusion_data.heatsink_temp = 42.5;
        
        // Update uptime
        g_fusion_data.uptime_sec = esp_timer_get_time() / 1000000;
        
        // Run fusion engine to select optimal profiles
        fusion_update(&g_fusion_data);
        
        // Log historical data to MRAM
        mram_history_entry_t entry = {
            .timestamp = (uint32_t)g_fusion_data.timestamp,
            .ambient_lux = (int16_t)g_fusion_data.weather.ambient_lux,
            .temperature = (int16_t)(g_fusion_data.weather.temperature_c * 100),
            .pv_voltage = (int16_t)(g_fusion_data.weather.pv_voltage * 100),
            .weather_state = g_fusion_data.weather.state,
            .time_period = g_fusion_data.astro.current_period,
            .bms_soc = (uint16_t)(g_fusion_data.bms.soc_percent * 100),
            .bms_voltage = (int16_t)(g_fusion_data.bms.voltage * 100),
            .bms_current = (int16_t)(g_fusion_data.bms.current * 100),
            .bms_temperature = (int16_t)(g_fusion_data.bms.temperature * 100),
            .v2g_power = g_fusion_data.active_v2g_profile ? 
                         g_fusion_data.active_v2g_profile->power_target : 0,
            .v2g_profile_id = g_fusion_data.active_v2g_profile ? 
                              g_fusion_data.active_v2g_profile->id : 0,
            .led_warm = g_fusion_data.active_led_profile ? 
                        g_fusion_data.active_led_profile->warm_intensity : 0,
            .led_cool = g_fusion_data.active_led_profile ? 
                        g_fusion_data.active_led_profile->cool_intensity : 0,
            .led_profile_id = g_fusion_data.active_led_profile ? 
                              g_fusion_data.active_led_profile->id : 0,
            .board_temp = (int16_t)(g_fusion_data.board_temp * 100),
            .heatsink_temp = (int16_t)(g_fusion_data.heatsink_temp * 100),
        };
        
        mram_history_add(&entry);
        
        // Update every 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "DIAMANT v2.1 System Starting...");
    
    // Initialize MRAM storage
    esp_err_t ret = mram_storage_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MRAM storage");
        return;
    }
    
    // Initialize fusion engine
    ret = fusion_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize fusion engine");
        return;
    }
    
    // Initialize WiFi Access Point
    ret = web_server_wifi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi AP");
        return;
    }
    
    // Start HTTP web server
    ret = web_server_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server");
        return;
    }
    
    ESP_LOGI(TAG, "Web server started successfully");
    ESP_LOGI(TAG, "Connect to WiFi: SSID=DIAMANT_AP, Password=diamant2026");
    ESP_LOGI(TAG, "Dashboard: http://192.168.4.1/");
    
    // Create fusion data update task
    xTaskCreate(
        update_fusion_data_task,
        "fusion_update",
        4096,
        NULL,
        5,
        NULL
    );
    
    ESP_LOGI(TAG, "System initialization complete");
    
    // Main loop - could monitor system health, handle events, etc.
    while (1) {
        // Your main application logic here
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/**
 * INTEGRATION CHECKLIST:
 * 
 * 1. Add to CMakeLists.txt:
 *    - web_server.c to SRCS
 *    - cJSON component requirement
 *    - esp_http_server component requirement
 * 
 * 2. Ensure these components are available:
 *    - fusion.c (fusion engine)
 *    - mram_storage.c (MRAM operations)
 *    - astro.c (astronomical calculations)
 *    - calendar.c (calendar functions)
 *    - weather.c (weather detection)
 * 
 * 3. Build HTML header:
 *    $ python3 tools/html_to_header.py main/www/dashboard.html main/www/dashboard.html.h
 * 
 * 4. Build project:
 *    $ idf.py build
 * 
 * 5. Flash to ESP32-C6:
 *    $ idf.py -p /dev/ttyUSB0 flash monitor
 * 
 * 6. Connect to WiFi AP and access dashboard:
 *    - WiFi SSID: DIAMANT_AP
 *    - Password: diamant2026
 *    - URL: http://192.168.4.1/
 */
