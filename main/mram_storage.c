/**
 * @file mram_storage.c
 * @brief MRAM storage management implementation for AT24C256 EEPROM
 * 
 * Implements persistent storage for configuration, historical logs,
 * predictive analytics, and client profiles with CRC validation.
 */

#include "mram_storage.h"
#include "drivers/at24c256.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>

static const char *TAG = "MRAM";

static mram_config_t g_config;  // Cached configuration
static bool g_initialized = false;

//============================================================================
// CRC16-CCITT IMPLEMENTATION
//============================================================================

uint16_t mram_crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF;  // Initial value for CRC16-CCITT
    
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;  // CRC16-CCITT polynomial
            } else {
                crc = crc << 1;
            }
        }
    }
    
    return crc;
}

//============================================================================
// INITIALIZATION
//============================================================================

esp_err_t mram_storage_init(void)
{
    esp_err_t ret;

    if (g_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    // Initialize AT24C256 driver
    ret = at24c256_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize AT24C256: %s", esp_err_to_name(ret));
        return ret;
    }

    // Load configuration
    ret = mram_config_load(&g_config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load config, formatting MRAM");
        ret = mram_config_format();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to format MRAM: %s", esp_err_to_name(ret));
            return ret;
        }
        ret = mram_config_load(&g_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to load config after format: %s", esp_err_to_name(ret));
            return ret;
        }
    }

    g_initialized = true;
    ESP_LOGI(TAG, "MRAM storage initialized (device=%s, version=%u)", 
             g_config.device_id, g_config.version);

    return ESP_OK;
}

//============================================================================
// CONFIGURATION MANAGEMENT
//============================================================================

esp_err_t mram_config_load(mram_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    // Read configuration from MRAM
    esp_err_t ret = at24c256_read(MRAM_ADDR_CONFIG, (uint8_t *)config, sizeof(mram_config_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read config: %s", esp_err_to_name(ret));
        return ret;
    }

    // Validate magic number
    if (config->magic != MRAM_CONFIG_MAGIC) {
        ESP_LOGE(TAG, "Invalid config magic: 0x%08X (expected 0x%08X)", 
                 config->magic, MRAM_CONFIG_MAGIC);
        return ESP_ERR_INVALID_CRC;
    }

    // Validate CRC
    uint16_t stored_crc = config->crc16;
    config->crc16 = 0;  // Zero out CRC field for calculation
    uint16_t calculated_crc = mram_crc16((uint8_t *)config, sizeof(mram_config_t));
    config->crc16 = stored_crc;  // Restore CRC

    if (stored_crc != calculated_crc) {
        ESP_LOGE(TAG, "Config CRC mismatch: stored=0x%04X, calculated=0x%04X",
                 stored_crc, calculated_crc);
        return ESP_ERR_INVALID_CRC;
    }

    ESP_LOGI(TAG, "Config loaded (magic=0x%08X, version=%u, crc=0x%04X)",
             config->magic, config->version, config->crc16);

    return ESP_OK;
}

esp_err_t mram_config_save(const mram_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    // Create writable copy for CRC calculation
    mram_config_t config_copy;
    memcpy(&config_copy, config, sizeof(mram_config_t));

    // Set magic number
    config_copy.magic = MRAM_CONFIG_MAGIC;

    // Calculate CRC
    config_copy.crc16 = 0;
    config_copy.crc16 = mram_crc16((uint8_t *)&config_copy, sizeof(mram_config_t));

    // Write to MRAM
    esp_err_t ret = at24c256_write(MRAM_ADDR_CONFIG, (uint8_t *)&config_copy, sizeof(mram_config_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write config: %s", esp_err_to_name(ret));
        return ret;
    }

    // Update cached config
    memcpy(&g_config, &config_copy, sizeof(mram_config_t));

    ESP_LOGI(TAG, "Config saved (version=%u, crc=0x%04X)", 
             config_copy.version, config_copy.crc16);

    return ESP_OK;
}

esp_err_t mram_config_format(void)
{
    ESP_LOGI(TAG, "Formatting MRAM with default configuration");

    mram_config_t default_config = {
        .magic = MRAM_CONFIG_MAGIC,
        .version = 1,
        .crc16 = 0,
        .device_id = "DIAMANT-ESP32C6-001",
        .location = "Unknown",
        .latitude = 0.0f,
        .longitude = 0.0f,
        .timezone_offset = 0,
        .log_interval_sec = 60,
        .stats_interval_sec = 300,
        .batt_low_threshold = 20.0f,
        .batt_high_threshold = 90.0f,
        .temp_max_threshold = 60.0f,
        .total_runtime_hours = 0,
        .boot_count = 0,
        .history_write_index = 0,
        .history_count = 0,
    };

    // Save default configuration
    esp_err_t ret = mram_config_save(&default_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save default config: %s", esp_err_to_name(ret));
        return ret;
    }

    // Clear history buffer
    uint8_t zero_page[AT24C256_PAGE_SIZE] = {0};
    for (uint16_t addr = MRAM_ADDR_HISTORY; addr < MRAM_ADDR_HISTORY + MRAM_SIZE_HISTORY; addr += AT24C256_PAGE_SIZE) {
        ret = at24c256_write(addr, zero_page, AT24C256_PAGE_SIZE);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to clear history at 0x%04X", addr);
            return ret;
        }
    }

    ESP_LOGI(TAG, "MRAM format complete");
    return ESP_OK;
}

//============================================================================
// HISTORICAL LOG MANAGEMENT
//============================================================================

esp_err_t mram_history_add(const mram_history_entry_t *entry)
{
    if (!entry || !g_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    // Calculate write address (circular buffer)
    uint16_t write_addr = MRAM_ADDR_HISTORY + 
                         (g_config.history_write_index * MRAM_HISTORY_ENTRY_SIZE);

    // Write entry to MRAM
    esp_err_t ret = at24c256_write(write_addr, (uint8_t *)entry, sizeof(mram_history_entry_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write history entry: %s", esp_err_to_name(ret));
        return ret;
    }

    // Update write index and count
    g_config.history_write_index = (g_config.history_write_index + 1) % MRAM_HISTORY_MAX_ENTRIES;
    if (g_config.history_count < MRAM_HISTORY_MAX_ENTRIES) {
        g_config.history_count++;
    }

    // Save updated config
    ret = mram_config_save(&g_config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to update config after history add");
    }

    ESP_LOGD(TAG, "History entry added (index=%u, count=%u)", 
             g_config.history_write_index, g_config.history_count);

    return ESP_OK;
}

esp_err_t mram_history_read(mram_history_entry_t *entries, uint16_t max_count, uint16_t *count_out)
{
    if (!entries || !count_out || !g_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t entries_to_read = (max_count < g_config.history_count) ? max_count : g_config.history_count;
    *count_out = 0;

    if (entries_to_read == 0) {
        return ESP_OK;
    }

    // Calculate starting index (read backwards from most recent)
    int16_t read_index = g_config.history_write_index - 1;
    if (read_index < 0) {
        read_index = g_config.history_count - 1;
    }

    for (uint16_t i = 0; i < entries_to_read; i++) {
        uint16_t read_addr = MRAM_ADDR_HISTORY + (read_index * MRAM_HISTORY_ENTRY_SIZE);
        
        esp_err_t ret = at24c256_read(read_addr, (uint8_t *)&entries[i], sizeof(mram_history_entry_t));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read history entry %u: %s", i, esp_err_to_name(ret));
            return ret;
        }

        (*count_out)++;

        // Move to previous entry
        read_index--;
        if (read_index < 0) {
            read_index = MRAM_HISTORY_MAX_ENTRIES - 1;
        }
    }

    ESP_LOGI(TAG, "Read %u history entries", *count_out);
    return ESP_OK;
}

//============================================================================
// STATISTICS CALCULATION
//============================================================================

esp_err_t mram_stats_calculate(mram_stats_t *stats)
{
    if (!stats || !g_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(stats, 0, sizeof(mram_stats_t));

    if (g_config.history_count == 0) {
        ESP_LOGW(TAG, "No history data for statistics");
        return ESP_OK;
    }

    // Read all history entries
    mram_history_entry_t *entries = malloc(g_config.history_count * sizeof(mram_history_entry_t));
    if (!entries) {
        ESP_LOGE(TAG, "Failed to allocate memory for history entries");
        return ESP_ERR_NO_MEM;
    }

    uint16_t count_read = 0;
    esp_err_t ret = mram_history_read(entries, g_config.history_count, &count_read);
    if (ret != ESP_OK || count_read == 0) {
        free(entries);
        return ret;
    }

    // Calculate statistics
    float pv_energy_sum = 0;
    float batt_charge_sum = 0;
    float batt_discharge_sum = 0;
    float v2g_export_sum = 0;
    float v2g_import_sum = 0;
    float led_energy_sum = 0;
    float pv_voltage_sum = 0;
    uint32_t valid_entries = 0;

    time_t now = time(NULL);
    time_t today_start = now - (now % 86400);

    for (uint16_t i = 0; i < count_read; i++) {
        // PV statistics
        float pv_voltage = entries[i].pv_voltage / 100.0f;
        pv_voltage_sum += pv_voltage;
        
        if (entries[i].timestamp >= today_start) {
            float pv_power = pv_voltage * 1.0f;  // Simplified power calculation
            pv_energy_sum += pv_power * (g_config.log_interval_sec / 3600.0f);
        }

        // Battery statistics
        if (entries[i].bms_current > 0) {
            batt_charge_sum += (entries[i].bms_current / 100.0f) * (entries[i].bms_voltage / 100.0f) * (g_config.log_interval_sec / 3600.0f) / 1000.0f;
        } else {
            batt_discharge_sum += fabsf(entries[i].bms_current / 100.0f) * (entries[i].bms_voltage / 100.0f) * (g_config.log_interval_sec / 3600.0f) / 1000.0f;
        }

        // V2G statistics
        if (entries[i].timestamp >= today_start) {
            if (entries[i].v2g_power > 0) {
                v2g_export_sum += entries[i].v2g_power * (g_config.log_interval_sec / 3600.0f) / 1000.0f;
            } else {
                v2g_import_sum += fabsf(entries[i].v2g_power) * (g_config.log_interval_sec / 3600.0f) / 1000.0f;
            }
        }

        // LED statistics
        if (entries[i].timestamp >= today_start) {
            float led_power = (entries[i].led_warm + entries[i].led_cool) * 0.5f;  // Simplified
            led_energy_sum += led_power * (g_config.log_interval_sec / 3600.0f) / 1000.0f;
        }

        valid_entries++;
    }

    // Calculate averages and totals
    if (valid_entries > 0) {
        stats->pv_energy_kwh_today = pv_energy_sum / 1000.0f;
        stats->pv_avg_voltage = pv_voltage_sum / valid_entries;
        stats->batt_energy_charged_kwh = batt_charge_sum;
        stats->batt_energy_discharged_kwh = batt_discharge_sum;
        stats->batt_cycles_total = batt_discharge_sum / 5.0f;  // Assuming 5kWh battery
        stats->batt_health_percent = 100.0f - (stats->batt_cycles_total * 0.01f);
        stats->v2g_export_kwh_today = v2g_export_sum;
        stats->v2g_import_kwh_today = v2g_import_sum;
        stats->v2g_net_kwh = v2g_export_sum - v2g_import_sum;
        stats->led_energy_kwh_today = led_energy_sum;
        stats->uptime_hours = g_config.total_runtime_hours;
    }

    free(entries);

    ESP_LOGI(TAG, "Statistics calculated (PV=%.2fkWh, Batt=%.1f%%)", 
             stats->pv_energy_kwh_today, stats->batt_health_percent);

    return ESP_OK;
}

//============================================================================
// PREDICTIVE ANALYTICS
//============================================================================

esp_err_t mram_prediction_generate(mram_prediction_t *prediction)
{
    if (!prediction || !g_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(prediction, 0, sizeof(mram_prediction_t));

    // Get statistics for prediction base
    mram_stats_t stats;
    esp_err_t ret = mram_stats_calculate(&stats);
    if (ret != ESP_OK) {
        return ret;
    }

    // PV production forecast (simplified model)
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    float hour_factor = 1.0f - fabsf(timeinfo.tm_hour - 12.0f) / 12.0f;  // Peak at noon
    prediction->pv_forecast_next_hour_kwh = stats.pv_energy_kwh_today * 0.1f * hour_factor;
    prediction->pv_forecast_today_kwh = stats.pv_energy_kwh_today * 1.2f;

    // Battery health prediction
    prediction->batt_predicted_cycles_remaining = (100.0f - stats.batt_health_percent) * 100.0f;
    prediction->batt_predicted_capacity_fade = stats.batt_cycles_total * 0.001f;

    // Maintenance prediction
    if (stats.batt_health_percent < 80.0f) {
        prediction->days_until_maintenance = 30;
        snprintf(prediction->maintenance_reason, sizeof(prediction->maintenance_reason),
                 "Battery health below 80%%");
    } else if (stats.error_count > 100) {
        prediction->days_until_maintenance = 7;
        snprintf(prediction->maintenance_reason, sizeof(prediction->maintenance_reason),
                 "High error count detected");
    } else {
        prediction->days_until_maintenance = 365;
        snprintf(prediction->maintenance_reason, sizeof(prediction->maintenance_reason),
                 "Routine maintenance");
    }

    ESP_LOGI(TAG, "Predictions generated (PV forecast=%.2fkWh, Maint=%u days)",
             prediction->pv_forecast_today_kwh, prediction->days_until_maintenance);

    return ESP_OK;
}

//============================================================================
// CLIENT PROFILE MANAGEMENT
//============================================================================

esp_err_t mram_client_profile_save(uint8_t index, const mram_client_profile_t *profile)
{
    if (!profile || index >= MRAM_CLIENT_MAX_PROFILES) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t addr = MRAM_ADDR_CLIENT_PROFILES + (index * MRAM_CLIENT_PROFILE_SIZE);

    esp_err_t ret = at24c256_write(addr, (uint8_t *)profile, sizeof(mram_client_profile_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save client profile %u: %s", index, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Client profile %u saved: %s", index, profile->name);
    return ESP_OK;
}

esp_err_t mram_client_profile_load(uint8_t index, mram_client_profile_t *profile)
{
    if (!profile || index >= MRAM_CLIENT_MAX_PROFILES) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t addr = MRAM_ADDR_CLIENT_PROFILES + (index * MRAM_CLIENT_PROFILE_SIZE);

    esp_err_t ret = at24c256_read(addr, (uint8_t *)profile, sizeof(mram_client_profile_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load client profile %u: %s", index, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Client profile %u loaded: %s", index, profile->name);
    return ESP_OK;
}

//============================================================================
// TEST FUNCTION
//============================================================================

esp_err_t mram_test(void)
{
    ESP_LOGI(TAG, "Starting MRAM test...");

    // Test AT24C256 driver
    esp_err_t ret = at24c256_test();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "AT24C256 test failed");
        return ret;
    }

    // Test configuration save/load
    mram_config_t test_config;
    memcpy(&test_config, &g_config, sizeof(mram_config_t));
    test_config.boot_count++;

    ret = mram_config_save(&test_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Config save test failed");
        return ret;
    }

    mram_config_t loaded_config;
    ret = mram_config_load(&loaded_config);
    if (ret != ESP_OK || loaded_config.boot_count != test_config.boot_count) {
        ESP_LOGE(TAG, "Config load test failed");
        return ESP_FAIL;
    }

    // Test history entry
    mram_history_entry_t test_entry = {
        .timestamp = (uint32_t)time(NULL),
        .ambient_lux = 5000,
        .temperature = 2500,
        .pv_voltage = 1800,
        .bms_soc = 8000,
        .bms_voltage = 4800,
        .bms_current = 100,
    };

    ret = mram_history_add(&test_entry);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "History add test failed");
        return ret;
    }

    ESP_LOGI(TAG, "MRAM test PASSED");
    return ESP_OK;
}
