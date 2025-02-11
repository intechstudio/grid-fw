idf_component_register(
    SRCS
        "grid_esp32_trace.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver"
)

set_target_properties(${COMPONENT_LIB} PROPERTIES COMPILE_FLAGS -Wno-int-conversion)