#  bsp cmake file for gpio
cmake minimum_required(VERSION 3.13)
project(bsp_gpio)

add_library (bsp_gpio STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/gpio.c
)
target_include_directories (bsp_gpio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/
)
target_link_libraries (bsp_gpio PRIVATE_HEADERS
    cpu_precompiled_hal
)