/**
 * @file web_server.h
 * @brief Web server configuration and API endpoints
 * 
 * HTTP server implementation with REST API for real-time monitoring
 * and configuration of the DIAMANT v2.1 system.
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <esp_http_server.h>
#include "profiles.h"
#include "mram_storage.h"

//============================================================================
// CONFIGURATION
//============================================================================

#define WEB_SERVER_PORT                 80
#define WEB_SERVER_MAX_URI_LEN          512
#define WEB_SERVER_MAX_RESP_LEN         4096

// WiFi AP Configuration
#define WIFI_AP_SSID                    "DIAMANT_AP"
#define WIFI_AP_PASSWORD                "diamant2026"
#define WIFI_AP_CHANNEL                 1
#define WIFI_AP_MAX_CONNECTIONS         4
#define WIFI_AP_IP                      "192.168.4.1"

//============================================================================
// API ENDPOINTS
//============================================================================

#define API_ENDPOINT_STATUS             "/api/status"
#define API_ENDPOINT_CONFIG             "/api/config"
#define API_ENDPOINT_STATS              "/api/stats"
#define API_ENDPOINT_HISTORY            "/api/history"
#define API_ENDPOINT_DASHBOARD          "/"

//============================================================================
// FUNCTION PROTOTYPES
//============================================================================

/**
 * @brief Initialize WiFi Access Point
 * @return ESP_OK on success
 */
esp_err_t web_server_wifi_init(void);

/**
 * @brief Start HTTP web server
 * @return ESP_OK on success
 */
esp_err_t web_server_start(void);

/**
 * @brief Stop HTTP web server
 */
void web_server_stop(void);

/**
 * @brief Get server handle
 * @return HTTP server handle or NULL
 */
httpd_handle_t web_server_get_handle(void);

//============================================================================
// API HANDLERS
//============================================================================

/**
 * @brief Handler for /api/status endpoint
 * Returns real-time system status
 */
esp_err_t api_status_handler(httpd_req_t *req);

/**
 * @brief Handler for /api/config endpoint
 * Returns MRAM configuration
 */
esp_err_t api_config_handler(httpd_req_t *req);

/**
 * @brief Handler for /api/stats endpoint
 * Returns predictive statistics
 */
esp_err_t api_stats_handler(httpd_req_t *req);

/**
 * @brief Handler for /api/history endpoint
 * Returns historical data (query by hours)
 */
esp_err_t api_history_handler(httpd_req_t *req);

/**
 * @brief Handler for / (dashboard) endpoint
 * Serves HTML dashboard
 */
esp_err_t dashboard_handler(httpd_req_t *req);

#endif // WEB_SERVER_H
