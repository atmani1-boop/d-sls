/**
 * @file task_fusion.c
 * @brief Fusion task implementation - sensor fusion and profile selection
 * 
 * This FreeRTOS task runs at 500ms intervals to:
 *   - Update astronomical data (sunrise/sunset/twilight)
 *   - Update calendar data (season/holidays/weekend)
 *   - Update weather detection from sensors
 *   - Update BMS battery data
 *   - Determine current time period
 *   - Apply V2G and LED profiles using fusion engine
 *   - Publish fusion data globally
 */

#include "task_fusion.h"
#include "include/profiles.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include <time.h>
#include <sys/time.h>

//============================================================================
// EXTERNAL DEPENDENCIES
//============================================================================

// External function prototypes
extern esp_err_t astro_calculate(astro_data_t *astro, time_t timestamp);
extern void astro_update_period(astro_data_t *astro, time_t timestamp);
extern esp_err_t calendar_calculate(calendar_data_t *calendar, time_t timestamp);
extern esp_err_t weather_update(weather_data_t *weather, bool is_daytime);
extern esp_err_t fusion_update(fusion_data_t *fusion);

//============================================================================
// GLOBAL DATA
//============================================================================

static const char *TAG = "TASK_FUSION";
static TaskHandle_t s_fusion_task_handle = NULL;
static bool s_task_running = false;

// Global fusion data (published to other modules)
fusion_data_t g_fusion_data = {0};

//============================================================================
// PRIVATE FUNCTIONS
//============================================================================

/**
 * @brief Simulate BMS data update
 * @note Replace with actual BMS driver when available
 */
static esp_err_t bms_update_data(bms_data_t *bms)
{
    if (!bms) {
        return ESP_ERR_INVALID_ARG;
    }

    // Simulate BMS data (replace with actual driver calls)
    // For now, use simulated values for testing
    static float sim_soc = 75.0f;
    
    // Slowly vary SOC for testing
    sim_soc += ((float)(esp_random() % 20) - 10.0f) / 100.0f;
    if (sim_soc > 100.0f) sim_soc = 100.0f;
    if (sim_soc < 0.0f) sim_soc = 0.0f;

    bms->soc_percent = sim_soc;
    bms->voltage = 48.0f + (sim_soc / 100.0f) * 6.0f;  // 48-54V range
    bms->current = ((float)(esp_random() % 200) - 100.0f) / 10.0f;  // -10 to +10A
    bms->temperature = 25.0f + ((float)(esp_random() % 100)) / 10.0f;  // 25-35°C
    bms->alert_active = false;
    bms->low_battery = (bms->soc_percent < 20.0f);
    bms->high_battery = (bms->soc_percent > 90.0f);

    return ESP_OK;
}

/**
 * @brief Update board temperature
 * @note Replace with actual temperature sensor driver when available
 */
static float get_board_temperature(void)
{
    // Simulate board temperature (replace with actual sensor)
    return 35.0f + ((float)(esp_random() % 100)) / 10.0f;  // 35-45°C
}

/**
 * @brief Update heatsink temperature
 * @note Replace with actual temperature sensor driver when available
 */
static float get_heatsink_temperature(void)
{
    // Simulate heatsink temperature (replace with actual sensor)
    return 40.0f + ((float)(esp_random() % 150)) / 10.0f;  // 40-55°C
}

/**
 * @brief Main fusion task function
 */
static void fusion_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Fusion task started (Priority: %d, Cycle: %dms)",
             FUSION_TASK_PRIORITY, FUSION_TASK_CYCLE_MS);

    TickType_t last_wake_time = xTaskGetTickCount();
    uint32_t cycle_count = 0;

    while (s_task_running) {
        esp_err_t ret;
        time_t now;
        struct tm timeinfo;

        // Get current time
        time(&now);
        localtime_r(&now, &timeinfo);

        // Update timestamp
        g_fusion_data.timestamp = now;

        // Update uptime
        g_fusion_data.uptime_sec = esp_log_timestamp() / 1000;

        // Update astronomical data (sunrise/sunset/twilight)
        ret = astro_calculate(&g_fusion_data.astro, now);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to calculate astro data: %s", esp_err_to_name(ret));
        } else {
            astro_update_period(&g_fusion_data.astro, now);
        }

        // Update calendar data (season/holidays/weekend)
        ret = calendar_calculate(&g_fusion_data.calendar, now);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to calculate calendar data: %s", esp_err_to_name(ret));
        }

        // Determine if daytime for weather update
        bool is_daytime = (g_fusion_data.astro.current_period == PERIOD_DAY);

        // Update weather detection from sensors
        ret = weather_update(&g_fusion_data.weather, is_daytime);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to update weather data: %s", esp_err_to_name(ret));
        }

        // Update BMS data
        ret = bms_update_data(&g_fusion_data.bms);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to update BMS data: %s", esp_err_to_name(ret));
        }

        // Update board and heatsink temperatures
        g_fusion_data.board_temp = get_board_temperature();
        g_fusion_data.heatsink_temp = get_heatsink_temperature();

        // Apply V2G and LED profiles using fusion engine
        ret = fusion_update(&g_fusion_data);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Fusion engine update failed: %s", esp_err_to_name(ret));
        }

        // Log status every 10 seconds (20 cycles at 500ms)
        cycle_count++;
        if (cycle_count % 20 == 0) {
            ESP_LOGI(TAG, "Cycle %lu | Period: %d | Weather: %d | SOC: %.1f%% | V2G: %s | LED: %s",
                     cycle_count,
                     g_fusion_data.astro.current_period,
                     g_fusion_data.weather.state,
                     g_fusion_data.bms.soc_percent,
                     g_fusion_data.active_v2g_profile ? g_fusion_data.active_v2g_profile->name : "NONE",
                     g_fusion_data.active_led_profile ? g_fusion_data.active_led_profile->name : "NONE");
        }

        // Wait for next cycle
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(FUSION_TASK_CYCLE_MS));
    }

    ESP_LOGI(TAG, "Fusion task stopped");
    s_fusion_task_handle = NULL;
    vTaskDelete(NULL);
}

//============================================================================
// PUBLIC FUNCTIONS
//============================================================================

esp_err_t task_fusion_start(void)
{
    if (s_fusion_task_handle != NULL) {
        ESP_LOGW(TAG, "Fusion task already running");
        return ESP_ERR_INVALID_STATE;
    }

    // Initialize global fusion data
    memset(&g_fusion_data, 0, sizeof(fusion_data_t));
    
    // Set default location (replace with actual configuration)
    g_fusion_data.astro.latitude = 48.8566f;   // Paris latitude
    g_fusion_data.astro.longitude = 2.3522f;   // Paris longitude

    s_task_running = true;

    BaseType_t ret = xTaskCreate(
        fusion_task,
        FUSION_TASK_NAME,
        FUSION_TASK_STACK_SIZE,
        NULL,
        FUSION_TASK_PRIORITY,
        &s_fusion_task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create fusion task");
        s_task_running = false;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Fusion task created successfully");
    return ESP_OK;
}

void task_fusion_stop(void)
{
    if (s_fusion_task_handle == NULL) {
        ESP_LOGW(TAG, "Fusion task not running");
        return;
    }

    ESP_LOGI(TAG, "Stopping fusion task...");
    s_task_running = false;

    // Wait for task to terminate (with timeout)
    for (int i = 0; i < 20 && s_fusion_task_handle != NULL; i++) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (s_fusion_task_handle != NULL) {
        ESP_LOGW(TAG, "Force deleting fusion task");
        vTaskDelete(s_fusion_task_handle);
        s_fusion_task_handle = NULL;
    }

    ESP_LOGI(TAG, "Fusion task stopped");
}

TaskHandle_t task_fusion_get_handle(void)
{
    return s_fusion_task_handle;
}
