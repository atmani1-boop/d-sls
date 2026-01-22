/**
 * @file sacn_receiver.c
 * @brief sACN (E1.31) receiver implementation
 */

#include "sacn_receiver.h"
#include "config_manager.h"
#include "logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <string.h>

static const char *TAG = "SACN";

// E1.31 constants
#define SACN_PORT 5568
#define ACN_PACKET_IDENTIFIER "ASC-E1.17\0\0\0"
#define ACN_ID_LEN 12

// Global variables
static uint8_t g_dmx_buffer[DMX_CHANNELS] = {0};
static uint32_t g_last_frame_time = 0;
static bool g_sacn_running = false;
static int g_socket = -1;
static EventGroupHandle_t g_sacn_event_group = NULL;

/**
 * @brief Parse E1.31 packet
 */
static bool parse_sacn_packet(const uint8_t *data, size_t len, uint16_t *universe, uint8_t **dmx_data, size_t *dmx_len)
{
    // Minimum E1.31 packet size check
    if (len < 126) {
        return false;
    }
    
    // Verify ACN Packet Identifier
    if (memcmp(data, ACN_PACKET_IDENTIFIER, ACN_ID_LEN) != 0) {
        return false;
    }
    
    // Extract universe (bytes 113-114)
    *universe = (data[113] << 8) | data[114];
    
    // DMX data starts at byte 126
    *dmx_data = (uint8_t *)(data + 126);
    *dmx_len = len - 126;
    
    if (*dmx_len > DMX_CHANNELS) {
        *dmx_len = DMX_CHANNELS;
    }
    
    return true;
}

/**
 * @brief sACN receiver task
 */
static void task_sacn(void *pvParameters)
{
    ESP_LOGI(TAG, "sACN receiver task started");
    
    const system_config_t *config = config_manager_get();
    
    // Create UDP socket
    g_socket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (g_socket < 0) {
        ESP_LOGE(TAG, "Failed to create socket: errno %d", errno);
        goto exit;
    }
    
    // Bind to sACN port
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(SACN_PORT);
    addr.sin6_addr = in6addr_any;
    
    int err = bind(g_socket, (struct sockaddr *)&addr, sizeof(addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Failed to bind socket: errno %d", errno);
        goto exit;
    }
    
    ESP_LOGI(TAG, "Socket bound to port %d", SACN_PORT);
    
    // Join multicast group for configured universe
    // Multicast address format: ff18::83:00:XX:XX (XX:XX = universe in hex)
    char mcast_addr[64];
    snprintf(mcast_addr, sizeof(mcast_addr), "ff18::83:00:%02x:%02x",
             (config->universe >> 8) & 0xFF, config->universe & 0xFF);
    
    struct ipv6_mreq mreq;
    inet_pton(AF_INET6, mcast_addr, &mreq.ipv6mr_multiaddr);
    mreq.ipv6mr_interface = 0;  // Any interface
    
    err = setsockopt(g_socket, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq));
    if (err < 0) {
        ESP_LOGW(TAG, "Failed to join multicast group %s: errno %d", mcast_addr, errno);
    } else {
        ESP_LOGI(TAG, "Joined multicast group: %s (Universe %d)", mcast_addr, config->universe);
    }
    
    // Set socket receive timeout
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(g_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    // Receive buffer
    uint8_t rx_buffer[1500];
    
    // Main receive loop
    while (g_sacn_running) {
        struct sockaddr_in6 source_addr;
        socklen_t socklen = sizeof(source_addr);
        
        int len = recvfrom(g_socket, rx_buffer, sizeof(rx_buffer), 0,
                          (struct sockaddr *)&source_addr, &socklen);
        
        if (len > 0) {
            // Parse sACN packet
            uint16_t universe;
            uint8_t *dmx_data;
            size_t dmx_len;
            
            if (parse_sacn_packet(rx_buffer, len, &universe, &dmx_data, &dmx_len)) {
                // Check if universe matches
                if (universe == config->universe) {
                    // Copy DMX data
                    memcpy(g_dmx_buffer, dmx_data, dmx_len);
                    g_last_frame_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
                    
                    // Set event bit
                    if (g_sacn_event_group) {
                        xEventGroupSetBits(g_sacn_event_group, EVT_SACN_FRAME);
                    }
                    
                    ESP_LOGD(TAG, "Frame received: Universe %d, %d channels", universe, dmx_len);
                }
            }
        } else if (len < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            ESP_LOGW(TAG, "recvfrom failed: errno %d", errno);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
exit:
    if (g_socket >= 0) {
        close(g_socket);
        g_socket = -1;
    }
    
    ESP_LOGI(TAG, "sACN receiver task stopped");
    vTaskDelete(NULL);
}

esp_err_t sacn_receiver_init(void)
{
    ESP_LOGI(TAG, "Initializing sACN receiver");
    
    // Create event group
    if (!g_sacn_event_group) {
        g_sacn_event_group = xEventGroupCreate();
        if (!g_sacn_event_group) {
            ESP_LOGE(TAG, "Failed to create event group");
            return ESP_FAIL;
        }
    }
    
    ESP_LOGI(TAG, "sACN receiver initialized");
    return ESP_OK;
}

esp_err_t sacn_receiver_start(void)
{
    if (g_sacn_running) {
        ESP_LOGW(TAG, "sACN receiver already running");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Starting sACN receiver");
    g_sacn_running = true;
    
    // Create sACN task
    BaseType_t ret = xTaskCreate(
        task_sacn,
        "sacn",
        4096,
        NULL,
        8,  // Priority 8
        NULL
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sACN task");
        g_sacn_running = false;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "sACN receiver started");
    return ESP_OK;
}

esp_err_t sacn_receiver_stop(void)
{
    ESP_LOGI(TAG, "Stopping sACN receiver");
    g_sacn_running = false;
    return ESP_OK;
}

esp_err_t sacn_receiver_get_dmx(const uint8_t **buffer)
{
    if (!buffer) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *buffer = g_dmx_buffer;
    return ESP_OK;
}

bool sacn_receiver_is_active(void)
{
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    return (now - g_last_frame_time) < SACN_TIMEOUT_MS;
}

uint32_t sacn_receiver_get_last_frame_time(void)
{
    return g_last_frame_time;
}
