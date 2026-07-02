add_executable(main
    main.c
)

# Import pico_stdlib which imports commonly used features
target_link_libraries(main PRIVATE pico_stdlib)

# Route stdio to USB CDC, UART0 (GPIO0 TX / GPIO1 RX, 115200 8N1), and RTT-over-SWD
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 1)
pico_enable_stdio_rtt(main 1)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(main)
