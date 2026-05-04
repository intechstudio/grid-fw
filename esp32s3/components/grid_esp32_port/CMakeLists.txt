idf_component_register(
    SRCS
        "grid_esp32_port.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "grid_common" "esp_tinyusb" "grid_esp32_platform" "grid_esp32_pins" "grid_esp32_lcd"
)
