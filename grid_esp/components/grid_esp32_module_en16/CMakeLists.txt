idf_component_register(
    SRCS 
        "grid_esp32_module_en16.c"
    INCLUDE_DIRS 
        "."
    REQUIRES
        "grid_esp32_encoder" "grid_common"
)
