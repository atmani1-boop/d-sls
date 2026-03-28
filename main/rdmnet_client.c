/**
 * @file rdmnet_client.c
 * @brief RDMnet (E1.33) client implementation
 */

#include "rdmnet_client.h"
#include "config_manager.h"
#include "logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "RDMNET";

// Global state
static bool g_rdmnet_running = false;
static bool g_identify_mode = false;

/**
 * @brief Handle RDMnet GET request
 */
static esp_err_t handle_get_request(rdmnet_pid_t pid, uint8_t *response, size_t *response_len)
{
    const system_config_t *config = config_manager_get();
    
    switch (pid) {
        case PID_DEVICE_INFO:
            ESP_LOGI(TAG, "GET DEVICE_INFO");
            // TODO: Fill device info structure
            *response_len = 0;
            break;
            
        case PID_DMX_START_ADDRESS:
            ESP_LOGI(TAG, "GET DMX_START_ADDRESS: %d", config->start_address);
            if (response && response_len) {
                response[0] = (config->start_address >> 8) & 0xFF;
                response[1] = config->start_address & 0xFF;
                *response_len = 2;
            }
            break;
            
        case PID_DEVICE_LABEL:
            ESP_LOGI(TAG, "GET DEVICE_LABEL: %s", config->device_label);
            if (response && response_len) {
                strncpy((char *)response, config->device_label, 32);
                *response_len = strlen(config->device_label);
            }
            break;
            
        case PID_IDENTIFY_DEVICE:
            ESP_LOGI(TAG, "GET IDENTIFY_DEVICE: %s", g_identify_mode ? "ON" : "OFF");
            if (response && response_len) {
                response[0] = g_identify_mode ? 1 : 0;
                *response_len = 1;
            }
            break;
            
        default:
            ESP_LOGW(TAG, "Unsupported GET PID: 0x%04X", pid);
            return ESP_ERR_NOT_SUPPORTED;
    }
    
    return ESP_OK;
}

/**
 * @brief Handle RDMnet SET request
 */
static esp_err_t handle_set_request(rdmnet_pid_t pid, const uint8_t *data, size_t data_len)
{
    switch (pid) {
        case PID_DMX_START_ADDRESS:
            if (data_len >= 2) {
                uint16_t address = (data[0] << 8) | data[1];
                ESP_LOGI(TAG, "SET DMX_START_ADDRESS: %d", address);
                config_manager_set_start_address(address);
                config_manager_save();
            }
            break;
            
        case PID_DEVICE_LABEL:
            if (data && data_len > 0) {
                char label[33] = {0};
                strncpy(label, (const char *)data, data_len < 32 ? data_len : 32);
                ESP_LOGI(TAG, "SET DEVICE_LABEL: %s", label);
                config_manager_set_device_label(label);
                config_manager_save();
            }
            break;
            
        case PID_IDENTIFY_DEVICE:
            if (data_len >= 1) {
                g_identify_mode = (data[0] != 0);
                ESP_LOGI(TAG, "SET IDENTIFY_DEVICE: %s", g_identify_mode ? "ON" : "OFF");
                rdmnet_client_identify(g_identify_mode);
            }
            break;
            
        default:
            ESP_LOGW(TAG, "Unsupported SET PID: 0x%04X", pid);
            return ESP_ERR_NOT_SUPPORTED;
    }
    
    return ESP_OK;
}

/**
 * @brief RDMnet client task
 */
static void task_rdmnet(void *pvParameters)
{
    ESP_LOGI(TAG, "RDMnet client task started");
    
    // TODO: Connect to RDMnet broker
    // For now, this is a skeleton implementation
    
    while (g_rdmnet_running) {
        // Simulate RDMnet processing
        // In real implementation, this would:
        // - Maintain connection to broker
        // - Process GET/SET requests
        // - Send sensor data
        // - Handle identify commands
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI(TAG, "RDMnet client task stopped");
    vTaskDelete(NULL);
}

esp_err_t rdmnet_client_init(void)
{
    ESP_LOGI(TAG, "Initializing RDMnet client");
    
    // TODO: Initialize RDMnet library
    // For now, this is a placeholder
    
    ESP_LOGI(TAG, "RDMnet client initialized (skeleton implementation)");
    return ESP_OK;
}

esp_err_t rdmnet_client_start(void)
{
    if (g_rdmnet_running) {
        ESP_LOGW(TAG, "RDMnet client already running");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Starting RDMnet client");
    g_rdmnet_running = true;
    
    // Create RDMnet task
    BaseType_t ret = xTaskCreate(
        task_rdmnet,
        "rdmnet",
        4096,
        NULL,
        8,  // Priority 8
        NULL
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create RDMnet task");
        g_rdmnet_running = false;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "RDMnet client started");
    return ESP_OK;
}

esp_err_t rdmnet_client_stop(void)
{
    ESP_LOGI(TAG, "Stopping RDMnet client");
    g_rdmnet_running = false;
    return ESP_OK;
}

esp_err_t rdmnet_client_send_sensor(uint8_t sensor_id, float value)
{
    if (!g_rdmnet_running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // TODO: Send sensor data via RDMnet
    ESP_LOGD(TAG, "Sensor %d: %.2f", sensor_id, value);
    
    return ESP_OK;
}

esp_err_t rdmnet_client_identify(bool enable)
{
    g_identify_mode = enable;
    ESP_LOGI(TAG, "Identify mode: %s", enable ? "ENABLED" : "DISABLED");
    
    // TODO: Notify UI manager to blink LED in identify mode
    
    return ESP_OK;
}
