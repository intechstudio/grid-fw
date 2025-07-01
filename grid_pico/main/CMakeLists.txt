add_executable(main
    main.c
    grid_pico_spi.c
    grid_pico_platform.c
    ../../grid_common/grid_msg.c
    ../../grid_common/grid_port.c
    ../../grid_common/grid_transport.c
    ../../grid_common/grid_rollid.c
    ../../grid_common/grid_utask.c
    pico_pool.c
    pico_swsr.c
    vmp/vmp_def.c
    ../../vmp/vmp.c
)

target_include_directories(main PRIVATE ../../vmp)

pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/uart_tx.pio)

pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/uart_rx.pio)

target_sources(main PRIVATE)

 # Import pico_stdlib which imports commonly used features
 target_link_libraries(main PRIVATE pico_stdlib pico_multicore hardware_pio hardware_pwm hardware_dma hardware_spi hardware_watchdog)


 # Create map/bin/hex file etc.
 pico_add_extra_outputs(main)


 pico_set_binary_type(main no_flash)


 # Add path via pico_set_program_url CMAKE
 example_auto_set_url(main)
