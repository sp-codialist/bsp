include (cmake/deps.unity.cmake)
include (cmake/deps.cmock.cmake)
include (cmake/unity.hal.cmake)
include (cmake/utils.cmake)
include (cmake/coverage.cmake)

# Generate mocks for STM32 HAL (shared by all tests)
include (cmake/mock.stm32_hal.cmake)

# add subdirectories for test cases
add_subdirectory (bsp_gpio)
add_subdirectory (bsp_led)
add_subdirectory (bsp_swtimer)
add_subdirectory (bsp_adc)
add_subdirectory (bsp_spi)
add_subdirectory (bsp_i2c)
add_subdirectory (bsp_can)
add_subdirectory (bsp_pwm)
