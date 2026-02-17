idf_component_register(
    SRCS
        "night_saver.c"
        "cct_control.c"
        "fusion.c"
        "web_server.c"
        "tasks/task_led.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        esp_http_server
        json
        driver
)

# Embed dashboard HTML
target_add_binary_data(${COMPONENT_LIB} "www/dashboard.html" TEXT)
