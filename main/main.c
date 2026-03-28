/**
 * @file main.c
 * @brief Main entry point for Atmani Smart Lighting Node firmware
 * 
 * Orchestrates system initialization and startup sequence for the complete
 * lighting control system with Thread mesh, RDMnet, sACN, and energy management.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

// Include all module headers
#include "logger.h"
#include "config_manager.h"
#include "thread_network.h"
#include "rdmnet_client.h"
#include "sacn_receiver.h"
#include "lighting_engine.h"
#include "energy_manager.h"
#include "scenes_engine.h"
#include "mesh_sync.h"
#include "ui_manager.h"

static const char *TAG = "MAIN";

/**
 * @brief Initialize base system services
 */
static esp_err_t init_base_system(void)
{
    ESP_LOGI(TAG, "Initializing base system");
    
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_LOGI(TAG, "NVS initialized");
    
    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_LOGI(TAG, "Network interface initialized");
    
    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(TAG, "Event loop created");
    
    return ESP_OK;
}

/**
 * @brief Initialize all modules in correct order
 */
static esp_err_t init_all_modules(void)
{
    ESP_LOGI(TAG, "=== Initializing All Modules ===");
    
    // Phase 1: Core system
    ESP_LOGI(TAG, "Phase 1: Core system");
    ESP_ERROR_CHECK(logger_init());
    ESP_ERROR_CHECK(config_manager_init());
    
    // Phase 2: Network
    ESP_LOGI(TAG, "Phase 2: Network layer");
    ESP_ERROR_CHECK(thread_network_init());
    ESP_ERROR_CHECK(rdmnet_client_init());
    ESP_ERROR_CHECK(sacn_receiver_init());
    ESP_ERROR_CHECK(mesh_sync_init());
    
    // Phase 3: Lighting
    ESP_LOGI(TAG, "Phase 3: Lighting control");
    ESP_ERROR_CHECK(lighting_engine_init());
    ESP_ERROR_CHECK(scenes_engine_init());
    
    // Phase 4: Power management
    ESP_LOGI(TAG, "Phase 4: Power management");
    ESP_ERROR_CHECK(energy_manager_init());
    
    // Phase 5: User interface
    ESP_LOGI(TAG, "Phase 5: User interface");
    ESP_ERROR_CHECK(ui_manager_init());
    
    ESP_LOGI(TAG, "=== All Modules Initialized Successfully ===");
    return ESP_OK;
}

/**
 * @brief Start all tasks in priority order
 */
static esp_err_t start_all_tasks(void)
{
    ESP_LOGI(TAG, "=== Starting All Tasks ===");
    
    // Set UI to booting state
    ui_manager_set_state(UI_STATE_BOOTING);
    
    // Start tasks in order (highest priority first)
    ESP_LOGI(TAG, "Starting Thread network (P9)");
    ESP_ERROR_CHECK(thread_network_start());
    
    ESP_LOGI(TAG, "Starting RDMnet client (P8)");
    ESP_ERROR_CHECK(rdmnet_client_start());
    
    ESP_LOGI(TAG, "Starting sACN receiver (P8)");
    ESP_ERROR_CHECK(sacn_receiver_start());
    
    ESP_LOGI(TAG, "Starting lighting engine (P7)");
    ESP_ERROR_CHECK(lighting_engine_start());
    
    ESP_LOGI(TAG, "Starting energy manager (P6)");
    ESP_ERROR_CHECK(energy_manager_start());
    
    ESP_LOGI(TAG, "Starting mesh sync (P6)");
    ESP_ERROR_CHECK(mesh_sync_start());
    
    ESP_LOGI(TAG, "Starting scenes engine (P5)");
    ESP_ERROR_CHECK(scenes_engine_start());
    
    ESP_LOGI(TAG, "Starting UI manager (P4)");
    ESP_ERROR_CHECK(ui_manager_start());
    
    ESP_LOGI(TAG, "=== All Tasks Started Successfully ===");
    
    // Set UI to normal state
    vTaskDelay(pdMS_TO_TICKS(2000));  // Wait 2s for boot
    ui_manager_set_state(UI_STATE_NORMAL);
    
    return ESP_OK;
}

/**
 * @brief Main application task - heartbeat and monitoring
 */
static void app_main_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Main application task started");
    
    const system_config_t *config = config_manager_get();
    
    // Print system configuration
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Atmani Smart Lighting Node - READY");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Configuration:");
    ESP_LOGI(TAG, "  Mode: %s", config->mode == MODE_AUTO ? "AUTO" : "MANUAL");
    ESP_LOGI(TAG, "  Universe: %d", config->universe);
    ESP_LOGI(TAG, "  Start Address: %d", config->start_address);
    ESP_LOGI(TAG, "  Default Scene: %d", config->default_scene);
    ESP_LOGI(TAG, "  Device Label: %s", config->device_label);
    ESP_LOGI(TAG, "  Device ID: %02X:%02X:%02X:%02X:%02X:%02X",
             config->device_id[0], config->device_id[1], config->device_id[2],
             config->device_id[3], config->device_id[4], config->device_id[5]);
    ESP_LOGI(TAG, "Energy Limits:");
    ESP_LOGI(TAG, "  Max Current: %d mA", config->max_current_ma);
    ESP_LOGI(TAG, "  Max Power: %d W", config->max_power_w);
    ESP_LOGI(TAG, "  Min Voltage: %.1f V", config->min_voltage_v);
    ESP_LOGI(TAG, "========================================");
    
    // Main monitoring loop
    uint32_t heartbeat_counter = 0;
    
    while (1) {
        // Heartbeat every 10 seconds
        if (heartbeat_counter % 100 == 0) {
            ESP_LOGI(TAG, "Heartbeat: uptime=%d s, free_heap=%d bytes",
                     heartbeat_counter / 10,
                     esp_get_free_heap_size());
            
            // Get Thread network state
            thread_state_t thread_state = thread_network_get_state();
            const char *state_names[] = {"DISABLED", "DETACHED", "CHILD", "ROUTER", "LEADER"};
            ESP_LOGI(TAG, "Thread state: %s", state_names[thread_state]);
            
            // Get energy status
            energy_status_t energy;
            if (energy_manager_get_status(&energy) == ESP_OK) {
                ESP_LOGI(TAG, "Energy: %.2fV, %dmA, %dW (Prot: %s)",
                         energy.voltage_v, energy.current_ma, energy.power_w,
                         energy_manager_is_protection_active() ? "ACTIVE" : "OK");
            }
            
            // Check sACN
            if (sacn_receiver_is_active()) {
                ESP_LOGI(TAG, "sACN: ACTIVE");
            } else {
                uint8_t active_scene = scenes_engine_get_active();
                if (active_scene != 255) {
                    ESP_LOGI(TAG, "sACN: TIMEOUT - Scene %d active", active_scene);
                } else {
                    ESP_LOGI(TAG, "sACN: TIMEOUT");
                }
            }
        }
        
        // Update lighting from sACN if active
        if (sacn_receiver_is_active() && config->mode == MODE_AUTO) {
            const uint8_t *dmx_data;
            if (sacn_receiver_get_dmx(&dmx_data) == ESP_OK) {
                lighting_engine_update_from_dmx(dmx_data, config->start_address);
            }
        }
        
        heartbeat_counter++;
        vTaskDelay(pdMS_TO_TICKS(100));  // 10Hz update rate
    }
}

/**
 * @brief Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Atmani Smart Lighting Node");
    ESP_LOGI(TAG, "  ESP32-C6 Firmware v1.0.0");
    ESP_LOGI(TAG, "  Build: " __DATE__ " " __TIME__);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");
    
    // Initialize base system
    ESP_ERROR_CHECK(init_base_system());
    
    // Initialize all modules
    esp_err_t err = init_all_modules();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Module initialization failed: %s", esp_err_to_name(err));
        ESP_LOGE(TAG, "System halted");
        return;
    }
    
    // Start all tasks
    err = start_all_tasks();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Task startup failed: %s", esp_err_to_name(err));
        ESP_LOGE(TAG, "System halted");
        return;
    }
    
    // Create main application task
    xTaskCreate(
        app_main_task,
        "app_main",
        4096,
        NULL,
        5,  // Priority 5
        NULL
    );
    
    ESP_LOGI(TAG, "Firmware initialization complete");
}
