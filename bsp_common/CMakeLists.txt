cmake_minimum_required(VERSION 3.12)

# Create interface library for common BSP headers
add_library(bsp_common INTERFACE)

# Specify include directory for downstream targets
target_include_directories(bsp_common
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
