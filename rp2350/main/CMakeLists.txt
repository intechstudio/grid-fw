add_executable(grid_rp2350
    grid_rp2350.c
    grid_rp2350_led.c
    grid_rp2350_adc.c
    grid_rp2350_littlefs.c
    grid_rp2350_littlefs_api.c
    grid_rp2350_nvm.c
    grid_rp2350_usb.c
    grid_rp2350_platform.c
    grid_rp2350_module_bu16.c
    grid_rp2350_encoder.c
    grid_rp2350_module_ef44.c
    grid_rp2350_uart.c
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
    # Grid protocol/message/transport/UI/Lua stack -- the same file list every
    # other platform (D51, ESP32-S3) compiles in full and dispatches at
    # runtime via hwcfg; see d51n20a/gcc/Makefile and
    # esp32s3/components/grid_common/CMakeLists.txt. RP2350 only ever
    # exercises the BU16 paths today, but grid_module.c bundles every module
    # type's UI-init in one translation unit, so the full UI element-type
    # surface (button/encoder/endless/lcd/potmeter/system/touch) has to be
    # linked regardless.
    ../../common/src/c/grid_ain.c
    ../../common/src/c/grid_allocator.c
    ../../common/src/c/grid_asc.c
    ../../common/src/c/grid_cal.c
    ../../common/src/c/grid_config.c
    ../../common/src/c/grid_decode.c
    ../../common/src/c/grid_health.c
    ../../common/src/c/grid_lua.c
    ../../common/src/c/grid_lua_api.c
    ../../common/src/c/grid_math.c
    ../../common/src/c/grid_module.c
    ../../common/src/c/grid_msg.c
    ../../common/src/c/grid_noflash.c
    ../../common/src/c/grid_port.c
    ../../common/src/c/grid_protocol.c
    ../../common/src/c/grid_rollid.c
    ../../common/src/c/grid_transport.c
    ../../common/src/c/grid_ui.c
    ../../common/src/c/grid_ui_button.c
    ../../common/src/c/grid_ui_encoder.c
    ../../common/src/c/grid_ui_endless.c
    ../../common/src/c/grid_ui_lcd.c
    ../../common/src/c/grid_ui_potmeter.c
    ../../common/src/c/grid_ui_system.c
    ../../common/src/c/grid_ui_touch.c
    ../../common/src/c/grid_utask.c
    # Vendored Lua 5.5.0 interpreter (common/dep), same source list as
    # d51n20a/gcc/Makefile.
    ../../common/dep/lua-5.5.0/src/dirent.c
    ../../common/dep/lua-5.5.0/src/lapi.c
    ../../common/dep/lua-5.5.0/src/lauxlib.c
    ../../common/dep/lua-5.5.0/src/lbaselib.c
    ../../common/dep/lua-5.5.0/src/lcode.c
    ../../common/dep/lua-5.5.0/src/lcorolib.c
    ../../common/dep/lua-5.5.0/src/lctype.c
    ../../common/dep/lua-5.5.0/src/ldblib.c
    ../../common/dep/lua-5.5.0/src/ldebug.c
    ../../common/dep/lua-5.5.0/src/ldo.c
    ../../common/dep/lua-5.5.0/src/ldump.c
    ../../common/dep/lua-5.5.0/src/lfunc.c
    ../../common/dep/lua-5.5.0/src/lgc.c
    ../../common/dep/lua-5.5.0/src/linit.c
    ../../common/dep/lua-5.5.0/src/liolib.c
    ../../common/dep/lua-5.5.0/src/llex.c
    ../../common/dep/lua-5.5.0/src/lmathlib.c
    ../../common/dep/lua-5.5.0/src/lmem.c
    ../../common/dep/lua-5.5.0/src/loadlib.c
    ../../common/dep/lua-5.5.0/src/lobject.c
    ../../common/dep/lua-5.5.0/src/lopcodes.c
    ../../common/dep/lua-5.5.0/src/loslib.c
    ../../common/dep/lua-5.5.0/src/lparser.c
    ../../common/dep/lua-5.5.0/src/lstate.c
    ../../common/dep/lua-5.5.0/src/lstring.c
    ../../common/dep/lua-5.5.0/src/lstrlib.c
    ../../common/dep/lua-5.5.0/src/ltable.c
    ../../common/dep/lua-5.5.0/src/ltablib.c
    ../../common/dep/lua-5.5.0/src/ltm.c
    ../../common/dep/lua-5.5.0/src/lundump.c
    ../../common/dep/lua-5.5.0/src/lutf8lib.c
    ../../common/dep/lua-5.5.0/src/lvm.c
    ../../common/dep/lua-5.5.0/src/lzio.c
    # TOML config parsing, needed by grid_config.c for calibration persistence.
    ../../common/dep/tomlc99/toml.c
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

# PIO programs for the 4-way USART daisy-chain (grid_rp2350_uart.c), copied
# from rp2040/src/ -- see that file's header comment.
pico_generate_pio_header(grid_rp2350 ${CMAKE_CURRENT_LIST_DIR}/uart_tx.pio)
pico_generate_pio_header(grid_rp2350 ${CMAKE_CURRENT_LIST_DIR}/uart_rx.pio)

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
    ../../common/dep/proto
    ../../common/dep/littlefs
    ../../common/dep/tinyusb/src
    ../../common/build/lua
)

# RP2354 has 2 MB of internal stacked flash; the pico2 board otherwise assumes
# 4 MB. Guarded #ifndef in the board header, so this -D wins without a warning.
target_compile_definitions(grid_rp2350 PRIVATE PICO_FLASH_SIZE_BYTES=0x200000)

# Pico SDK defaults CMAKE_BUILD_TYPE to Release when unset, which defines
# NDEBUG and silently compiles out every assert() in common/src/c (bounds
# checks in grid_swsr_write, element-index checks in
# grid_rp2350_module_bu16.c, etc.) -- turning a would-be clean abort into
# silent memory corruption. -UNDEBUG (processed after CMake's own -DNDEBUG)
# keeps asserts active regardless of build type.
target_compile_options(grid_rp2350 PRIVATE -UNDEBUG)

# Import pico_stdlib which imports commonly used features. hardware_resets is
# needed by TinyUSB's rp2040 DCD (dcd_rp2040.c, RESETS_RESET_USBCTRL_BITS) and
# pico_sync by its OPT_OS_PICO OSAL (osal_pico.h: pico/sem.h, pico/mutex.h,
# pico/critical_section.h); pico_unique_id backs grid_rp2350_platform.c's
# grid_platform_get_id().
target_link_libraries(grid_rp2350 PRIVATE pico_stdlib hardware_uart hardware_dma hardware_adc hardware_spi hardware_pio hardware_irq hardware_flash hardware_sync hardware_resets hardware_watchdog pico_sync pico_unique_id pico_rand)

# Route stdio to UART0 (GPIO0 TX, 115200 8N1) and RTT-over-SWD. USB CDC is NOT
# a stdio console here -- the composite grid_usb.c device stack owns the USB
# CDC/ACM channel as the framed Grid protocol channel, matching D51/ESP32-S3
# (pico_enable_stdio_usb would pull in pico-sdk's own vendored TinyUSB and
# fight grid_usb.c for descriptor/enumeration ownership).
#
# The pico2 board's default UART0 RX pin is GPIO1, which collides with the
# HWCFG SHIFT strap pin (see grid_rp2350_platform.c). Console RX is otherwise
# unused (nothing reads stdin), so it's moved off GPIO1 rather than giving up
# a stdio console entirely.
target_compile_definitions(grid_rp2350 PRIVATE PICO_DEFAULT_UART_RX_PIN=9)
pico_enable_stdio_uart(grid_rp2350 1)
pico_enable_stdio_rtt(grid_rp2350 1)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(grid_rp2350)
