idf_component_register(
    SRCS
        "grid_esp32_touch.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "esp_driver_gpio" "freertos"
)
