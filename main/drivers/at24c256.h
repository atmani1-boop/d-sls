/**
 * @file at24c256.h
 * @brief I2C driver for AT24C256 EEPROM (32KB)
 * 
 * Driver for Microchip AT24C256 I2C EEPROM with 64-byte page write capability.
 * Supports read/write operations with automatic page alignment and error handling.
 */

#ifndef AT24C256_H
#define AT24C256_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

//============================================================================
// AT24C256 SPECIFICATIONS
//============================================================================

#define AT24C256_I2C_ADDR           0x50    /**< Default I2C address */
#define AT24C256_SIZE               32768   /**< Total memory size (32KB) */
#define AT24C256_PAGE_SIZE          64      /**< Page write size (64 bytes) */
#define AT24C256_WRITE_TIME_MS      5       /**< Maximum write cycle time */

#define AT24C256_SDA_GPIO           20      /**< I2C SDA GPIO */
#define AT24C256_SCL_GPIO           21      /**< I2C SCL GPIO */
#define AT24C256_I2C_FREQ_HZ        400000  /**< I2C clock frequency (400kHz) */

//============================================================================
// FUNCTION PROTOTYPES
//============================================================================

/**
 * @brief Initialize AT24C256 I2C driver
 * 
 * Initializes I2C master bus and device handle for AT24C256 EEPROM.
 * Configures GPIO20 as SDA and GPIO21 as SCL at 400kHz.
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t at24c256_init(void);

/**
 * @brief Deinitialize AT24C256 I2C driver
 * 
 * @return ESP_OK on success
 */
esp_err_t at24c256_deinit(void);

/**
 * @brief Read data from AT24C256
 * 
 * Reads arbitrary length data from EEPROM. Handles 16-bit addressing
 * and automatically handles read operations across page boundaries.
 * 
 * @param addr Memory address (0x0000 - 0x7FFF)
 * @param data Buffer to store read data
 * @param len Number of bytes to read
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG for invalid parameters
 */
esp_err_t at24c256_read(uint16_t addr, uint8_t *data, size_t len);

/**
 * @brief Write data to AT24C256
 * 
 * Writes data to EEPROM with automatic page alignment. If write crosses
 * page boundary, it will be split into multiple page writes. Each page
 * write waits for write cycle completion (5ms).
 * 
 * @param addr Memory address (0x0000 - 0x7FFF)
 * @param data Buffer containing data to write
 * @param len Number of bytes to write
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG for invalid parameters
 */
esp_err_t at24c256_write(uint16_t addr, const uint8_t *data, size_t len);

/**
 * @brief Test AT24C256 functionality
 * 
 * Performs read/write test on first page of EEPROM.
 * Writes test pattern, reads it back, and verifies data integrity.
 * 
 * @return ESP_OK if test passes, ESP_FAIL if test fails
 */
esp_err_t at24c256_test(void);

#endif // AT24C256_H
