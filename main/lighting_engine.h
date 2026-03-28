/**
 * @file lighting_engine.h
 * @brief Lighting engine module
 * 
 * Controls 56 RGBW fixtures via PWM with gamma correction,
 * global intensity, and emergency dimming.
 */

#ifndef LIGHTING_ENGINE_H
#define LIGHTING_ENGINE_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief Number of RGBW fixtures
 */
#define NUM_FIXTURES 56

/**
 * @brief Channels per fixture (RGBW)
 */
#define CHANNELS_PER_FIXTURE 4

/**
 * @brief Total DMX channels needed
 */
#define TOTAL_DMX_CHANNELS (NUM_FIXTURES * CHANNELS_PER_FIXTURE)  // 224

/**
 * @brief RGBW fixture structure
 */
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t white;
} fixture_rgbw_t;

/**
 * @brief Initialize lighting engine
 * 
 * Sets up PWM channels and gamma correction table.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lighting_engine_init(void);

/**
 * @brief Start lighting engine task
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lighting_engine_start(void);

/**
 * @brief Stop lighting engine task
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lighting_engine_stop(void);

/**
 * @brief Update fixtures from DMX data
 * 
 * @param dmx_data DMX buffer (512 channels)
 * @param start_address DMX start address (1-512)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lighting_engine_update_from_dmx(const uint8_t *dmx_data, uint16_t start_address);

/**
 * @brief Set fixture directly
 * 
 * @param fixture_id Fixture index (0-55)
 * @param rgbw RGBW values
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lighting_engine_set_fixture(uint8_t fixture_id, const fixture_rgbw_t *rgbw);

/**
 * @brief Set all fixtures
 * 
 * @param fixtures Array of 56 fixtures
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lighting_engine_set_all_fixtures(const fixture_rgbw_t *fixtures);

/**
 * @brief Set global intensity
 * 
 * @param intensity Intensity (0-255)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lighting_engine_set_global_intensity(uint8_t intensity);

/**
 * @brief Get global intensity
 * 
 * @return Current global intensity (0-255)
 */
uint8_t lighting_engine_get_global_intensity(void);

/**
 * @brief Set emergency dimming percentage
 * 
 * @param percent Dimming percentage (0-100)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lighting_engine_set_emergency_dim(uint8_t percent);

/**
 * @brief Blackout all fixtures
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lighting_engine_blackout(void);

/**
 * @brief Apply gamma correction to 8-bit value
 * 
 * @param value Input value (0-255)
 * @return Gamma-corrected value (0-8191)
 */
uint16_t lighting_engine_gamma_correct(uint8_t value);

#endif // LIGHTING_ENGINE_H
