idf_component_register(
    SRCS
        "grid_esp32_touch.cpp"
        "bb_captouch.cpp"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "esp_driver_gpio" "freertos"
)
