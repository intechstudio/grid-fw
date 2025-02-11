idf_component_register(
    SRCS
        "grid_fw.c"
        "grid_esp32.c"
        "pico_firmware.h"

    INCLUDE_DIRS
        "."
)

set_target_properties(${COMPONENT_LIB} PROPERTIES COMPILE_FLAGS -Wno-incompatible-pointer-types)