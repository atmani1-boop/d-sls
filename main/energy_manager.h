/**
 * @file energy_manager.h
 * @brief Energy management module
 * 
 * Monitors INA226 power sensor and implements protection features.
 */

#ifndef ENERGY_MANAGER_H
#define ENERGY_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Energy alert event
 */
#define EVT_ENERGY_ALERT  (1 << 2)

/**
 * @brief Energy status structure
 */
typedef struct {
    float voltage_v;
    uint32_t current_ma;
    uint32_t power_w;
    
    // Status flags
    bool overcurrent;
    bool overpower;
    bool undervoltage;
} energy_status_t;

/**
 * @brief Initialize energy manager
 * 
 * Sets up I2C bus and INA226 sensor.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t energy_manager_init(void);

/**
 * @brief Start energy manager task
 * 
 * Begins monitoring power consumption.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t energy_manager_start(void);

/**
 * @brief Stop energy manager task
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t energy_manager_stop(void);

/**
 * @brief Get current energy status
 * 
 * @param status Pointer to status structure to fill
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t energy_manager_get_status(energy_status_t *status);

/**
 * @brief Check if any protection is active
 * 
 * @return true if protection triggered, false otherwise
 */
bool energy_manager_is_protection_active(void);

#endif // ENERGY_MANAGER_H
