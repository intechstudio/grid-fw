cmake_minimum_required(VERSION 3.10)

idf_component_register(
    SRCS
        "grid_common/grid_protocol.h"
        "grid_common/grid_ain.c"
        "grid_common/grid_led.c"
        "grid_common/grid_sys.c"
        "grid_common/grid_msg.c"
        "grid_common/grid_buf.c"
        "grid_common/grid_decode.c"
        "grid_common/grid_port.c"
        "grid_common/grid_usb.c"
        "grid_common/grid_ui.c"
        "grid_common/grid_module.c"
        "grid_common/grid_lua_api.c"
    INCLUDE_DIRS
	include
)