# BSP library installation configuration
# This file defines install rules for the bsp library package

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install the unified bsp library
install(TARGETS bsp
    EXPORT bspTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT library
)

# Install headers in modular structure
# Note: Config headers (bsp_can_config.h) are NOT installed - users must provide their own

# bsp_adc headers
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_adc/bsp_adc.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bsp/adc
    COMPONENT library
)

# bsp_can headers (excluding bsp_can_config.h)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_can/bsp_can.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bsp/can
    COMPONENT library
)

# bsp_common headers
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_common/bsp_compiler_attributes.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bsp/common
    COMPONENT library
)

# bsp_gpio headers
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_gpio/bsp_gpio.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bsp/gpio
    COMPONENT library
)

# bsp_i2c headers
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_i2c/bsp_i2c.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bsp/i2c
    COMPONENT library
)

# bsp_led headers
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_led/bsp_led.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bsp/led
    COMPONENT library
)

# bsp_pwm headers
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_pwm/bsp_pwm.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bsp/pwm
    COMPONENT library
)

# bsp_rtc headers
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_rtc/bsp_rtc.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bsp/rtc
    COMPONENT library
)

# bsp_spi headers
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_spi/bsp_spi.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bsp/spi
    COMPONENT library
)

# bsp_swtimer headers
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_swtimer/bsp_swtimer.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bsp/swtimer
    COMPONENT library
)

# Generate version file with SameMajorVersion compatibility
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/bspConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Configure package config file from template
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/bspConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/bspConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bsp
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)

# Install CMake package files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/bspConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/bspConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bsp
    COMPONENT library
)

# Install export targets with bsp:: namespace
install(EXPORT bspTargets
    FILE bspTargets.cmake
    NAMESPACE bsp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bsp
    COMPONENT library
)
