set(srcs
    "app_trace.c"
    "app_trace_util.c"
    "host_file_io.c")


set(include_dirs "include" "sys_view/SEGGER")

set(priv_include_dirs "private_include" "port/include")

list(APPEND include_dirs
    sys_view/Config
    sys_view/SEGGER
    sys_view/Sample/FreeRTOSV10.4)

list(APPEND srcs
    "sys_view/SEGGER/SEGGER_SYSVIEW.c"
    "sys_view/Sample/FreeRTOSV10.4/Config/esp/SEGGER_SYSVIEW_Config_FreeRTOS.c"
    "sys_view/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c"
    "sys_view/esp/SEGGER_RTT_esp.c"
    "sys_view/ext/heap_trace_module.c"
    "sys_view/ext/logging.c")


list(APPEND srcs
"port/port_uart.c")


if(CONFIG_HEAP_TRACING_TOHOST)
    list(APPEND srcs "heap_trace_tohost.c")
    set_source_files_properties(heap_trace_tohost.c
        PROPERTIES COMPILE_FLAGS
        -Wno-frame-address)
endif()

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       PRIV_INCLUDE_DIRS "${priv_include_dirs}"
                       # Requires "driver" for GPTimer in "SEGGER_SYSVIEW_Config_FreeRTOS.c"
                       PRIV_REQUIRES soc driver
                       REQUIRES esp_timer
                       LDFRAGMENTS linker.lf)

# Force app_trace to also appear later than gcov in link line
idf_component_get_property(app_trace app_trace COMPONENT_LIB)


target_link_libraries(${COMPONENT_LIB} INTERFACE $<TARGET_FILE:${app_trace}> c)


# This function adds a dependency on the given component if the component is included into the build.
function(maybe_add_component component_name)
    idf_build_get_property(components BUILD_COMPONENTS)
    if(${component_name} IN_LIST components)
        idf_component_get_property(lib_name ${component_name} COMPONENT_LIB)
        target_link_libraries(${COMPONENT_LIB} PUBLIC ${lib_name})
    endif()
endfunction()

maybe_add_component(driver)
