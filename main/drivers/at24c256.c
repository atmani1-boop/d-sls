/**
 * @file at24c256.c
 * @brief I2C driver implementation for AT24C256 EEPROM (32KB)
 */

#include "at24c256.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "AT24C256";

static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t at24c256_dev_handle = NULL;

//============================================================================
// INITIALIZATION
//============================================================================

esp_err_t at24c256_init(void)
{
    esp_err_t ret;

    // Configure I2C master bus
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = AT24C256_SDA_GPIO,
        .scl_io_num = AT24C256_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure AT24C256 device
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AT24C256_I2C_ADDR,
        .scl_speed_hz = AT24C256_I2C_FREQ_HZ,
    };

    ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &at24c256_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add AT24C256 device: %s", esp_err_to_name(ret));
        i2c_del_master_bus(i2c_bus_handle);
        i2c_bus_handle = NULL;
        return ret;
    }

    ESP_LOGI(TAG, "AT24C256 initialized (I2C addr=0x%02X, SDA=%d, SCL=%d, %dkHz)",
             AT24C256_I2C_ADDR, AT24C256_SDA_GPIO, AT24C256_SCL_GPIO, 
             AT24C256_I2C_FREQ_HZ / 1000);

    return ESP_OK;
}

esp_err_t at24c256_deinit(void)
{
    if (at24c256_dev_handle) {
        i2c_master_bus_rm_device(at24c256_dev_handle);
        at24c256_dev_handle = NULL;
    }

    if (i2c_bus_handle) {
        i2c_del_master_bus(i2c_bus_handle);
        i2c_bus_handle = NULL;
    }

    ESP_LOGI(TAG, "AT24C256 deinitialized");
    return ESP_OK;
}

//============================================================================
// READ OPERATIONS
//============================================================================

esp_err_t at24c256_read(uint16_t addr, uint8_t *data, size_t len)
{
    if (!at24c256_dev_handle) {
        ESP_LOGE(TAG, "Device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!data || len == 0) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    if (addr + len > AT24C256_SIZE) {
        ESP_LOGE(TAG, "Read exceeds memory bounds (addr=0x%04X, len=%u)", addr, len);
        return ESP_ERR_INVALID_ARG;
    }

    // Prepare address (16-bit, big-endian)
    uint8_t addr_buf[2] = {
        (uint8_t)(addr >> 8),   // High byte
        (uint8_t)(addr & 0xFF)  // Low byte
    };

    // Write address, then read data
    esp_err_t ret = i2c_master_transmit_receive(at24c256_dev_handle, 
                                                 addr_buf, sizeof(addr_buf),
                                                 data, len, 
                                                 pdMS_TO_TICKS(1000));

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read failed at 0x%04X: %s", addr, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGD(TAG, "Read %u bytes from 0x%04X", len, addr);
    return ESP_OK;
}

//============================================================================
// WRITE OPERATIONS
//============================================================================

esp_err_t at24c256_write(uint16_t addr, const uint8_t *data, size_t len)
{
    if (!at24c256_dev_handle) {
        ESP_LOGE(TAG, "Device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!data || len == 0) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    if (addr + len > AT24C256_SIZE) {
        ESP_LOGE(TAG, "Write exceeds memory bounds (addr=0x%04X, len=%u)", addr, len);
        return ESP_ERR_INVALID_ARG;
    }

    size_t bytes_written = 0;
    esp_err_t ret = ESP_OK;

    while (bytes_written < len) {
        // Calculate current page boundary
        uint16_t current_addr = addr + bytes_written;
        uint16_t page_offset = current_addr % AT24C256_PAGE_SIZE;
        size_t bytes_remaining = len - bytes_written;
        size_t bytes_in_page = AT24C256_PAGE_SIZE - page_offset;
        size_t write_size = (bytes_remaining < bytes_in_page) ? bytes_remaining : bytes_in_page;

        // Prepare write buffer: [addr_high, addr_low, data...]
        uint8_t write_buf[AT24C256_PAGE_SIZE + 2];
        write_buf[0] = (uint8_t)(current_addr >> 8);   // High byte
        write_buf[1] = (uint8_t)(current_addr & 0xFF); // Low byte
        memcpy(&write_buf[2], &data[bytes_written], write_size);

        // Perform page write
        ret = i2c_master_transmit(at24c256_dev_handle,
                                  write_buf, write_size + 2,
                                  pdMS_TO_TICKS(1000));

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Write failed at 0x%04X: %s", current_addr, esp_err_to_name(ret));
            return ret;
        }

        // Wait for write cycle to complete
        vTaskDelay(pdMS_TO_TICKS(AT24C256_WRITE_TIME_MS));

        bytes_written += write_size;
        ESP_LOGD(TAG, "Wrote %u bytes to 0x%04X", write_size, current_addr);
    }

    ESP_LOGD(TAG, "Write complete: %u bytes at 0x%04X", len, addr);
    return ESP_OK;
}

//============================================================================
// TEST FUNCTION
//============================================================================

esp_err_t at24c256_test(void)
{
    ESP_LOGI(TAG, "Starting AT24C256 test...");

    // Test pattern
    uint8_t test_data[64];
    uint8_t read_buf[64];
    
    // Initialize test pattern
    for (int i = 0; i < sizeof(test_data); i++) {
        test_data[i] = (uint8_t)(i ^ 0xAA);
    }

    // Write test data to address 0x1000 (avoid config area)
    ESP_LOGI(TAG, "Writing test pattern...");
    esp_err_t ret = at24c256_write(0x1000, test_data, sizeof(test_data));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Test write failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Read back test data
    ESP_LOGI(TAG, "Reading test pattern...");
    ret = at24c256_read(0x1000, read_buf, sizeof(read_buf));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Test read failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Verify data
    if (memcmp(test_data, read_buf, sizeof(test_data)) != 0) {
        ESP_LOGE(TAG, "Test data verification failed!");
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, test_data, 16, ESP_LOG_ERROR);
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, read_buf, 16, ESP_LOG_ERROR);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "AT24C256 test PASSED");
    return ESP_OK;
}
