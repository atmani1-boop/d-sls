/**
 * @file profiles.h
 * @brief Profile definitions for V2G and LED control systems
 * 
 * Defines 19 V2G profiles and 15 LED profiles with intelligent selection
 * based on multiple sensor inputs and operational criteria.
 */

#ifndef PROFILES_H
#define PROFILES_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "esp_err.h"

//============================================================================
// V2G PROFILE DEFINITIONS (19 profiles)
//============================================================================

/** V2G Operation Modes */
typedef enum {
    V2G_MODE_EXPORT = 0,    /**< Export to grid */
    V2G_MODE_IMPORT,        /**< Import from grid */
    V2G_MODE_HOLD,          /**< Hold/idle state */
    V2G_MODE_CHARGE,        /**< Charge battery */
    V2G_MODE_DISCHARGE      /**< Discharge battery */
} v2g_mode_t;

/** V2G Profile Structure */
typedef struct {
    uint8_t id;             /**< Profile ID (0-18) */
    const char *name;       /**< Profile name */
    v2g_mode_t mode;        /**< Operation mode */
    int16_t power_target;   /**< Power target in watts (-500 to +500) */
    uint8_t priority;       /**< Priority (255=highest, 0=lowest) */
    const char *conditions; /**< Human-readable activation conditions */
} v2g_profile_t;

/** V2G Profile IDs */
#define V2G_EMERGENCY_BATT_LOW          0   // Priority 255
#define V2G_EMERGENCY_GRID_FAULT        1   // Priority 254
#define V2G_PEAK_SHAVING                2   // Priority 200
#define V2G_SOLAR_EXPORT_MAX            3   // Priority 180
#define V2G_NIGHT_IMPORT_CHEAP          4   // Priority 160
#define V2G_MORNING_PRECHARGE           5   // Priority 150
#define V2G_AFTERNOON_HOLD              6   // Priority 140
#define V2G_EVENING_DISCHARGE           7   // Priority 130
#define V2G_WEEKEND_ECO                 8   // Priority 120
#define V2G_HOLIDAY_MINIMAL             9   // Priority 110
#define V2G_CLOUDY_IMPORT               10  // Priority 100
#define V2G_INTERMITTENT_HOLD           11  // Priority 90
#define V2G_RAIN_CHARGE_GRID            12  // Priority 85
#define V2G_FOG_MINIMAL                 13  // Priority 80
#define V2G_SUMMER_EXPORT               14  // Priority 70
#define V2G_WINTER_IMPORT               15  // Priority 65
#define V2G_AUTUMN_BALANCE              16  // Priority 60
#define V2G_SPRING_CHARGE               17  // Priority 55
#define V2G_DEFAULT_HOLD                18  // Priority 50

//============================================================================
// LED PROFILE DEFINITIONS (15 profiles)
//============================================================================

/** LED Profile Structure */
typedef struct {
    uint8_t id;                 /**< Profile ID (0-14) */
    const char *name;           /**< Profile name */
    uint8_t warm_intensity;     /**< Warm LED intensity (0-100%) */
    uint8_t cool_intensity;     /**< Cool LED intensity (0-100%) */
    uint8_t priority;           /**< Priority (255=highest, 0=lowest) */
    const char *conditions;     /**< Human-readable activation conditions */
} led_profile_t;

/** LED Profile IDs */
#define LED_EMERGENCY_FAULT             0   // Priority 255
#define LED_SAFETY_MINIMUM              1   // Priority 250
#define LED_SUNSET_ON                   2   // Priority 200
#define LED_TWILIGHT_SOFT               3   // Priority 190
#define LED_MIDNIGHT_ECO                4   // Priority 180
#define LED_DAWN_WARMUP                 5   // Priority 170
#define LED_SUNRISE_OFF                 6   // Priority 160
#define LED_CLOUDY_BOOST                7   // Priority 150
#define LED_RAIN_COMFORT                8   // Priority 140
#define LED_FOG_HIGH                    9   // Priority 135
#define LED_SUNNY_OFF                   10  // Priority 100
#define LED_WEEKEND_DIM                 11  // Priority 90
#define LED_HOLIDAY_AWAY                12  // Priority 85
#define LED_NIGHT_FULL                  13  // Priority 80
#define LED_DEFAULT_AUTO                14  // Priority 50

//============================================================================
// WEATHER STATE
//============================================================================

typedef enum {
    WEATHER_CLEAR = 0,
    WEATHER_CLOUDY,
    WEATHER_INTERMITTENT,
    WEATHER_RAIN,
    WEATHER_FOG,
    WEATHER_UNKNOWN
} weather_state_t;

typedef struct {
    weather_state_t state;
    float ambient_lux;
    float temperature_c;
    float pv_voltage;
    bool pv_oscillating;        /**< Cloud edge effect detection */
} weather_data_t;

//============================================================================
// ASTRONOMICAL DATA
//============================================================================

typedef enum {
    PERIOD_DAY = 0,
    PERIOD_CIVIL_TWILIGHT,
    PERIOD_NIGHT
} time_period_t;

typedef struct {
    time_t sunrise;             /**< Unix timestamp of sunrise */
    time_t sunset;              /**< Unix timestamp of sunset */
    time_t civil_twilight_start;/**< Morning civil twilight */
    time_t civil_twilight_end;  /**< Evening civil twilight */
    time_period_t current_period;
    float latitude;             /**< Location latitude */
    float longitude;            /**< Location longitude */
} astro_data_t;

//============================================================================
// CALENDAR DATA
//============================================================================

typedef enum {
    SEASON_SPRING = 0,
    SEASON_SUMMER,
    SEASON_AUTUMN,
    SEASON_WINTER
} season_t;

typedef struct {
    season_t season;
    bool is_weekend;
    bool is_holiday;
    uint16_t day_of_year;
    const char *holiday_name;   /**< NULL if not a holiday */
} calendar_data_t;

//============================================================================
// BMS DATA
//============================================================================

typedef struct {
    float soc_percent;          /**< State of charge 0-100% */
    float voltage;              /**< Battery voltage */
    float current;              /**< Battery current (+ = charging) */
    float temperature;          /**< Battery temperature °C */
    bool alert_active;          /**< BMS alert status */
    bool low_battery;           /**< SOC < 20% */
    bool high_battery;          /**< SOC > 90% */
} bms_data_t;

//============================================================================
// FUSION DATA (Combined sensor inputs)
//============================================================================

typedef struct {
    // Sensor inputs
    weather_data_t weather;
    astro_data_t astro;
    calendar_data_t calendar;
    bms_data_t bms;
    
    // System state
    float board_temp;
    float heatsink_temp;
    uint32_t uptime_sec;
    time_t timestamp;
    
    // Active profiles
    const v2g_profile_t *active_v2g_profile;
    const led_profile_t *active_led_profile;
    
    // Profile selection reasons
    char v2g_reason[128];
    char led_reason[128];
} fusion_data_t;

//============================================================================
// FUNCTION PROTOTYPES
//============================================================================

/**
 * @brief Get V2G profile by ID
 * @param id Profile ID (0-18)
 * @return Pointer to profile or NULL if invalid
 */
const v2g_profile_t* get_v2g_profile(uint8_t id);

/**
 * @brief Get LED profile by ID
 * @param id Profile ID (0-14)
 * @return Pointer to profile or NULL if invalid
 */
const led_profile_t* get_led_profile(uint8_t id);

/**
 * @brief Get total number of V2G profiles
 * @return Number of V2G profiles (19)
 */
uint8_t get_v2g_profile_count(void);

/**
 * @brief Get total number of LED profiles
 * @return Number of LED profiles (15)
 */
uint8_t get_led_profile_count(void);

/**
 * @brief Get weather state string
 * @param state Weather state enum
 * @return String representation
 */
const char* get_weather_state_str(weather_state_t state);

/**
 * @brief Get time period string
 * @param period Time period enum
 * @return String representation
 */
const char* get_time_period_str(time_period_t period);

/**
 * @brief Get season string
 * @param season Season enum
 * @return String representation
 */
const char* get_season_str(season_t season);

//============================================================================
// ASTRONOMICAL FUNCTIONS (astro.c)
//============================================================================

/**
 * @brief Calculate astronomical data (sunrise, sunset, twilight times)
 * @param astro Pointer to astro_data_t structure to fill
 * @param timestamp Current Unix timestamp
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t astro_calculate(astro_data_t *astro, time_t timestamp);

/**
 * @brief Update current time period based on timestamp
 * @param astro Pointer to astro_data_t structure
 * @param timestamp Current Unix timestamp
 */
void astro_update_period(astro_data_t *astro, time_t timestamp);

//============================================================================
// CALENDAR FUNCTIONS (calendar.c)
//============================================================================

/**
 * @brief Calculate calendar data (season, holidays, weekend)
 * @param calendar Pointer to calendar_data_t structure to fill
 * @param timestamp Current Unix timestamp
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t calendar_calculate(calendar_data_t *calendar, time_t timestamp);

/**
 * @brief Get number of days in a specific month
 * @param year Year (e.g., 2026)
 * @param month Month (1-12)
 * @return Number of days in month (28-31) or 0 on error
 */
uint8_t calendar_get_days_in_month(int year, int month);

/**
 * @brief Check if a specific date is a holiday
 * @param day Day of month
 * @param month Month (1-12)
 * @return Pointer to holiday name or NULL if not a holiday
 */
const char* calendar_is_holiday(uint8_t day, uint8_t month);

/**
 * @brief Get season name for a specific day of year
 * @param day_of_year Day of year (1-366)
 * @param year Year (for leap year adjustment)
 * @return Season name string
 */
const char* calendar_get_season_for_day(uint16_t day_of_year, int year);

//============================================================================
// WEATHER FUNCTIONS (weather.c)
//============================================================================

/**
 * @brief Read sensors and determine current weather state
 * @param weather Pointer to weather_data_t structure to fill
 * @param is_daytime true if currently daytime (for accurate classification)
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t weather_update(weather_data_t *weather, bool is_daytime);

/**
 * @brief Get weather state from sensor readings without updating internal state
 * @param lux Ambient light level (lux)
 * @param temperature Temperature (°C)
 * @param pv_voltage PV panel voltage (V)
 * @param is_daytime true if currently daytime
 * @return Classified weather state
 */
weather_state_t weather_classify(float lux, float temperature, float pv_voltage, bool is_daytime);

/**
 * @brief Reset PV oscillation detector
 */
void weather_reset_oscillation_detector(void);

/**
 * @brief Get current PV oscillation state
 * @return true if PV is currently oscillating (cloud edge effects)
 */
bool weather_is_pv_oscillating(void);

//============================================================================
// FUSION ENGINE FUNCTIONS (fusion.c)
//============================================================================

/**
 * @brief Initialize fusion engine
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t fusion_init(void);

/**
 * @brief Update fusion engine and select optimal profiles
 * @param fusion Pointer to fusion_data_t structure with current sensor data
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t fusion_update(fusion_data_t *fusion);

/**
 * @brief Get current fusion state as string (for debugging/web interface)
 * @param fusion Pointer to fusion_data_t structure
 * @param buffer Output buffer for status string
 * @param buffer_size Size of output buffer
 */
void fusion_get_status_string(const fusion_data_t *fusion, char *buffer, size_t buffer_size);

#endif // PROFILES_H
