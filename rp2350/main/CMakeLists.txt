add_executable(grid_rp2350
    grid_rp2350.c
    grid_rp2350_led.c
    grid_rp2350_adc.c
    grid_rp2350_littlefs.c
    grid_rp2350_littlefs_api.c
    grid_rp2350_nvm.c
    ../../common/src/c/grid_led.c
    # Shared LittleFS wrapper + portable FS platform API + the vendored
    # LittleFS engine (all from common/).
    ../../common/src/c/grid_littlefs.c
    ../../common/src/c/grid_platform.c
    ../../common/dep/littlefs/lfs.c
    ../../common/dep/littlefs/lfs_util.c
)

# Access to the shared grid_led / grid_littlefs models (common/src/c), the
# LittleFS public header via "littlefs/lfs.h" (common/dep), and the engine's own
# "lfs.h" / "lfs_util.h" includes (common/dep/littlefs).
target_include_directories(grid_rp2350 PRIVATE
    ../../common/src/c
    ../../common/dep
    ../../common/dep/littlefs
)

# RP2354 has 2 MB of internal stacked flash; the pico2 board otherwise assumes
# 4 MB. Guarded #ifndef in the board header, so this -D wins without a warning.
target_compile_definitions(grid_rp2350 PRIVATE PICO_FLASH_SIZE_BYTES=0x200000)

# Import pico_stdlib which imports commonly used features
target_link_libraries(grid_rp2350 PRIVATE pico_stdlib hardware_uart hardware_dma hardware_adc hardware_irq hardware_flash hardware_sync)

# Route stdio to USB CDC, UART0 (GPIO0 TX / GPIO1 RX, 115200 8N1), and RTT-over-SWD
pico_enable_stdio_usb(grid_rp2350 1)
pico_enable_stdio_uart(grid_rp2350 1)
pico_enable_stdio_rtt(grid_rp2350 1)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(grid_rp2350)
