#  bsp cmake file for led
cmake_minimum_required(VERSION 3.13)
set (libName bsp_led)
project(${libName} C)

add_library (${libName} STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/${libName}.c
)
target_include_directories (${libName} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
    ${CPB_INCLUDE_DIRS}
)
target_link_libraries (${libName} PRIVATE
    ${CPB_LIBRARIES}
    bsp_gpio
    bsp_swtimer
)
