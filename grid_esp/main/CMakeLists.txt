idf_component_register(
    SRCS 
        "grid_fw.c" 
        "led_driver.c" 
        "led_strip_encoder.c"
        "grid_esp32.c"
        "grid_esp32_swd.c"
        "pico_firmware.h"
        "grid_esp32_pins.h"
        "grid_esp32_adc.c"
        "../../grid_common/grid_protocol.h"
        "../../grid_common/grid_protocol.c"
        "../../grid_common/grid_ain.c"

    INCLUDE_DIRS 
        "."
)