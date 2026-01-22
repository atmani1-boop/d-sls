/**
 * @file thread_network.c
 * @brief Thread IPv6 mesh networking implementation
 */

#include "thread_network.h"
#include "logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_openthread.h"
#include "esp_netif.h"
#include "openthread/thread.h"
#include "openthread/instance.h"
#include <string.h>

static const char *TAG = "THREAD";

// Global variables
static thread_state_t g_thread_state = THREAD_STATE_DISABLED;
static EventGroupHandle_t g_net_event_group = NULL;

/**
 * @brief Thread network task
 */
static void task_network_thread(void *pvParameters)
{
    ESP_LOGI(TAG, "Thread network task started");
    
    otInstance *instance = esp_openthread_get_instance();
    
    // Configure Thread network
    otThreadSetEnabled(instance, true);
    
    // Set network name
    otThreadSetNetworkName(instance, "AtmaniNet");
    
    ESP_LOGI(TAG, "Thread network configured: AtmaniNet");
    
    // Main loop - monitor state changes
    while (1) {
        otDeviceRole role = otThreadGetDeviceRole(instance);
        thread_state_t new_state = THREAD_STATE_DETACHED;
        
        switch (role) {
            case OT_DEVICE_ROLE_DISABLED:
                new_state = THREAD_STATE_DISABLED;
                break;
            case OT_DEVICE_ROLE_DETACHED:
                new_state = THREAD_STATE_DETACHED;
                break;
            case OT_DEVICE_ROLE_CHILD:
                new_state = THREAD_STATE_CHILD;
                break;
            case OT_DEVICE_ROLE_ROUTER:
                new_state = THREAD_STATE_ROUTER;
                break;
            case OT_DEVICE_ROLE_LEADER:
                new_state = THREAD_STATE_LEADER;
                break;
            default:
                break;
        }
        
        // State change notification
        if (new_state != g_thread_state) {
            g_thread_state = new_state;
            
            const char *state_str[] = {"DISABLED", "DETACHED", "CHILD", "ROUTER", "LEADER"};
            ESP_LOGI(TAG, "State changed: %s", state_str[g_thread_state]);
            
            // Set event bit when network is ready (Router or Leader)
            if (g_thread_state == THREAD_STATE_ROUTER || g_thread_state == THREAD_STATE_LEADER) {
                if (g_net_event_group) {
                    xEventGroupSetBits(g_net_event_group, EVT_NET_READY);
                    ESP_LOGI(TAG, "Network ready - EVT_NET_READY set");
                }
            } else {
                if (g_net_event_group) {
                    xEventGroupClearBits(g_net_event_group, EVT_NET_READY);
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

esp_err_t thread_network_init(void)
{
    ESP_LOGI(TAG, "Initializing Thread network");
    
    // Create event group if not exists
    if (!g_net_event_group) {
        g_net_event_group = xEventGroupCreate();
        if (!g_net_event_group) {
            ESP_LOGE(TAG, "Failed to create event group");
            return ESP_FAIL;
        }
    }
    
    // Initialize ESP-OpenThread
    esp_err_t err = esp_openthread_init(NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OpenThread init failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "OpenThread initialized");
    
    // Create Thread network task
    BaseType_t ret = xTaskCreate(
        task_network_thread,
        "thread_net",
        4096,
        NULL,
        9,  // Priority 9 (highest)
        NULL
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Thread task");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Thread task created");
    return ESP_OK;
}

esp_err_t thread_network_start(void)
{
    ESP_LOGI(TAG, "Starting Thread network");
    
    otInstance *instance = esp_openthread_get_instance();
    if (!instance) {
        ESP_LOGE(TAG, "OpenThread instance not available");
        return ESP_FAIL;
    }
    
    otThreadSetEnabled(instance, true);
    ESP_LOGI(TAG, "Thread network started");
    
    return ESP_OK;
}

esp_err_t thread_network_stop(void)
{
    ESP_LOGI(TAG, "Stopping Thread network");
    
    otInstance *instance = esp_openthread_get_instance();
    if (!instance) {
        return ESP_OK;
    }
    
    otThreadSetEnabled(instance, false);
    ESP_LOGI(TAG, "Thread network stopped");
    
    return ESP_OK;
}

thread_state_t thread_network_get_state(void)
{
    return g_thread_state;
}

esp_err_t thread_network_get_info(thread_network_info_t *info)
{
    if (!info) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(info, 0, sizeof(thread_network_info_t));
    
    otInstance *instance = esp_openthread_get_instance();
    if (!instance) {
        return ESP_FAIL;
    }
    
    info->state = g_thread_state;
    info->channel = otLinkGetChannel(instance);
    info->pan_id = otLinkGetPanId(instance);
    
    // Get IPv6 addresses
    const otNetifAddress *addr = otIp6GetUnicastAddresses(instance);
    if (addr) {
        otIp6AddressToString(&addr->mAddress, info->ipv6_addr, sizeof(info->ipv6_addr));
    }
    
    // Determine node type
    otDeviceRole role = otThreadGetDeviceRole(instance);
    if (role == OT_DEVICE_ROLE_LEADER) {
        info->node_type = 2;
    } else if (role == OT_DEVICE_ROLE_ROUTER) {
        info->node_type = 1;
    } else {
        info->node_type = 0;
    }
    
    return ESP_OK;
}
