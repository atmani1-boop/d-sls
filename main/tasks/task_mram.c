/**
 * @file task_mram.c
 * @brief MRAM storage task implementation - periodic logging and analytics
 * 
 * This FreeRTOS task runs at 30-second intervals to:
 *   - Log data every 30 minutes (configurable)
 *   - Calculate statistics every hour
 *   - Update configuration every 24 hours
 *   - Display predictive analytics in logs
 */

#include "task_mram.h"
#include "include/mram_storage.h"
#include "include/profiles.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include <time.h>
#include <string.h>

//============================================================================
// EXTERNAL DEPENDENCIES
//============================================================================

// Global fusion data (from task_fusion.c)
extern fusion_data_t g_fusion_data;

// MRAM storage functions
extern esp_err_t mram_storage_init(void);
extern esp_err_t mram_config_load(mram_config_t *config);
extern esp_err_t mram_config_save(const mram_config_t *config);
extern esp_err_t mram_history_add(const mram_history_entry_t *entry);
extern esp_err_t mram_stats_calculate(mram_stats_t *stats);
extern esp_err_t mram_prediction_generate(mram_prediction_t *prediction);

//============================================================================
// GLOBAL DATA
//============================================================================

static const char *TAG = "TASK_MRAM";
static TaskHandle_t s_mram_task_handle = NULL;
static bool s_task_running = false;
static bool s_log_now_requested = false;

// Configuration
static mram_config_t s_config = {0};

// Timing counters (in seconds)
static uint32_t s_last_log_time = 0;
static uint32_t s_last_stats_time = 0;
static uint32_t s_last_config_time = 0;

//============================================================================
// PRIVATE FUNCTIONS
//============================================================================

/**
 * @brief Convert fusion data to history entry
 */
static void fusion_to_history_entry(const fusion_data_t *fusion, mram_history_entry_t *entry)
{
    if (!fusion || !entry) {
        return;
    }

    memset(entry, 0, sizeof(mram_history_entry_t));

    entry->timestamp = (uint32_t)fusion->timestamp;
    
    // Environmental data (scaled for compression)
    entry->ambient_lux = (int16_t)fusion->weather.ambient_lux;
    entry->temperature = (int16_t)(fusion->weather.temperature_c * 100);
    entry->pv_voltage = (int16_t)(fusion->weather.pv_voltage * 100);
    entry->weather_state = (uint8_t)fusion->weather.state;
    entry->time_period = (uint8_t)fusion->astro.current_period;
    
    // Battery data (scaled)
    entry->batt_soc = (uint8_t)fusion->bms.soc_percent;
    entry->batt_voltage = (int16_t)(fusion->bms.voltage * 100);
    entry->batt_current = (int16_t)(fusion->bms.current * 100);
    entry->batt_temp = (int8_t)fusion->bms.temperature;
    
    // System data
    entry->board_temp = (int8_t)fusion->board_temp;
    entry->heatsink_temp = (int8_t)fusion->heatsink_temp;
    
    // Profile IDs
    entry->v2g_profile_id = fusion->active_v2g_profile ? fusion->active_v2g_profile->id : 0xFF;
    entry->led_profile_id = fusion->active_led_profile ? fusion->active_led_profile->id : 0xFF;
    
    // LED intensities (if LED profile active)
    if (fusion->active_led_profile) {
        entry->led_warm = fusion->active_led_profile->warm_intensity;
        entry->led_cool = fusion->active_led_profile->cool_intensity;
    } else {
        entry->led_warm = 0;
        entry->led_cool = 0;
    }
    
    // Status flags
    entry->flags = 0;
    if (fusion->bms.alert_active) entry->flags |= 0x01;
    if (fusion->weather.pv_oscillating) entry->flags |= 0x02;
    if (fusion->calendar.is_weekend) entry->flags |= 0x04;
    if (fusion->calendar.is_holiday) entry->flags |= 0x08;
}

/**
 * @brief Log current data to MRAM
 */
static esp_err_t log_data_to_mram(void)
{
    mram_history_entry_t entry;
    
    // Convert fusion data to history entry
    fusion_to_history_entry(&g_fusion_data, &entry);
    
    // Write to MRAM
    esp_err_t ret = mram_history_add(&entry);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add history entry: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Data logged: SOC=%.1f%%, Temp=%.1f°C, V2G=%s",
             g_fusion_data.bms.soc_percent,
             g_fusion_data.weather.temperature_c,
             g_fusion_data.active_v2g_profile ? g_fusion_data.active_v2g_profile->name : "NONE");
    
    return ESP_OK;
}

/**
 * @brief Calculate and display statistics
 */
static esp_err_t calculate_statistics(void)
{
    mram_stats_t stats = {0};
    
    esp_err_t ret = mram_stats_calculate(&stats);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to calculate statistics: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "=== Hourly Statistics ===");
    ESP_LOGI(TAG, "PV Energy: Today=%.2f kWh, Week=%.2f kWh, Peak=%.0f W",
             stats.pv_energy_kwh_today,
             stats.pv_energy_kwh_week,
             stats.pv_peak_power_w);
    ESP_LOGI(TAG, "Battery: Cycles=%.1f, Health=%.0f%%",
             stats.batt_cycles_total,
             stats.batt_health_percent);
    ESP_LOGI(TAG, "V2G: Export=%.2f kWh, Import=%.2f kWh, Net=%.2f kWh",
             stats.v2g_export_kwh_today,
             stats.v2g_import_kwh_today,
             stats.v2g_net_kwh);
    ESP_LOGI(TAG, "LED Energy: %.2f kWh, Avg Runtime: %.1f hours",
             stats.led_energy_kwh_today,
             stats.led_avg_runtime_hours);
    ESP_LOGI(TAG, "Uptime: %.1f hours, Errors: %lu",
             stats.uptime_hours,
             stats.error_count);
    
    return ESP_OK;
}

/**
 * @brief Generate and display predictive analytics
 */
static esp_err_t display_predictions(void)
{
    mram_prediction_t prediction = {0};
    
    esp_err_t ret = mram_prediction_generate(&prediction);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to generate predictions: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "=== Predictive Analytics ===");
    ESP_LOGI(TAG, "PV Forecast: Next Hour=%.2f kWh, Today=%.2f kWh",
             prediction.pv_forecast_next_hour_kwh,
             prediction.pv_forecast_today_kwh);
    ESP_LOGI(TAG, "Battery: Cycles Remaining=%.0f, SOH Target=%.0f%%",
             prediction.batt_predicted_cycles_remaining,
             prediction.batt_predicted_soh_1year);
    ESP_LOGI(TAG, "Recommended Actions: V2G=%s, LED=%s",
             prediction.recommended_v2g_action,
             prediction.recommended_led_action);
    
    return ESP_OK;
}

/**
 * @brief Update configuration and save to MRAM
 */
static esp_err_t update_configuration(void)
{
    // Update runtime counters
    s_config.total_runtime_hours += 24;  // Add 24 hours
    
    ESP_LOGI(TAG, "Updating configuration: Runtime=%lu hours, Boots=%lu",
             s_config.total_runtime_hours,
             s_config.boot_count);
    
    esp_err_t ret = mram_config_save(&s_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save configuration: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Configuration saved successfully");
    return ESP_OK;
}

/**
 * @brief Main MRAM task function
 */
static void mram_task(void *pvParameters)
{
    ESP_LOGI(TAG, "MRAM task started (Priority: %d, Cycle: %dms)",
             MRAM_TASK_PRIORITY, MRAM_TASK_CYCLE_MS);

    TickType_t last_wake_time = xTaskGetTickCount();
    uint32_t current_time = 0;

    while (s_task_running) {
        current_time = esp_log_timestamp() / 1000;  // Convert to seconds

        // Check if immediate log requested
        if (s_log_now_requested) {
            ESP_LOGI(TAG, "Immediate log requested");
            log_data_to_mram();
            s_log_now_requested = false;
        }

        // Periodic data logging (every 30 minutes by default)
        if (current_time - s_last_log_time >= s_config.log_interval_sec) {
            ESP_LOGI(TAG, "Periodic data logging...");
            log_data_to_mram();
            s_last_log_time = current_time;
        }

        // Statistics calculation (every hour)
        if (current_time - s_last_stats_time >= MRAM_STATS_INTERVAL_SEC) {
            ESP_LOGI(TAG, "Calculating statistics...");
            calculate_statistics();
            display_predictions();
            s_last_stats_time = current_time;
        }

        // Configuration update (every 24 hours)
        if (current_time - s_last_config_time >= MRAM_CONFIG_UPDATE_SEC) {
            ESP_LOGI(TAG, "Updating configuration...");
            update_configuration();
            s_last_config_time = current_time;
        }

        // Wait for next cycle
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(MRAM_TASK_CYCLE_MS));
    }

    ESP_LOGI(TAG, "MRAM task stopped");
    s_mram_task_handle = NULL;
    vTaskDelete(NULL);
}

//============================================================================
// PUBLIC FUNCTIONS
//============================================================================

esp_err_t task_mram_start(void)
{
    if (s_mram_task_handle != NULL) {
        ESP_LOGW(TAG, "MRAM task already running");
        return ESP_ERR_INVALID_STATE;
    }

    // Initialize MRAM storage
    esp_err_t ret = mram_storage_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MRAM storage: %s", esp_err_to_name(ret));
        return ret;
    }

    // Load configuration
    ret = mram_config_load(&s_config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load config, using defaults");
        
        // Initialize default configuration
        memset(&s_config, 0, sizeof(mram_config_t));
        s_config.log_interval_sec = MRAM_LOG_INTERVAL_SEC;
        s_config.stats_interval_sec = MRAM_STATS_INTERVAL_SEC;
        s_config.latitude = 48.8566f;
        s_config.longitude = 2.3522f;
        s_config.timezone_offset = 1;
        s_config.batt_low_threshold = 20.0f;
        s_config.batt_high_threshold = 90.0f;
        s_config.temp_max_threshold = 70.0f;
        s_config.boot_count = 1;
        strcpy(s_config.device_id, "DIAMANT_v2.1");
        strcpy(s_config.location, "Lab");
        
        // Save initial configuration
        mram_config_save(&s_config);
    } else {
        // Increment boot count
        s_config.boot_count++;
        mram_config_save(&s_config);
        ESP_LOGI(TAG, "Configuration loaded: Boot #%lu", s_config.boot_count);
    }

    // Initialize timing counters
    uint32_t now = esp_log_timestamp() / 1000;
    s_last_log_time = now;
    s_last_stats_time = now;
    s_last_config_time = now;

    s_task_running = true;

    BaseType_t task_ret = xTaskCreate(
        mram_task,
        MRAM_TASK_NAME,
        MRAM_TASK_STACK_SIZE,
        NULL,
        MRAM_TASK_PRIORITY,
        &s_mram_task_handle
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create MRAM task");
        s_task_running = false;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MRAM task created successfully");
    return ESP_OK;
}

void task_mram_stop(void)
{
    if (s_mram_task_handle == NULL) {
        ESP_LOGW(TAG, "MRAM task not running");
        return;
    }

    ESP_LOGI(TAG, "Stopping MRAM task...");
    s_task_running = false;

    // Wait for task to terminate (with timeout)
    for (int i = 0; i < 20 && s_mram_task_handle != NULL; i++) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (s_mram_task_handle != NULL) {
        ESP_LOGW(TAG, "Force deleting MRAM task");
        vTaskDelete(s_mram_task_handle);
        s_mram_task_handle = NULL;
    }

    ESP_LOGI(TAG, "MRAM task stopped");
}

TaskHandle_t task_mram_get_handle(void)
{
    return s_mram_task_handle;
}

esp_err_t task_mram_log_now(void)
{
    if (s_mram_task_handle == NULL) {
        ESP_LOGE(TAG, "MRAM task not running");
        return ESP_ERR_INVALID_STATE;
    }

    s_log_now_requested = true;
    ESP_LOGI(TAG, "Immediate log requested");
    
    return ESP_OK;
}
