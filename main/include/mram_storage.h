/**
 * @file mram_storage.h
 * @brief DIAMANT v2.1 REV E - MRAM Structured Logging
 * @version 2.1.0
 * @date 2026-02-17
 * 
 * Defines typed log structures for different system subsystems
 */

#ifndef MRAM_STORAGE_H
#define MRAM_STORAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// LOG TYPES
// ============================================================================

typedef enum {
    MRAM_LOG_TYPE_MPPT = 1,         // MPPT tracking, efficiency
    MRAM_LOG_TYPE_V2G = 2,          // Power flow, grid interaction
    MRAM_LOG_TYPE_BMS = 3,          // SOC, voltage, current, cells
    MRAM_LOG_TYPE_THERMAL = 7,      // Temperatures, thermal limits
    MRAM_LOG_TYPE_ASTRO = 9,        // Sunrise/sunset, weather, calendar
    MRAM_LOG_TYPE_SYSTEM = 15,      // System events, faults
} mram_log_type_t;

// ============================================================================
// TYPED LOG ENTRY STRUCTURE
// ============================================================================

typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    uint8_t log_type;               // mram_log_type_t
    uint8_t profile_id;             // Hex ID (0x00-0x62)
    uint8_t priority;               // Fusion priority (P0-P7)
    uint8_t reserved;
    
    union {
        // Type 1: MPPT
        struct {
            uint16_t vpv;           // PV voltage × 10
            uint16_t ipv;           // PV current × 100
            uint8_t duty;           // PWM duty cycle
            uint8_t efficiency;     // MPPT efficiency %
        } mppt;
        
        // Type 2: V2G
        struct {
            int16_t power_w;        // Power (+ export, - import)
            uint16_t vbus;          // Bus voltage × 10
            uint8_t mode;           // 0=export, 1=import, 2=hold
            uint8_t flags;          // Status flags
        } v2g;
        
        // Type 3: BMS
        struct {
            uint8_t soc;            // State of charge %
            uint16_t voltage;       // Pack voltage × 10
            int16_t current;        // Pack current mA
            uint8_t temp_min;       // Min cell temp °C
            uint8_t temp_max;       // Max cell temp °C
        } bms;
        
        // Type 7: Thermal
        struct {
            int16_t temp_board;     // Board temp × 10
            int16_t temp_heatsink;  // Heatsink temp × 10
            int16_t temp_battery;   // Battery temp × 10
            uint8_t thermal_flags;  // Limit flags
        } thermal;
        
        // Type 9: Astro
        struct {
            uint16_t sunrise_min;   // Minutes from midnight
            uint16_t sunset_min;
            uint16_t lux;           // Ambient light
            uint8_t weather_state;  // Weather enum
            uint8_t calendar_flags; // Weekend/holiday
        } astro;
        
        uint8_t raw[8];             // Raw data
    } data;
} mram_log_entry_typed_t;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * @brief Get size of a typed log entry
 * @return Size in bytes (16 bytes)
 */
static inline size_t mram_log_entry_size(void) {
    return sizeof(mram_log_entry_typed_t);
}

/**
 * @brief Validate log type
 * @param log_type Type to validate
 * @return true if valid
 */
static inline bool mram_log_type_valid(uint8_t log_type) {
    return (log_type == MRAM_LOG_TYPE_MPPT ||
            log_type == MRAM_LOG_TYPE_V2G ||
            log_type == MRAM_LOG_TYPE_BMS ||
            log_type == MRAM_LOG_TYPE_THERMAL ||
            log_type == MRAM_LOG_TYPE_ASTRO ||
            log_type == MRAM_LOG_TYPE_SYSTEM);
}

#ifdef __cplusplus
}
#endif

#endif // MRAM_STORAGE_H
