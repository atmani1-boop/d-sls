/**
 * @file config_manager.h
 * @brief Configuration management module
 * 
 * Handles DIP switch reading, profile application, NVS storage/loading,
 * and system configuration management.
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Operating modes
 */
typedef enum {
    MODE_AUTO = 0,      ///< Auto mode - sACN streaming
    MODE_MANUAL = 1     ///< Manual mode - internal scene
} operating_mode_t;

/**
 * @brief System configuration structure
 */
typedef struct {
    operating_mode_t mode;          ///< Operating mode (AUTO/MANUAL)
    uint16_t universe;               ///< sACN universe (1-63999)
    uint16_t start_address;          ///< DMX start address (1-512)
    uint8_t default_scene;           ///< Default scene index (0-15)
    
    // Energy limits
    uint32_t max_current_ma;         ///< Maximum current (mA)
    uint32_t max_power_w;            ///< Maximum power (W)
    float min_voltage_v;             ///< Minimum voltage (V)
    
    // Device identity
    char device_label[32];           ///< Device label for RDMnet
    uint8_t device_id[6];            ///< Device ID (MAC-based)
} system_config_t;

/**
 * @brief Initialize configuration manager
 * 
 * - Reads DIP switch settings
 * - Loads config from NVS or applies defaults
 * - Initializes GPIO for DIP switches
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t config_manager_init(void);

/**
 * @brief Get current system configuration
 * 
 * @return Pointer to system configuration structure
 */
const system_config_t* config_manager_get(void);

/**
 * @brief Save configuration to NVS
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t config_manager_save(void);

/**
 * @brief Factory reset - restore default configuration
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t config_manager_factory_reset(void);

/**
 * @brief Set sACN universe
 * 
 * @param universe Universe number (1-63999)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t config_manager_set_universe(uint16_t universe);

/**
 * @brief Set DMX start address
 * 
 * @param address Start address (1-512)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t config_manager_set_start_address(uint16_t address);

/**
 * @brief Set default scene
 * 
 * @param scene Scene index (0-15)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t config_manager_set_default_scene(uint8_t scene);

/**
 * @brief Set device label
 * 
 * @param label Device label string (max 31 chars)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t config_manager_set_device_label(const char *label);

#endif // CONFIG_MANAGER_H
