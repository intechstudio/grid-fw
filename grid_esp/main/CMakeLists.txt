idf_component_register(
    SRCS 
        "grid_fw.c" 
        "grid_esp32.c"
        "grid_esp32_swd.c"
        "pico_firmware.h"
        "grid_esp32_module_pbf4.c"
        "grid_esp32_module_po16.c"
        "grid_esp32_module_bu16.c"
        "grid_esp32_module_en16.c"
        "grid_esp32_module_ef44.c"
        "grid_esp32_module_tek2.c"
        "grid_esp32_port.c"
        "grid_esp32_nvm.c"
        "grid_esp32_usb.c"
        "grid_esp32_encoder.c"

    INCLUDE_DIRS 
        "."

    
)
