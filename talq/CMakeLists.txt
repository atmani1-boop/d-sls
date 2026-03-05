# TALQ conformance component for ESP-IDF
#
# Requires:
#   - esp_http_server  (HTTP/1.1 server)
#   - json             (cJSON bundled with ESP-IDF)
#   - freertos         (FreeRTOS semaphores)
#   - esp_log          (logging)

idf_component_register(
    SRCS
        "talq.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        esp_http_server
        json
        freertos
        esp_log
)
