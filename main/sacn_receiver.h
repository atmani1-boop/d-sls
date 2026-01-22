/**
 * @file sacn_receiver.h
 * @brief sACN (E1.31) receiver module
 * 
 * Implements sACN streaming DMX protocol over IPv6 multicast.
 */

#ifndef SACN_RECEIVER_H
#define SACN_RECEIVER_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief sACN frame received event
 */
#define EVT_SACN_FRAME  (1 << 1)

/**
 * @brief DMX data buffer (512 channels)
 */
#define DMX_CHANNELS 512

/**
 * @brief sACN timeout (ms)
 */
#define SACN_TIMEOUT_MS 2500

/**
 * @brief Initialize sACN receiver
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sacn_receiver_init(void);

/**
 * @brief Start sACN receiver
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sacn_receiver_start(void);

/**
 * @brief Stop sACN receiver
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sacn_receiver_stop(void);

/**
 * @brief Get DMX data buffer
 * 
 * @param buffer Pointer to receive buffer pointer (512 bytes)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sacn_receiver_get_dmx(const uint8_t **buffer);

/**
 * @brief Check if sACN data is fresh (received within timeout)
 * 
 * @return true if fresh, false if timed out
 */
bool sacn_receiver_is_active(void);

/**
 * @brief Get last frame timestamp (ms)
 * 
 * @return Timestamp of last received frame
 */
uint32_t sacn_receiver_get_last_frame_time(void);

#endif // SACN_RECEIVER_H
