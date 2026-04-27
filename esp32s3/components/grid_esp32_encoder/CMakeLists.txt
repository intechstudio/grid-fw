idf_component_register(
    SRCS
        "grid_esp32_encoder.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "esp_driver_i2s" "grid_esp32_pins" "grid_common"
)
