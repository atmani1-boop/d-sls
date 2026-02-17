/**
 * @file cli_mram.c
 * @brief Console commands implementation for MRAM storage system
 */

#include "cli_mram.h"
#include "mram_storage.h"
#include "esp_log.h"
#include "esp_console.h"
#include <string.h>
#include <time.h>

static const char *TAG = "CLI_MRAM";

//============================================================================
// HELPER FUNCTIONS
//============================================================================

static void print_timestamp(uint32_t timestamp)
{
    time_t t = (time_t)timestamp;
    struct tm timeinfo;
    localtime_r(&t, &timeinfo);
    printf("%04d-%02d-%02d %02d:%02d:%02d",
           timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

static const char* get_weather_string(uint8_t state)
{
    const char* weather_states[] = {
        "Unknown", "Sunny", "Cloudy", "Overcast", "Rainy", "Stormy"
    };
    return (state < 6) ? weather_states[state] : "Invalid";
}

static const char* get_time_period_string(uint8_t period)
{
    const char* periods[] = {
        "Night", "Morning", "Noon", "Afternoon", "Evening"
    };
    return (period < 5) ? periods[period] : "Invalid";
}

//============================================================================
// COMMAND: mram-info
//============================================================================

static int cmd_mram_info(int argc, char **argv)
{
    mram_config_t config;
    esp_err_t ret = mram_config_load(&config);
    
    if (ret != ESP_OK) {
        printf("Error loading configuration: %s\n", esp_err_to_name(ret));
        return 1;
    }

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║                    MRAM CONFIGURATION INFO                       ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    printf("Device Information:\n");
    printf("  Device ID:           %s\n", config.device_id);
    printf("  Location:            %s\n", config.location);
    printf("  Coordinates:         %.6f, %.6f\n", config.latitude, config.longitude);
    printf("  Timezone:            UTC%+d\n", config.timezone_offset);
    printf("\n");

    printf("Configuration:\n");
    printf("  Magic:               0x%08X\n", config.magic);
    printf("  Version:             %u\n", config.version);
    printf("  CRC16:               0x%04X\n", config.crc16);
    printf("  Log Interval:        %u seconds\n", config.log_interval_sec);
    printf("  Stats Interval:      %u seconds\n", config.stats_interval_sec);
    printf("\n");

    printf("Thresholds:\n");
    printf("  Battery Low:         %.1f%%\n", config.batt_low_threshold);
    printf("  Battery High:        %.1f%%\n", config.batt_high_threshold);
    printf("  Temperature Max:     %.1f°C\n", config.temp_max_threshold);
    printf("\n");

    printf("Counters:\n");
    printf("  Total Runtime:       %u hours\n", config.total_runtime_hours);
    printf("  Boot Count:          %u\n", config.boot_count);
    printf("  History Entries:     %u / %u\n", config.history_count, MRAM_HISTORY_MAX_ENTRIES);
    printf("  History Write Index: %u\n", config.history_write_index);
    printf("\n");

    printf("Memory Layout:\n");
    printf("  Total Size:          %u bytes (32KB)\n", MRAM_SIZE);
    printf("  Config:              0x%04X - 0x%04X (%u bytes)\n", 
           MRAM_ADDR_CONFIG, MRAM_ADDR_CONFIG + MRAM_SIZE_CONFIG - 1, MRAM_SIZE_CONFIG);
    printf("  Client Profiles:     0x%04X - 0x%04X (%u bytes)\n",
           MRAM_ADDR_CLIENT_PROFILES, MRAM_ADDR_CLIENT_PROFILES + MRAM_SIZE_CLIENT_PROFILES - 1, 
           MRAM_SIZE_CLIENT_PROFILES);
    printf("  History:             0x%04X - 0x%04X (%u bytes)\n",
           MRAM_ADDR_HISTORY, MRAM_ADDR_HISTORY + MRAM_SIZE_HISTORY - 1, MRAM_SIZE_HISTORY);
    printf("  Prediction:          0x%04X - 0x%04X (%u bytes)\n",
           MRAM_ADDR_PREDICTION, MRAM_ADDR_PREDICTION + MRAM_SIZE_PREDICTION - 1, MRAM_SIZE_PREDICTION);
    printf("  Maintenance:         0x%04X - 0x%04X (%u bytes)\n",
           MRAM_ADDR_MAINTENANCE, MRAM_ADDR_MAINTENANCE + MRAM_SIZE_MAINTENANCE - 1, MRAM_SIZE_MAINTENANCE);
    printf("  Extended:            0x%04X - 0x%04X (%u bytes)\n",
           MRAM_ADDR_EXTENDED, MRAM_ADDR_EXTENDED + MRAM_SIZE_EXTENDED - 1, MRAM_SIZE_EXTENDED);
    printf("\n");

    return 0;
}

//============================================================================
// COMMAND: mram-stats
//============================================================================

static int cmd_mram_stats(int argc, char **argv)
{
    mram_stats_t stats;
    mram_prediction_t prediction;

    // Calculate statistics
    esp_err_t ret = mram_stats_calculate(&stats);
    if (ret != ESP_OK) {
        printf("Error calculating statistics: %s\n", esp_err_to_name(ret));
        return 1;
    }

    // Generate predictions
    ret = mram_prediction_generate(&prediction);
    if (ret != ESP_OK) {
        printf("Error generating predictions: %s\n", esp_err_to_name(ret));
        return 1;
    }

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║                    STATISTICS & PREDICTIONS                      ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    printf("PV Statistics:\n");
    printf("  Energy Today:        %.3f kWh\n", stats.pv_energy_kwh_today);
    printf("  Energy This Week:    %.3f kWh\n", stats.pv_energy_kwh_week);
    printf("  Peak Power:          %.1f W\n", stats.pv_peak_power_w);
    printf("  Average Voltage:     %.2f V\n", stats.pv_avg_voltage);
    printf("\n");

    printf("Battery Statistics:\n");
    printf("  Total Cycles:        %.1f\n", stats.batt_cycles_total);
    printf("  Energy Charged:      %.3f kWh\n", stats.batt_energy_charged_kwh);
    printf("  Energy Discharged:   %.3f kWh\n", stats.batt_energy_discharged_kwh);
    printf("  Health:              %.1f%%\n", stats.batt_health_percent);
    printf("\n");

    printf("V2G Statistics:\n");
    printf("  Export Today:        %.3f kWh\n", stats.v2g_export_kwh_today);
    printf("  Import Today:        %.3f kWh\n", stats.v2g_import_kwh_today);
    printf("  Net Energy:          %.3f kWh\n", stats.v2g_net_kwh);
    printf("\n");

    printf("LED Statistics:\n");
    printf("  Energy Today:        %.3f kWh\n", stats.led_energy_kwh_today);
    printf("  Avg Runtime:         %.1f hours\n", stats.led_avg_runtime_hours);
    printf("\n");

    printf("System Statistics:\n");
    printf("  Uptime:              %.1f hours\n", stats.uptime_hours);
    printf("  Error Count:         %u\n", stats.error_count);
    printf("\n");

    printf("PV Production Forecast:\n");
    printf("  Next Hour:           %.3f kWh\n", prediction.pv_forecast_next_hour_kwh);
    printf("  Today Total:         %.3f kWh\n", prediction.pv_forecast_today_kwh);
    printf("\n");

    printf("Battery Health Prediction:\n");
    printf("  Cycles Remaining:    %.0f\n", prediction.batt_predicted_cycles_remaining);
    printf("  Capacity Fade:       %.2f%%\n", prediction.batt_predicted_capacity_fade * 100.0f);
    printf("\n");

    printf("Maintenance Prediction:\n");
    printf("  Days Until Service:  %u\n", prediction.days_until_maintenance);
    printf("  Reason:              %s\n", prediction.maintenance_reason);
    printf("\n");

    return 0;
}

//============================================================================
// COMMAND: mram-dump
//============================================================================

static int cmd_mram_dump(int argc, char **argv)
{
    uint16_t count = 10;  // Default: show last 10 entries

    // Parse count argument
    if (argc > 1) {
        count = (uint16_t)atoi(argv[1]);
        if (count == 0 || count > MRAM_HISTORY_MAX_ENTRIES) {
            printf("Error: count must be between 1 and %u\n", MRAM_HISTORY_MAX_ENTRIES);
            return 1;
        }
    }

    // Read history entries
    mram_history_entry_t *entries = malloc(count * sizeof(mram_history_entry_t));
    if (!entries) {
        printf("Error: Failed to allocate memory\n");
        return 1;
    }

    uint16_t count_read = 0;
    esp_err_t ret = mram_history_read(entries, count, &count_read);
    if (ret != ESP_OK) {
        printf("Error reading history: %s\n", esp_err_to_name(ret));
        free(entries);
        return 1;
    }

    if (count_read == 0) {
        printf("No history entries available\n");
        free(entries);
        return 0;
    }

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                                   HISTORICAL LOG ENTRIES                                     ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Showing %u most recent entries:\n\n", count_read);

    for (uint16_t i = 0; i < count_read; i++) {
        mram_history_entry_t *e = &entries[i];

        printf("Entry #%u:\n", i + 1);
        printf("  Timestamp:     ");
        print_timestamp(e->timestamp);
        printf("\n");

        printf("  Environment:\n");
        printf("    Ambient Lux:    %d lux\n", e->ambient_lux);
        printf("    Temperature:    %.2f°C\n", e->temperature / 100.0f);
        printf("    PV Voltage:     %.2fV\n", e->pv_voltage / 100.0f);
        printf("    Weather:        %s\n", get_weather_string(e->weather_state));
        printf("    Time Period:    %s\n", get_time_period_string(e->time_period));

        printf("  Battery:\n");
        printf("    SOC:            %.2f%%\n", e->bms_soc / 100.0f);
        printf("    Voltage:        %.2fV\n", e->bms_voltage / 100.0f);
        printf("    Current:        %.2fA\n", e->bms_current / 100.0f);
        printf("    Temperature:    %.2f°C\n", e->bms_temperature / 100.0f);

        printf("  Power:\n");
        printf("    V2G Power:      %dW\n", e->v2g_power);
        printf("    V2G Profile:    %u\n", e->v2g_profile_id);
        printf("    LED Warm:       %u\n", e->led_warm);
        printf("    LED Cool:       %u\n", e->led_cool);
        printf("    LED Profile:    %u\n", e->led_profile_id);

        printf("  System:\n");
        printf("    Board Temp:     %.2f°C\n", e->board_temp / 100.0f);
        printf("    Heatsink Temp:  %.2f°C\n", e->heatsink_temp / 100.0f);

        printf("\n");
    }

    free(entries);
    return 0;
}

//============================================================================
// COMMAND REGISTRATION
//============================================================================

esp_err_t cli_mram_register(void)
{
    esp_err_t ret;

    // Register mram-info command
    const esp_console_cmd_t info_cmd = {
        .command = "mram-info",
        .help = "Display MRAM configuration and system info",
        .hint = NULL,
        .func = &cmd_mram_info,
    };
    ret = esp_console_cmd_register(&info_cmd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register mram-info command: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register mram-stats command
    const esp_console_cmd_t stats_cmd = {
        .command = "mram-stats",
        .help = "Show statistics and predictions",
        .hint = NULL,
        .func = &cmd_mram_stats,
    };
    ret = esp_console_cmd_register(&stats_cmd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register mram-stats command: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register mram-dump command
    const esp_console_cmd_t dump_cmd = {
        .command = "mram-dump",
        .help = "Dump recent historical entries",
        .hint = "[count]",
        .func = &cmd_mram_dump,
    };
    ret = esp_console_cmd_register(&dump_cmd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register mram-dump command: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "MRAM CLI commands registered");
    return ESP_OK;
}
