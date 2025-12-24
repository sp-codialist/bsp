#  bsp cmake file for led
cmake_minimum_required(VERSION 3.13)
set (libName bsp_led)
project(${libName} C)

add_library (${libName} STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/${libName}.c
)
target_include_directories (${libName}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
    $<$<NOT:$<BOOL:${BUILD_TESTING}>>:${CPB_INCLUDE_DIRS}>
)
target_link_libraries (${libName}
    PUBLIC
    bsp_common
    $<$<BOOL:${BUILD_TESTING}>:mock_stm32_hal>
    PRIVATE
    $<$<NOT:$<BOOL:${BUILD_TESTING}>>:${CPB_LIBRARIES}>
    bsp_gpio
    bsp_swtimer
)

target_compile_options (${libName} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    $<$<BOOL:${BUILD_TESTING}>:--coverage -fprofile-arcs -ftest-coverage>
)

target_compile_definitions(${libName} PRIVATE
    $<$<BOOL:${BUILD_TESTING}>:UNITY_UNIT_TESTS>
)

target_link_options(${libName} PRIVATE
    $<$<BOOL:${BUILD_TESTING}>:--coverage>
)
