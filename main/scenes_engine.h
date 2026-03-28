/**
 * @file scenes_engine.h
 * @brief Scene management engine
 * 
 * Manages 16 programmable scenes with fade transitions and failover logic.
 */

#ifndef SCENES_ENGINE_H
#define SCENES_ENGINE_H

#include "esp_err.h"
#include "lighting_engine.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Maximum number of scenes
 */
#define MAX_SCENES 16

/**
 * @brief Scene name length
 */
#define SCENE_NAME_LEN 32

/**
 * @brief Scene structure
 */
typedef struct {
    char name[SCENE_NAME_LEN];
    fixture_rgbw_t fixtures[NUM_FIXTURES];
    uint16_t fade_time_ms;
    bool enabled;
} scene_t;

/**
 * @brief Initialize scenes engine
 * 
 * Loads scenes from NVS or creates defaults.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t scenes_engine_init(void);

/**
 * @brief Start scenes engine task
 * 
 * Monitors sACN failover and manages scene activation.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t scenes_engine_start(void);

/**
 * @brief Stop scenes engine task
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t scenes_engine_stop(void);

/**
 * @brief Activate a scene
 * 
 * @param scene_id Scene index (0-15)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t scenes_engine_activate(uint8_t scene_id);

/**
 * @brief Get scene by ID
 * 
 * @param scene_id Scene index (0-15)
 * @param scene Pointer to scene structure to fill
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t scenes_engine_get(uint8_t scene_id, scene_t *scene);

/**
 * @brief Save scene to NVS
 * 
 * @param scene_id Scene index (0-15)
 * @param scene Scene structure to save
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t scenes_engine_save(uint8_t scene_id, const scene_t *scene);

/**
 * @brief Load all scenes from NVS
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t scenes_engine_load_all(void);

/**
 * @brief Save all scenes to NVS
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t scenes_engine_save_all(void);

/**
 * @brief Get active scene ID
 * 
 * @return Active scene ID (0-15) or 255 if none
 */
uint8_t scenes_engine_get_active(void);

#endif // SCENES_ENGINE_H
