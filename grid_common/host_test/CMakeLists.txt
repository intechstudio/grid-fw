cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(UnitTest VERSION 1.0)

# Add linker flags to ignore undefined references
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--unresolved-symbols=ignore-in-object-files")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")  # If you're using C++

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")  # If using C++

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie")

# add the executable
add_executable(${PROJECT_NAME} main.c unity.c ../grid_ui_encoder.c ../grid_ui.c ../grid_math.c ../grid_noflash.c)
