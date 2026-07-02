add_executable(grid_rp2350
    grid_rp2350.c
    grid_rp2350_led.c
    ../../common/src/c/grid_led.c
)

# Access to the shared grid_led model (grid_led.h / grid_led.c) in common.
target_include_directories(grid_rp2350 PRIVATE ../../common/src/c)

# Import pico_stdlib which imports commonly used features
target_link_libraries(grid_rp2350 PRIVATE pico_stdlib hardware_uart hardware_dma)

# Route stdio to USB CDC, UART0 (GPIO0 TX / GPIO1 RX, 115200 8N1), and RTT-over-SWD
pico_enable_stdio_usb(grid_rp2350 1)
pico_enable_stdio_uart(grid_rp2350 1)
pico_enable_stdio_rtt(grid_rp2350 1)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(grid_rp2350)
