set(TINYUSB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../managed_components/espressif__tinyusb")

idf_component_register(
    SRCS
        "grid_esp32_usb.c"
        "grid_esp32_usb_acm.c"
        "grid_esp32_usb_midi.c"
        "grid_esp32_usb_hid.c"
        "grid_esp32_usb_ncm.c"
        "${TINYUSB_PATH}/lib/networking/dhserver.c"
        "${TINYUSB_PATH}/lib/networking/dnserver.c"
    INCLUDE_DIRS
        "."
        "${TINYUSB_PATH}/lib/networking"
    REQUIRES
        "driver" "grid_common" "esp_tinyusb" "lwip"
)

# Suppress warnings in TinyUSB networking library (external code)
set_source_files_properties(
    "${TINYUSB_PATH}/lib/networking/dhserver.c"
    "${TINYUSB_PATH}/lib/networking/dnserver.c"
    PROPERTIES COMPILE_FLAGS "-Wno-address"
)
