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

# Include Unity/CMock when testing is enabled
if(DEFINED BUILD_TESTING)
    enable_testing()
    add_subdirectory (tests)
endif()
