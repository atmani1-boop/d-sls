/**
 * @file fusion.c
 * @brief DIAMANT v2.1 REV E - Priority-Based Profile Fusion Logic
 * @version 2.1.0
 * @date 2026-02-17
 * 
 * Implements intelligent profile selection based on sensor data and priorities
 */

#include "include/profiles.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// For logging
#ifdef ESP_PLATFORM
#include "esp_log.h"
static const char *TAG = "FUSION";
#else
#include <stdio.h>
#define ESP_LOGI(tag, fmt, ...) printf("[%s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) printf("[%s] WARN: " fmt "\n", tag, ##__VA_ARGS__)
#endif

// ============================================================================
// FUSION DATA STRUCTURES
// ============================================================================

typedef enum {
    TIME_NIGHT_DEEP,        // 00:00-03:00
    TIME_EARLY_MORNING,     // 03:00-sunrise
    TIME_DAY,               // Sunrise-sunset
    TIME_TWILIGHT,          // Sunset+30min
    TIME_EVENING,           // After twilight
} time_period_t;

typedef enum {
    SEASON_SPRING,
    SEASON_SUMMER,
    SEASON_AUTUMN,
    SEASON_WINTER,
} season_t;

typedef enum {
    WEATHER_CLEAR,
    WEATHER_CLOUDY,
    WEATHER_FOG,
    WEATHER_RAIN,
} weather_state_t;

typedef struct {
    float soc_percent;              // Battery SOC (0-100%)
    float voltage;                  // Pack voltage
    float current;                  // Pack current (+ charge, - discharge)
    float temp_min;                 // Min cell temp
    float temp_max;                 // Max cell temp
} bms_data_t;

typedef struct {
    weather_state_t state;          // Current weather condition
    float lux;                      // Ambient light (TSL2591)
    float pv_voltage;               // PV voltage
    float pv_current;               // PV current
    bool pv_drop_60pct_5min;       // Rapid PV drop detection
    float pv_fluctuation;           // PV fluctuation rate
} weather_data_t;

typedef struct {
    season_t season;                // Current season
    bool is_weekend;                // Weekend flag
    bool is_holiday;                // Holiday flag
    uint8_t hour;                   // Hour 0-23
    uint8_t minute;                 // Minute 0-59
} calendar_data_t;

typedef struct {
    float temp_board;               // Board temp
    float temp_heatsink;            // Heatsink temp
    float temp_battery;             // Battery temp
    bool overheat_flag;             // Thermal limit active
} thermal_data_t;

typedef struct {
    bms_data_t bms;
    weather_data_t weather;
    calendar_data_t calendar;
    thermal_data_t thermal;
    time_period_t time_period;
    led_profile_t active_led_profile;
    mppt_profile_t active_mppt_profile;
    v2g_profile_t active_v2g_profile;
} fusion_data_t;

// ============================================================================
// GLOBAL FUSION DATA
// ============================================================================

static fusion_data_t g_fusion_data = {0};

/**
 * @brief Get pointer to fusion data
 * @return Pointer to current fusion data
 */
fusion_data_t* fusion_get_data(void) {
    return &g_fusion_data;
}

// ============================================================================
// LED PROFILE SELECTION
// ============================================================================

/**
 * @brief Select LED profile with priority-based fusion
 * @param fusion Current fusion data
 * @return Selected LED profile
 * 
 * Priority order (lower number = higher priority):
 * P0: Safety overrides (EMERGENCY, CRITICAL)
 * P2: Weather safety (FOG, RAIN)
 * P3: SOC protection
 * P5: Astronomical events
 * P6: Calendar events
 * P7: Default operation
 */
led_profile_t fusion_select_led_profile(const fusion_data_t *fusion) {
    uint8_t max_priority = 255;
    led_profile_t selected = LED_CLEAR_NIGHT;
    
    // P3: SOC critical protection
    if (fusion->bms.soc_percent < 20.0f) {
#ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "LED Profile: SOC<20%% → LOW_SOC_PROTECT (P3)");
#endif
        return LED_LOW_SOC_PROTECT;  // P3
    }
    
    // P2: Weather safety (no Night-Saver)
    if (fusion->weather.state == WEATHER_FOG && fusion->weather.lux < 0.5f) {
#ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "LED Profile: Fog detected → FOG_BOOST (P2)");
#endif
        return LED_FOG_BOOST;  // P2
    }
    
    if (fusion->weather.pv_drop_60pct_5min) {
#ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "LED Profile: Rain detected → RAIN_ALERT (P2)");
#endif
        return LED_RAIN_ALERT;  // P2
    }
    
    // P5: Astronomical profiles (Night-Saver enabled)
    if (fusion->time_period == TIME_TWILIGHT) {
        selected = LED_TWILIGHT_SOFT;
        max_priority = PRIORITY_ASTRO;
    } else if (fusion->time_period == TIME_NIGHT_DEEP && fusion->bms.soc_percent > 40.0f) {
        selected = LED_MIDNIGHT_ECO;
        max_priority = PRIORITY_ASTRO;
    } else if (fusion->time_period == TIME_EARLY_MORNING) {
        selected = LED_EARLY_MORNING;
        max_priority = PRIORITY_ASTRO;
    } else if (fusion->time_period == TIME_EVENING) {
        selected = LED_SUNSET_ON;
        max_priority = PRIORITY_ASTRO;
    }
    
    // P6: Calendar overrides (Night-Saver enabled)
    if (fusion->calendar.season == SEASON_WINTER && max_priority >= PRIORITY_CALENDAR) {
        selected = LED_WINTER_MODE;
        max_priority = PRIORITY_CALENDAR;
    } else if (fusion->calendar.season == SEASON_SUMMER && max_priority >= PRIORITY_CALENDAR) {
        selected = LED_SUMMER_MODE;
        max_priority = PRIORITY_CALENDAR;
    }
    
#ifdef ESP_PLATFORM
    ESP_LOGI(TAG, "LED Profile: 0x%02X %s (P%d)", 
             led_configs[selected].id_hex,
             led_configs[selected].name,
             led_configs[selected].priority);
#endif
    
    return selected;
}

// ============================================================================
// MPPT PROFILE SELECTION
// ============================================================================

/**
 * @brief Select MPPT profile based on weather and thermal conditions
 * @param fusion Current fusion data
 * @return Selected MPPT profile
 */
mppt_profile_t fusion_select_mppt_profile(const fusion_data_t *fusion) {
    // P0: Thermal safety
    if (fusion->thermal.temp_battery < 0.0f) {
#ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "MPPT Profile: Battery<0°C → SAFE_CHARGE (P0)");
#endif
        return MPPT_SAFE_CHARGE;
    }
    
    if (fusion->thermal.temp_battery < 5.0f) {
#ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "MPPT Profile: Battery<5°C → WINTER_PROTECT (P0)");
#endif
        return MPPT_WINTER_PROTECT;
    }
    
    // P2: Weather-based selection
    if (fusion->weather.state == WEATHER_CLOUDY && fusion->weather.pv_fluctuation > 0.2f) {
#ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "MPPT Profile: High PV fluctuation → CLOUD_EDGE_BOOST (P0)");
#endif
        return MPPT_CLOUD_EDGE_BOOST;
    }
    
    if (fusion->weather.state == WEATHER_CLOUDY) {
#ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "MPPT Profile: Cloudy → ADAPTIVE_CLOUDY (P0)");
#endif
        return MPPT_ADAPTIVE_CLOUDY;
    }
    
    // Low light conditions
    if (fusion->weather.lux < 100.0f || 
        fusion->time_period == TIME_EARLY_MORNING || 
        fusion->time_period == TIME_EVENING) {
#ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "MPPT Profile: Low light → LOW_LIGHT (P0)");
#endif
        return MPPT_LOW_LIGHT;
    }
    
    // Clear sky - aggressive tracking
#ifdef ESP_PLATFORM
    ESP_LOGI(TAG, "MPPT Profile: Clear sky → AGGRESSIVE_SUNNY (P0)");
#endif
    return MPPT_AGGRESSIVE_SUNNY;
}

// ============================================================================
// V2G PROFILE SELECTION
// ============================================================================

/**
 * @brief Select V2G profile based on SOC, weather, and grid conditions
 * @param fusion Current fusion data
 * @return Selected V2G profile
 */
v2g_profile_t fusion_select_v2g_profile(const fusion_data_t *fusion) {
    // P0: Emergency island mode (grid loss detection would go here)
    // For now, checking basic conditions
    
    // P3: Import if SOC critical
    if (fusion->bms.soc_percent < 20.0f) {
#ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "V2G Profile: SOC<20%% → IMPORT_LOW_SOC (P3)");
#endif
        return V2G_IMPORT_LOW_SOC;
    }
    
    // Export with high SOC and good PV
    if (fusion->bms.soc_percent > 80.0f && fusion->weather.state == WEATHER_CLEAR) {
#ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "V2G Profile: SOC>80%% + Clear → EXPORT_DAY_SUNNY (P0)");
#endif
        return V2G_EXPORT_DAY_SUNNY;
    }
    
    // Export with moderate SOC and variable conditions
    if (fusion->bms.soc_percent > 60.0f && fusion->weather.pv_fluctuation > 0.1f) {
#ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "V2G Profile: SOC>60%% + Clouds → EXPORT_CLOUD_EDGE (P0)");
#endif
        return V2G_EXPORT_CLOUD_EDGE;
    }
    
    // Import in winter with low SOC
    if (fusion->calendar.season == SEASON_WINTER && fusion->bms.soc_percent < 60.0f) {
#ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "V2G Profile: Winter + SOC<60%% → IMPORT_WINTER (P6)");
#endif
        return V2G_IMPORT_WINTER;
    }
    
    // Balanced hold
#ifdef ESP_PLATFORM
    ESP_LOGI(TAG, "V2G Profile: Balanced mode → HOLD_BALANCED (P7)");
#endif
    return V2G_HOLD_BALANCED;
}

// ============================================================================
// UPDATE FUNCTIONS
// ============================================================================

/**
 * @brief Update all active profiles based on current conditions
 * @param fusion Fusion data structure to update
 */
void fusion_update_profiles(fusion_data_t *fusion) {
    fusion->active_led_profile = fusion_select_led_profile(fusion);
    fusion->active_mppt_profile = fusion_select_mppt_profile(fusion);
    fusion->active_v2g_profile = fusion_select_v2g_profile(fusion);
}
