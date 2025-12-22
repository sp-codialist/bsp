#  bsp cmake file for gpio
cmake_minimum_required(VERSION 3.13)
project(bsp_gpio)

add_library (bsp_gpio STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_gpio.c
)
target_include_directories (bsp_gpio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
    ${CPB_INCLUDE_DIRS}
)
target_link_libraries (bsp_gpio PRIVATE
    ${CPB_LIBRARIES}
)
