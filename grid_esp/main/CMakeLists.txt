idf_component_register(
    SRCS 
        "grid_fw.c" 
        "led_driver.c" 
        "led_strip_encoder.c"
        "grid_esp32.c"
        "pico_firmware.h"
        "grid_esp32_pins.h"

    INCLUDE_DIRS 
        "."
)
