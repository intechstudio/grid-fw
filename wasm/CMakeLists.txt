cmake_minimum_required(VERSION 3.10)
project(grid_sim)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/../common/build/lua)
include_directories(${CMAKE_SOURCE_DIR}/../common/src/c)
include_directories(${CMAKE_SOURCE_DIR}/../common/dep)
include_directories(${CMAKE_SOURCE_DIR}/../common/dep/lua-5.5.0/src)
include_directories(${CMAKE_SOURCE_DIR}/../common/dep/proto)
include_directories(${CMAKE_SOURCE_DIR}/../esp32s3/components/grid_esp32_lcd)

set(COMMON_SRC ${CMAKE_SOURCE_DIR}/../common/src/c)
set(COMMON_DEP ${CMAKE_SOURCE_DIR}/../common/dep)
set(ESP_COMPONENTS ${CMAKE_SOURCE_DIR}/../esp32s3/components)

# Shared simulator core: the real module logic (grid_ui/grid_led/grid_lua/
# grid_config/...), the wasm platform glue, and the LCD renderer stack the
# module's "lcd" element draws into. Linked into both the browser GUI target
# (index) and the headless target (grid_sim) - see wasm/sim_core.h.
set(CORE_SRC_FILES
    ${CMAKE_SOURCE_DIR}/sim_core.c
    ${CMAKE_SOURCE_DIR}/platform.c

    ${COMMON_SRC}/grid_ain.c
    ${COMMON_SRC}/grid_asc.c
    ${COMMON_SRC}/grid_cal.c
    ${COMMON_SRC}/grid_config.c
    ${COMMON_SRC}/grid_decode.c
    ${COMMON_SRC}/grid_health.c
    ${COMMON_SRC}/grid_led.c
    ${COMMON_SRC}/grid_lua.c
    ${COMMON_SRC}/grid_lua_api.c
    ${COMMON_SRC}/grid_math.c
    ${COMMON_SRC}/grid_module.c
    ${COMMON_SRC}/grid_msg.c
    ${COMMON_SRC}/grid_noflash.c
    ${COMMON_SRC}/grid_port.c
    ${COMMON_SRC}/grid_protocol.c
    ${COMMON_SRC}/grid_rollid.c
    ${COMMON_SRC}/grid_swsr.c
    ${COMMON_SRC}/grid_sys.c
    ${COMMON_SRC}/grid_transport.c
    ${COMMON_SRC}/grid_utask.c
    ${COMMON_SRC}/grid_ui.c
    ${COMMON_SRC}/grid_ui_button.c
    ${COMMON_SRC}/grid_ui_encoder.c
    ${COMMON_SRC}/grid_ui_endless.c
    ${COMMON_SRC}/grid_ui_lcd.c
    ${COMMON_SRC}/grid_ui_potmeter.c
    ${COMMON_SRC}/grid_ui_system.c
    ${COMMON_SRC}/grid_ui_touch.c

    ${CMAKE_SOURCE_DIR}/usb_stub.c

    ${COMMON_DEP}/tomlc99/toml.c

    ${ESP_COMPONENTS}/grid_esp32_lcd/grid_gui.c
    ${ESP_COMPONENTS}/grid_esp32_lcd/grid_font.c
    ${ESP_COMPONENTS}/grid_esp32_lcd/generated_fonts/interdisplay_regular.c
    ${ESP_COMPONENTS}/grid_esp32_lcd/generated_fonts/spacemono_regular.c
    ${ESP_COMPONENTS}/grid_esp32_lcd/generated_fonts/spacemono_bold.c
    ${ESP_COMPONENTS}/grid_esp32_lcd/grid_lua_api_gui.c
)

# Add Lua source files
file(GLOB LUA_SOURCE_FILES ${COMMON_DEP}/lua-5.5.0/src/*.c)
list(FILTER LUA_SOURCE_FILES EXCLUDE REGEX "lua.c|luac.c")
list(APPEND CORE_SRC_FILES ${LUA_SOURCE_FILES})

# Specify emscripten as the compiler
set(CMAKE_C_COMPILER emcc)

# ---------------------------------------------------------------------------
# Browser GUI target: renders the VSN1 module's LCD to a canvas and forwards
# captured keyboard input into the simulator (wasm/gui_main.c).
# ---------------------------------------------------------------------------

add_executable(index ${CORE_SRC_FILES} ${CMAKE_SOURCE_DIR}/gui_main.c)

set_target_properties(index PROPERTIES
    SUFFIX ".html"
    COMPILE_FLAGS "-s NO_EXIT_RUNTIME=1 -sSTACK_SIZE=2000000"
    LINK_FLAGS "-s 'EXPORTED_RUNTIME_METHODS=[\"ccall\",\"cwrap\",\"HEAPU8\"]' -s 'EXPORTED_FUNCTIONS=[\"_main\",\"_malloc\",\"_free\"]' --shell-file ${CMAKE_SOURCE_DIR}/shell_minimal.html"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ---------------------------------------------------------------------------
# Headless target: no SDL, no HTML shell - a plain module usable from
# Node.js (require()) or a browser page (import), driven entirely through
# the sim_core.h API (wasm/headless_main.c, wasm/example.js).
# ---------------------------------------------------------------------------

add_executable(grid_sim ${CORE_SRC_FILES} ${CMAKE_SOURCE_DIR}/headless_main.c)

set_target_properties(grid_sim PROPERTIES
    SUFFIX ".js"
    COMPILE_FLAGS "-s NO_EXIT_RUNTIME=1 -sSTACK_SIZE=2000000"
    LINK_FLAGS "-sENVIRONMENT=web,node -sMODULARIZE=1 -sEXPORT_NAME=GridModuleSim -sALLOW_MEMORY_GROWTH=1 -s 'EXPORTED_RUNTIME_METHODS=[\"ccall\",\"cwrap\",\"HEAPU8\"]' -s 'EXPORTED_FUNCTIONS=[\"_main\",\"_malloc\",\"_free\"]'"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
