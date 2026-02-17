/**
 * @file profiles.h
 * @brief DIAMANT v2.1 REV E - Complete 35-Profile Specification
 * @version 2.1.0
 * @date 2026-02-17
 * 
 * Defines all system profiles with hexadecimal IDs, priorities, and fusion logic
 */

#ifndef PROFILES_H
#define PROFILES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// LED PROFILES (9 profiles: 0x00-0x09)
// ============================================================================

typedef enum {
    LED_SUNSET_ON = 0x00,           // 100%, 2700K, P5 Astro
    LED_TWILIGHT_SOFT = 0x01,       // 80%, 2800K, P5 Astro
    LED_MIDNIGHT_ECO = 0x02,        // 60%, 3000K, P5 Astro+SOC
    LED_EARLY_MORNING = 0x03,       // 40%, 4000K, P5 Astro
    LED_FOG_BOOST = 0x04,           // 100%, 5000K, P2 Weather (no Night-Saver)
    LED_CLEAR_NIGHT = 0x05,         // 60%, 3000K, P5 Astro+Weather
    LED_LOW_SOC_PROTECT = 0x06,     // 30%, 2700K, P3 SOC
    LED_WINTER_MODE = 0x07,         // 70%, 3000K, P6 Calendar
    LED_SUMMER_MODE = 0x08,         // 50%, 2700K, P6 Calendar
    LED_RAIN_ALERT = 0x09,          // 90%, 5000K, P2 Weather (no Night-Saver)
} led_profile_t;

typedef struct {
    led_profile_t profile;
    uint8_t id_hex;                 // Hex ID for traceability
    const char *name;
    uint8_t intensity;              // Base intensity 0-100%
    uint16_t cct_kelvin;            // Color temperature
    uint8_t priority;               // P0-P7 fusion priority
    bool night_saver_enabled;       // Apply SOC factor?
    const char *activation_conditions;
    const char *sensor_inputs;
} led_config_t;

// LED Profile Configurations
static const led_config_t led_configs[] = {
    {LED_SUNSET_ON, 0x00, "SUNSET_ON", 100, 2700, 5, true, 
     "Au coucher du soleil", "DS3231+Meeus"},
    {LED_TWILIGHT_SOFT, 0x01, "TWILIGHT_SOFT", 80, 2800, 5, true,
     "Crépuscule civil +30min", "DS3231+Meeus"},
    {LED_MIDNIGHT_ECO, 0x02, "MIDNIGHT_ECO", 60, 3000, 5, true,
     "00:00-03:00 · SOC>40%", "DS3231+BQ76952"},
    {LED_EARLY_MORNING, 0x03, "EARLY_MORNING", 40, 4000, 5, true,
     "03:00→sunrise", "DS3231+Meeus"},
    {LED_FOG_BOOST, 0x04, "FOG_BOOST", 100, 5000, 2, false,
     "TSL2591<0.5lux+PV low", "TSL2591+ADS1115"},
    {LED_CLEAR_NIGHT, 0x05, "CLEAR_NIGHT", 60, 3000, 5, true,
     "Nuit+ciel clair", "TSL2591"},
    {LED_LOW_SOC_PROTECT, 0x06, "LOW_SOC_PROTECT", 30, 2700, 3, false,
     "SOC<20%", "BQ76952"},
    {LED_WINTER_MODE, 0x07, "WINTER_MODE", 70, 3000, 6, true,
     "Saison hiver", "DS3231+MRAM"},
    {LED_SUMMER_MODE, 0x08, "SUMMER_MODE", 50, 2700, 6, true,
     "Saison été", "DS3231+MRAM"},
    {LED_RAIN_ALERT, 0x09, "RAIN_ALERT", 90, 5000, 2, false,
     "PV drop>60% <5min", "ADS1115+TSL2591"},
};

// ============================================================================
// MPPT PROFILES (7 profiles: 0x10-0x16)
// ============================================================================

typedef enum {
    MPPT_AGGRESSIVE_SUNNY = 0x10,   // Clear sky, stable PV
    MPPT_ADAPTIVE_CLOUDY = 0x11,    // Variable clouds, fluctuating PV
    MPPT_LOW_LIGHT = 0x12,          // Morning/evening/winter
    MPPT_WINTER_PROTECT = 0x13,     // Cold < 5°C
    MPPT_SAFE_CHARGE = 0x14,        // Very cold < 0°C LFP / 5°C NMC
    MPPT_CLOUD_EDGE_BOOST = 0x15,   // Intermittent clouds, PV peaks
    MPPT_EVENING_SAVE = 0x16,       // After sunset, residual PV
} mppt_profile_t;

typedef struct {
    mppt_profile_t profile;
    uint8_t id_hex;
    const char *name;
    float step_multiplier;          // P&O step size multiplier
    float update_period_multiplier; // Update period multiplier
    float max_current_limit;        // Max charge current (C-rate)
    uint8_t priority;
    const char *conditions;
    const char *sensor_inputs;
} mppt_config_t;

// MPPT Profile Configurations
static const mppt_config_t mppt_configs[] = {
    {MPPT_AGGRESSIVE_SUNNY, 0x10, "AGGRESSIVE_SUNNY", 0.5, 1.0, 1.0, 0,
     "Ciel clair·PV stable·TSL2591 élevé", "TSL2591+ADS1115"},
    {MPPT_ADAPTIVE_CLOUDY, 0x11, "ADAPTIVE_CLOUDY", 1.0, 1.0, 1.0, 0,
     "Nuages variables·PV fluctuant", "ADS1115 PV ratio<0.6"},
    {MPPT_LOW_LIGHT, 0x12, "LOW_LIGHT", 2.0, 3.0, 1.0, 0,
     "Matin/soir/hiver·faible irradiance", "DS3231+ADS1115"},
    {MPPT_WINTER_PROTECT, 0x13, "WINTER_PROTECT", 1.0, 1.0, 0.5, 0,
     "Froid TMP117<5°C", "TMP117#2"},
    {MPPT_SAFE_CHARGE, 0x14, "SAFE_CHARGE", 1.0, 1.0, 0.1, 0,
     "Batterie froide T<0°C LFP/5°C NMC", "TMP117#2+BQ76952"},
    {MPPT_CLOUD_EDGE_BOOST, 0x15, "CLOUD_EDGE_BOOST", 1.0, 0.5, 1.0, 0,
     "Nuages intermittents·pics PV courts", "ADS1115 oscillation>20%/min"},
    {MPPT_EVENING_SAVE, 0x16, "EVENING_SAVE", 0.0, 10.0, 0.1, 0,
     "Après sunset·PV résiduel", "DS3231 sunset+PV<10%"},
};

// ============================================================================
// V2G PROFILES (7 profiles: 0x20-0x26)
// ============================================================================

typedef enum {
    V2G_EXPORT_DAY_SUNNY = 0x20,    // Clear + PV high + SOC>80%
    V2G_EXPORT_CLOUD_EDGE = 0x21,   // Intermittent clouds + SOC>60%
    V2G_IMPORT_LOW_SOC = 0x22,      // SOC<20%
    V2G_IMPORT_WINTER = 0x23,       // Winter + low PV + SOC<60%
    V2G_HOLD_BALANCED = 0x24,       // SOC 40-70%, no special condition
    V2G_DISCHARGE_PEAK = 0x25,      // Peak hours (configurable)
    V2G_EMERGENCY_ISLAND = 0x26,    // Grid loss (LTC4364 fault)
} v2g_profile_t;

typedef struct {
    v2g_profile_t profile;
    uint8_t id_hex;
    const char *name;
    int16_t power_w;                // Power flow (+ export, - import)
    uint8_t priority;
    const char *conditions;
    const char *sensor_inputs;
} v2g_config_t;

// V2G Profile Configurations
static const v2g_config_t v2g_configs[] = {
    {V2G_EXPORT_DAY_SUNNY, 0x20, "EXPORT_DAY_SUNNY", 500, 0,
     "Ciel clair+PV fort+SOC>80%", "TSL2591+ADS1115+BQ76952"},
    {V2G_EXPORT_CLOUD_EDGE, 0x21, "EXPORT_CLOUD_EDGE", 200, 0,
     "Nuages intermittents+SOC>60%", "ADS1115 oscillation+BQ76952"},
    {V2G_IMPORT_LOW_SOC, 0x22, "IMPORT_LOW_SOC", -300, 3,
     "SOC<20%", "BQ76952"},
    {V2G_IMPORT_WINTER, 0x23, "IMPORT_WINTER", -150, 6,
     "Hiver·PV faible·SOC<60%", "DS3231+ADS1115+SOC"},
    {V2G_HOLD_BALANCED, 0x24, "HOLD_BALANCED", 0, 7,
     "SOC 40-70%·pas de condition spéciale", "BQ76952"},
    {V2G_DISCHARGE_PEAK, 0x25, "DISCHARGE_PEAK", 350, 6,
     "Heures de pointe réseau", "DS3231+MRAM calendrier"},
    {V2G_EMERGENCY_ISLAND, 0x26, "EMERGENCY_ISLAND", 0, 0,
     "Perte réseau LTC4364 fault", "ADS1115 V_BUS+LTC4364"},
};

// ============================================================================
// BATTERY PROFILES (4 profiles: 0x30-0x33)
// ============================================================================

typedef enum {
    BATTERY_SAFE_CHARGE = 0x30,     // T<0°C LFP or T<5°C NMC
    BATTERY_SUMMER_LIMIT = 0x31,    // T>40°C heatsink
    BATTERY_WINTER_LIMIT = 0x32,    // Winter season
    BATTERY_CRITICAL_SOC = 0x33,    // SOC<10%
} battery_profile_t;

typedef struct {
    battery_profile_t profile;
    uint8_t id_hex;
    const char *name;
    float charge_limit;             // Charge current limit multiplier
    uint8_t priority;
    const char *conditions;
} battery_config_t;

// Battery Profile Configurations
static const battery_config_t battery_configs[] = {
    {BATTERY_SAFE_CHARGE, 0x30, "SAFE_CHARGE", 0.1, 0,
     "T<0°C LFP ou T<5°C NMC"},
    {BATTERY_SUMMER_LIMIT, 0x31, "SUMMER_LIMIT", 0.5, 1,
     "T>40°C dissipateur thermique"},
    {BATTERY_WINTER_LIMIT, 0x32, "WINTER_LIMIT", 0.7, 2,
     "Saison hiver"},
    {BATTERY_CRITICAL_SOC, 0x33, "CRITICAL_SOC", 0.05, 0,
     "SOC<10% protection batterie"},
};

// ============================================================================
// WEATHER DETECTION PROFILES (4 profiles: 0x40-0x43)
// ============================================================================

typedef enum {
    WEATHER_FOG_DETECT = 0x40,      // TSL2591<0.5lux + PV low
    WEATHER_CLOUD_DETECT = 0x41,    // PV ratio<0.6 fluctuating
    WEATHER_RAIN_DETECT = 0x42,     // PV drop>60% in <5min
    WEATHER_HEAT_ALERT = 0x43,      // TMP117>45°C
} weather_detect_t;

typedef struct {
    weather_detect_t profile;
    uint8_t id_hex;
    const char *name;
    const char *detection_criteria;
    const char *sensor_inputs;
} weather_config_t;

// Weather Detection Profile Configurations
static const weather_config_t weather_configs[] = {
    {WEATHER_FOG_DETECT, 0x40, "FOG_DETECT",
     "TSL2591<0.5lux + PV faible", "TSL2591+ADS1115"},
    {WEATHER_CLOUD_DETECT, 0x41, "CLOUD_DETECT",
     "PV ratio<0.6 fluctuant", "ADS1115 PV monitoring"},
    {WEATHER_RAIN_DETECT, 0x42, "RAIN_DETECT",
     "PV chute>60% en <5min", "ADS1115 PV+TSL2591"},
    {WEATHER_HEAT_ALERT, 0x43, "HEAT_ALERT",
     "TMP117>45°C surchauffe", "TMP117#2 heatsink"},
};

// ============================================================================
// ASTRONOMICAL PROFILES (3 profiles: 0x50-0x52)
// ============================================================================

typedef enum {
    ASTRO_SUNRISE = 0x50,           // Sunrise calculation (Meeus)
    ASTRO_SUNSET = 0x51,            // Sunset calculation (Meeus)
    ASTRO_DAY_LENGTH = 0x52,        // Day duration
} astro_event_t;

typedef struct {
    astro_event_t profile;
    uint8_t id_hex;
    const char *name;
    const char *calculation_method;
} astro_config_t;

// Astronomical Profile Configurations
static const astro_config_t astro_configs[] = {
    {ASTRO_SUNRISE, 0x50, "SUNRISE", "Algorithme Meeus DS3231"},
    {ASTRO_SUNSET, 0x51, "SUNSET", "Algorithme Meeus DS3231"},
    {ASTRO_DAY_LENGTH, 0x52, "DAY_LENGTH", "Calcul durée jour"},
};

// ============================================================================
// CALENDAR PROFILES (3 profiles: 0x60-0x62)
// ============================================================================

typedef enum {
    CALENDAR_WEEKEND = 0x60,        // Saturday/Sunday
    CALENDAR_HOLIDAY = 0x61,        // Public holidays (MRAM bitfield)
    CALENDAR_SEASON_CHANGE = 0x62,  // Solstice/equinox
} calendar_event_t;

typedef struct {
    calendar_event_t profile;
    uint8_t id_hex;
    const char *name;
    const char *detection_criteria;
} calendar_config_t;

// Calendar Profile Configurations
static const calendar_config_t calendar_configs[] = {
    {CALENDAR_WEEKEND, 0x60, "WEEKEND", "Samedi/Dimanche DS3231"},
    {CALENDAR_HOLIDAY, 0x61, "HOLIDAY", "Jours fériés MRAM bitfield"},
    {CALENDAR_SEASON_CHANGE, 0x62, "SEASON_CHANGE", "Solstice/équinoxe"},
};

// ============================================================================
// PRIORITY LEVELS
// ============================================================================

#define PRIORITY_SAFETY     0   // P0: Emergency (ISLAND, CRITICAL_SOC)
#define PRIORITY_THERMAL    1   // P1: Thermal limits
#define PRIORITY_WEATHER    2   // P2: Weather (FOG, RAIN, HEAT)
#define PRIORITY_SOC        3   // P3: SOC critical
#define PRIORITY_BATTERY    4   // P4: Battery protection
#define PRIORITY_ASTRO      5   // P5: Astronomical events
#define PRIORITY_CALENDAR   6   // P6: Calendar events
#define PRIORITY_DEFAULT    7   // P7: Default/balanced mode

// ============================================================================
// HELPER MACROS
// ============================================================================

#define LED_PROFILE_COUNT   (sizeof(led_configs) / sizeof(led_config_t))
#define MPPT_PROFILE_COUNT  (sizeof(mppt_configs) / sizeof(mppt_config_t))
#define V2G_PROFILE_COUNT   (sizeof(v2g_configs) / sizeof(v2g_config_t))
#define BATTERY_PROFILE_COUNT (sizeof(battery_configs) / sizeof(battery_config_t))
#define WEATHER_PROFILE_COUNT (sizeof(weather_configs) / sizeof(weather_config_t))
#define ASTRO_PROFILE_COUNT (sizeof(astro_configs) / sizeof(astro_config_t))
#define CALENDAR_PROFILE_COUNT (sizeof(calendar_configs) / sizeof(calendar_config_t))

#ifdef __cplusplus
}
#endif

#endif // PROFILES_H
