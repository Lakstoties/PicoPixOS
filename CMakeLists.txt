cmake_minimum_required(VERSION 3.17)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

# Set the project
project(picopixos C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_subdirectory(ws2812)

# Create the executable
add_executable(picopixos main.c)

target_link_libraries(picopixos pico_stdlib hardware_pio pico_multicore)


# create map/bin/hex file etc.
pico_add_extra_outputs(picopixos)


# enable usb output, disable uart output
pico_enable_stdio_usb(picopixos 1)
pico_enable_stdio_uart(picopixos 0)