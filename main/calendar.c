/**
 * @file calendar.c
 * @brief Calendar calculations including seasons, holidays, and date utilities
 * 
 * Provides season detection based on astronomical dates, Italian public holiday
 * recognition for 2026, weekend detection, and day-of-year calculations.
 * 
 * @note Season dates:
 *   - Spring: March 20
 *   - Summer: June 21
 *   - Autumn: September 22
 *   - Winter: December 21
 */

#include "include/profiles.h"
#include <time.h>
#include <string.h>
#include "esp_log.h"

static const char *TAG = "CALENDAR";

/**
 * @brief Italian public holiday definition
 */
typedef struct {
    uint8_t day;        /**< Day of month */
    uint8_t month;      /**< Month (1-12) */
    const char *name;   /**< Holiday name */
} holiday_t;

/**
 * @brief Italian public holidays for 2026
 * @note Some holidays (like Easter) vary by year; only fixed dates included
 */
static const holiday_t ITALIAN_HOLIDAYS_2026[] = {
    {1,  1,  "Capodanno (New Year's Day)"},
    {6,  1,  "Epifania (Epiphany)"},
    {5,  4,  "Pasqua (Easter Sunday)"},         // 2026: April 5
    {6,  4,  "Lunedì di Pasqua (Easter Monday)"}, // 2026: April 6
    {25, 4,  "Festa della Liberazione (Liberation Day)"},
    {1,  5,  "Festa dei Lavoratori (Labour Day)"},
    {2,  6,  "Festa della Repubblica (Republic Day)"},
    {15, 8,  "Ferragosto (Assumption of Mary)"},
    {1,  11, "Ognissanti (All Saints' Day)"},
    {8,  12, "Immacolata Concezione (Immaculate Conception)"},
    {25, 12, "Natale (Christmas Day)"},
    {26, 12, "Santo Stefano (St. Stephen's Day)"}
};

#define HOLIDAY_COUNT (sizeof(ITALIAN_HOLIDAYS_2026) / sizeof(holiday_t))

/**
 * @brief Season start dates (day of year, non-leap year)
 */
typedef struct {
    uint16_t day_of_year;  /**< Day of year (1-365) */
    season_t season;       /**< Season enum */
} season_start_t;

/**
 * @brief Season boundaries based on astronomical dates
 * @note Days are calculated for non-leap years
 */
static const season_start_t SEASON_STARTS[] = {
    {80,  SEASON_SPRING},  // March 20 (day 79 + 1)
    {172, SEASON_SUMMER},  // June 21 (day 171 + 1)
    {265, SEASON_AUTUMN},  // September 22 (day 264 + 1)
    {355, SEASON_WINTER}   // December 21 (day 354 + 1)
};

/**
 * @brief Check if a year is a leap year
 * 
 * @param year Year (e.g., 2026)
 * @return true if leap year, false otherwise
 */
static bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/**
 * @brief Calculate day of year from date
 * 
 * @param year Year (e.g., 2026)
 * @param month Month (1-12)
 * @param day Day of month (1-31)
 * @return Day of year (1-366) or 0 on error
 */
static uint16_t calculate_day_of_year(int year, int month, int day) {
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        ESP_LOGE(TAG, "Invalid date: %04d-%02d-%02d", year, month, day);
        return 0;
    }
    
    // Days in each month (non-leap year)
    static const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    uint16_t doy = day;
    for (int m = 0; m < month - 1; m++) {
        doy += days_in_month[m];
    }
    
    // Add leap day if needed
    if (month > 2 && is_leap_year(year)) {
        doy++;
    }
    
    return doy;
}

/**
 * @brief Determine season from day of year
 * 
 * @param day_of_year Day of year (1-366)
 * @param year Year (for leap year adjustment)
 * @return Current season
 */
static season_t determine_season(uint16_t day_of_year, int year) {
    // Adjust for leap years (all dates after Feb 29 shift by 1)
    uint16_t adjusted_doy = day_of_year;
    if (is_leap_year(year) && day_of_year > 60) { // After Feb 29
        adjusted_doy--;
    }
    
    // Find current season
    season_t current_season = SEASON_WINTER; // Default for early January
    
    for (int i = 0; i < 4; i++) {
        if (adjusted_doy >= SEASON_STARTS[i].day_of_year) {
            current_season = SEASON_STARTS[i].season;
        }
    }
    
    return current_season;
}

/**
 * @brief Check if date is a weekend (Saturday or Sunday)
 * 
 * @param timeinfo Pointer to tm structure
 * @return true if weekend, false otherwise
 */
static bool is_weekend(const struct tm *timeinfo) {
    // tm_wday: 0=Sunday, 6=Saturday
    return (timeinfo->tm_wday == 0 || timeinfo->tm_wday == 6);
}

/**
 * @brief Find holiday by date
 * 
 * @param day Day of month
 * @param month Month (1-12)
 * @return Pointer to holiday name or NULL if not a holiday
 */
static const char* find_holiday(uint8_t day, uint8_t month) {
    for (size_t i = 0; i < HOLIDAY_COUNT; i++) {
        if (ITALIAN_HOLIDAYS_2026[i].day == day && 
            ITALIAN_HOLIDAYS_2026[i].month == month) {
            return ITALIAN_HOLIDAYS_2026[i].name;
        }
    }
    return NULL;
}

/**
 * @brief Calculate calendar data from Unix timestamp
 * 
 * @param calendar Pointer to calendar_data_t structure to fill
 * @param timestamp Current Unix timestamp
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t calendar_calculate(calendar_data_t *calendar, time_t timestamp) {
    if (!calendar) {
        ESP_LOGE(TAG, "NULL calendar pointer");
        return ESP_FAIL;
    }
    
    struct tm timeinfo;
    if (localtime_r(&timestamp, &timeinfo) == NULL) {
        ESP_LOGE(TAG, "Failed to convert timestamp to local time");
        return ESP_FAIL;
    }
    
    // Calculate day of year
    calendar->day_of_year = calculate_day_of_year(
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday
    );
    
    if (calendar->day_of_year == 0) {
        ESP_LOGE(TAG, "Failed to calculate day of year");
        return ESP_FAIL;
    }
    
    // Determine season
    calendar->season = determine_season(calendar->day_of_year, timeinfo.tm_year + 1900);
    
    // Check weekend
    calendar->is_weekend = is_weekend(&timeinfo);
    
    // Check holiday
    calendar->holiday_name = find_holiday(timeinfo.tm_mday, timeinfo.tm_mon + 1);
    calendar->is_holiday = (calendar->holiday_name != NULL);
    
    ESP_LOGI(TAG, "Calendar data for %04d-%02d-%02d:",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    ESP_LOGI(TAG, "  Day of year: %d", calendar->day_of_year);
    ESP_LOGI(TAG, "  Season: %s", get_season_str(calendar->season));
    ESP_LOGI(TAG, "  Weekend: %s", calendar->is_weekend ? "Yes" : "No");
    
    if (calendar->is_holiday) {
        ESP_LOGI(TAG, "  Holiday: %s", calendar->holiday_name);
    }
    
    return ESP_OK;
}

/**
 * @brief Get number of days in a specific month
 * 
 * @param year Year (e.g., 2026)
 * @param month Month (1-12)
 * @return Number of days in month (28-31) or 0 on error
 */
uint8_t calendar_get_days_in_month(int year, int month) {
    if (month < 1 || month > 12) {
        ESP_LOGE(TAG, "Invalid month: %d", month);
        return 0;
    }
    
    static const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    uint8_t days = days_in_month[month - 1];
    
    // Adjust for leap year February
    if (month == 2 && is_leap_year(year)) {
        days = 29;
    }
    
    return days;
}

/**
 * @brief Check if a specific date is a holiday
 * 
 * @param day Day of month
 * @param month Month (1-12)
 * @return Pointer to holiday name or NULL if not a holiday
 */
const char* calendar_is_holiday(uint8_t day, uint8_t month) {
    return find_holiday(day, month);
}

/**
 * @brief Get season name for a specific day of year
 * 
 * @param day_of_year Day of year (1-366)
 * @param year Year (for leap year adjustment)
 * @return Season name string
 */
const char* calendar_get_season_for_day(uint16_t day_of_year, int year) {
    season_t season = determine_season(day_of_year, year);
    return get_season_str(season);
}
