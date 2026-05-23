idf_component_register(
    SRCS
        "grid_esp32_usb.c"
        "grid_esp32_usb_acm.c"
        "grid_esp32_usb_hid.c"
        "grid_esp32_usb_midi.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "grid_common" "esp_tinyusb"
)
