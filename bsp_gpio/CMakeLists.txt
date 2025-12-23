#  bsp cmake file for gpio
cmake_minimum_required(VERSION 3.13)
set (libName bsp_gpio)
project(${libName} C)

# add list of gpio pins to be used for this bsp
set (MAIN_HEADER ${cpu_precompiled_hal_SOURCE_DIR}/include/cpb/main.h CACHE PATH "Path to main.h file")
set (GPIO_STRUCT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gpio_structs CACHE PATH "Path to GPIO structs output directory")
include (${cmake_scripts_SOURCE_DIR}/silicon/STMicroelectronics/stm32cubexx.gpio-struct.cmake)

add_library (${libName} STATIC)
target_sources (${libName}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/${libName}.c
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/gpio_structs/gpio_struct.c
)
target_include_directories (${libName} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
    ${CPB_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/gpio_structs
)
target_link_libraries (${libName} PRIVATE
    ${CPB_LIBRARIES}
)
target_compile_options (${libName} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    $<$<BOOL:${BUILD_TESTING}>:--coverage -fprofile-arcs -ftest-coverage>
)

target_link_options(${libName} PRIVATE
    $<$<BOOL:${BUILD_TESTING}>:--coverage>
)
