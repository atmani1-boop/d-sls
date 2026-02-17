/**
 * @file task_fusion.h
 * @brief Fusion task header - intelligent sensor fusion and profile selection
 * 
 * FreeRTOS task that performs periodic sensor fusion, profile selection,
 * and global state updates for the DIAMANT v2.1 system.
 */

#ifndef TASK_FUSION_H
#define TASK_FUSION_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

//============================================================================
// TASK CONFIGURATION
//============================================================================

#define FUSION_TASK_NAME            "fusion_task"
#define FUSION_TASK_PRIORITY        3
#define FUSION_TASK_STACK_SIZE      4096
#define FUSION_TASK_CYCLE_MS        500     // 500ms cycle time

//============================================================================
// FUNCTION PROTOTYPES
//============================================================================

/**
 * @brief Create and start the fusion task
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t task_fusion_start(void);

/**
 * @brief Stop the fusion task
 */
void task_fusion_stop(void);

/**
 * @brief Get fusion task handle
 * @return Task handle or NULL if not running
 */
TaskHandle_t task_fusion_get_handle(void);

#endif // TASK_FUSION_H
