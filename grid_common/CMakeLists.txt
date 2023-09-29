cmake_minimum_required(VERSION 3.10)

idf_component_register(
    SRCS
        "grid_protocol.c"
        "grid_ain.c"
        "grid_led.c"
        "grid_sys.c"
        "grid_msg.c"
        "grid_buf.c"
        "grid_decode.c"
        "grid_port.c"
        "grid_usb.c"
        "grid_ui.c"
        "grid_module.c"
        "grid_lua_api.c"
    INCLUDE_DIRS
	include
)