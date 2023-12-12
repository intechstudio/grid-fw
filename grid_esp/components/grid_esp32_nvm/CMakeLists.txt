idf_component_register(
    SRCS 
        "grid_esp32_nvm.c"
    INCLUDE_DIRS 
        "."
    REQUIRES
        "driver" "littlefs" "grid_common" "spi_flash"
)
