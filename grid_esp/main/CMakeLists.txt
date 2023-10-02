idf_component_register(
    SRCS 
        "grid_fw.c" 
        "grid_esp32.c"
        "grid_esp32_swd.c"
        "pico_firmware.h"
        "grid_esp32_pins.h"
        "grid_esp32_module_pbf4.c"
        "grid_esp32_module_po16.c"
        "grid_esp32_module_bu16.c"
        "grid_esp32_module_en16.c"
        "grid_esp32_module_ef44.c"
        "grid_esp32_module_tek2.c"
        "grid_esp32_port.c"
        "grid_esp32_nvm.c"
        "grid_esp32_usb.c"
        "grid_esp32_adc.c"
        "grid_esp32_encoder.c"

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
