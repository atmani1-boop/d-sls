/**
 * @file night_saver.c
 * @brief DIAMANT v2.1 REV E - Night-Saver SOC-Based Dimming Algorithm
 * @version 2.1.0
 * @date 2026-02-17
 * 
 * Implements battery SOC-based LED intensity reduction to extend runtime
 */

#include "include/profiles.h"
#include <stdint.h>
#include <stdbool.h>

// For logging (can be replaced with custom logger)
#ifdef ESP_PLATFORM
#include "esp_log.h"
static const char *TAG = "NIGHT_SAVER";
#else
#include <stdio.h>
#define ESP_LOGD(tag, fmt, ...) printf("[%s] " fmt "\n", tag, ##__VA_ARGS__)
#endif

/**
 * @brief Calculate SOC factor for Night-Saver algorithm
 * @param soc_percent Battery state of charge (0-100%)
 * @return Multiplier factor (0.3-1.0)
 * 
 * SOC Thresholds:
 * - ≥60%: 100% brightness (1.0)
 * - 40-59%: 80% brightness (0.8)
 * - 20-39%: 60% brightness (0.6)
 * - <20%: 30% brightness (0.3) - minimum for safety
 */
float night_saver_get_soc_factor(float soc_percent) {
    if (soc_percent >= 60.0f) {
        return 1.0f;  // Full brightness
    } else if (soc_percent >= 40.0f) {
        return 0.8f;  // 80% brightness
    } else if (soc_percent >= 20.0f) {
        return 0.6f;  // 60% brightness
    } else {
        return 0.3f;  // 30% brightness (minimum for safety)
    }
}

/**
 * @brief Apply Night-Saver algorithm to LED output
 * @param base_intensity Base LED intensity from profile (0-100%)
 * @param soc_percent Current battery SOC (0-100%)
 * @param night_saver_enabled Whether Night-Saver is enabled for this profile
 * @return Adjusted LED intensity (0-100%)
 * 
 * Safety profiles (FOG_BOOST, RAIN_ALERT, LOW_SOC_PROTECT) bypass Night-Saver
 * to maintain visibility in critical conditions.
 */
uint8_t night_saver_apply(uint8_t base_intensity, float soc_percent, bool night_saver_enabled) {
    if (!night_saver_enabled) {
        // Safety profiles bypass Night-Saver
#ifdef ESP_PLATFORM
        ESP_LOGD(TAG, "Night-Saver bypassed (safety profile): base=%d%%", base_intensity);
#endif
        return base_intensity;
    }
    
    float soc_factor = night_saver_get_soc_factor(soc_percent);
    uint8_t adjusted = (uint8_t)(base_intensity * soc_factor);
    
    // Ensure minimum visibility (never go below 10%)
    if (adjusted < 10 && base_intensity > 0) {
        adjusted = 10;
    }
    
#ifdef ESP_PLATFORM
    ESP_LOGD(TAG, "Night-Saver: base=%d%%, SOC=%.1f%%, factor=%.2f, adjusted=%d%%",
             base_intensity, soc_percent, soc_factor, adjusted);
#endif
    
    return adjusted;
}

/**
 * @brief Get human-readable description of SOC factor
 * @param soc_percent Battery state of charge (0-100%)
 * @return String description
 */
const char* night_saver_get_status_string(float soc_percent) {
    if (soc_percent >= 60.0f) {
        return "Full Brightness";
    } else if (soc_percent >= 40.0f) {
        return "80% Brightness";
    } else if (soc_percent >= 20.0f) {
        return "60% Brightness";
    } else {
        return "30% Brightness (Min)";
    }
}
