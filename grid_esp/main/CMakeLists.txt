idf_component_register(
    SRCS 
        "grid_fw.c" 
        "grid_esp32.c"
        "grid_esp32_swd.c"
        "pico_firmware.h"
        "grid_esp32_port.c"
        "grid_esp32_nvm.c"
        "grid_esp32_usb.c"

    INCLUDE_DIRS 
        "."

    
)
