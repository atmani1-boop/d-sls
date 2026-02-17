/**
 * @file cli_mram.h
 * @brief Console commands for MRAM storage system
 * 
 * Provides CLI commands for displaying configuration, statistics,
 * and historical data from MRAM storage.
 */

#ifndef CLI_MRAM_H
#define CLI_MRAM_H

#include "esp_err.h"

/**
 * @brief Register MRAM console commands
 * 
 * Registers the following commands:
 * - mram-info: Display configuration and system info
 * - mram-stats: Show statistics and predictions
 * - mram-dump: Dump recent historical entries
 * 
 * @return ESP_OK on success
 */
esp_err_t cli_mram_register(void);

#endif // CLI_MRAM_H
