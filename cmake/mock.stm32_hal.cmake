# Mock library for STM32 HAL GPIO functions

set(libName mock_stm32_hal)

# HAL headers to mock
set(${libName}_HEADERS
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_gpio.h
)

# Additional HAL headers that need to be copied (dependencies)
set(${libName}_DEPENDENCY_HEADERS
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_def.h
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_gpio_ex.h
)

# Create directory for mocks
file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/${libName})
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${libName})

# Set environment for mock generation
set(ENV{MOCK_OUT} ${CMAKE_CURRENT_BINARY_DIR}/${libName}/mocks)
set(ENV{MOCK_PREFIX} Mock)

# Copy dependency headers first
foreach(dep_header IN LISTS ${libName}_DEPENDENCY_HEADERS)
    if(EXISTS ${dep_header})
        configure_file(${dep_header} ${CMAKE_CURRENT_BINARY_DIR}/${libName} COPYONLY)
    else()
        message(WARNING "Dependency header not found: ${dep_header}")
    endif()
endforeach()

# Copy host-compatible hal_def.h to override the STM32 version
configure_file(${CMAKE_SOURCE_DIR}/cmake/host.hal_def.h
               ${CMAKE_CURRENT_BINARY_DIR}/${libName}/stm32f4xx_hal_def.h
               COPYONLY)

# Generate mocks for each header
foreach(element IN LISTS ${libName}_HEADERS)
    # Copy header to build directory
    configure_file(${element} ${CMAKE_CURRENT_BINARY_DIR}/${libName} COPYONLY)

    # Extract filename
    get_filename_component(fileName ${element} NAME)

    # Patch stm32f4xx_hal_gpio.h to remove duplicate GPIO_PinState definition
    if(fileName STREQUAL "stm32f4xx_hal_gpio.h")
        file(READ ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} FILE_CONTENTS)
        # Remove the GPIO_PinState typedef that conflicts with hal_def.h
        string(REGEX REPLACE "typedef enum[\r\n\t ]*\\{[\r\n\t ]*GPIO_PIN_RESET[\r\n\t ]*=[\r\n\t ]*0[\r\n\t ]*,[\r\n\t ]*GPIO_PIN_SET[\r\n\t ]*\\}[\r\n\t ]*GPIO_PinState[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Also remove the comment block for GPIO Bit SET and Bit RESET enumeration if it exists
        string(REGEX REPLACE "/\\*\\*[\r\n\t ]*\\*[\r\n\t ]*@brief[ \t]+GPIO Bit SET and Bit RESET enumeration[^\n]*\n[^\n]*\\*/" "" FILE_CONTENTS "${FILE_CONTENTS}")
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} "${FILE_CONTENTS}")
    endif()

    # Generate mock
    message(STATUS "Creating mock for ${fileName}...")
    execute_process(
        COMMAND ruby ${CMAKE_SOURCE_DIR}/cmake/create_mocks.rb
                ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName}
        RESULT_VARIABLE mock_result
    )

    if(NOT mock_result EQUAL 0)
        message(WARNING "Failed to generate mock for ${fileName}")
    endif()
endforeach()

# Create mock library
add_library(${libName} STATIC)

# Add generated mock sources
file(GLOB_RECURSE ${libName}_MOCK_SOURCES $ENV{MOCK_OUT}/*.c)
file(GLOB_RECURSE ${libName}_MOCK_HEADERS $ENV{MOCK_OUT}/*.h)

target_sources(${libName}
    PRIVATE
        ${${libName}_MOCK_SOURCES}
)

target_include_directories(${libName}
    PUBLIC
        $ENV{MOCK_OUT}
        ${CMAKE_CURRENT_BINARY_DIR}/${libName}
        ${cpu_precompiled_hal_SOURCE_DIR}/include/cpb
        ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Inc
        ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Drivers/CMSIS/Device/ST/STM32F4xx/Include
        ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Drivers/CMSIS/Core/Include
)

target_link_libraries(${libName}
    PUBLIC
        cmock::framework
)

target_compile_definitions(${libName}
    PUBLIC
        STM32F429xx
)

unset(libName)
