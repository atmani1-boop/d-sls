/**
 * @file config_manager.c
 * @brief Configuration management implementation
 */

#include "config_manager.h"
#include "logger.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include <string.h>

static const char *TAG = "CONFIG";

// DIP switch GPIO pins (4-bit configuration)
#define DIP_PIN_0  GPIO_NUM_0
#define DIP_PIN_1  GPIO_NUM_1
#define DIP_PIN_2  GPIO_NUM_2
#define DIP_PIN_3  GPIO_NUM_3

// NVS namespace
#define NVS_NAMESPACE "config"

// Global configuration
static system_config_t g_config;

/**
 * @brief DIP switch profile definitions
 */
static const struct {
    uint8_t dip_value;
    operating_mode_t mode;
    uint16_t universe;
    uint16_t start_address;
    uint8_t default_scene;
} dip_profiles[] = {
    {0x00, MODE_AUTO,   1,   1, 0},  // 0000: AUTO, U1, Addr 1, Scene 0
    {0x01, MODE_MANUAL, 1,   1, 1},  // 0001: MANUAL, U1, Addr 1, Scene 1
    {0x02, MODE_MANUAL, 1,  57, 0},  // 0010: MANUAL, U1, Addr 57, Scene 0
    {0x03, MODE_AUTO,   1,  57, 0},  // 0011: AUTO, U1, Addr 57, Scene 0
    {0x04, MODE_AUTO,   2,   1, 0},  // 0100: AUTO, U2, Addr 1, Scene 0
    {0x05, MODE_MANUAL, 2,   1, 2},  // 0101: MANUAL, U2, Addr 1, Scene 2
    {0x06, MODE_AUTO,   2,  57, 0},  // 0110: AUTO, U2, Addr 57, Scene 0
    {0x07, MODE_MANUAL, 2,  57, 3},  // 0111: MANUAL, U2, Addr 57, Scene 3
};

/**
 * @brief Read DIP switch value
 */
static uint8_t read_dip_switch(void)
{
    uint8_t value = 0;
    
    value |= (gpio_get_level(DIP_PIN_0) ? 0 : 1) << 0;
    value |= (gpio_get_level(DIP_PIN_1) ? 0 : 1) << 1;
    value |= (gpio_get_level(DIP_PIN_2) ? 0 : 1) << 2;
    value |= (gpio_get_level(DIP_PIN_3) ? 0 : 1) << 3;
    
    return value;
}

/**
 * @brief Apply DIP switch profile to configuration
 */
static void apply_dip_profile(uint8_t dip_value)
{
    // Find matching profile
    for (int i = 0; i < sizeof(dip_profiles) / sizeof(dip_profiles[0]); i++) {
        if (dip_profiles[i].dip_value == dip_value) {
            g_config.mode = dip_profiles[i].mode;
            g_config.universe = dip_profiles[i].universe;
            g_config.start_address = dip_profiles[i].start_address;
            g_config.default_scene = dip_profiles[i].default_scene;
            
            ESP_LOGI(TAG, "Applied DIP profile 0x%02X: Mode=%s, U=%d, Addr=%d, Scene=%d",
                     dip_value,
                     g_config.mode == MODE_AUTO ? "AUTO" : "MANUAL",
                     g_config.universe,
                     g_config.start_address,
                     g_config.default_scene);
            return;
        }
    }
    
    // Default if not found (use first profile)
    ESP_LOGW(TAG, "DIP value 0x%02X not found, using default profile", dip_value);
    g_config.mode = MODE_AUTO;
    g_config.universe = 1;
    g_config.start_address = 1;
    g_config.default_scene = 0;
}

/**
 * @brief Load configuration from NVS
 */
static esp_err_t load_from_nvs(void)
{
    nvs_handle_t handle;
    esp_err_t err;
    
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS not found, using defaults");
        return err;
    }
    
    // Load energy limits
    nvs_get_u32(handle, "max_current", &g_config.max_current_ma);
    nvs_get_u32(handle, "max_power", &g_config.max_power_w);
    
    // Load voltage (stored as integer millivolts)
    uint32_t voltage_mv = 0;
    if (nvs_get_u32(handle, "min_voltage", &voltage_mv) == ESP_OK) {
        g_config.min_voltage_v = voltage_mv / 1000.0f;
    }
    
    // Load device label
    size_t label_len = sizeof(g_config.device_label);
    nvs_get_str(handle, "device_label", g_config.device_label, &label_len);
    
    nvs_close(handle);
    
    ESP_LOGI(TAG, "Configuration loaded from NVS");
    return ESP_OK;
}

/**
 * @brief Initialize GPIO for DIP switches
 */
static void init_dip_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = ((1ULL << DIP_PIN_0) | (1ULL << DIP_PIN_1) |
                         (1ULL << DIP_PIN_2) | (1ULL << DIP_PIN_3)),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

esp_err_t config_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing configuration manager");
    
    // Set default values
    memset(&g_config, 0, sizeof(g_config));
    g_config.max_current_ma = 20000;  // 20A
    g_config.max_power_w = 4800;      // 4.8kW
    g_config.min_voltage_v = 22.0f;   // 22V
    snprintf(g_config.device_label, sizeof(g_config.device_label), "Atmani-Node");
    
    // Get device ID from MAC address
    esp_read_mac(g_config.device_id, ESP_MAC_WIFI_STA);
    
    // Initialize DIP switch GPIO
    init_dip_gpio();
    
    // Read DIP switch
    uint8_t dip_value = read_dip_switch();
    ESP_LOGI(TAG, "DIP switch value: 0x%02X", dip_value);
    
    // Apply DIP profile
    apply_dip_profile(dip_value);
    
    // Try to load from NVS (non-critical if fails)
    load_from_nvs();
    
    ESP_LOGI(TAG, "Config: Mode=%s, Universe=%d, StartAddr=%d, Scene=%d",
             g_config.mode == MODE_AUTO ? "AUTO" : "MANUAL",
             g_config.universe,
             g_config.start_address,
             g_config.default_scene);
    
    return ESP_OK;
}

const system_config_t* config_manager_get(void)
{
    return &g_config;
}

esp_err_t config_manager_save(void)
{
    nvs_handle_t handle;
    esp_err_t err;
    
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for writing: %s", esp_err_to_name(err));
        return err;
    }
    
    // Save energy limits
    nvs_set_u32(handle, "max_current", g_config.max_current_ma);
    nvs_set_u32(handle, "max_power", g_config.max_power_w);
    
    // Save voltage as millivolts
    uint32_t voltage_mv = (uint32_t)(g_config.min_voltage_v * 1000);
    nvs_set_u32(handle, "min_voltage", voltage_mv);
    
    // Save device label
    nvs_set_str(handle, "device_label", g_config.device_label);
    
    err = nvs_commit(handle);
    nvs_close(handle);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Configuration saved to NVS");
    } else {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
    }
    
    return err;
}

esp_err_t config_manager_factory_reset(void)
{
    ESP_LOGI(TAG, "Factory reset requested");
    
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        nvs_erase_all(handle);
        nvs_commit(handle);
        nvs_close(handle);
    }
    
    // Re-initialize with defaults
    return config_manager_init();
}

esp_err_t config_manager_set_universe(uint16_t universe)
{
    if (universe < 1 || universe > 63999) {
        ESP_LOGE(TAG, "Invalid universe: %d", universe);
        return ESP_ERR_INVALID_ARG;
    }
    
    g_config.universe = universe;
    ESP_LOGI(TAG, "Universe set to %d", universe);
    return ESP_OK;
}

esp_err_t config_manager_set_start_address(uint16_t address)
{
    if (address < 1 || address > 512) {
        ESP_LOGE(TAG, "Invalid start address: %d", address);
        return ESP_ERR_INVALID_ARG;
    }
    
    g_config.start_address = address;
    ESP_LOGI(TAG, "Start address set to %d", address);
    return ESP_OK;
}

esp_err_t config_manager_set_default_scene(uint8_t scene)
{
    if (scene > 15) {
        ESP_LOGE(TAG, "Invalid scene: %d", scene);
        return ESP_ERR_INVALID_ARG;
    }
    
    g_config.default_scene = scene;
    ESP_LOGI(TAG, "Default scene set to %d", scene);
    return ESP_OK;
}

esp_err_t config_manager_set_device_label(const char *label)
{
    if (!label) {
        return ESP_ERR_INVALID_ARG;
    }
    
    strncpy(g_config.device_label, label, sizeof(g_config.device_label) - 1);
    g_config.device_label[sizeof(g_config.device_label) - 1] = '\0';
    
    ESP_LOGI(TAG, "Device label set to: %s", g_config.device_label);
    return ESP_OK;
}
