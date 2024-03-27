cmake_minimum_required(VERSION 3.10)
project(GridGUI)

# Add your C source file
add_executable(index main.c ../grid_esp/components/grid_esp32_lcd/grid_gui.c)

# Specify Emscripten as the compiler
set(CMAKE_C_COMPILER emcc)

# Set Emscripten compiler options
set_target_properties(index PROPERTIES
    COMPILE_FLAGS "-s NO_EXIT_RUNTIME=1"
    LINK_FLAGS "-s 'EXPORTED_RUNTIME_METHODS=[\"ccall\"]'"
)

# Set Emscripten-specific flags
set(CMAKE_EXECUTABLE_SUFFIX ".html")

# Specify output directory for HTML file
set_target_properties(index PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)


