/**
 * @file web_server.c
 * @brief DIAMANT v2.1 REV E - Web Server with Profile API
 * @version 2.1.0
 * @date 2026-02-17
 * 
 * HTTP server providing REST API for system monitoring and control
 */

#include "include/profiles.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// External function declarations
extern void* fusion_get_data(void);

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"

static const char *TAG = "WEB_SERVER";

// Forward declaration of fusion data type
typedef struct {
    struct {
        float soc_percent;
        float voltage;
        float current;
    } bms;
    led_profile_t active_led_profile;
    mppt_profile_t active_mppt_profile;
    v2g_profile_t active_v2g_profile;
} fusion_data_t;

/**
 * @brief Handler for /api/profiles endpoint
 * @param req HTTP request
 * @return ESP_OK on success
 */
static esp_err_t api_profiles_handler(httpd_req_t *req) {
    fusion_data_t *fusion = (fusion_data_t*)fusion_get_data();
    
    if (fusion == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Fusion data unavailable");
        return ESP_FAIL;
    }
    
    // Create JSON response
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON allocation failed");
        return ESP_FAIL;
    }
    
    // Add LED profile info
    cJSON *led = cJSON_CreateObject();
    if (fusion->active_led_profile < LED_PROFILE_COUNT) {
        const led_config_t *led_cfg = &led_configs[fusion->active_led_profile];
        cJSON_AddNumberToObject(led, "profile_id", led_cfg->id_hex);
        cJSON_AddStringToObject(led, "profile_name", led_cfg->name);
        cJSON_AddNumberToObject(led, "priority", led_cfg->priority);
        cJSON_AddNumberToObject(led, "cct_kelvin", led_cfg->cct_kelvin);
        cJSON_AddNumberToObject(led, "base_intensity", led_cfg->intensity);
        cJSON_AddBoolToObject(led, "night_saver_enabled", led_cfg->night_saver_enabled);
        cJSON_AddStringToObject(led, "conditions", led_cfg->activation_conditions);
    }
    cJSON_AddItemToObject(root, "led_profile", led);
    
    // Add MPPT profile info
    cJSON *mppt = cJSON_CreateObject();
    if (fusion->active_mppt_profile >= MPPT_AGGRESSIVE_SUNNY && 
        fusion->active_mppt_profile <= MPPT_EVENING_SAVE) {
        uint8_t mppt_idx = fusion->active_mppt_profile - MPPT_AGGRESSIVE_SUNNY;
        if (mppt_idx < MPPT_PROFILE_COUNT) {
            const mppt_config_t *mppt_cfg = &mppt_configs[mppt_idx];
            cJSON_AddNumberToObject(mppt, "profile_id", mppt_cfg->id_hex);
            cJSON_AddStringToObject(mppt, "profile_name", mppt_cfg->name);
            cJSON_AddNumberToObject(mppt, "step_multiplier", mppt_cfg->step_multiplier);
            cJSON_AddNumberToObject(mppt, "update_period_multiplier", mppt_cfg->update_period_multiplier);
            cJSON_AddNumberToObject(mppt, "max_current_limit", mppt_cfg->max_current_limit);
        }
    }
    cJSON_AddItemToObject(root, "mppt_profile", mppt);
    
    // Add V2G profile info
    cJSON *v2g = cJSON_CreateObject();
    if (fusion->active_v2g_profile >= V2G_EXPORT_DAY_SUNNY && 
        fusion->active_v2g_profile <= V2G_EMERGENCY_ISLAND) {
        uint8_t v2g_idx = fusion->active_v2g_profile - V2G_EXPORT_DAY_SUNNY;
        if (v2g_idx < V2G_PROFILE_COUNT) {
            const v2g_config_t *v2g_cfg = &v2g_configs[v2g_idx];
            cJSON_AddNumberToObject(v2g, "profile_id", v2g_cfg->id_hex);
            cJSON_AddStringToObject(v2g, "profile_name", v2g_cfg->name);
            cJSON_AddNumberToObject(v2g, "power_w", v2g_cfg->power_w);
            cJSON_AddNumberToObject(v2g, "priority", v2g_cfg->priority);
        }
    }
    cJSON_AddItemToObject(root, "v2g_profile", v2g);
    
    // Add BMS data
    cJSON *bms = cJSON_CreateObject();
    cJSON_AddNumberToObject(bms, "soc_percent", fusion->bms.soc_percent);
    cJSON_AddNumberToObject(bms, "voltage", fusion->bms.voltage);
    cJSON_AddNumberToObject(bms, "current", fusion->bms.current);
    cJSON_AddItemToObject(root, "bms", bms);
    
    // Convert to string and send
    char *json_str = cJSON_Print(root);
    if (json_str == NULL) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON print failed");
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);
    
    // Cleanup
    cJSON_free(json_str);
    cJSON_Delete(root);
    
    return ESP_OK;
}

/**
 * @brief Handler for /api/status endpoint (legacy compatibility)
 * @param req HTTP request
 * @return ESP_OK on success
 */
static esp_err_t api_status_handler(httpd_req_t *req) {
    // Redirect to profiles endpoint
    return api_profiles_handler(req);
}

/**
 * @brief Start web server
 * @return HTTP server handle or NULL on failure
 */
httpd_handle_t web_server_start(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 10;
    
    httpd_handle_t server = NULL;
    
    // Start server
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return NULL;
    }
    
    // Register URI handlers
    httpd_uri_t profiles_uri = {
        .uri       = "/api/profiles",
        .method    = HTTP_GET,
        .handler   = api_profiles_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &profiles_uri);
    
    httpd_uri_t status_uri = {
        .uri       = "/api/status",
        .method    = HTTP_GET,
        .handler   = api_status_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &status_uri);
    
    ESP_LOGI(TAG, "Web server started on port %d", config.server_port);
    ESP_LOGI(TAG, "Endpoints: /api/profiles, /api/status");
    
    return server;
}

/**
 * @brief Stop web server
 * @param server Server handle
 */
void web_server_stop(httpd_handle_t server) {
    if (server) {
        httpd_stop(server);
        ESP_LOGI(TAG, "Web server stopped");
    }
}

#else
// Stub implementation for non-ESP platforms
#include <stdio.h>

void* web_server_start(void) {
    printf("[WEB_SERVER] Web server stub started\n");
    return NULL;
}

void web_server_stop(void *server) {
    printf("[WEB_SERVER] Web server stub stopped\n");
}

#endif
