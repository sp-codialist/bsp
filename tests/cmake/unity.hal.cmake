include (CMakePrintHelpers)

# Only create cpb library if not in testing mode
# During testing, BSP modules use mock_stm32_hal instead of CPB
if(NOT BUILD_TESTING)
    set (libName cpb)
    message (STATUS "${libName}: is supported")
    # Main target ------------------------------------------------------------------
    add_library (${libName} INTERFACE)
    add_library (${libName}::v02 ALIAS ${libName})
    # replace definition of __RAM_FUNC
    configure_file (${CMAKE_CURRENT_LIST_DIR}/host.hal_def.h ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32g4xx_hal_def.h COPYONLY)

    # set own CPB Include and Link directories
    set (CPB_INCLUDE_DIRS
        "${cpu_precompiled_hal_SOURCE_DIR}/include/cpb"
        "${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4"
        "${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Legacy"
        "${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Device"
        "${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Core"
        "${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/CMSIS"
        CACHE STRING "CPB include directories"
    )
    set (CPB_LIBRARIES ${libName} CACHE STRING "CPB libraries")

    target_compile_definitions (${libName}
        INTERFACE
        STM32F429xx
    )

    set (${libName}_HEADER_FILES
        ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_gpio.h
        ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Device/stm32f429xx.h
        ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/CMSIS/core_cm4.h
        ${cpu_precompiled_hal_SOURCE_DIR}/include/cpb/main.h
    )

    set (${libName}_BASE_DIRECTORIES
        "${cpu_precompiled_hal_SOURCE_DIR}/include/cpb"
        "${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4"
        #"${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Legacy"
        #"${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Device"
        #"${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Core"
        #"${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/CMSIS"
    )

    target_sources (${libName}
        PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${${libName}_BASE_DIRECTORIES}
            FILES     ${${libName}_HEADER_FILES}
    )

    target_compile_options(${libName}
        INTERFACE
            -isystem${cpu_precompiled_hal_SOURCE_DIR}/include
            -isystem${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4
            -isystem${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Device
            -isystem${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Legacy
            -isystem${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Core
            -isystem${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/CMSIS
            -isystem${cpu_precompiled_hal_SOURCE_DIR}/include/cpb
    )

    unset (libName)
endif()
