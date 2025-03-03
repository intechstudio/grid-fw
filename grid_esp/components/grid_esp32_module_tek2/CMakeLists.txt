idf_component_register(
    SRCS
        "grid_esp32_module_tek2.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "grid_esp32_adc" "grid_common grid_esp32_codec"
)
