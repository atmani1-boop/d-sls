/**
 * @file ui_manager.h
 * @brief User interface management module
 * 
 * Controls RGB LED status indicator and UART CLI.
 */

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief UI states
 */
typedef enum {
    UI_STATE_OFF = 0,
    UI_STATE_BOOTING,
    UI_STATE_NORMAL,
    UI_STATE_WARNING,
    UI_STATE_ERROR,
    UI_STATE_IDENTIFY
} ui_state_t;

/**
 * @brief Initialize UI manager
 * 
 * Sets up RGB LED GPIOs and UART CLI.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_manager_init(void);

/**
 * @brief Start UI manager task
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_manager_start(void);

/**
 * @brief Stop UI manager task
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_manager_stop(void);

/**
 * @brief Set UI state
 * 
 * Changes RGB LED behavior based on state.
 * 
 * @param state New UI state
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_manager_set_state(ui_state_t state);

/**
 * @brief Get current UI state
 * 
 * @return Current UI state
 */
ui_state_t ui_manager_get_state(void);

#endif // UI_MANAGER_H
