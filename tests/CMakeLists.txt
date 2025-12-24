include (cmake/deps.unity.cmake)
include (cmake/deps.cmock.cmake)
include (cmake/unity.hal.cmake)
include (cmake/utils.cmake)
include (cmake/coverage.cmake)
# add subdirectories for test cases
add_subdirectory (bsp_gpio)
