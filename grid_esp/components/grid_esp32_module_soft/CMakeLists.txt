idf_component_register(
    SRCS
        "grid_esp32_module_soft.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "grid_esp32_adc" "grid_esp32_encoder" "grid_common"
)
