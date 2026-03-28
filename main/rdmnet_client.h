/**
 * @file rdmnet_client.h
 * @brief RDMnet (E1.33) client module
 * 
 * Implements RDMnet protocol for remote configuration, sensor reporting,
 * and device identification.
 */

#ifndef RDMNET_CLIENT_H
#define RDMNET_CLIENT_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief RDMnet PIDs (Parameter IDs)
 */
typedef enum {
    PID_DEVICE_INFO = 0x0060,
    PID_DMX_START_ADDRESS = 0x00F0,
    PID_IDENTIFY_DEVICE = 0x1000,
    PID_DEVICE_LABEL = 0x0082,
    PID_SENSOR_DEFINITION = 0x0200,
    PID_SENSOR_VALUE = 0x0201,
} rdmnet_pid_t;

/**
 * @brief Initialize RDMnet client
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t rdmnet_client_init(void);

/**
 * @brief Start RDMnet client
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t rdmnet_client_start(void);

/**
 * @brief Stop RDMnet client
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t rdmnet_client_stop(void);

/**
 * @brief Send sensor data via RDMnet
 * 
 * @param sensor_id Sensor identifier
 * @param value Sensor value
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t rdmnet_client_send_sensor(uint8_t sensor_id, float value);

/**
 * @brief Trigger identify mode
 * 
 * @param enable Enable or disable identify
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t rdmnet_client_identify(bool enable);

#endif // RDMNET_CLIENT_H
