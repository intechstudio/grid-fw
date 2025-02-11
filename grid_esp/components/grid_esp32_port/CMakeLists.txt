idf_component_register(
    SRCS
        "grid_esp32_port.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "grid_common" "esp_tinyusb" "grid_esp32_platform" "grid_esp32_pins"
)

set_target_properties(${COMPONENT_LIB} PROPERTIES COMPILE_FLAGS -Wno-incompatible-pointer-types)