idf_component_register(
    SRCS
        "grid_esp32_lcd.c"
        "grid_gui.c"
        "grid_lua_api_gui.c"
        "grid_font.c"
        "grid_image.c"
        "./generated_fonts/interdisplay_regular.c"
        "./generated_fonts/spacemono_regular.c"
        "./generated_fonts/spacemono_bold.c"
        "./generated_images/test_bmp.c"
        "./generated_images/test_png.c"
        "./generated_images/test_jpg.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver" "esp_lcd" "grid_esp32_pins" "grid_common"
)
