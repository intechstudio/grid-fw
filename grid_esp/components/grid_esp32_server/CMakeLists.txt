idf_component_register(
    SRCS
        "grid_esp32_server.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "esp_http_server"
)
