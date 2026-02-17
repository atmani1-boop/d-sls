/**
 * @file test_profiles.c
 * @brief Validate all 35 profiles and their configurations
 */

#include "include/profiles.h"
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

// Test counters
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            tests_passed++; \
            printf("✓ %s\n", message); \
        } else { \
            tests_failed++; \
            printf("✗ FAILED: %s\n", message); \
        } \
    } while(0)

void test_led_profiles(void) {
    printf("\n=== Testing LED Profiles ===\n");
    
    // Test profile count
    TEST_ASSERT(LED_PROFILE_COUNT == 10, "LED profile count is 10");
    
    // Test all profiles have unique hex IDs
    bool unique = true;
    for (size_t i = 0; i < LED_PROFILE_COUNT; i++) {
        for (size_t j = i + 1; j < LED_PROFILE_COUNT; j++) {
            if (led_configs[i].id_hex == led_configs[j].id_hex) {
                unique = false;
                printf("  Duplicate ID found: 0x%02X\n", led_configs[i].id_hex);
            }
        }
    }
    TEST_ASSERT(unique, "All LED profiles have unique hex IDs");
    
    // Test hex IDs are in range 0x00-0x09
    bool in_range = true;
    for (size_t i = 0; i < LED_PROFILE_COUNT; i++) {
        if (led_configs[i].id_hex < 0x00 || led_configs[i].id_hex > 0x09) {
            in_range = false;
            printf("  ID out of range: 0x%02X (%s)\n", 
                   led_configs[i].id_hex, led_configs[i].name);
        }
    }
    TEST_ASSERT(in_range, "All LED profile IDs in range 0x00-0x09");
    
    // Test intensity range (0-100)
    bool intensity_valid = true;
    for (size_t i = 0; i < LED_PROFILE_COUNT; i++) {
        if (led_configs[i].intensity > 100) {
            intensity_valid = false;
            printf("  Invalid intensity: %d%% (%s)\n", 
                   led_configs[i].intensity, led_configs[i].name);
        }
    }
    TEST_ASSERT(intensity_valid, "All LED intensities are 0-100%");
    
    // Test CCT range (2700-5000K)
    bool cct_valid = true;
    for (size_t i = 0; i < LED_PROFILE_COUNT; i++) {
        if (led_configs[i].cct_kelvin < 2700 || led_configs[i].cct_kelvin > 5000) {
            cct_valid = false;
            printf("  Invalid CCT: %dK (%s)\n", 
                   led_configs[i].cct_kelvin, led_configs[i].name);
        }
    }
    TEST_ASSERT(cct_valid, "All LED CCT values in range 2700-5000K");
    
    // Test priority range (0-7)
    bool priority_valid = true;
    for (size_t i = 0; i < LED_PROFILE_COUNT; i++) {
        if (led_configs[i].priority > 7) {
            priority_valid = false;
            printf("  Invalid priority: P%d (%s)\n", 
                   led_configs[i].priority, led_configs[i].name);
        }
    }
    TEST_ASSERT(priority_valid, "All LED priorities in range P0-P7");
    
    // Test specific profiles
    TEST_ASSERT(led_configs[LED_SUNSET_ON].id_hex == 0x00, "SUNSET_ON has ID 0x00");
    TEST_ASSERT(led_configs[LED_FOG_BOOST].night_saver_enabled == false, 
                "FOG_BOOST bypasses Night-Saver (safety)");
    TEST_ASSERT(led_configs[LED_RAIN_ALERT].night_saver_enabled == false, 
                "RAIN_ALERT bypasses Night-Saver (safety)");
    TEST_ASSERT(led_configs[LED_LOW_SOC_PROTECT].night_saver_enabled == false, 
                "LOW_SOC_PROTECT bypasses Night-Saver");
}

void test_mppt_profiles(void) {
    printf("\n=== Testing MPPT Profiles ===\n");
    
    TEST_ASSERT(MPPT_PROFILE_COUNT == 7, "MPPT profile count is 7");
    
    // Test hex IDs are in range 0x10-0x16
    bool in_range = true;
    for (size_t i = 0; i < MPPT_PROFILE_COUNT; i++) {
        if (mppt_configs[i].id_hex < 0x10 || mppt_configs[i].id_hex > 0x16) {
            in_range = false;
            printf("  ID out of range: 0x%02X (%s)\n", 
                   mppt_configs[i].id_hex, mppt_configs[i].name);
        }
    }
    TEST_ASSERT(in_range, "All MPPT profile IDs in range 0x10-0x16");
    
    // Test multipliers are positive
    bool multipliers_valid = true;
    for (size_t i = 0; i < MPPT_PROFILE_COUNT; i++) {
        if (mppt_configs[i].step_multiplier < 0 || 
            mppt_configs[i].update_period_multiplier < 0 ||
            mppt_configs[i].max_current_limit < 0) {
            multipliers_valid = false;
            printf("  Invalid multipliers: %s\n", mppt_configs[i].name);
        }
    }
    TEST_ASSERT(multipliers_valid, "All MPPT multipliers are positive");
    
    TEST_ASSERT(mppt_configs[0].id_hex == 0x10, "AGGRESSIVE_SUNNY has ID 0x10");
    TEST_ASSERT(mppt_configs[6].id_hex == 0x16, "EVENING_SAVE has ID 0x16");
}

void test_v2g_profiles(void) {
    printf("\n=== Testing V2G Profiles ===\n");
    
    TEST_ASSERT(V2G_PROFILE_COUNT == 7, "V2G profile count is 7");
    
    // Test hex IDs are in range 0x20-0x26
    bool in_range = true;
    for (size_t i = 0; i < V2G_PROFILE_COUNT; i++) {
        if (v2g_configs[i].id_hex < 0x20 || v2g_configs[i].id_hex > 0x26) {
            in_range = false;
            printf("  ID out of range: 0x%02X (%s)\n", 
                   v2g_configs[i].id_hex, v2g_configs[i].name);
        }
    }
    TEST_ASSERT(in_range, "All V2G profile IDs in range 0x20-0x26");
    
    TEST_ASSERT(v2g_configs[0].id_hex == 0x20, "EXPORT_DAY_SUNNY has ID 0x20");
    TEST_ASSERT(v2g_configs[6].id_hex == 0x26, "EMERGENCY_ISLAND has ID 0x26");
    
    // Test power flow logic
    TEST_ASSERT(v2g_configs[0].power_w > 0, "EXPORT_DAY_SUNNY exports power (+)");
    TEST_ASSERT(v2g_configs[2].power_w < 0, "IMPORT_LOW_SOC imports power (-)");
    TEST_ASSERT(v2g_configs[4].power_w == 0, "HOLD_BALANCED has no power flow");
}

void test_battery_profiles(void) {
    printf("\n=== Testing Battery Profiles ===\n");
    
    TEST_ASSERT(BATTERY_PROFILE_COUNT == 4, "Battery profile count is 4");
    
    // Test hex IDs are in range 0x30-0x33
    bool in_range = true;
    for (size_t i = 0; i < BATTERY_PROFILE_COUNT; i++) {
        if (battery_configs[i].id_hex < 0x30 || battery_configs[i].id_hex > 0x33) {
            in_range = false;
        }
    }
    TEST_ASSERT(in_range, "All Battery profile IDs in range 0x30-0x33");
    
    TEST_ASSERT(battery_configs[0].id_hex == 0x30, "SAFE_CHARGE has ID 0x30");
}

void test_weather_profiles(void) {
    printf("\n=== Testing Weather Detection Profiles ===\n");
    
    TEST_ASSERT(WEATHER_PROFILE_COUNT == 4, "Weather profile count is 4");
    
    // Test hex IDs are in range 0x40-0x43
    bool in_range = true;
    for (size_t i = 0; i < WEATHER_PROFILE_COUNT; i++) {
        if (weather_configs[i].id_hex < 0x40 || weather_configs[i].id_hex > 0x43) {
            in_range = false;
        }
    }
    TEST_ASSERT(in_range, "All Weather profile IDs in range 0x40-0x43");
}

void test_astro_profiles(void) {
    printf("\n=== Testing Astronomical Profiles ===\n");
    
    TEST_ASSERT(ASTRO_PROFILE_COUNT == 3, "Astro profile count is 3");
    
    // Test hex IDs are in range 0x50-0x52
    bool in_range = true;
    for (size_t i = 0; i < ASTRO_PROFILE_COUNT; i++) {
        if (astro_configs[i].id_hex < 0x50 || astro_configs[i].id_hex > 0x52) {
            in_range = false;
        }
    }
    TEST_ASSERT(in_range, "All Astro profile IDs in range 0x50-0x52");
}

void test_calendar_profiles(void) {
    printf("\n=== Testing Calendar Profiles ===\n");
    
    TEST_ASSERT(CALENDAR_PROFILE_COUNT == 3, "Calendar profile count is 3");
    
    // Test hex IDs are in range 0x60-0x62
    bool in_range = true;
    for (size_t i = 0; i < CALENDAR_PROFILE_COUNT; i++) {
        if (calendar_configs[i].id_hex < 0x60 || calendar_configs[i].id_hex > 0x62) {
            in_range = false;
        }
    }
    TEST_ASSERT(in_range, "All Calendar profile IDs in range 0x60-0x62");
}

void test_total_profiles(void) {
    printf("\n=== Testing Total Profile Count ===\n");
    
    size_t total = LED_PROFILE_COUNT + MPPT_PROFILE_COUNT + V2G_PROFILE_COUNT +
                   BATTERY_PROFILE_COUNT + WEATHER_PROFILE_COUNT + 
                   ASTRO_PROFILE_COUNT + CALENDAR_PROFILE_COUNT;
    
    TEST_ASSERT(total == 38, "Total profile count is 38 (10+7+7+4+4+3+3)");
    
    printf("\nProfile breakdown:\n");
    printf("  LED: %zu\n", LED_PROFILE_COUNT);
    printf("  MPPT: %zu\n", MPPT_PROFILE_COUNT);
    printf("  V2G: %zu\n", V2G_PROFILE_COUNT);
    printf("  Battery: %zu\n", BATTERY_PROFILE_COUNT);
    printf("  Weather: %zu\n", WEATHER_PROFILE_COUNT);
    printf("  Astro: %zu\n", ASTRO_PROFILE_COUNT);
    printf("  Calendar: %zu\n", CALENDAR_PROFILE_COUNT);
    printf("  TOTAL: %zu\n", total);
}

int main(void) {
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║  DIAMANT v2.1 REV E - Profile Validation Tests     ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    
    test_led_profiles();
    test_mppt_profiles();
    test_v2g_profiles();
    test_battery_profiles();
    test_weather_profiles();
    test_astro_profiles();
    test_calendar_profiles();
    test_total_profiles();
    
    printf("\n╔══════════════════════════════════════════════════════╗\n");
    printf("║  Test Results                                       ║\n");
    printf("╠══════════════════════════════════════════════════════╣\n");
    printf("║  Passed: %-3d                                        ║\n", tests_passed);
    printf("║  Failed: %-3d                                        ║\n", tests_failed);
    printf("╚══════════════════════════════════════════════════════╝\n");
    
    return tests_failed > 0 ? 1 : 0;
}
