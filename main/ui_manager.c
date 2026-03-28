/**
 * @file ui_manager.c
 * @brief User interface management implementation
 */

#include "ui_manager.h"
#include "logger.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "UI";

// RGB LED GPIO pins
#define LED_RED_GPIO    GPIO_NUM_25
#define LED_GREEN_GPIO  GPIO_NUM_26
#define LED_BLUE_GPIO   GPIO_NUM_27

// UART configuration
#define UART_NUM        UART_NUM_0
#define UART_BAUD_RATE  115200
#define UART_BUF_SIZE   256

// Global state
static ui_state_t g_ui_state = UI_STATE_OFF;
static bool g_ui_running = false;

/**
 * @brief Set RGB LED color
 */
static void set_led_color(bool red, bool green, bool blue)
{
    gpio_set_level(LED_RED_GPIO, red ? 1 : 0);
    gpio_set_level(LED_GREEN_GPIO, green ? 1 : 0);
    gpio_set_level(LED_BLUE_GPIO, blue ? 1 : 0);
}

/**
 * @brief Initialize RGB LED
 */
static esp_err_t init_rgb_led(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = ((1ULL << LED_RED_GPIO) |
                         (1ULL << LED_GREEN_GPIO) |
                         (1ULL << LED_BLUE_GPIO)),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure RGB LED: %s", esp_err_to_name(err));
        return err;
    }
    
    // Turn off LED initially
    set_led_color(false, false, false);
    
    ESP_LOGI(TAG, "RGB LED initialized: R=%d, G=%d, B=%d",
             LED_RED_GPIO, LED_GREEN_GPIO, LED_BLUE_GPIO);
    
    return ESP_OK;
}

/**
 * @brief Initialize UART CLI
 */
static esp_err_t init_uart_cli(void)
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    
    esp_err_t err = uart_param_config(UART_NUM, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART: %s", esp_err_to_name(err));
        return err;
    }
    
    err = uart_driver_install(UART_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "UART CLI initialized: %d baud", UART_BAUD_RATE);
    
    // Print banner
    const char *banner = "\r\n"
                         "======================================\r\n"
                         " Atmani Smart Lighting Node\r\n"
                         " ESP32-C6 Firmware v1.0\r\n"
                         "======================================\r\n"
                         "Type 'help' for commands\r\n\r\n";
    uart_write_bytes(UART_NUM, banner, strlen(banner));
    
    return ESP_OK;
}

/**
 * @brief Handle CLI commands
 */
static void handle_cli_command(const char *cmd)
{
    char response[256];
    
    if (strcmp(cmd, "help") == 0) {
        snprintf(response, sizeof(response),
                 "Available commands:\r\n"
                 "  help     - Show this help\r\n"
                 "  status   - Show system status\r\n"
                 "  version  - Show firmware version\r\n\r\n");
    } else if (strcmp(cmd, "status") == 0) {
        snprintf(response, sizeof(response),
                 "System Status:\r\n"
                 "  State: %s\r\n"
                 "  TODO: Add more status info\r\n\r\n",
                 g_ui_state == UI_STATE_NORMAL ? "NORMAL" : "OTHER");
    } else if (strcmp(cmd, "version") == 0) {
        snprintf(response, sizeof(response),
                 "Firmware Version: 1.0.0\r\n"
                 "Build Date: " __DATE__ " " __TIME__ "\r\n\r\n");
    } else {
        snprintf(response, sizeof(response),
                 "Unknown command: %s\r\n"
                 "Type 'help' for available commands\r\n\r\n", cmd);
    }
    
    uart_write_bytes(UART_NUM, response, strlen(response));
}

/**
 * @brief UI manager task
 */
static void task_ui(void *pvParameters)
{
    ESP_LOGI(TAG, "UI manager task started");
    
    uint8_t uart_buf[UART_BUF_SIZE];
    char cmd_buf[64] = {0};
    int cmd_idx = 0;
    
    uint32_t blink_counter = 0;
    
    while (g_ui_running) {
        // Update LED based on state
        switch (g_ui_state) {
            case UI_STATE_OFF:
                set_led_color(false, false, false);
                break;
                
            case UI_STATE_BOOTING:
                // Orange blink (Red + Green)
                set_led_color(blink_counter & 0x01, blink_counter & 0x01, false);
                break;
                
            case UI_STATE_NORMAL:
                // Green solid
                set_led_color(false, true, false);
                break;
                
            case UI_STATE_WARNING:
                // Yellow solid (Red + Green)
                set_led_color(true, true, false);
                break;
                
            case UI_STATE_ERROR:
                // Red solid
                set_led_color(true, false, false);
                break;
                
            case UI_STATE_IDENTIFY:
                // Blue blink
                set_led_color(false, false, blink_counter & 0x01);
                break;
        }
        
        blink_counter++;
        
        // Check for UART input
        int len = uart_read_bytes(UART_NUM, uart_buf, UART_BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                char c = uart_buf[i];
                
                if (c == '\r' || c == '\n') {
                    if (cmd_idx > 0) {
                        cmd_buf[cmd_idx] = '\0';
                        handle_cli_command(cmd_buf);
                        cmd_idx = 0;
                    }
                    uart_write_bytes(UART_NUM, "> ", 2);
                } else if (c == '\b' || c == 0x7F) {  // Backspace
                    if (cmd_idx > 0) {
                        cmd_idx--;
                        uart_write_bytes(UART_NUM, "\b \b", 3);
                    }
                } else if (cmd_idx < sizeof(cmd_buf) - 1) {
                    cmd_buf[cmd_idx++] = c;
                    uart_write_bytes(UART_NUM, &c, 1);
                }
            }
        }
    }
    
    ESP_LOGI(TAG, "UI manager task stopped");
    vTaskDelete(NULL);
}

esp_err_t ui_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing UI manager");
    
    // Initialize RGB LED
    esp_err_t err = init_rgb_led();
    if (err != ESP_OK) {
        return err;
    }
    
    // Initialize UART CLI
    err = init_uart_cli();
    if (err != ESP_OK) {
        return err;
    }
    
    ESP_LOGI(TAG, "UI manager initialized");
    return ESP_OK;
}

esp_err_t ui_manager_start(void)
{
    if (g_ui_running) {
        ESP_LOGW(TAG, "UI manager already running");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Starting UI manager");
    g_ui_running = true;
    
    // Create UI task
    BaseType_t ret = xTaskCreate(
        task_ui,
        "ui",
        3072,
        NULL,
        4,  // Priority 4
        NULL
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UI task");
        g_ui_running = false;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "UI manager started");
    return ESP_OK;
}

esp_err_t ui_manager_stop(void)
{
    ESP_LOGI(TAG, "Stopping UI manager");
    g_ui_running = false;
    return ESP_OK;
}

esp_err_t ui_manager_set_state(ui_state_t state)
{
    if (state != g_ui_state) {
        const char *state_names[] = {"OFF", "BOOTING", "NORMAL", "WARNING", "ERROR", "IDENTIFY"};
        ESP_LOGI(TAG, "UI state changed: %s -> %s",
                 state_names[g_ui_state], state_names[state]);
        g_ui_state = state;
    }
    
    return ESP_OK;
}

ui_state_t ui_manager_get_state(void)
{
    return g_ui_state;
}
