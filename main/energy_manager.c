/**
 * @file energy_manager.c
 * @brief Energy management implementation
 */

#include "energy_manager.h"
#include "config_manager.h"
#include "lighting_engine.h"
#include "rdmnet_client.h"
#include "logger.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "ENERGY";

// I2C configuration
#define I2C_MASTER_SCL_IO    22
#define I2C_MASTER_SDA_IO    21
#define I2C_MASTER_FREQ_HZ   100000
#define I2C_MASTER_NUM       I2C_NUM_0

// INA226 I2C address and registers
#define INA226_ADDR          0x40
#define INA226_REG_CONFIG    0x00
#define INA226_REG_SHUNT_V   0x01
#define INA226_REG_BUS_V     0x02
#define INA226_REG_POWER     0x03
#define INA226_REG_CURRENT   0x04
#define INA226_REG_CAL       0x05

// Global state
static energy_status_t g_energy_status = {0};
static bool g_energy_running = false;
static EventGroupHandle_t g_energy_event_group = NULL;

/**
 * @brief Initialize I2C master
 */
static esp_err_t init_i2c_master(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure I2C: %s", esp_err_to_name(err));
        return err;
    }
    
    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "I2C initialized: SDA=%d, SCL=%d, Freq=%dHz",
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQ_HZ);
    
    return ESP_OK;
}

/**
 * @brief Read INA226 register
 */
static esp_err_t ina226_read_reg(uint8_t reg, uint16_t *value)
{
    uint8_t data[2];
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (INA226_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (INA226_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    
    if (err == ESP_OK) {
        *value = (data[0] << 8) | data[1];
    }
    
    return err;
}

/**
 * @brief Write INA226 register
 */
static esp_err_t ina226_write_reg(uint8_t reg, uint16_t value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (INA226_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, (value >> 8) & 0xFF, true);
    i2c_master_write_byte(cmd, value & 0xFF, true);
    i2c_master_stop(cmd);
    
    esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    
    return err;
}

/**
 * @brief Initialize INA226 sensor
 */
static esp_err_t init_ina226(void)
{
    // Configure INA226
    // Config: Averaging=1, Bus V CT=1.1ms, Shunt V CT=1.1ms, Mode=Continuous
    uint16_t config = 0x4127;
    esp_err_t err = ina226_write_reg(INA226_REG_CONFIG, config);
    
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "INA226 not found or failed to initialize");
        return err;
    }
    
    // Set calibration (assuming 0.1 ohm shunt, 20A max)
    // Cal = 0.00512 / (Current_LSB * R_shunt)
    // Current_LSB = Max_Expected_Current / 32768 = 20A / 32768 = 0.00061 A
    // Cal = 0.00512 / (0.00061 * 0.1) = 83.9 ≈ 84
    uint16_t calibration = 84;
    ina226_write_reg(INA226_REG_CAL, calibration);
    
    ESP_LOGI(TAG, "INA226 initialized");
    return ESP_OK;
}

/**
 * @brief Read INA226 measurements
 */
static void read_ina226_data(void)
{
    uint16_t bus_v_raw = 0;
    uint16_t current_raw = 0;
    uint16_t power_raw = 0;
    
    // Read bus voltage
    if (ina226_read_reg(INA226_REG_BUS_V, &bus_v_raw) == ESP_OK) {
        // Bus voltage LSB = 1.25 mV
        g_energy_status.voltage_v = (bus_v_raw * 1.25f) / 1000.0f;
    }
    
    // Read current
    if (ina226_read_reg(INA226_REG_CURRENT, &current_raw) == ESP_OK) {
        // Current LSB = 0.61 mA (from calibration)
        int16_t current_signed = (int16_t)current_raw;
        g_energy_status.current_ma = current_signed * 0.61f;
    }
    
    // Read power
    if (ina226_read_reg(INA226_REG_POWER, &power_raw) == ESP_OK) {
        // Power LSB = 25 * Current_LSB = 15.25 mW
        g_energy_status.power_w = (power_raw * 15.25f) / 1000.0f;
    }
}

/**
 * @brief Check protection limits
 */
static void check_protection_limits(void)
{
    const system_config_t *config = config_manager_get();
    
    bool prev_overcurrent = g_energy_status.overcurrent;
    bool prev_overpower = g_energy_status.overpower;
    bool prev_undervoltage = g_energy_status.undervoltage;
    
    // Check overcurrent
    g_energy_status.overcurrent = (g_energy_status.current_ma > config->max_current_ma);
    
    // Check overpower
    g_energy_status.overpower = (g_energy_status.power_w > config->max_power_w);
    
    // Check undervoltage
    g_energy_status.undervoltage = (g_energy_status.voltage_v < config->min_voltage_v);
    
    // Take action on protection triggers
    if (g_energy_status.overcurrent || g_energy_status.overpower) {
        if (!prev_overcurrent && !prev_overpower) {
            ESP_LOGW(TAG, "Protection triggered: OC=%d, OP=%d",
                     g_energy_status.overcurrent, g_energy_status.overpower);
            
            // Apply emergency dimming to 70%
            lighting_engine_set_emergency_dim(70);
            
            // Set alert event
            if (g_energy_event_group) {
                xEventGroupSetBits(g_energy_event_group, EVT_ENERGY_ALERT);
            }
        }
    } else if (prev_overcurrent || prev_overpower) {
        // Protection cleared
        ESP_LOGI(TAG, "Protection cleared");
        lighting_engine_set_emergency_dim(100);
    }
    
    if (g_energy_status.undervoltage && !prev_undervoltage) {
        ESP_LOGW(TAG, "Undervoltage detected: %.2fV", g_energy_status.voltage_v);
    }
}

/**
 * @brief Energy manager task
 */
static void task_energy(void *pvParameters)
{
    ESP_LOGI(TAG, "Energy manager task started");
    
    while (g_energy_running) {
        // Read INA226 data
        read_ina226_data();
        
        // Check protection limits
        check_protection_limits();
        
        // Send sensors to RDMnet
        rdmnet_client_send_sensor(0, g_energy_status.voltage_v);
        rdmnet_client_send_sensor(1, g_energy_status.current_ma / 1000.0f);  // A
        rdmnet_client_send_sensor(2, g_energy_status.power_w);
        
        ESP_LOGD(TAG, "V=%.2fV, I=%dmA, P=%dW", 
                 g_energy_status.voltage_v,
                 g_energy_status.current_ma,
                 g_energy_status.power_w);
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    ESP_LOGI(TAG, "Energy manager task stopped");
    vTaskDelete(NULL);
}

esp_err_t energy_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing energy manager");
    
    // Create event group
    if (!g_energy_event_group) {
        g_energy_event_group = xEventGroupCreate();
        if (!g_energy_event_group) {
            ESP_LOGE(TAG, "Failed to create event group");
            return ESP_FAIL;
        }
    }
    
    // Initialize I2C
    esp_err_t err = init_i2c_master();
    if (err != ESP_OK) {
        return err;
    }
    
    // Initialize INA226
    err = init_ina226();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "INA226 initialization failed, continuing without power monitoring");
    }
    
    // Clear status
    memset(&g_energy_status, 0, sizeof(g_energy_status));
    
    ESP_LOGI(TAG, "Energy manager initialized");
    return ESP_OK;
}

esp_err_t energy_manager_start(void)
{
    if (g_energy_running) {
        ESP_LOGW(TAG, "Energy manager already running");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Starting energy manager");
    g_energy_running = true;
    
    // Create energy task
    BaseType_t ret = xTaskCreate(
        task_energy,
        "energy",
        3072,
        NULL,
        6,  // Priority 6
        NULL
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create energy task");
        g_energy_running = false;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Energy manager started");
    return ESP_OK;
}

esp_err_t energy_manager_stop(void)
{
    ESP_LOGI(TAG, "Stopping energy manager");
    g_energy_running = false;
    return ESP_OK;
}

esp_err_t energy_manager_get_status(energy_status_t *status)
{
    if (!status) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(status, &g_energy_status, sizeof(energy_status_t));
    return ESP_OK;
}

bool energy_manager_is_protection_active(void)
{
    return g_energy_status.overcurrent || 
           g_energy_status.overpower || 
           g_energy_status.undervoltage;
}
