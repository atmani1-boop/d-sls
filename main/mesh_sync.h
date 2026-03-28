/**
 * @file mesh_sync.h
 * @brief Mesh synchronization module
 * 
 * Synchronizes scenes and global intensity across Thread mesh nodes.
 */

#ifndef MESH_SYNC_H
#define MESH_SYNC_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief Initialize mesh sync module
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mesh_sync_init(void);

/**
 * @brief Start mesh sync task
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mesh_sync_start(void);

/**
 * @brief Stop mesh sync task
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mesh_sync_stop(void);

/**
 * @brief Broadcast scene activation to mesh
 * 
 * @param scene_id Scene index (0-15)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mesh_sync_broadcast_scene(uint8_t scene_id);

/**
 * @brief Broadcast global intensity to mesh
 * 
 * @param intensity Global intensity (0-255)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mesh_sync_broadcast_intensity(uint8_t intensity);

#endif // MESH_SYNC_H
