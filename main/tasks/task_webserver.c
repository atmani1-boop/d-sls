/**
 * @file task_webserver.c
 * @brief Web server task implementation - WiFi AP and HTTP server
 * 
 * This FreeRTOS task manages:
 *   - WiFi Access Point initialization
 *   - HTTP web server startup
 *   - Event handlers for client connect/disconnect
 *   - Server health monitoring
 */

#include "task_webserver.h"
#include "include/web_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

//============================================================================
// GLOBAL DATA
//============================================================================

static const char *TAG = "TASK_WEBSERVER";
static TaskHandle_t s_webserver_task_handle = NULL;
static bool s_task_running = false;
static bool s_server_started = false;

// Event group for WiFi status
static EventGroupHandle_t s_wifi_event_group = NULL;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// Client connection tracking
static uint8_t s_connected_clients = 0;

//============================================================================
// PRIVATE FUNCTIONS
//============================================================================

/**
 * @brief WiFi event handler
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_STACONNECTED: {
                wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
                s_connected_clients++;
                ESP_LOGI(TAG, "Client connected: MAC=" MACSTR " AID=%d (Total: %d)",
                         MAC2STR(event->mac), event->aid, s_connected_clients);
                break;
            }
            
            case WIFI_EVENT_AP_STADISCONNECTED: {
                wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
                if (s_connected_clients > 0) {
                    s_connected_clients--;
                }
                ESP_LOGI(TAG, "Client disconnected: MAC=" MACSTR " AID=%d (Total: %d)",
                         MAC2STR(event->mac), event->aid, s_connected_clients);
                break;
            }
            
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "WiFi AP started successfully");
                if (s_wifi_event_group) {
                    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                }
                break;
            
            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "WiFi AP stopped");
                if (s_wifi_event_group) {
                    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                }
                break;
            
            default:
                break;
        }
    }
}

/**
 * @brief Initialize NVS (required for WiFi)
 */
static esp_err_t init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition full or version mismatch, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "NVS initialized successfully");
    return ESP_OK;
}

/**
 * @brief Initialize WiFi Access Point
 */
static esp_err_t init_wifi_ap(void)
{
    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Create default event loop if not exists
    esp_err_t ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Create WiFi AP interface
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    if (ap_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default WiFi AP interface");
        return ESP_FAIL;
    }
    
    // Initialize WiFi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Register WiFi event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                         ESP_EVENT_ANY_ID,
                                                         &wifi_event_handler,
                                                         NULL,
                                                         NULL));
    
    // Configure WiFi AP
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .ssid_len = strlen(WIFI_AP_SSID),
            .channel = WIFI_AP_CHANNEL,
            .password = WIFI_AP_PASSWORD,
            .max_connection = WIFI_AP_MAX_CONNECTIONS,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };
    
    // Use open authentication if password is empty
    if (strlen(WIFI_AP_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi AP initialized: SSID=%s, Password=%s, Channel=%d",
             WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL);
    ESP_LOGI(TAG, "Connect to WiFi and navigate to http://%s", WIFI_AP_IP);
    
    return ESP_OK;
}

/**
 * @brief Main web server task function
 */
static void webserver_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Web server task started (Priority: %d)", WEBSERVER_TASK_PRIORITY);

    esp_err_t ret;

    // Initialize NVS
    ret = init_nvs();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS initialization failed, cannot start WiFi");
        goto task_exit;
    }

    // Create event group for WiFi status
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        goto task_exit;
    }

    // Initialize WiFi Access Point
    ret = init_wifi_ap();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi AP initialization failed");
        goto task_exit;
    }

    // Wait for WiFi AP to start
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                            pdFALSE,
                                            pdFALSE,
                                            pdMS_TO_TICKS(10000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi AP ready, starting web server...");
        
        // Start HTTP web server
        ret = web_server_start();
        if (ret == ESP_OK) {
            s_server_started = true;
            ESP_LOGI(TAG, "Web server started successfully");
        } else {
            ESP_LOGE(TAG, "Failed to start web server: %s", esp_err_to_name(ret));
            goto task_exit;
        }
    } else {
        ESP_LOGE(TAG, "WiFi AP failed to start");
        goto task_exit;
    }

    // Monitor server health
    ESP_LOGI(TAG, "Web server task entering monitoring mode");
    while (s_task_running) {
        // Log status every 60 seconds
        vTaskDelay(pdMS_TO_TICKS(60000));
        
        if (s_server_started) {
            ESP_LOGI(TAG, "Server status: Running | Clients: %d", s_connected_clients);
        }
    }

task_exit:
    // Cleanup
    if (s_server_started) {
        ESP_LOGI(TAG, "Stopping web server...");
        web_server_stop();
        s_server_started = false;
    }

    if (s_wifi_event_group) {
        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    }

    ESP_LOGI(TAG, "Web server task stopped");
    s_webserver_task_handle = NULL;
    s_task_running = false;
    vTaskDelete(NULL);
}

//============================================================================
// PUBLIC FUNCTIONS
//============================================================================

esp_err_t task_webserver_start(void)
{
    if (s_webserver_task_handle != NULL) {
        ESP_LOGW(TAG, "Web server task already running");
        return ESP_ERR_INVALID_STATE;
    }

    s_task_running = true;
    s_connected_clients = 0;

    BaseType_t ret = xTaskCreate(
        webserver_task,
        WEBSERVER_TASK_NAME,
        WEBSERVER_TASK_STACK_SIZE,
        NULL,
        WEBSERVER_TASK_PRIORITY,
        &s_webserver_task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create web server task");
        s_task_running = false;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Web server task created successfully");
    return ESP_OK;
}

void task_webserver_stop(void)
{
    if (s_webserver_task_handle == NULL) {
        ESP_LOGW(TAG, "Web server task not running");
        return;
    }

    ESP_LOGI(TAG, "Stopping web server task...");
    s_task_running = false;

    // Wait for task to terminate (with timeout)
    for (int i = 0; i < 30 && s_webserver_task_handle != NULL; i++) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (s_webserver_task_handle != NULL) {
        ESP_LOGW(TAG, "Force deleting web server task");
        vTaskDelete(s_webserver_task_handle);
        s_webserver_task_handle = NULL;
    }

    ESP_LOGI(TAG, "Web server task stopped");
}

TaskHandle_t task_webserver_get_handle(void)
{
    return s_webserver_task_handle;
}

bool task_webserver_is_running(void)
{
    return (s_webserver_task_handle != NULL) && s_server_started;
}
