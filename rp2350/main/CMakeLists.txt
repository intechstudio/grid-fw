add_executable(grid_rp2350
    grid_rp2350.c
    grid_rp2350_led.c
    grid_rp2350_adc.c
    grid_rp2350_littlefs.c
    grid_rp2350_littlefs_api.c
    grid_rp2350_nvm.c
    grid_rp2350_usb.c
    grid_rp2350_platform.c
    ../../common/src/c/grid_led.c
    # Shared LittleFS wrapper + portable FS platform API + the vendored
    # LittleFS engine (all from common/).
    ../../common/src/c/grid_littlefs.c
    ../../common/src/c/grid_platform.c
    ../../common/dep/littlefs/lfs.c
    ../../common/dep/littlefs/lfs_util.c
    # Shared, platform-agnostic composite CDC+MIDI+HID USB device stack
    # (common/src/c) -- identical to the D51/ESP32-S3 integrations.
    ../../common/src/c/grid_usb.c
    ../../common/src/c/grid_usb_acm.c
    ../../common/src/c/grid_usb_midi.c
    ../../common/src/c/grid_usb_hid.c
    ../../common/src/c/grid_usb_keyboard.c
    ../../common/src/c/grid_usb_mouse.c
    ../../common/src/c/grid_usb_gamepad.c
    ../../common/src/c/grid_usb_macro.c
    # grid_usb_acm/midi/gamepad/macro.c depend on the swsr ring buffer and on
    # grid_sys_state/grid_sys_get_rx_mode (USB rx-mode gating) even though this
    # bringup doesn't wire the rest of grid_transport/grid_port.
    ../../common/src/c/grid_swsr.c
    ../../common/src/c/grid_sys.c
    # TinyUSB sourced from the shared common/dep/tinyusb submodule (an
    # Intech Studio fork), NOT pico-sdk's own vendored lib/tinyusb -- mirrors
    # the file list d51n20a/gcc/Makefile compiles, with the SAMD DCD swapped
    # for the rp2040 DCD (TinyUSB has no distinct RP2350 port; rp2040 covers
    # both chips, see common/dep/tinyusb/src/common/tusb_mcu.h).
    ../../common/dep/tinyusb/src/tusb.c
    ../../common/dep/tinyusb/src/common/tusb_fifo.c
    ../../common/dep/tinyusb/src/device/usbd.c
    ../../common/dep/tinyusb/src/device/usbd_control.c
    ../../common/dep/tinyusb/src/class/cdc/cdc_device.c
    ../../common/dep/tinyusb/src/class/midi/midi_device.c
    ../../common/dep/tinyusb/src/class/hid/hid_device.c
    ../../common/dep/tinyusb/src/portable/raspberrypi/rp2040/dcd_rp2040.c
    ../../common/dep/tinyusb/src/portable/raspberrypi/rp2040/rp2040_usb.c
)

# Access to the shared grid_led / grid_littlefs / grid_usb models (common/src/c),
# the LittleFS public header via "littlefs/lfs.h" (common/dep), the engine's own
# "lfs.h" / "lfs_util.h" includes (common/dep/littlefs), TinyUSB's public headers
# (common/dep/tinyusb/src), and this directory's own tusb_config.h (TinyUSB's
# tusb_option.h resolves "tusb_config.h" via the include path, not relative to
# the includer, so "." must be listed explicitly).
target_include_directories(grid_rp2350 PRIVATE
    .
    ../../common/src/c
    ../../common/dep
    ../../common/dep/littlefs
    ../../common/dep/tinyusb/src
)

# RP2354 has 2 MB of internal stacked flash; the pico2 board otherwise assumes
# 4 MB. Guarded #ifndef in the board header, so this -D wins without a warning.
target_compile_definitions(grid_rp2350 PRIVATE PICO_FLASH_SIZE_BYTES=0x200000)

# Import pico_stdlib which imports commonly used features. hardware_resets is
# needed by TinyUSB's rp2040 DCD (dcd_rp2040.c, RESETS_RESET_USBCTRL_BITS) and
# pico_sync by its OPT_OS_PICO OSAL (osal_pico.h: pico/sem.h, pico/mutex.h,
# pico/critical_section.h); pico_unique_id backs grid_rp2350_platform.c's
# grid_platform_get_id().
target_link_libraries(grid_rp2350 PRIVATE pico_stdlib hardware_uart hardware_dma hardware_adc hardware_irq hardware_flash hardware_sync hardware_resets pico_sync pico_unique_id)

# Route stdio to UART0 (GPIO0 TX / GPIO1 RX, 115200 8N1) and RTT-over-SWD. USB
# CDC is NOT a stdio console here -- the composite grid_usb.c device stack
# owns the USB CDC/ACM channel as the framed Grid protocol channel, matching
# D51/ESP32-S3 (pico_enable_stdio_usb would pull in pico-sdk's own vendored
# TinyUSB and fight grid_usb.c for descriptor/enumeration ownership).
pico_enable_stdio_uart(grid_rp2350 1)
pico_enable_stdio_rtt(grid_rp2350 1)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(grid_rp2350)
