# minimum cmake version
cmake_minimum_required (VERSION 3.12)
include (CMakePrintHelpers)

# set the location of all fetched dependencies
set (FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/_deps")
# download cpm.cmake
file (
    DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/${GITHUB_BRANCH_cpm}/CPM.cmake
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake"
    EXPECTED_HASH SHA256=${GITHUB_BRANCH_cpm_SHA256}
)

include (cmake/CPM.cmake)
include (cmake/deps.toolchain.cmake)
project (
    bsp_lib
    VERSION 0.0.0.255
    LANGUAGES CXX C
    DESCRIPTION "BSP library with CMake integration"
)

# Set default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug only)" FORCE)
    message(STATUS "Build type not specified, defaulting to Debug")
endif()

include (cmake/deps.hal.cmake)

add_subdirectory (bsp_common)
add_subdirectory (bsp_gpio)
add_subdirectory (bsp_swtimer)
add_subdirectory (bsp_led)
add_subdirectory (bsp_adc)
add_subdirectory (bsp_spi)
add_subdirectory (bsp_i2c)
add_subdirectory (bsp_can)
add_subdirectory (bsp_pwm)
add_subdirectory (bsp_rtc)

# Create unified bsp static library from all OBJECT libraries
add_library(bsp STATIC
    $<TARGET_OBJECTS:bsp_adc>
    $<TARGET_OBJECTS:bsp_can>
    $<TARGET_OBJECTS:bsp_gpio>
    $<TARGET_OBJECTS:bsp_i2c>
    $<TARGET_OBJECTS:bsp_led>
    $<TARGET_OBJECTS:bsp_pwm>
    $<TARGET_OBJECTS:bsp_rtc>
    $<TARGET_OBJECTS:bsp_spi>
    $<TARGET_OBJECTS:bsp_swtimer>
)

# Set include directories for the unified library
target_include_directories(bsp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bsp_adc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bsp_can>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bsp_common>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bsp_gpio>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bsp_i2c>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bsp_led>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bsp_pwm>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bsp_rtc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bsp_spi>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bsp_swtimer>
        $<INSTALL_INTERFACE:include/bsp/adc>
        $<INSTALL_INTERFACE:include/bsp/can>
        $<INSTALL_INTERFACE:include/bsp/common>
        $<INSTALL_INTERFACE:include/bsp/gpio>
        $<INSTALL_INTERFACE:include/bsp/i2c>
        $<INSTALL_INTERFACE:include/bsp/led>
        $<INSTALL_INTERFACE:include/bsp/pwm>
        $<INSTALL_INTERFACE:include/bsp/rtc>
        $<INSTALL_INTERFACE:include/bsp/spi>
        $<INSTALL_INTERFACE:include/bsp/swtimer>
    PRIVATE
        $<$<NOT:$<BOOL:${BUILD_TESTING}>>:${CPB_INCLUDE_DIRS}>
)

# Link HAL library privately (not exported)
target_link_libraries(bsp
    PRIVATE
        $<$<NOT:$<BOOL:${BUILD_TESTING}>>:${CPB_LIBRARIES}>
)

# Include Unity/CMock when testing is enabled
if(DEFINED BUILD_TESTING)
    enable_testing()
    add_subdirectory (tests)
endif()

# Include install and CPack configuration only for production builds
# Skip when BUILD_TESTING is enabled
if(NOT BUILD_TESTING)
    include(cmake/bsp-install.cmake)
    include(cmake/cpack.cmake)
    message(STATUS "Install and CPack configuration included")
endif()
