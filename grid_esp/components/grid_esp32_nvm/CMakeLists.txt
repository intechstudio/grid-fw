idf_component_register(
    SRCS
        "grid_esp32_nvm.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "grid_common" "grid_esp32_littlefs" "spi_flash"
)
