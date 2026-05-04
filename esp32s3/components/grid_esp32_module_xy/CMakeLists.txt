idf_component_register(
    SRCS
        "grid_esp32_module_xy.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "grid_common" "grid_esp32_touch"
)
