#  bsp cmake file for rtc
cmake_minimum_required(VERSION 3.13)
set (libName bsp_rtc)
project(${libName} C)

add_library (${libName} OBJECT)
target_sources (${libName}
    PRIVATE
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
