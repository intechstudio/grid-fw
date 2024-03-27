idf_component_register(
    SRCS
        "grid_esp32_lcd.c"
        "grid_gui.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "esp_lcd" "grid_esp32_pins"
)
