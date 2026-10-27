cmake_minimum_required(VERSION 3.13)

# Target the RP2350 (Pico 2). Set before the SDK is imported/initialised.
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Import Pico SDK (requires SDK 2.x for RP2350 support)
include(pico_sdk_import.cmake)

project(rp2350 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

# Add main folder
add_subdirectory(main)
