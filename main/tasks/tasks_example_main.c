/**
 * @file tasks_example_main.c
 * @brief Example main file demonstrating FreeRTOS task usage
 * 
 * This example shows how to initialize and start all three tasks:
 *   - Fusion task (Priority 3, 500ms cycle)
 *   - MRAM task (Priority 2, 30-second cycle)
 *   - Web server task (Priority 2)
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

// Task headers
#include "tasks/task_fusion.h"
#include "tasks/task_mram.h"
#include "tasks/task_webserver.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "  DIAMANT v2.1 - ESP32-C6 System Starting");
    ESP_LOGI(TAG, "==============================================");

    // Initialize NVS (required for WiFi and storage)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI(TAG, "Erasing NVS flash...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Flash initialized");

    // Start fusion task (Priority 3, 500ms cycle)
    ESP_LOGI(TAG, "Starting Fusion Task...");
    ret = task_fusion_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start fusion task: %s", esp_err_to_name(ret));
    }

    // Give fusion task time to initialize
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Start MRAM task (Priority 2, 30-second cycle)
    ESP_LOGI(TAG, "Starting MRAM Task...");
    ret = task_mram_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MRAM task: %s", esp_err_to_name(ret));
    }

    // Start web server task (Priority 2)
    ESP_LOGI(TAG, "Starting Web Server Task...");
    ret = task_webserver_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server task: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "  All tasks started successfully");
    ESP_LOGI(TAG, "  System is now running");
    ESP_LOGI(TAG, "==============================================");

    // Main loop - monitor system health
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000));  // Every 30 seconds
        
        ESP_LOGI(TAG, "System Status:");
        ESP_LOGI(TAG, "  - Fusion Task: %s", 
                 task_fusion_get_handle() ? "Running" : "Stopped");
        ESP_LOGI(TAG, "  - MRAM Task: %s", 
                 task_mram_get_handle() ? "Running" : "Stopped");
        ESP_LOGI(TAG, "  - Web Server: %s", 
                 task_webserver_is_running() ? "Running" : "Stopped");
        ESP_LOGI(TAG, "  - Free Heap: %lu bytes", esp_get_free_heap_size());
        ESP_LOGI(TAG, "  - Min Free Heap: %lu bytes", esp_get_minimum_free_heap_size());
    }
}
