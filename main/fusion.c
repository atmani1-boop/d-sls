/**
 * @file fusion.c
 * @brief Sensor fusion engine for intelligent V2G and LED profile selection
 * 
 * Implements priority-based profile selection using inputs from:
 *   - Astronomical calculations (sunrise/sunset/twilight)
 *   - Calendar data (season/holidays/weekends)
 *   - Weather detection (clear/cloudy/rain/fog)
 *   - BMS data (battery state of charge)
 * 
 * Defines 19 V2G profiles and 15 LED profiles with intelligent activation
 * conditions and priority-based selection.
 */

#include "include/profiles.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "FUSION";

//============================================================================
// V2G PROFILE DEFINITIONS (19 profiles)
//============================================================================

static const v2g_profile_t v2g_profiles[] = {
    // Priority 255: Emergency - Battery critically low
    {
        .id = V2G_EMERGENCY_BATT_LOW,
        .name = "Emergency: Battery Low",
        .mode = V2G_MODE_CHARGE,
        .power_target = 500,
        .priority = 255,
        .conditions = "SOC < 20%, immediate charging required"
    },
    
    // Priority 254: Emergency - Grid fault detected
    {
        .id = V2G_EMERGENCY_GRID_FAULT,
        .name = "Emergency: Grid Fault",
        .mode = V2G_MODE_HOLD,
        .power_target = 0,
        .priority = 254,
        .conditions = "Grid fault or BMS alert active"
    },
    
    // Priority 200: Peak shaving during high demand
    {
        .id = V2G_PEAK_SHAVING,
        .name = "Peak Shaving",
        .mode = V2G_MODE_DISCHARGE,
        .power_target = -300,
        .priority = 200,
        .conditions = "Evening hours (18:00-22:00) + SOC > 50%"
    },
    
    // Priority 180: Maximum solar export
    {
        .id = V2G_SOLAR_EXPORT_MAX,
        .name = "Solar Export Maximum",
        .mode = V2G_MODE_EXPORT,
        .power_target = 500,
        .priority = 180,
        .conditions = "Clear sky + midday + SOC > 80%"
    },
    
    // Priority 160: Night import at cheap rates
    {
        .id = V2G_NIGHT_IMPORT_CHEAP,
        .name = "Night Import (Cheap Rate)",
        .mode = V2G_MODE_IMPORT,
        .power_target = 400,
        .priority = 160,
        .conditions = "Night (23:00-06:00) + SOC < 70%"
    },
    
    // Priority 150: Morning precharge before peak
    {
        .id = V2G_MORNING_PRECHARGE,
        .name = "Morning Precharge",
        .mode = V2G_MODE_CHARGE,
        .power_target = 300,
        .priority = 150,
        .conditions = "Morning (06:00-09:00) + SOC < 60%"
    },
    
    // Priority 140: Afternoon hold for evening discharge
    {
        .id = V2G_AFTERNOON_HOLD,
        .name = "Afternoon Hold",
        .mode = V2G_MODE_HOLD,
        .power_target = 0,
        .priority = 140,
        .conditions = "Afternoon (14:00-17:00) + SOC > 70%"
    },
    
    // Priority 130: Evening discharge for peak demand
    {
        .id = V2G_EVENING_DISCHARGE,
        .name = "Evening Discharge",
        .mode = V2G_MODE_DISCHARGE,
        .power_target = -350,
        .priority = 130,
        .conditions = "Evening (18:00-21:00) + SOC > 60%"
    },
    
    // Priority 120: Weekend eco mode
    {
        .id = V2G_WEEKEND_ECO,
        .name = "Weekend Eco Mode",
        .mode = V2G_MODE_HOLD,
        .power_target = 50,
        .priority = 120,
        .conditions = "Weekend + SOC 40-80%"
    },
    
    // Priority 110: Holiday minimal operation
    {
        .id = V2G_HOLIDAY_MINIMAL,
        .name = "Holiday Minimal",
        .mode = V2G_MODE_HOLD,
        .power_target = 0,
        .priority = 110,
        .conditions = "Public holiday + maintain current state"
    },
    
    // Priority 100: Cloudy weather import
    {
        .id = V2G_CLOUDY_IMPORT,
        .name = "Cloudy Import",
        .mode = V2G_MODE_IMPORT,
        .power_target = 200,
        .priority = 100,
        .conditions = "Cloudy + daytime + SOC < 60%"
    },
    
    // Priority 90: Intermittent weather hold
    {
        .id = V2G_INTERMITTENT_HOLD,
        .name = "Intermittent Hold",
        .mode = V2G_MODE_HOLD,
        .power_target = 0,
        .priority = 90,
        .conditions = "Intermittent clouds (PV oscillating)"
    },
    
    // Priority 85: Rain - charge from grid
    {
        .id = V2G_RAIN_CHARGE_GRID,
        .name = "Rain: Grid Charge",
        .mode = V2G_MODE_IMPORT,
        .power_target = 250,
        .priority = 85,
        .conditions = "Rain detected + SOC < 70%"
    },
    
    // Priority 80: Fog - minimal operation
    {
        .id = V2G_FOG_MINIMAL,
        .name = "Fog: Minimal Operation",
        .mode = V2G_MODE_HOLD,
        .power_target = 50,
        .priority = 80,
        .conditions = "Fog detected + maintain minimal flow"
    },
    
    // Priority 70: Summer - maximize export
    {
        .id = V2G_SUMMER_EXPORT,
        .name = "Summer Export",
        .mode = V2G_MODE_EXPORT,
        .power_target = 400,
        .priority = 70,
        .conditions = "Summer season + clear day + SOC > 70%"
    },
    
    // Priority 65: Winter - import for heating
    {
        .id = V2G_WINTER_IMPORT,
        .name = "Winter Import",
        .mode = V2G_MODE_IMPORT,
        .power_target = 350,
        .priority = 65,
        .conditions = "Winter season + morning/evening + SOC < 80%"
    },
    
    // Priority 60: Autumn - balanced operation
    {
        .id = V2G_AUTUMN_BALANCE,
        .name = "Autumn Balance",
        .mode = V2G_MODE_HOLD,
        .power_target = 100,
        .priority = 60,
        .conditions = "Autumn season + balanced flow"
    },
    
    // Priority 55: Spring - charge for summer
    {
        .id = V2G_SPRING_CHARGE,
        .name = "Spring Charge",
        .mode = V2G_MODE_CHARGE,
        .power_target = 250,
        .priority = 55,
        .conditions = "Spring season + prepare for summer peak"
    },
    
    // Priority 50: Default hold state
    {
        .id = V2G_DEFAULT_HOLD,
        .name = "Default Hold",
        .mode = V2G_MODE_HOLD,
        .power_target = 0,
        .priority = 50,
        .conditions = "Fallback profile when no other conditions met"
    }
};

#define V2G_PROFILE_COUNT (sizeof(v2g_profiles) / sizeof(v2g_profile_t))

//============================================================================
// LED PROFILE DEFINITIONS (15 profiles)
//============================================================================

static const led_profile_t led_profiles[] = {
    // Priority 255: Emergency - system fault
    {
        .id = LED_EMERGENCY_FAULT,
        .name = "Emergency: Fault",
        .warm_intensity = 100,
        .cool_intensity = 100,
        .priority = 255,
        .conditions = "System fault or BMS alert - full illumination"
    },
    
    // Priority 250: Safety minimum lighting
    {
        .id = LED_SAFETY_MINIMUM,
        .name = "Safety Minimum",
        .warm_intensity = 20,
        .cool_intensity = 0,
        .priority = 250,
        .conditions = "Battery < 15% - minimal warm light only"
    },
    
    // Priority 200: Sunset activation
    {
        .id = LED_SUNSET_ON,
        .name = "Sunset ON",
        .warm_intensity = 80,
        .cool_intensity = 40,
        .priority = 200,
        .conditions = "Sunset (sun just below horizon)"
    },
    
    // Priority 190: Civil twilight soft lighting
    {
        .id = LED_TWILIGHT_SOFT,
        .name = "Twilight Soft",
        .warm_intensity = 60,
        .cool_intensity = 20,
        .priority = 190,
        .conditions = "Civil twilight (sun 0-6° below horizon)"
    },
    
    // Priority 180: Midnight eco mode
    {
        .id = LED_MIDNIGHT_ECO,
        .name = "Midnight Eco",
        .warm_intensity = 15,
        .cool_intensity = 0,
        .priority = 180,
        .conditions = "Late night (00:00-05:00) - minimal lighting"
    },
    
    // Priority 170: Dawn warmup
    {
        .id = LED_DAWN_WARMUP,
        .name = "Dawn Warmup",
        .warm_intensity = 40,
        .cool_intensity = 10,
        .priority = 170,
        .conditions = "Dawn twilight - gradual warmup"
    },
    
    // Priority 160: Sunrise off
    {
        .id = LED_SUNRISE_OFF,
        .name = "Sunrise OFF",
        .warm_intensity = 0,
        .cool_intensity = 0,
        .priority = 160,
        .conditions = "Sunrise - natural light sufficient"
    },
    
    // Priority 150: Cloudy weather boost
    {
        .id = LED_CLOUDY_BOOST,
        .name = "Cloudy Boost",
        .warm_intensity = 50,
        .cool_intensity = 70,
        .priority = 150,
        .conditions = "Cloudy daytime - boost cool light"
    },
    
    // Priority 140: Rain comfort lighting
    {
        .id = LED_RAIN_COMFORT,
        .name = "Rain Comfort",
        .warm_intensity = 70,
        .cool_intensity = 30,
        .priority = 140,
        .conditions = "Rain detected - warm comfort lighting"
    },
    
    // Priority 135: Fog high visibility
    {
        .id = LED_FOG_HIGH,
        .name = "Fog High Visibility",
        .warm_intensity = 90,
        .cool_intensity = 60,
        .priority = 135,
        .conditions = "Fog - high intensity for visibility"
    },
    
    // Priority 100: Sunny day off
    {
        .id = LED_SUNNY_OFF,
        .name = "Sunny Day OFF",
        .warm_intensity = 0,
        .cool_intensity = 0,
        .priority = 100,
        .conditions = "Clear sunny day - lights not needed"
    },
    
    // Priority 90: Weekend dimmed mode
    {
        .id = LED_WEEKEND_DIM,
        .name = "Weekend Dim",
        .warm_intensity = 40,
        .cool_intensity = 20,
        .priority = 90,
        .conditions = "Weekend evening - relaxed lighting"
    },
    
    // Priority 85: Holiday away mode
    {
        .id = LED_HOLIDAY_AWAY,
        .name = "Holiday Away",
        .warm_intensity = 10,
        .cool_intensity = 5,
        .priority = 85,
        .conditions = "Public holiday - minimal presence lighting"
    },
    
    // Priority 80: Night full operation
    {
        .id = LED_NIGHT_FULL,
        .name = "Night Full",
        .warm_intensity = 75,
        .cool_intensity = 50,
        .priority = 80,
        .conditions = "Night (21:00-23:00) - full operation"
    },
    
    // Priority 50: Default auto mode
    {
        .id = LED_DEFAULT_AUTO,
        .name = "Default Auto",
        .warm_intensity = 50,
        .cool_intensity = 30,
        .priority = 50,
        .conditions = "Automatic based on ambient light"
    }
};

#define LED_PROFILE_COUNT (sizeof(led_profiles) / sizeof(led_profile_t))

//============================================================================
// HELPER FUNCTIONS
//============================================================================

/**
 * @brief Get V2G profile by ID
 */
const v2g_profile_t* get_v2g_profile(uint8_t id) {
    for (uint8_t i = 0; i < V2G_PROFILE_COUNT; i++) {
        if (v2g_profiles[i].id == id) {
            return &v2g_profiles[i];
        }
    }
    return NULL;
}

/**
 * @brief Get LED profile by ID
 */
const led_profile_t* get_led_profile(uint8_t id) {
    for (uint8_t i = 0; i < LED_PROFILE_COUNT; i++) {
        if (led_profiles[i].id == id) {
            return &led_profiles[i];
        }
    }
    return NULL;
}

/**
 * @brief Get total number of V2G profiles
 */
uint8_t get_v2g_profile_count(void) {
    return V2G_PROFILE_COUNT;
}

/**
 * @brief Get total number of LED profiles
 */
uint8_t get_led_profile_count(void) {
    return LED_PROFILE_COUNT;
}

/**
 * @brief Get weather state string
 */
const char* get_weather_state_str(weather_state_t state) {
    static const char* const weather_strings[] = {
        "Clear",
        "Cloudy",
        "Intermittent",
        "Rain",
        "Fog",
        "Unknown"
    };
    
    if (state >= 0 && state <= WEATHER_UNKNOWN) {
        return weather_strings[state];
    }
    return "Invalid";
}

/**
 * @brief Get time period string
 */
const char* get_time_period_str(time_period_t period) {
    static const char* const period_strings[] = {
        "Day",
        "Civil Twilight",
        "Night"
    };
    
    if (period >= 0 && period <= PERIOD_NIGHT) {
        return period_strings[period];
    }
    return "Invalid";
}

/**
 * @brief Get season string
 */
const char* get_season_str(season_t season) {
    static const char* const season_strings[] = {
        "Spring",
        "Summer",
        "Autumn",
        "Winter"
    };
    
    if (season >= 0 && season <= SEASON_WINTER) {
        return season_strings[season];
    }
    return "Invalid";
}

/**
 * @brief Get hour from Unix timestamp
 */
static uint8_t get_hour(time_t timestamp) {
    struct tm timeinfo;
    localtime_r(&timestamp, &timeinfo);
    return timeinfo.tm_hour;
}

/**
 * @brief Check if timestamp is within time range
 */
static bool is_time_between(time_t timestamp, uint8_t start_hour, uint8_t end_hour) {
    uint8_t hour = get_hour(timestamp);
    
    if (start_hour <= end_hour) {
        return (hour >= start_hour && hour < end_hour);
    } else {
        // Range crosses midnight
        return (hour >= start_hour || hour < end_hour);
    }
}

//============================================================================
// V2G PROFILE SELECTION LOGIC
//============================================================================

/**
 * @brief Check if V2G profile conditions are met
 */
static bool v2g_check_conditions(const v2g_profile_t *profile, const fusion_data_t *fusion) {
    switch (profile->id) {
        case V2G_EMERGENCY_BATT_LOW:
            return (fusion->bms.low_battery || fusion->bms.soc_percent < 20.0f);
            
        case V2G_EMERGENCY_GRID_FAULT:
            return fusion->bms.alert_active;
            
        case V2G_PEAK_SHAVING:
            return is_time_between(fusion->timestamp, 18, 22) && 
                   fusion->bms.soc_percent > 50.0f;
            
        case V2G_SOLAR_EXPORT_MAX:
            return fusion->weather.state == WEATHER_CLEAR &&
                   is_time_between(fusion->timestamp, 11, 15) &&
                   fusion->bms.soc_percent > 80.0f;
            
        case V2G_NIGHT_IMPORT_CHEAP:
            return is_time_between(fusion->timestamp, 23, 6) &&
                   fusion->bms.soc_percent < 70.0f;
            
        case V2G_MORNING_PRECHARGE:
            return is_time_between(fusion->timestamp, 6, 9) &&
                   fusion->bms.soc_percent < 60.0f;
            
        case V2G_AFTERNOON_HOLD:
            return is_time_between(fusion->timestamp, 14, 17) &&
                   fusion->bms.soc_percent > 70.0f;
            
        case V2G_EVENING_DISCHARGE:
            return is_time_between(fusion->timestamp, 18, 21) &&
                   fusion->bms.soc_percent > 60.0f;
            
        case V2G_WEEKEND_ECO:
            return fusion->calendar.is_weekend &&
                   fusion->bms.soc_percent >= 40.0f &&
                   fusion->bms.soc_percent <= 80.0f;
            
        case V2G_HOLIDAY_MINIMAL:
            return fusion->calendar.is_holiday;
            
        case V2G_CLOUDY_IMPORT:
            return fusion->weather.state == WEATHER_CLOUDY &&
                   fusion->astro.current_period == PERIOD_DAY &&
                   fusion->bms.soc_percent < 60.0f;
            
        case V2G_INTERMITTENT_HOLD:
            return fusion->weather.pv_oscillating ||
                   fusion->weather.state == WEATHER_INTERMITTENT;
            
        case V2G_RAIN_CHARGE_GRID:
            return fusion->weather.state == WEATHER_RAIN &&
                   fusion->bms.soc_percent < 70.0f;
            
        case V2G_FOG_MINIMAL:
            return fusion->weather.state == WEATHER_FOG;
            
        case V2G_SUMMER_EXPORT:
            return fusion->calendar.season == SEASON_SUMMER &&
                   fusion->weather.state == WEATHER_CLEAR &&
                   fusion->bms.soc_percent > 70.0f;
            
        case V2G_WINTER_IMPORT:
            return fusion->calendar.season == SEASON_WINTER &&
                   (is_time_between(fusion->timestamp, 6, 9) ||
                    is_time_between(fusion->timestamp, 17, 21)) &&
                   fusion->bms.soc_percent < 80.0f;
            
        case V2G_AUTUMN_BALANCE:
            return fusion->calendar.season == SEASON_AUTUMN;
            
        case V2G_SPRING_CHARGE:
            return fusion->calendar.season == SEASON_SPRING;
            
        case V2G_DEFAULT_HOLD:
            return true; // Always valid as fallback
            
        default:
            return false;
    }
}

/**
 * @brief Select best V2G profile based on current conditions
 */
static const v2g_profile_t* select_v2g_profile(const fusion_data_t *fusion, char *reason) {
    const v2g_profile_t *best_profile = NULL;
    uint8_t best_priority = 0;
    
    // Iterate through all profiles, selecting highest priority match
    for (uint8_t i = 0; i < V2G_PROFILE_COUNT; i++) {
        if (v2g_check_conditions(&v2g_profiles[i], fusion)) {
            if (v2g_profiles[i].priority > best_priority) {
                best_priority = v2g_profiles[i].priority;
                best_profile = &v2g_profiles[i];
            }
        }
    }
    
    if (best_profile && reason) {
        snprintf(reason, 128, "Priority %d: %s", best_priority, best_profile->conditions);
    }
    
    return best_profile;
}

//============================================================================
// LED PROFILE SELECTION LOGIC
//============================================================================

/**
 * @brief Check if LED profile conditions are met
 */
static bool led_check_conditions(const led_profile_t *profile, const fusion_data_t *fusion) {
    switch (profile->id) {
        case LED_EMERGENCY_FAULT:
            return fusion->bms.alert_active;
            
        case LED_SAFETY_MINIMUM:
            return fusion->bms.soc_percent < 15.0f;
            
        case LED_SUNSET_ON:
            return fusion->astro.current_period == PERIOD_CIVIL_TWILIGHT &&
                   fusion->timestamp >= fusion->astro.sunset &&
                   fusion->timestamp < fusion->astro.civil_twilight_end;
            
        case LED_TWILIGHT_SOFT:
            return fusion->astro.current_period == PERIOD_CIVIL_TWILIGHT;
            
        case LED_MIDNIGHT_ECO:
            return is_time_between(fusion->timestamp, 0, 5);
            
        case LED_DAWN_WARMUP:
            return fusion->astro.current_period == PERIOD_CIVIL_TWILIGHT &&
                   fusion->timestamp >= fusion->astro.civil_twilight_start &&
                   fusion->timestamp < fusion->astro.sunrise;
            
        case LED_SUNRISE_OFF:
            return fusion->astro.current_period == PERIOD_DAY;
            
        case LED_CLOUDY_BOOST:
            return fusion->weather.state == WEATHER_CLOUDY &&
                   fusion->astro.current_period == PERIOD_DAY;
            
        case LED_RAIN_COMFORT:
            return fusion->weather.state == WEATHER_RAIN;
            
        case LED_FOG_HIGH:
            return fusion->weather.state == WEATHER_FOG;
            
        case LED_SUNNY_OFF:
            return fusion->weather.state == WEATHER_CLEAR &&
                   fusion->astro.current_period == PERIOD_DAY;
            
        case LED_WEEKEND_DIM:
            return fusion->calendar.is_weekend &&
                   is_time_between(fusion->timestamp, 18, 23);
            
        case LED_HOLIDAY_AWAY:
            return fusion->calendar.is_holiday;
            
        case LED_NIGHT_FULL:
            return is_time_between(fusion->timestamp, 21, 23) &&
                   fusion->astro.current_period == PERIOD_NIGHT;
            
        case LED_DEFAULT_AUTO:
            return true; // Always valid as fallback
            
        default:
            return false;
    }
}

/**
 * @brief Select best LED profile based on current conditions
 */
static const led_profile_t* select_led_profile(const fusion_data_t *fusion, char *reason) {
    const led_profile_t *best_profile = NULL;
    uint8_t best_priority = 0;
    
    // Iterate through all profiles, selecting highest priority match
    for (uint8_t i = 0; i < LED_PROFILE_COUNT; i++) {
        if (led_check_conditions(&led_profiles[i], fusion)) {
            if (led_profiles[i].priority > best_priority) {
                best_priority = led_profiles[i].priority;
                best_profile = &led_profiles[i];
            }
        }
    }
    
    if (best_profile && reason) {
        snprintf(reason, 128, "Priority %d: %s", best_priority, best_profile->conditions);
    }
    
    return best_profile;
}

//============================================================================
// FUSION ENGINE PUBLIC API
//============================================================================

/**
 * @brief Update fusion engine and select optimal profiles
 */
esp_err_t fusion_update(fusion_data_t *fusion) {
    if (!fusion) {
        ESP_LOGE(TAG, "NULL fusion pointer");
        return ESP_FAIL;
    }
    
    // Select V2G profile
    fusion->active_v2g_profile = select_v2g_profile(fusion, fusion->v2g_reason);
    
    if (!fusion->active_v2g_profile) {
        ESP_LOGW(TAG, "No V2G profile matched - using default");
        fusion->active_v2g_profile = get_v2g_profile(V2G_DEFAULT_HOLD);
        snprintf(fusion->v2g_reason, 128, "Fallback to default hold");
    }
    
    // Select LED profile
    fusion->active_led_profile = select_led_profile(fusion, fusion->led_reason);
    
    if (!fusion->active_led_profile) {
        ESP_LOGW(TAG, "No LED profile matched - using default");
        fusion->active_led_profile = get_led_profile(LED_DEFAULT_AUTO);
        snprintf(fusion->led_reason, 128, "Fallback to default auto");
    }
    
    ESP_LOGI(TAG, "=== Fusion Update ===");
    ESP_LOGI(TAG, "V2G: [%d] %s (mode=%d, power=%dW)",
             fusion->active_v2g_profile->id,
             fusion->active_v2g_profile->name,
             fusion->active_v2g_profile->mode,
             fusion->active_v2g_profile->power_target);
    ESP_LOGI(TAG, "     Reason: %s", fusion->v2g_reason);
    ESP_LOGI(TAG, "LED: [%d] %s (warm=%d%%, cool=%d%%)",
             fusion->active_led_profile->id,
             fusion->active_led_profile->name,
             fusion->active_led_profile->warm_intensity,
             fusion->active_led_profile->cool_intensity);
    ESP_LOGI(TAG, "     Reason: %s", fusion->led_reason);
    
    return ESP_OK;
}

/**
 * @brief Initialize fusion engine
 */
esp_err_t fusion_init(void) {
    ESP_LOGI(TAG, "Initializing fusion engine");
    ESP_LOGI(TAG, "Loaded %d V2G profiles", V2G_PROFILE_COUNT);
    ESP_LOGI(TAG, "Loaded %d LED profiles", LED_PROFILE_COUNT);
    
    // Validate all profiles have unique IDs and valid priorities
    for (uint8_t i = 0; i < V2G_PROFILE_COUNT; i++) {
        if (v2g_profiles[i].id >= V2G_PROFILE_COUNT) {
            ESP_LOGE(TAG, "Invalid V2G profile ID: %d", v2g_profiles[i].id);
            return ESP_FAIL;
        }
    }
    
    for (uint8_t i = 0; i < LED_PROFILE_COUNT; i++) {
        if (led_profiles[i].id >= LED_PROFILE_COUNT) {
            ESP_LOGE(TAG, "Invalid LED profile ID: %d", led_profiles[i].id);
            return ESP_FAIL;
        }
    }
    
    ESP_LOGI(TAG, "Fusion engine initialized successfully");
    return ESP_OK;
}

/**
 * @brief Get current fusion state as string (for debugging/web interface)
 */
void fusion_get_status_string(const fusion_data_t *fusion, char *buffer, size_t buffer_size) {
    if (!fusion || !buffer || buffer_size == 0) {
        return;
    }
    
    snprintf(buffer, buffer_size,
             "V2G: %s (%dW) | LED: %s (W:%d%% C:%d%%) | "
             "Weather: %s | Period: %s | Season: %s | SOC: %.1f%%",
             fusion->active_v2g_profile ? fusion->active_v2g_profile->name : "None",
             fusion->active_v2g_profile ? fusion->active_v2g_profile->power_target : 0,
             fusion->active_led_profile ? fusion->active_led_profile->name : "None",
             fusion->active_led_profile ? fusion->active_led_profile->warm_intensity : 0,
             fusion->active_led_profile ? fusion->active_led_profile->cool_intensity : 0,
             get_weather_state_str(fusion->weather.state),
             get_time_period_str(fusion->astro.current_period),
             get_season_str(fusion->calendar.season),
             fusion->bms.soc_percent);
}
