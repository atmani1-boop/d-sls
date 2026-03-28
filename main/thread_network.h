/**
 * @file thread_network.h
 * @brief Thread IPv6 mesh networking module
 * 
 * Implements Thread 1.3 Full Thread Device (FTD) with border router capabilities.
 */

#ifndef THREAD_NETWORK_H
#define THREAD_NETWORK_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief Thread network states
 */
typedef enum {
    THREAD_STATE_DISABLED = 0,
    THREAD_STATE_DETACHED,
    THREAD_STATE_CHILD,
    THREAD_STATE_ROUTER,
    THREAD_STATE_LEADER
} thread_state_t;

/**
 * @brief Thread network information
 */
typedef struct {
    thread_state_t state;
    uint8_t channel;
    uint16_t pan_id;
    char ipv6_addr[64];
    uint8_t node_type;  // 0=End Device, 1=Router, 2=Leader
} thread_network_info_t;

/**
 * @brief Network ready event bit
 */
#define EVT_NET_READY  (1 << 0)

/**
 * @brief Initialize Thread network module
 * 
 * Sets up OpenThread stack and starts Thread FTD task.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t thread_network_init(void);

/**
 * @brief Start Thread network
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t thread_network_start(void);

/**
 * @brief Stop Thread network
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t thread_network_stop(void);

/**
 * @brief Get current Thread network state
 * 
 * @return Current Thread state
 */
thread_state_t thread_network_get_state(void);

/**
 * @brief Get Thread network information
 * 
 * @param info Pointer to info structure to fill
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t thread_network_get_info(thread_network_info_t *info);

#endif // THREAD_NETWORK_H
