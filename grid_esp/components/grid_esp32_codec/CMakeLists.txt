idf_component_register(
    SRCS
        "grid_esp32_codec.c"
        "grid_lua_api_codec.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "grid_esp32_pins" "grid_esp32_platform" "grid_common"
)
