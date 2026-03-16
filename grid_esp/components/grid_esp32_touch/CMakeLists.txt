idf_component_register(
    SRCS
        "grid_esp32_touch.cpp"
        "bb_captouch.cpp"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "freertos"
)
