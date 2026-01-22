/**
 * @file logger.c
 * @brief System logging implementation
 */

#include "logger.h"
#include "esp_log.h"

static const char *TAG = "LOGGER";

void logger_init(void)
{
    // Set default log level to INFO
    esp_log_level_set("*", ESP_LOG_INFO);
    
    ESP_LOGI(TAG, "Logger initialized - Level: INFO, Colors: ENABLED");
}
