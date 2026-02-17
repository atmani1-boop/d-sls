/**
 * @file mram_storage.h
 * @brief MRAM storage system for AT24C256 EEPROM (32KB)
 * 
 * Memory layout and management for persistent data storage including
 * configuration, historical logs, predictive analytics, and client profiles.
 */

#ifndef MRAM_STORAGE_H
#define MRAM_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "profiles.h"

//============================================================================
// MEMORY LAYOUT (AT24C256 - 32KB)
//============================================================================

#define MRAM_SIZE                   32768   // 32KB total

// Memory regions
#define MRAM_ADDR_CONFIG            0x0000  // Configuration (256B)
#define MRAM_ADDR_CLIENT_PROFILES   0x0100  // Client profiles (256B, 4×64B)
#define MRAM_ADDR_HISTORY           0x0200  // Historical logs (7680B)
#define MRAM_ADDR_PREDICTION        0x2000  // Prediction models (4KB)
#define MRAM_ADDR_MAINTENANCE       0x3000  // Maintenance logs (4KB)
#define MRAM_ADDR_EXTENDED          0x4000  // Extended data (16KB)

// Region sizes
#define MRAM_SIZE_CONFIG            256
#define MRAM_SIZE_CLIENT_PROFILES   256
#define MRAM_SIZE_HISTORY           7680
#define MRAM_SIZE_PREDICTION        4096
#define MRAM_SIZE_MAINTENANCE       4096
#define MRAM_SIZE_EXTENDED          16384

// History buffer
#define MRAM_HISTORY_ENTRY_SIZE     32      // Bytes per entry
#define MRAM_HISTORY_MAX_ENTRIES    (MRAM_SIZE_HISTORY / MRAM_HISTORY_ENTRY_SIZE)  // 240 entries

// Client profiles
#define MRAM_CLIENT_PROFILE_SIZE    64
#define MRAM_CLIENT_MAX_PROFILES    4

//============================================================================
// CONFIGURATION STRUCTURE
//============================================================================

#define MRAM_CONFIG_MAGIC           0x44494D41  // "DIMA" in ASCII

typedef struct __attribute__((packed)) {
    uint32_t magic;                 /**< Magic number for validation */
    uint16_t version;               /**< Config version */
    uint16_t crc16;                 /**< CRC16-CCITT checksum */
    
    // Device identity
    char device_id[32];             /**< Device ID string */
    char location[32];              /**< Installation location */
    
    // System configuration
    float latitude;                 /**< Location latitude */
    float longitude;                /**< Location longitude */
    int8_t timezone_offset;         /**< Timezone offset from UTC */
    
    // Operational parameters
    uint16_t log_interval_sec;      /**< Data logging interval */
    uint16_t stats_interval_sec;    /**< Statistics calculation interval */
    
    // Thresholds
    float batt_low_threshold;       /**< Low battery threshold (%) */
    float batt_high_threshold;      /**< High battery threshold (%) */
    float temp_max_threshold;       /**< Maximum temperature (°C) */
    
    // Counters
    uint32_t total_runtime_hours;   /**< Total device runtime */
    uint32_t boot_count;            /**< Number of boots */
    
    // History buffer state
    uint16_t history_write_index;   /**< Circular buffer write position */
    uint16_t history_count;         /**< Number of valid entries */
    
    // Reserved for future use
    uint8_t reserved[126];
} mram_config_t;

//============================================================================
// HISTORICAL LOG ENTRY (32 bytes - compressed format)
//============================================================================

typedef struct __attribute__((packed)) {
    uint32_t timestamp;             /**< Unix timestamp (4B) */
    
    // Environmental (8B)
    int16_t ambient_lux;            /**< Ambient light (scaled) */
    int16_t temperature;            /**< Temperature * 100 */
    int16_t pv_voltage;             /**< PV voltage * 100 */
    uint8_t weather_state;          /**< Weather state enum */
    uint8_t time_period;            /**< Time period enum */
    
    // Battery (8B)
    uint16_t bms_soc;               /**< SOC * 100 */
    int16_t bms_voltage;            /**< Voltage * 100 */
    int16_t bms_current;            /**< Current * 100 */
    int16_t bms_temperature;        /**< Temperature * 100 */
    
    // Power (6B)
    int16_t v2g_power;              /**< V2G power (W) */
    uint8_t v2g_profile_id;         /**< Active V2G profile */
    uint8_t led_warm;               /**< Warm LED intensity */
    uint8_t led_cool;               /**< Cool LED intensity */
    uint8_t led_profile_id;         /**< Active LED profile */
    
    // System (4B)
    int16_t board_temp;             /**< Board temp * 100 */
    int16_t heatsink_temp;          /**< Heatsink temp * 100 */
    
    // Reserved (2B)
    uint16_t reserved;
} mram_history_entry_t;

//============================================================================
// CLIENT PROFILE (64 bytes)
//============================================================================

typedef struct __attribute__((packed)) {
    char name[32];                  /**< Profile name */
    uint8_t v2g_profile_id;         /**< Preferred V2G profile */
    uint8_t led_profile_id;         /**< Preferred LED profile */
    uint8_t priority;               /**< Profile priority */
    uint8_t enabled;                /**< Profile enabled flag */
    uint8_t schedule_mask;          /**< Days of week (bit mask) */
    uint8_t start_hour;             /**< Start hour (0-23) */
    uint8_t end_hour;               /**< End hour (0-23) */
    uint8_t reserved[25];           /**< Reserved for future use */
} mram_client_profile_t;

//============================================================================
// STATISTICS STRUCTURE
//============================================================================

typedef struct {
    // PV statistics
    float pv_energy_kwh_today;
    float pv_energy_kwh_week;
    float pv_peak_power_w;
    float pv_avg_voltage;
    
    // Battery statistics
    float batt_cycles_total;
    float batt_energy_charged_kwh;
    float batt_energy_discharged_kwh;
    float batt_health_percent;
    
    // V2G statistics
    float v2g_export_kwh_today;
    float v2g_import_kwh_today;
    float v2g_net_kwh;
    
    // LED statistics
    float led_energy_kwh_today;
    float led_avg_runtime_hours;
    
    // System statistics
    float uptime_hours;
    uint32_t error_count;
} mram_stats_t;

//============================================================================
// PREDICTIVE ANALYTICS
//============================================================================

typedef struct {
    // PV production forecast
    float pv_forecast_next_hour_kwh;
    float pv_forecast_today_kwh;
    
    // Battery health prediction
    float batt_predicted_cycles_remaining;
    float batt_predicted_capacity_fade;
    
    // Maintenance prediction
    uint32_t days_until_maintenance;
    char maintenance_reason[64];
} mram_prediction_t;

//============================================================================
// FUNCTION PROTOTYPES
//============================================================================

/**
 * @brief Initialize MRAM storage system
 * @return ESP_OK on success
 */
esp_err_t mram_storage_init(void);

/**
 * @brief Load configuration from MRAM
 * @param config Pointer to configuration structure
 * @return ESP_OK on success
 */
esp_err_t mram_config_load(mram_config_t *config);

/**
 * @brief Save configuration to MRAM
 * @param config Pointer to configuration structure
 * @return ESP_OK on success
 */
esp_err_t mram_config_save(const mram_config_t *config);

/**
 * @brief Format MRAM with default configuration
 * @return ESP_OK on success
 */
esp_err_t mram_config_format(void);

/**
 * @brief Add historical log entry
 * @param entry Pointer to log entry
 * @return ESP_OK on success
 */
esp_err_t mram_history_add(const mram_history_entry_t *entry);

/**
 * @brief Read historical log entries
 * @param entries Output buffer for entries
 * @param max_count Maximum number of entries to read
 * @param count_out Actual number of entries read
 * @return ESP_OK on success
 */
esp_err_t mram_history_read(mram_history_entry_t *entries, uint16_t max_count, uint16_t *count_out);

/**
 * @brief Calculate statistics from historical data
 * @param stats Output statistics structure
 * @return ESP_OK on success
 */
esp_err_t mram_stats_calculate(mram_stats_t *stats);

/**
 * @brief Generate predictive analytics
 * @param prediction Output prediction structure
 * @return ESP_OK on success
 */
esp_err_t mram_prediction_generate(mram_prediction_t *prediction);

/**
 * @brief Save client profile
 * @param index Profile index (0-3)
 * @param profile Pointer to profile structure
 * @return ESP_OK on success
 */
esp_err_t mram_client_profile_save(uint8_t index, const mram_client_profile_t *profile);

/**
 * @brief Load client profile
 * @param index Profile index (0-3)
 * @param profile Output profile structure
 * @return ESP_OK on success
 */
esp_err_t mram_client_profile_load(uint8_t index, mram_client_profile_t *profile);

/**
 * @brief Calculate CRC16-CCITT checksum
 * @param data Pointer to data
 * @param length Data length
 * @return CRC16 checksum
 */
uint16_t mram_crc16(const uint8_t *data, size_t length);

/**
 * @brief Test MRAM functionality
 * @return ESP_OK on success
 */
esp_err_t mram_test(void);

#endif // MRAM_STORAGE_H
