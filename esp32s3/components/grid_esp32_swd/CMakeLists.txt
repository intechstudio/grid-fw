idf_component_register(
    SRCS
        "grid_esp32_swd.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "esp_driver_gpio" "esp_driver_ledc"
)
