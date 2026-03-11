add_executable(main
    main.c
    ../src/grid_pico_spi.c
    ../src/grid_pico_platform.c
    ../src/pico_pool.c
    ../src/pico_swsr.c
    ../../common/src/c/grid_msg.c
    ../../common/src/c/grid_port.c
    ../../common/src/c/grid_transport.c
    ../../common/src/c/grid_rollid.c
    ../../common/src/c/grid_utask.c
    ../vmp/vmp_def.c
    ../../common/dep/vmp/vmp.c
)

target_include_directories(main PRIVATE ../vmp ../../common/dep/vmp ../src ../../common/src/c)

pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/../src/uart_tx.pio)
pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/../src/uart_rx.pio)

target_sources(main PRIVATE)

# Import pico_stdlib which imports commonly used features
target_link_libraries(main PRIVATE pico_stdlib pico_multicore hardware_pio hardware_pwm hardware_dma hardware_spi hardware_watchdog)

# Create map/bin/hex file etc.
pico_add_extra_outputs(main)

pico_set_binary_type(main no_flash)

# Add path via pico_set_program_url CMAKE
example_auto_set_url(main)
