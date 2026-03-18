cmake_minimum_required(VERSION 3.10)
project(grid_gui)

include_directories(${CMAKE_SOURCE_DIR}/../common/build/lua)
include_directories(${CMAKE_SOURCE_DIR}/../common/src/c)
include_directories(${CMAKE_SOURCE_DIR}/../common/dep)
include_directories(${CMAKE_SOURCE_DIR}/../common/dep/lua-5.4.3/src)
include_directories(${CMAKE_SOURCE_DIR}/../common/dep/proto)
include_directories(${CMAKE_SOURCE_DIR}/../common/dep/tinyalloc)
include_directories(${CMAKE_SOURCE_DIR}/../esp32s3/components/grid_esp32_lcd)

list(APPEND SRC_FILES ${CMAKE_SOURCE_DIR}/main.c)

set(COMMON_SRC ${CMAKE_SOURCE_DIR}/../common/src/c)
list(APPEND SRC_FILES ${COMMON_SRC}/grid_lua.c)
list(APPEND SRC_FILES ${COMMON_SRC}/grid_math.c)
list(APPEND SRC_FILES ${COMMON_SRC}/grid_swsr.c)

set(COMMON_DEP ${CMAKE_SOURCE_DIR}/../common/dep)
list(APPEND SRC_FILES ${COMMON_DEP}/tinyalloc/tinyalloc.c)

set(ESP_COMPONENTS ${CMAKE_SOURCE_DIR}/../esp32s3/components)
list(APPEND SRC_FILES ${ESP_COMPONENTS}/grid_esp32_lcd/grid_gui.c)
list(APPEND SRC_FILES ${ESP_COMPONENTS}/grid_esp32_lcd/grid_font.c)
list(APPEND SRC_FILES ${ESP_COMPONENTS}/grid_esp32_lcd/generated_fonts/interdisplay_regular.c)
list(APPEND SRC_FILES ${ESP_COMPONENTS}/grid_esp32_lcd/generated_fonts/spacemono_regular.c)
list(APPEND SRC_FILES ${ESP_COMPONENTS}/grid_esp32_lcd/generated_fonts/spacemono_bold.c)
list(APPEND SRC_FILES ${ESP_COMPONENTS}/grid_esp32_lcd/grid_lua_api_gui.c)

add_executable(index ${SRC_FILES})

# Add lua source files
file(GLOB LUA_SOURCE_FILES ../common/dep/lua-5.4.3/src/*.c)
list(FILTER LUA_SOURCE_FILES EXCLUDE REGEX "lua.c|luac.c")
target_sources(index PRIVATE ${LUA_SOURCE_FILES})



# Specify emscripten as the compiler
set(CMAKE_C_COMPILER emcc)

# Set emscripten compiler options
set_target_properties(index PROPERTIES
    COMPILE_FLAGS "-s NO_EXIT_RUNTIME=1 -sSTACK_SIZE=2000000"
    LINK_FLAGS "-s 'EXPORTED_RUNTIME_METHODS=[\"ccall\"]' --shell-file ${CMAKE_SOURCE_DIR}/shell_minimal.html"
)

# Set emscripten-specific flags
set(CMAKE_EXECUTABLE_SUFFIX ".html")

# Specify output directory for html file
set_target_properties(index PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
