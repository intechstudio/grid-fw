idf_component_register(
    SRCS
        "grid_esp32_platform.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "efuse" "grid_esp32_adc" "grid_esp32_pins"
)
