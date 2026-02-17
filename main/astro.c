/**
 * @file astro.c
 * @brief Astronomical calculations for Milan, Italy (45.4642°N, 9.1900°E)
 * 
 * Calculates sunrise, sunset, and civil twilight times using astronomical
 * algorithms. Implements Julian day conversions and solar position calculations.
 * 
 * @note Civil twilight: Sun between 0° and -6° below horizon
 * @note All times are in UTC and should be converted to local time by caller
 */

#include "include/profiles.h"
#include <math.h>
#include <time.h>
#include "esp_log.h"

static const char *TAG = "ASTRO";

// Milan coordinates (default location)
#define MILAN_LATITUDE  45.4642f
#define MILAN_LONGITUDE 9.1900f

// Astronomical constants
#define SOLAR_ZENITH_SUNRISE   90.833f  /**< Sun position at sunrise/sunset (degrees) */
#define SOLAR_ZENITH_CIVIL_TWI 96.0f    /**< Sun position at civil twilight (degrees) */
#define DEG_TO_RAD             0.017453292519943295f
#define RAD_TO_DEG             57.29577951308232f
#define J2000_EPOCH            2451545.0  /**< Julian day of J2000.0 epoch */

/**
 * @brief Calculate Julian day number from Unix timestamp
 * 
 * @param timestamp Unix timestamp (seconds since 1970-01-01 00:00:00 UTC)
 * @return Julian day number
 */
static double calculate_julian_day(time_t timestamp) {
    // Unix epoch: 1970-01-01 00:00:00 UTC = JD 2440587.5
    return 2440587.5 + ((double)timestamp / 86400.0);
}

/**
 * @brief Calculate Julian century from Julian day
 * 
 * @param jd Julian day number
 * @return Julian century from J2000.0
 */
static double calculate_julian_century(double jd) {
    return (jd - J2000_EPOCH) / 36525.0;
}

/**
 * @brief Calculate geometric mean longitude of Sun (degrees)
 * 
 * @param t Julian century
 * @return Mean longitude in degrees
 */
static double sun_geometric_mean_longitude(double t) {
    double l0 = 280.46646 + t * (36000.76983 + t * 0.0003032);
    // Normalize to 0-360
    while (l0 > 360.0) l0 -= 360.0;
    while (l0 < 0.0) l0 += 360.0;
    return l0;
}

/**
 * @brief Calculate geometric mean anomaly of Sun (degrees)
 * 
 * @param t Julian century
 * @return Mean anomaly in degrees
 */
static double sun_geometric_mean_anomaly(double t) {
    return 357.52911 + t * (35999.05029 - 0.0001537 * t);
}

/**
 * @brief Calculate eccentricity of Earth's orbit
 * 
 * @param t Julian century
 * @return Eccentricity (unitless)
 */
static double earth_orbit_eccentricity(double t) {
    return 0.016708634 - t * (0.000042037 + 0.0000001267 * t);
}

/**
 * @brief Calculate Sun's equation of center (degrees)
 * 
 * @param t Julian century
 * @return Equation of center in degrees
 */
static double sun_equation_of_center(double t) {
    double m = sun_geometric_mean_anomaly(t);
    double mrad = m * DEG_TO_RAD;
    double sinm = sin(mrad);
    double sin2m = sin(2.0 * mrad);
    double sin3m = sin(3.0 * mrad);
    
    return sinm * (1.914602 - t * (0.004817 + 0.000014 * t)) +
           sin2m * (0.019993 - 0.000101 * t) +
           sin3m * 0.000289;
}

/**
 * @brief Calculate Sun's true longitude (degrees)
 * 
 * @param t Julian century
 * @return True longitude in degrees
 */
static double sun_true_longitude(double t) {
    double l0 = sun_geometric_mean_longitude(t);
    double c = sun_equation_of_center(t);
    return l0 + c;
}

/**
 * @brief Calculate Sun's apparent longitude (degrees)
 * 
 * @param t Julian century
 * @return Apparent longitude in degrees
 */
static double sun_apparent_longitude(double t) {
    double o = sun_true_longitude(t);
    return o - 0.00569 - 0.00478 * sin((125.04 - 1934.136 * t) * DEG_TO_RAD);
}

/**
 * @brief Calculate mean obliquity of ecliptic (degrees)
 * 
 * @param t Julian century
 * @return Mean obliquity in degrees
 */
static double mean_obliquity_of_ecliptic(double t) {
    double seconds = 21.448 - t * (46.8150 + t * (0.00059 - t * 0.001813));
    return 23.0 + (26.0 + (seconds / 60.0)) / 60.0;
}

/**
 * @brief Calculate corrected obliquity of ecliptic (degrees)
 * 
 * @param t Julian century
 * @return Corrected obliquity in degrees
 */
static double obliquity_correction(double t) {
    double e0 = mean_obliquity_of_ecliptic(t);
    double omega = 125.04 - 1934.136 * t;
    return e0 + 0.00256 * cos(omega * DEG_TO_RAD);
}

/**
 * @brief Calculate Sun's declination (degrees)
 * 
 * @param t Julian century
 * @return Declination in degrees
 */
static double sun_declination(double t) {
    double e = obliquity_correction(t);
    double lambda = sun_apparent_longitude(t);
    double sint = sin(e * DEG_TO_RAD) * sin(lambda * DEG_TO_RAD);
    return asin(sint) * RAD_TO_DEG;
}

/**
 * @brief Calculate equation of time (minutes)
 * 
 * @param t Julian century
 * @return Equation of time in minutes
 */
static double equation_of_time(double t) {
    double epsilon = obliquity_correction(t);
    double l0 = sun_geometric_mean_longitude(t);
    double e = earth_orbit_eccentricity(t);
    double m = sun_geometric_mean_anomaly(t);
    
    double y = tan((epsilon / 2.0) * DEG_TO_RAD);
    y *= y;
    
    double sin2l0 = sin(2.0 * l0 * DEG_TO_RAD);
    double sinm = sin(m * DEG_TO_RAD);
    double cos2l0 = cos(2.0 * l0 * DEG_TO_RAD);
    double sin4l0 = sin(4.0 * l0 * DEG_TO_RAD);
    double sin2m = sin(2.0 * m * DEG_TO_RAD);
    
    double etime = y * sin2l0 - 2.0 * e * sinm + 4.0 * e * y * sinm * cos2l0 -
                   0.5 * y * y * sin4l0 - 1.25 * e * e * sin2m;
    
    return etime * 4.0 * RAD_TO_DEG; // Convert to minutes
}

/**
 * @brief Calculate hour angle for given zenith angle (degrees)
 * 
 * @param latitude Observer latitude (degrees, positive north)
 * @param declination Sun's declination (degrees)
 * @param zenith Zenith angle (90.833° for sunrise, 96° for civil twilight)
 * @return Hour angle in degrees, or NAN if no sunrise/sunset
 */
static double calculate_hour_angle(float latitude, double declination, float zenith) {
    double lat_rad = latitude * DEG_TO_RAD;
    double dec_rad = declination * DEG_TO_RAD;
    double zenith_rad = zenith * DEG_TO_RAD;
    
    double cos_h = (cos(zenith_rad) - sin(lat_rad) * sin(dec_rad)) /
                   (cos(lat_rad) * cos(dec_rad));
    
    // Check for polar day/night
    if (cos_h > 1.0 || cos_h < -1.0) {
        ESP_LOGW(TAG, "No sunrise/sunset at this location/time (cos_h=%.3f)", cos_h);
        return NAN;
    }
    
    return acos(cos_h) * RAD_TO_DEG;
}

/**
 * @brief Calculate sunrise or sunset time
 * 
 * @param timestamp Unix timestamp for the date
 * @param latitude Observer latitude (degrees)
 * @param longitude Observer longitude (degrees)
 * @param zenith Zenith angle (90.833° sunrise, 96° civil twilight)
 * @param is_sunrise true for sunrise, false for sunset
 * @return Unix timestamp of event, or 0 if no event
 */
static time_t calculate_solar_event(time_t timestamp, float latitude, float longitude,
                                    float zenith, bool is_sunrise) {
    // Calculate at noon UTC for the given day
    struct tm timeinfo;
    gmtime_r(&timestamp, &timeinfo);
    timeinfo.tm_hour = 12;
    timeinfo.tm_min = 0;
    timeinfo.tm_sec = 0;
    time_t noon = mktime(&timeinfo);
    
    double jd = calculate_julian_day(noon);
    double t = calculate_julian_century(jd);
    
    double decl = sun_declination(t);
    double eqtime = equation_of_time(t);
    double ha = calculate_hour_angle(latitude, decl, zenith);
    
    if (isnan(ha)) {
        return 0; // No sunrise/sunset
    }
    
    // Calculate solar noon time (in minutes from midnight UTC)
    double solar_noon = (720.0 - 4.0 * longitude - eqtime) / 60.0; // Hours
    
    // Calculate sunrise/sunset time
    double event_time;
    if (is_sunrise) {
        event_time = solar_noon - (ha * 4.0 / 60.0); // Hours
    } else {
        event_time = solar_noon + (ha * 4.0 / 60.0); // Hours
    }
    
    // Convert to Unix timestamp
    timeinfo.tm_hour = (int)event_time;
    timeinfo.tm_min = (int)((event_time - timeinfo.tm_hour) * 60.0);
    timeinfo.tm_sec = 0;
    
    time_t result = mktime(&timeinfo);
    
    ESP_LOGD(TAG, "%s: %02d:%02d UTC (decl=%.2f°, ha=%.2f°)",
             is_sunrise ? "Sunrise" : "Sunset",
             timeinfo.tm_hour, timeinfo.tm_min, decl, ha);
    
    return result;
}

/**
 * @brief Calculate astronomical data for Milan
 * 
 * @param astro Pointer to astro_data_t structure to fill
 * @param timestamp Current Unix timestamp
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t astro_calculate(astro_data_t *astro, time_t timestamp) {
    if (!astro) {
        ESP_LOGE(TAG, "NULL astro pointer");
        return ESP_FAIL;
    }
    
    // Set location
    astro->latitude = MILAN_LATITUDE;
    astro->longitude = MILAN_LONGITUDE;
    
    // Calculate sunrise and sunset
    astro->sunrise = calculate_solar_event(timestamp, MILAN_LATITUDE, MILAN_LONGITUDE,
                                          SOLAR_ZENITH_SUNRISE, true);
    astro->sunset = calculate_solar_event(timestamp, MILAN_LATITUDE, MILAN_LONGITUDE,
                                         SOLAR_ZENITH_SUNRISE, false);
    
    // Calculate civil twilight times
    astro->civil_twilight_start = calculate_solar_event(timestamp, MILAN_LATITUDE, MILAN_LONGITUDE,
                                                        SOLAR_ZENITH_CIVIL_TWI, true);
    astro->civil_twilight_end = calculate_solar_event(timestamp, MILAN_LATITUDE, MILAN_LONGITUDE,
                                                      SOLAR_ZENITH_CIVIL_TWI, false);
    
    if (astro->sunrise == 0 || astro->sunset == 0) {
        ESP_LOGW(TAG, "Could not calculate sunrise/sunset times");
        astro->current_period = PERIOD_NIGHT;
        return ESP_OK;
    }
    
    // Determine current period
    if (timestamp >= astro->sunrise && timestamp < astro->sunset) {
        astro->current_period = PERIOD_DAY;
    } else if ((timestamp >= astro->civil_twilight_start && timestamp < astro->sunrise) ||
               (timestamp >= astro->sunset && timestamp < astro->civil_twilight_end)) {
        astro->current_period = PERIOD_CIVIL_TWILIGHT;
    } else {
        astro->current_period = PERIOD_NIGHT;
    }
    
    struct tm timeinfo;
    gmtime_r(&timestamp, &timeinfo);
    
    ESP_LOGI(TAG, "Astronomical data for %04d-%02d-%02d:",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    ESP_LOGI(TAG, "  Location: %.4f°N, %.4f°E", astro->latitude, astro->longitude);
    ESP_LOGI(TAG, "  Sunrise: %02d:%02d UTC", 
             gmtime_r(&astro->sunrise, &timeinfo)->tm_hour, timeinfo.tm_min);
    ESP_LOGI(TAG, "  Sunset:  %02d:%02d UTC",
             gmtime_r(&astro->sunset, &timeinfo)->tm_hour, timeinfo.tm_min);
    ESP_LOGI(TAG, "  Period: %s", get_time_period_str(astro->current_period));
    
    return ESP_OK;
}

/**
 * @brief Update current time period based on timestamp
 * 
 * @param astro Pointer to astro_data_t structure
 * @param timestamp Current Unix timestamp
 */
void astro_update_period(astro_data_t *astro, time_t timestamp) {
    if (!astro) {
        return;
    }
    
    if (timestamp >= astro->sunrise && timestamp < astro->sunset) {
        astro->current_period = PERIOD_DAY;
    } else if ((timestamp >= astro->civil_twilight_start && timestamp < astro->sunrise) ||
               (timestamp >= astro->sunset && timestamp < astro->civil_twilight_end)) {
        astro->current_period = PERIOD_CIVIL_TWILIGHT;
    } else {
        astro->current_period = PERIOD_NIGHT;
    }
}
