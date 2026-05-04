idf_component_register(
    SRCS
        "grid_esp32_led.c"
        "led_strip_encoder.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "esp_driver_rmt" "grid_common"
)
