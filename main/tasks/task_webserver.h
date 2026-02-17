/**
 * @file task_webserver.h
 * @brief Web server task header - WiFi AP and HTTP server management
 * 
 * FreeRTOS task that manages WiFi Access Point and HTTP web server
 * for real-time monitoring and configuration of the DIAMANT v2.1 system.
 */

#ifndef TASK_WEBSERVER_H
#define TASK_WEBSERVER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

//============================================================================
// TASK CONFIGURATION
//============================================================================

#define WEBSERVER_TASK_NAME         "webserver_task"
#define WEBSERVER_TASK_PRIORITY     2
#define WEBSERVER_TASK_STACK_SIZE   4096

//============================================================================
// FUNCTION PROTOTYPES
//============================================================================

/**
 * @brief Create and start the web server task
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t task_webserver_start(void);

/**
 * @brief Stop the web server task
 */
void task_webserver_stop(void);

/**
 * @brief Get web server task handle
 * @return Task handle or NULL if not running
 */
TaskHandle_t task_webserver_get_handle(void);

/**
 * @brief Check if web server is running
 * @return true if running, false otherwise
 */
bool task_webserver_is_running(void);

#endif // TASK_WEBSERVER_H
