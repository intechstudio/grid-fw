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
    # Shared LittleFS wrapper + FS platform API + vendored engine (common/).
    ../../common/src/c/grid_littlefs.c
    ../../common/src/c/grid_platform.c
    ../../common/dep/littlefs/lfs.c
    ../../common/dep/littlefs/lfs_util.c
    # Shared composite CDC+MIDI+HID USB device stack, identical to D51/ESP32-S3.
    ../../common/src/c/grid_usb.c
    ../../common/src/c/grid_usb_acm.c
    ../../common/src/c/grid_usb_midi.c
    ../../common/src/c/grid_usb_hid.c
    ../../common/src/c/grid_usb_keyboard.c
    ../../common/src/c/grid_usb_mouse.c
    ../../common/src/c/grid_usb_gamepad.c
    ../../common/src/c/grid_usb_macro.c
    # These depend on swsr + grid_sys_get_rx_mode (USB rx-mode gating) even
    # though this bringup doesn't wire grid_transport/grid_port.
    ../../common/src/c/grid_swsr.c
    ../../common/src/c/grid_sys.c
    # Same file list D51/ESP32-S3 compile; RP2350 only exercises BU16 today but
    # grid_module.c bundles every UI element type's init in one translation unit.
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
    # Vendored Lua 5.5.0 (common/dep), same source list as d51n20a/gcc/Makefile.
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
    # TinyUSB from common/dep/tinyusb (Intech fork), NOT pico-sdk's vendored
    # copy -- rp2040 DCD covers RP2350 too (no distinct TinyUSB RP2350 port).
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

# PIO programs for the 4-way USART daisy-chain, copied from rp2040/src/.
pico_generate_pio_header(grid_rp2350 ${CMAKE_CURRENT_LIST_DIR}/uart_tx.pio)
pico_generate_pio_header(grid_rp2350 ${CMAKE_CURRENT_LIST_DIR}/uart_rx.pio)

# Needed for grid_led/littlefs/usb headers, littlefs's own includes, TinyUSB's
# public headers, and this dir's tusb_config.h (resolved via include path).
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

# CMake defaults to Release (defines NDEBUG), silently compiling out every
# assert() in common/src/c. -UNDEBUG keeps them active regardless of build type.
target_compile_options(grid_rp2350 PRIVATE -UNDEBUG -DSEGGER_RTT_SECTION=".rtt")

# hardware_resets is needed by TinyUSB's rp2040 DCD; pico_unique_id backs
# grid_platform_get_id().
target_link_libraries(grid_rp2350 PRIVATE pico_stdlib hardware_uart hardware_dma hardware_adc hardware_spi hardware_pio hardware_irq hardware_flash hardware_sync hardware_resets hardware_watchdog pico_unique_id pico_rand)

# USB CDC is the Grid protocol channel (grid_usb.c), not stdio, matching
# D51/ESP32-S3. RTT-over-SWD is the only console -- UART0 stays off since every RX pin conflicts (HWCFG strap or a daisy-chain PIO line).
pico_enable_stdio_uart(grid_rp2350 0)
pico_enable_stdio_rtt(grid_rp2350 1)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(grid_rp2350)

pico_add_linker_script_override_path(grid_rp2350 ${CMAKE_CURRENT_LIST_DIR} FILES memory_extra.incl)
