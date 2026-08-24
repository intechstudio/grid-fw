idf_component_register(
    SRCS
        "grid_esp32_module_zona.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "grid_common" "grid_esp32_touch"
)
