idf_component_register(
    SRCS
        "grid_esp32_encoder.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "grid_esp32_pins"
)

set_target_properties(${COMPONENT_LIB} PROPERTIES COMPILE_FLAGS -Wno-incompatible-pointer-types)