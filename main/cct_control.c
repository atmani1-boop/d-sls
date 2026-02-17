/**
 * @file cct_control.c
 * @brief DIAMANT v2.1 REV E - Dynamic CCT (Color Temperature) Control
 * @version 2.1.0
 * @date 2026-02-17
 * 
 * Calculates warm/cool LED mixing ratios for target color temperatures
 */

#include "include/profiles.h"
#include <stdint.h>
#include <math.h>

// For logging (can be replaced with custom logger)
#ifdef ESP_PLATFORM
#include "esp_log.h"
static const char *TAG = "CCT_CONTROL";
#else
#include <stdio.h>
#define ESP_LOGD(tag, fmt, ...) printf("[%s] " fmt "\n", tag, ##__VA_ARGS__)
#endif

// System CCT range
#define CCT_WARM 2700  // Warm LED (2700K)
#define CCT_COOL 5000  // Cool LED (5000K)

/**
 * @brief Calculate warm/cool LED mix for target CCT
 * @param cct_kelvin Target color temperature (2700-5000K)
 * @param intensity_percent Overall intensity (0-100%)
 * @param warm_out Output: warm LED intensity (0-100%)
 * @param cool_out Output: cool LED intensity (0-100%)
 * 
 * Uses linear interpolation between warm (2700K) and cool (5000K) LEDs.
 * Future enhancement: perceptual color mixing for better color accuracy.
 */
void cct_calculate_mix(uint16_t cct_kelvin, uint8_t intensity_percent,
                       uint8_t *warm_out, uint8_t *cool_out) {
    // Clamp target CCT to system range
    if (cct_kelvin < CCT_WARM) cct_kelvin = CCT_WARM;
    if (cct_kelvin > CCT_COOL) cct_kelvin = CCT_COOL;
    
    // Calculate mixing ratio (0.0 = all warm, 1.0 = all cool)
    float ratio = (float)(cct_kelvin - CCT_WARM) / (CCT_COOL - CCT_WARM);
    
    // Linear interpolation
    *warm_out = (uint8_t)(intensity_percent * (1.0f - ratio));
    *cool_out = (uint8_t)(intensity_percent * ratio);
    
#ifdef ESP_PLATFORM
    ESP_LOGD(TAG, "CCT: %dK → Warm=%d%%, Cool=%d%% (intensity=%d%%)", 
             cct_kelvin, *warm_out, *cool_out, intensity_percent);
#endif
}

/**
 * @brief Calculate CCT from warm/cool mix percentages
 * @param warm_percent Warm LED percentage (0-100%)
 * @param cool_percent Cool LED percentage (0-100%)
 * @return Resulting CCT in Kelvin
 * 
 * Reverse calculation for diagnostics/feedback
 */
uint16_t cct_calculate_kelvin(uint8_t warm_percent, uint8_t cool_percent) {
    uint8_t total = warm_percent + cool_percent;
    if (total == 0) {
        return CCT_WARM; // Default to warm
    }
    
    float cool_ratio = (float)cool_percent / total;
    uint16_t cct = CCT_WARM + (uint16_t)(cool_ratio * (CCT_COOL - CCT_WARM));
    
    return cct;
}

/**
 * @brief Get CCT description string
 * @param cct_kelvin Color temperature in Kelvin
 * @return Human-readable description
 */
const char* cct_get_description(uint16_t cct_kelvin) {
    if (cct_kelvin < 2800) {
        return "Warm (Sunset)";
    } else if (cct_kelvin < 3200) {
        return "Warm White";
    } else if (cct_kelvin < 4200) {
        return "Neutral White";
    } else if (cct_kelvin < 4800) {
        return "Cool White";
    } else {
        return "Daylight (Alert)";
    }
}

/**
 * @brief Validate CCT value
 * @param cct_kelvin Color temperature to validate
 * @return true if within valid range
 */
bool cct_is_valid(uint16_t cct_kelvin) {
    return (cct_kelvin >= CCT_WARM && cct_kelvin <= CCT_COOL);
}
