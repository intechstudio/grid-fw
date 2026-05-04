idf_component_register(
    SRCS
        "grid_esp32_module_vsnx.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "grid_esp32_adc" "grid_esp32_encoder" "grid_esp32_lcd" "grid_common" "grid_esp32_port" "grid_esp32_vmp"
)
