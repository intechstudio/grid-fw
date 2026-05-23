set(TUSB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../common/dep/tinyusb")

idf_component_register(
    SRCS
        "grid_esp32_usb.c"
        "grid_esp32_usb_acm.c"
        "grid_esp32_usb_hid.c"
        "grid_esp32_usb_midi.c"
        "grid_esp32_usb_ncm.c"
        "${TUSB_ROOT}/lib/networking/dhserver.c"
        "${TUSB_ROOT}/lib/networking/dnserver.c"
    INCLUDE_DIRS
        "."
        "${TUSB_ROOT}/lib/networking"
    REQUIRES
        "driver" "grid_common" "tinyusb" "esp_hw_support" "lwip"
)

# Suppress warnings in TinyUSB networking library (external code)
set_source_files_properties(
    "${TUSB_ROOT}/lib/networking/dhserver.c"
    "${TUSB_ROOT}/lib/networking/dnserver.c"
    PROPERTIES COMPILE_FLAGS "-Wno-address"
)
