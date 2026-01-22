/**
 * @file logger.h
 * @brief System logging module for Atmani Smart Lighting Node
 * 
 * Provides ESP-IDF logging wrapper with configurable levels and colors.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include "esp_log.h"

/**
 * @brief Initialize logging system
 * 
 * Sets up ESP logging with default level INFO and color output enabled.
 */
void logger_init(void);

#endif // LOGGER_H
