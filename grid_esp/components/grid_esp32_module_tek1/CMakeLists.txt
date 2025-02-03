idf_component_register(
    SRCS
        "grid_esp32_module_tek1.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "grid_esp32_adc" "grid_esp32_lcd" "grid_common" "grid_esp32_vmp"
)
