/**
 * @file task_mram.h
 * @brief MRAM storage task header - periodic data logging and analytics
 * 
 * FreeRTOS task that manages persistent storage operations including
 * historical logging, statistics calculation, and predictive analytics.
 */

#ifndef TASK_MRAM_H
#define TASK_MRAM_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

//============================================================================
// TASK CONFIGURATION
//============================================================================

#define MRAM_TASK_NAME              "mram_task"
#define MRAM_TASK_PRIORITY          2
#define MRAM_TASK_STACK_SIZE        4096
#define MRAM_TASK_CYCLE_MS          30000   // 30-second cycle

// Operation intervals (in seconds)
#define MRAM_LOG_INTERVAL_SEC       1800    // 30 minutes (configurable)
#define MRAM_STATS_INTERVAL_SEC     3600    // 1 hour
#define MRAM_CONFIG_UPDATE_SEC      86400   // 24 hours

//============================================================================
// FUNCTION PROTOTYPES
//============================================================================

/**
 * @brief Create and start the MRAM task
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t task_mram_start(void);

/**
 * @brief Stop the MRAM task
 */
void task_mram_stop(void);

/**
 * @brief Get MRAM task handle
 * @return Task handle or NULL if not running
 */
TaskHandle_t task_mram_get_handle(void);

/**
 * @brief Trigger immediate data log (outside normal schedule)
 * @return ESP_OK on success
 */
esp_err_t task_mram_log_now(void);

#endif // TASK_MRAM_H
