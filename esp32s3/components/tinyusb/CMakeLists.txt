# Local TinyUSB component — compiles upstream hathach/tinyusb from the
# git submodule at common/dep/tinyusb (pinned to tag 0.19.0).
#
# This replaces the espressif/esp_tinyusb managed component and gives us
# full control over the TinyUSB version.

# Path to the submodule relative to this CMakeLists.txt
set(TUSB_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../common/dep/tinyusb")

idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "esp32s3" OR ${target} STREQUAL "esp32s2")
    set(tusb_mcu "OPT_MCU_ESP32S3")
    if(${target} STREQUAL "esp32s2")
        set(tusb_mcu "OPT_MCU_ESP32S2")
    endif()
else()
    message(FATAL_ERROR "grid tinyusb component: unsupported IDF_TARGET '${target}'")
endif()

set(tusb_srcs
    "${TUSB_ROOT}/src/tusb.c"
    "${TUSB_ROOT}/src/common/tusb_fifo.c"
    "${TUSB_ROOT}/src/device/usbd.c"
    "${TUSB_ROOT}/src/device/usbd_control.c"
    "${TUSB_ROOT}/src/class/cdc/cdc_device.c"
    "${TUSB_ROOT}/src/class/midi/midi_device.c"
    "${TUSB_ROOT}/src/class/hid/hid_device.c"
    "${TUSB_ROOT}/src/class/net/ncm_device.c"
    "${TUSB_ROOT}/src/portable/synopsys/dwc2/dcd_dwc2.c"
    "${TUSB_ROOT}/src/portable/synopsys/dwc2/dwc2_common.c"
)

idf_component_register(
    SRCS        ${tusb_srcs}
    INCLUDE_DIRS
        "include"
        "${TUSB_ROOT}/src"
    PRIV_INCLUDE_DIRS
        "${TUSB_ROOT}/src/device"
    REQUIRES
        freertos
)

target_compile_options(${COMPONENT_LIB} PUBLIC
    "-DCFG_TUSB_MCU=${tusb_mcu}"
)

# usbd.c compares uint8_t against BUILTIN_DRIVER_COUNT which may always be
# true when no built-in class drivers are enabled — suppress the warning.
set_source_files_properties(
    "${TUSB_ROOT}/src/device/usbd.c"
    PROPERTIES COMPILE_FLAGS "-Wno-type-limits"
)
