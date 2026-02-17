/**
 * @file web_server.c
 * @brief HTTP web server implementation for DIAMANT v2.1 system
 * 
 * Provides WiFi Access Point and REST API for real-time monitoring
 * and configuration through a web dashboard.
 */

#include "include/web_server.h"
#include "include/profiles.h"
#include "include/mram_storage.h"
#include "www/dashboard.html.h"
#include <string.h>
#include <sys/param.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "cJSON.h"

static const char *TAG = "WEB_SERVER";
static httpd_handle_t server = NULL;

// External fusion data - should be provided by main application
extern fusion_data_t g_fusion_data;

//============================================================================
// WIFI ACCESS POINT INITIALIZATION
//============================================================================

esp_err_t web_server_wifi_init(void)
{
    ESP_LOGI(TAG, "Initializing WiFi Access Point...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Create default WiFi AP
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    
    // Configure static IP
    esp_netif_dhcps_stop(ap_netif);
    
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(ap_netif));
    
    // Initialize WiFi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
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
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi AP started: SSID=%s, Password=%s, IP=%s", 
             WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_IP);
    
    return ESP_OK;
}

//============================================================================
// CORS HEADERS
//============================================================================

static void set_cors_headers(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
}

//============================================================================
// API HANDLER: /api/status
//============================================================================

esp_err_t api_status_handler(httpd_req_t *req)
{
    set_cors_headers(req);
    
    // Create JSON response
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    // System info
    cJSON_AddStringToObject(root, "device_id", "DIAMANT_v2.1_REV_F");
    cJSON_AddNumberToObject(root, "timestamp", (double)g_fusion_data.timestamp);
    cJSON_AddNumberToObject(root, "uptime_sec", g_fusion_data.uptime_sec);
    
    // Weather data
    cJSON *weather = cJSON_CreateObject();
    cJSON_AddStringToObject(weather, "state", get_weather_state_str(g_fusion_data.weather.state));
    cJSON_AddNumberToObject(weather, "ambient_lux", g_fusion_data.weather.ambient_lux);
    cJSON_AddNumberToObject(weather, "temperature_c", g_fusion_data.weather.temperature_c);
    cJSON_AddNumberToObject(weather, "pv_voltage", g_fusion_data.weather.pv_voltage);
    cJSON_AddBoolToObject(weather, "pv_oscillating", g_fusion_data.weather.pv_oscillating);
    cJSON_AddItemToObject(root, "weather", weather);
    
    // Astronomical data
    cJSON *astro = cJSON_CreateObject();
    cJSON_AddNumberToObject(astro, "sunrise", (double)g_fusion_data.astro.sunrise);
    cJSON_AddNumberToObject(astro, "sunset", (double)g_fusion_data.astro.sunset);
    cJSON_AddNumberToObject(astro, "civil_twilight_start", (double)g_fusion_data.astro.civil_twilight_start);
    cJSON_AddNumberToObject(astro, "civil_twilight_end", (double)g_fusion_data.astro.civil_twilight_end);
    cJSON_AddStringToObject(astro, "current_period", get_time_period_str(g_fusion_data.astro.current_period));
    cJSON_AddNumberToObject(astro, "latitude", g_fusion_data.astro.latitude);
    cJSON_AddNumberToObject(astro, "longitude", g_fusion_data.astro.longitude);
    cJSON_AddItemToObject(root, "astro", astro);
    
    // Calendar data
    cJSON *calendar = cJSON_CreateObject();
    cJSON_AddStringToObject(calendar, "season", get_season_str(g_fusion_data.calendar.season));
    cJSON_AddBoolToObject(calendar, "is_weekend", g_fusion_data.calendar.is_weekend);
    cJSON_AddBoolToObject(calendar, "is_holiday", g_fusion_data.calendar.is_holiday);
    cJSON_AddNumberToObject(calendar, "day_of_year", g_fusion_data.calendar.day_of_year);
    if (g_fusion_data.calendar.holiday_name) {
        cJSON_AddStringToObject(calendar, "holiday_name", g_fusion_data.calendar.holiday_name);
    }
    cJSON_AddItemToObject(root, "calendar", calendar);
    
    // BMS data
    cJSON *bms = cJSON_CreateObject();
    cJSON_AddNumberToObject(bms, "soc_percent", g_fusion_data.bms.soc_percent);
    cJSON_AddNumberToObject(bms, "voltage", g_fusion_data.bms.voltage);
    cJSON_AddNumberToObject(bms, "current", g_fusion_data.bms.current);
    cJSON_AddNumberToObject(bms, "temperature", g_fusion_data.bms.temperature);
    cJSON_AddBoolToObject(bms, "alert_active", g_fusion_data.bms.alert_active);
    cJSON_AddBoolToObject(bms, "low_battery", g_fusion_data.bms.low_battery);
    cJSON_AddBoolToObject(bms, "high_battery", g_fusion_data.bms.high_battery);
    cJSON_AddItemToObject(root, "bms", bms);
    
    // System temperatures
    cJSON_AddNumberToObject(root, "board_temp", g_fusion_data.board_temp);
    cJSON_AddNumberToObject(root, "heatsink_temp", g_fusion_data.heatsink_temp);
    
    // Active profiles
    cJSON *v2g = cJSON_CreateObject();
    if (g_fusion_data.active_v2g_profile) {
        cJSON_AddNumberToObject(v2g, "id", g_fusion_data.active_v2g_profile->id);
        cJSON_AddStringToObject(v2g, "name", g_fusion_data.active_v2g_profile->name);
        cJSON_AddNumberToObject(v2g, "power_target", g_fusion_data.active_v2g_profile->power_target);
        cJSON_AddStringToObject(v2g, "reason", g_fusion_data.v2g_reason);
    }
    cJSON_AddItemToObject(root, "v2g", v2g);
    
    cJSON *led = cJSON_CreateObject();
    if (g_fusion_data.active_led_profile) {
        cJSON_AddNumberToObject(led, "id", g_fusion_data.active_led_profile->id);
        cJSON_AddStringToObject(led, "name", g_fusion_data.active_led_profile->name);
        cJSON_AddNumberToObject(led, "warm_intensity", g_fusion_data.active_led_profile->warm_intensity);
        cJSON_AddNumberToObject(led, "cool_intensity", g_fusion_data.active_led_profile->cool_intensity);
        cJSON_AddStringToObject(led, "reason", g_fusion_data.led_reason);
    }
    cJSON_AddItemToObject(root, "led", led);
    
    // Convert to string
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    if (!json_str) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);
    
    free(json_str);
    return ESP_OK;
}

//============================================================================
// API HANDLER: /api/config
//============================================================================

esp_err_t api_config_handler(httpd_req_t *req)
{
    set_cors_headers(req);
    
    mram_config_t config;
    esp_err_t ret = mram_config_load(&config);
    
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to load config");
        return ESP_FAIL;
    }
    
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    cJSON_AddStringToObject(root, "device_id", config.device_id);
    cJSON_AddStringToObject(root, "location", config.location);
    cJSON_AddNumberToObject(root, "latitude", config.latitude);
    cJSON_AddNumberToObject(root, "longitude", config.longitude);
    cJSON_AddNumberToObject(root, "timezone_offset", config.timezone_offset);
    cJSON_AddNumberToObject(root, "log_interval_sec", config.log_interval_sec);
    cJSON_AddNumberToObject(root, "stats_interval_sec", config.stats_interval_sec);
    cJSON_AddNumberToObject(root, "batt_low_threshold", config.batt_low_threshold);
    cJSON_AddNumberToObject(root, "batt_high_threshold", config.batt_high_threshold);
    cJSON_AddNumberToObject(root, "temp_max_threshold", config.temp_max_threshold);
    cJSON_AddNumberToObject(root, "total_runtime_hours", config.total_runtime_hours);
    cJSON_AddNumberToObject(root, "boot_count", config.boot_count);
    cJSON_AddNumberToObject(root, "history_count", config.history_count);
    
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    if (!json_str) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);
    
    free(json_str);
    return ESP_OK;
}

//============================================================================
// API HANDLER: /api/stats
//============================================================================

esp_err_t api_stats_handler(httpd_req_t *req)
{
    set_cors_headers(req);
    
    mram_stats_t stats;
    mram_prediction_t prediction;
    
    esp_err_t ret = mram_stats_calculate(&stats);
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to calculate stats");
        return ESP_FAIL;
    }
    
    ret = mram_prediction_generate(&prediction);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to generate predictions");
    }
    
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    // Statistics
    cJSON *stats_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(stats_obj, "pv_energy_kwh_today", stats.pv_energy_kwh_today);
    cJSON_AddNumberToObject(stats_obj, "pv_energy_kwh_week", stats.pv_energy_kwh_week);
    cJSON_AddNumberToObject(stats_obj, "pv_peak_power_w", stats.pv_peak_power_w);
    cJSON_AddNumberToObject(stats_obj, "pv_avg_voltage", stats.pv_avg_voltage);
    cJSON_AddNumberToObject(stats_obj, "batt_cycles_total", stats.batt_cycles_total);
    cJSON_AddNumberToObject(stats_obj, "batt_energy_charged_kwh", stats.batt_energy_charged_kwh);
    cJSON_AddNumberToObject(stats_obj, "batt_energy_discharged_kwh", stats.batt_energy_discharged_kwh);
    cJSON_AddNumberToObject(stats_obj, "batt_health_percent", stats.batt_health_percent);
    cJSON_AddNumberToObject(stats_obj, "v2g_export_kwh_today", stats.v2g_export_kwh_today);
    cJSON_AddNumberToObject(stats_obj, "v2g_import_kwh_today", stats.v2g_import_kwh_today);
    cJSON_AddNumberToObject(stats_obj, "v2g_net_kwh", stats.v2g_net_kwh);
    cJSON_AddNumberToObject(stats_obj, "led_energy_kwh_today", stats.led_energy_kwh_today);
    cJSON_AddNumberToObject(stats_obj, "led_avg_runtime_hours", stats.led_avg_runtime_hours);
    cJSON_AddNumberToObject(stats_obj, "uptime_hours", stats.uptime_hours);
    cJSON_AddNumberToObject(stats_obj, "error_count", stats.error_count);
    cJSON_AddItemToObject(root, "stats", stats_obj);
    
    // Predictions
    cJSON *pred_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(pred_obj, "pv_forecast_next_hour_kwh", prediction.pv_forecast_next_hour_kwh);
    cJSON_AddNumberToObject(pred_obj, "pv_forecast_today_kwh", prediction.pv_forecast_today_kwh);
    cJSON_AddNumberToObject(pred_obj, "batt_predicted_cycles_remaining", prediction.batt_predicted_cycles_remaining);
    cJSON_AddNumberToObject(pred_obj, "batt_predicted_capacity_fade", prediction.batt_predicted_capacity_fade);
    cJSON_AddNumberToObject(pred_obj, "days_until_maintenance", prediction.days_until_maintenance);
    cJSON_AddStringToObject(pred_obj, "maintenance_reason", prediction.maintenance_reason);
    cJSON_AddItemToObject(root, "predictions", pred_obj);
    
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    if (!json_str) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);
    
    free(json_str);
    return ESP_OK;
}

//============================================================================
// API HANDLER: /api/history
//============================================================================

esp_err_t api_history_handler(httpd_req_t *req)
{
    set_cors_headers(req);
    
    // Parse query parameter for hours
    char query[64];
    uint16_t max_entries = 60; // Default: last 60 entries (~1 hour if logged per minute)
    
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char param[32];
        if (httpd_query_key_value(query, "hours", param, sizeof(param)) == ESP_OK) {
            int hours = atoi(param);
            if (hours > 0 && hours <= 24) {
                max_entries = hours * 60; // Assuming 1 entry per minute
            }
        }
    }
    
    // Read historical data
    mram_history_entry_t *entries = malloc(max_entries * sizeof(mram_history_entry_t));
    if (!entries) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }
    
    uint16_t count_out;
    esp_err_t ret = mram_history_read(entries, max_entries, &count_out);
    
    if (ret != ESP_OK) {
        free(entries);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read history");
        return ESP_FAIL;
    }
    
    // Create JSON array
    cJSON *root = cJSON_CreateArray();
    if (!root) {
        free(entries);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    for (uint16_t i = 0; i < count_out; i++) {
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddNumberToObject(entry, "timestamp", entries[i].timestamp);
        cJSON_AddNumberToObject(entry, "ambient_lux", entries[i].ambient_lux);
        cJSON_AddNumberToObject(entry, "temperature", entries[i].temperature / 100.0);
        cJSON_AddNumberToObject(entry, "pv_voltage", entries[i].pv_voltage / 100.0);
        cJSON_AddNumberToObject(entry, "bms_soc", entries[i].bms_soc / 100.0);
        cJSON_AddNumberToObject(entry, "bms_voltage", entries[i].bms_voltage / 100.0);
        cJSON_AddNumberToObject(entry, "bms_current", entries[i].bms_current / 100.0);
        cJSON_AddNumberToObject(entry, "bms_temperature", entries[i].bms_temperature / 100.0);
        cJSON_AddNumberToObject(entry, "v2g_power", entries[i].v2g_power);
        cJSON_AddNumberToObject(entry, "led_warm", entries[i].led_warm);
        cJSON_AddNumberToObject(entry, "led_cool", entries[i].led_cool);
        cJSON_AddNumberToObject(entry, "board_temp", entries[i].board_temp / 100.0);
        cJSON_AddNumberToObject(entry, "heatsink_temp", entries[i].heatsink_temp / 100.0);
        cJSON_AddItemToArray(root, entry);
    }
    
    free(entries);
    
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    if (!json_str) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);
    
    free(json_str);
    return ESP_OK;
}

//============================================================================
// HANDLER: / (Dashboard)
//============================================================================

esp_err_t dashboard_handler(httpd_req_t *req)
{
    // Send HTML dashboard (embedded as gzip compressed data)
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    
    return httpd_resp_send(req, (const char *)dashboard_gz, dashboard_gz_len);
}

//============================================================================
// HTTP SERVER START/STOP
//============================================================================

esp_err_t web_server_start(void)
{
    if (server) {
        ESP_LOGW(TAG, "Server already running");
        return ESP_OK;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = WEB_SERVER_PORT;
    config.max_uri_handlers = 16;
    config.stack_size = 8192;
    config.uri_match_fn = httpd_uri_match_wildcard;
    
    ESP_LOGI(TAG, "Starting HTTP server on port %d", config.server_port);
    
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return ESP_FAIL;
    }
    
    // Register URI handlers
    httpd_uri_t uri_status = {
        .uri = API_ENDPOINT_STATUS,
        .method = HTTP_GET,
        .handler = api_status_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_status);
    
    httpd_uri_t uri_config = {
        .uri = API_ENDPOINT_CONFIG,
        .method = HTTP_GET,
        .handler = api_config_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_config);
    
    httpd_uri_t uri_stats = {
        .uri = API_ENDPOINT_STATS,
        .method = HTTP_GET,
        .handler = api_stats_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_stats);
    
    httpd_uri_t uri_history = {
        .uri = API_ENDPOINT_HISTORY,
        .method = HTTP_GET,
        .handler = api_history_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_history);
    
    httpd_uri_t uri_dashboard = {
        .uri = API_ENDPOINT_DASHBOARD,
        .method = HTTP_GET,
        .handler = dashboard_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_dashboard);
    
    ESP_LOGI(TAG, "HTTP server started successfully");
    ESP_LOGI(TAG, "Dashboard: http://%s/", WIFI_AP_IP);
    
    return ESP_OK;
}

void web_server_stop(void)
{
    if (server) {
        httpd_stop(server);
        server = NULL;
        ESP_LOGI(TAG, "HTTP server stopped");
    }
}

httpd_handle_t web_server_get_handle(void)
{
    return server;
}
