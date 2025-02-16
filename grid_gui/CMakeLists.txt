cmake_minimum_required(VERSION 3.10)
project(GridGUI)


include_directories(${CMAKE_SOURCE_DIR}/../grid_esp/components/grid_esp32_lcd)
include_directories(${CMAKE_SOURCE_DIR}/../grid_common/lua-5.4.3/src/)
include_directories(${CMAKE_SOURCE_DIR}/../grid_common/)



set(SRC_FILES)
list(APPEND SRC_FILES ../grid_common/grid_lua.c)

list(APPEND SRC_FILES ../grid_esp/components/grid_esp32_lcd/grid_gui.c)
list(APPEND SRC_FILES ../grid_esp/components/grid_esp32_lcd/grid_font.c)
list(APPEND SRC_FILES ../grid_esp/components/grid_esp32_lcd/generated_fonts/interdisplay_regular.c)
list(APPEND SRC_FILES ../grid_esp/components/grid_esp32_lcd/generated_fonts/spacemono_regular.c)
list(APPEND SRC_FILES ../grid_esp/components/grid_esp32_lcd/generated_fonts/spacemono_bold.c)
list(APPEND SRC_FILES ../grid_esp/components/grid_esp32_lcd/grid_lua_api_gui.c)


list(APPEND SRC_FILES ../grid_common/tinyalloc/tinyalloc.c)

add_executable(index main.c ${SRC_FILES})

# Add Lua source files
file(GLOB LUA_SOURCE_FILES ../grid_common/lua-5.4.3/src/*.c)
list(FILTER LUA_SOURCE_FILES EXCLUDE REGEX "lua.c|luac.c")
target_sources(index PRIVATE ${LUA_SOURCE_FILES})


# Exclude a specific file



# Specify Emscripten as the compiler
set(CMAKE_C_COMPILER emcc)

# Set Emscripten compiler options
set_target_properties(index PROPERTIES
    COMPILE_FLAGS "-s NO_EXIT_RUNTIME=1 -sSTACK_SIZE=2000000"
    LINK_FLAGS "-s 'EXPORTED_RUNTIME_METHODS=[\"ccall\"]' --shell-file ${CMAKE_SOURCE_DIR}/shell_minimal.html"
)

# Set Emscripten-specific flags
set(CMAKE_EXECUTABLE_SUFFIX ".html")

# Specify output directory for HTML file
set_target_properties(index PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
