idf_component_register(
    SRCS 
        "grid_fw.c" 
        "led_strip_encoder.c"
        "grid_esp32.c"
        "grid_esp32_swd.c"
        "pico_firmware.h"
        "grid_esp32_pins.h"
        "grid_esp32_module_pbf4.c"
        "grid_esp32_module_po16.c"
        "grid_esp32_module_bu16.c"
        "grid_esp32_module_en16.c"
        "grid_esp32_module_ef44.c"
        "grid_esp32_led.c"
        "grid_esp32_port.c"
        "grid_esp32_nvm.c"
        "grid_esp32_usb.c"
        "grid_esp32_adc.c"
        "grid_esp32_encoder.c"

        "../../grid_common/grid_protocol.h"
        "../../grid_common/grid_ain.c"
        "../../grid_common/grid_led.c"
        "../../grid_common/grid_sys.c"
        "../../grid_common/grid_msg.c"
        "../../grid_common/grid_buf.c"
        "../../grid_common/grid_port.c"
        "../../grid_common/grid_usb.c"
        "../../grid_common/grid_ui.c"
        "../../grid_common/grid_module.c"
        "../../grid_common/grid_lua_api.c"
        
        "../../grid_common/lua-5.4.3/src/lapi.c"
        "../../grid_common/lua-5.4.3/src/lctype.c"
        "../../grid_common/lua-5.4.3/src/lfunc.c"
        "../../grid_common/lua-5.4.3/src/lmathlib.c"
        "../../grid_common/lua-5.4.3/src/loslib.c"
        "../../grid_common/lua-5.4.3/src/ltable.c"
        "../../grid_common/lua-5.4.3/src/lundump.c"
        "../../grid_common/lua-5.4.3/src/lauxlib.c"
        "../../grid_common/lua-5.4.3/src/ldblib.c"
        "../../grid_common/lua-5.4.3/src/lgc.c"
        "../../grid_common/lua-5.4.3/src/lmem.c"
        "../../grid_common/lua-5.4.3/src/lparser.c"
        "../../grid_common/lua-5.4.3/src/ltablib.c"
        "../../grid_common/lua-5.4.3/src/lutf8lib.c"
        "../../grid_common/lua-5.4.3/src/lbaselib.c"
        "../../grid_common/lua-5.4.3/src/ldebug.c"
        "../../grid_common/lua-5.4.3/src/linit.c"
        "../../grid_common/lua-5.4.3/src/loadlib.c"
        "../../grid_common/lua-5.4.3/src/lstate.c"
        "../../grid_common/lua-5.4.3/src/ltm.c"
        "../../grid_common/lua-5.4.3/src/lvm.c"
        "../../grid_common/lua-5.4.3/src/lcode.c"
        "../../grid_common/lua-5.4.3/src/ldo.c"
        "../../grid_common/lua-5.4.3/src/liolib.c"
        "../../grid_common/lua-5.4.3/src/lobject.c"
        "../../grid_common/lua-5.4.3/src/lstring.c"
        "../../grid_common/lua-5.4.3/src/lzio.c"
        "../../grid_common/lua-5.4.3/src/llex.c"
        "../../grid_common/lua-5.4.3/src/lopcodes.c"
        "../../grid_common/lua-5.4.3/src/ldump.c"
        "../../grid_common/lua-5.4.3/src/lstrlib.c"
        "../../grid_common/lua-5.4.3/src/lcorolib.c"



    INCLUDE_DIRS 
        "."
)

#
# ULP support additions to component CMakeLists.txt.
#
# 1. The ULP app name must be unique (if multiple components use ULP).
set(ulp_app_name ulp_${COMPONENT_NAME})
#
# 2. Specify all C and Assembly source files.
#    Files should be placed into a separate directory (in this case, ulp/),
#    which should not be added to COMPONENT_SRCS.
set(ulp_riscv_sources "ulp/main.c")

#
# 3. List all the component source files which include automatically
#    generated ULP export file, ${ulp_app_name}.h:
set(ulp_exp_dep_srcs "grid_fw.c")

#
# 4. Call function to build ULP binary and embed in project using the argument
#    values above.
ulp_embed_binary(${ulp_app_name} "${ulp_riscv_sources}" "${ulp_exp_dep_srcs}")
