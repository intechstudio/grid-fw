idf_component_register(
    SRCS
        "grid_esp32_encoder.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "grid_esp32_pins"
)
