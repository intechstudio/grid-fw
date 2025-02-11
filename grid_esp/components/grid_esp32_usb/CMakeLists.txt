idf_component_register(
    SRCS
        "grid_esp32_usb.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "grid_common" "esp_tinyusb"
)

set_target_properties(${COMPONENT_LIB} PROPERTIES COMPILE_FLAGS -Wno-incompatible-pointer-types)