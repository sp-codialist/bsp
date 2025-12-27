cmake_minimum_required(VERSION 3.21)

# Test target name
set(DUTName bsp_adc)
set(targetName test_${DUTName})

project(${targetName})

# Set CREATE_RUNNER_RUBY_PATH for runner generation script
set(CREATE_RUNNER_RUBY_PATH ${CMAKE_SOURCE_DIR}/tests/cmake CACHE PATH "Path to ruby scripts")

# Test source files
set(${targetName}_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ut_bsp_adc.c
)

# Test include directories
set(${targetName}_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../${DUTName}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp_swtimer
    ${CMAKE_BINARY_DIR}/tests/mock_stm32_hal
)

# Generate Unity test runner
set(UNITY_RUNNER_PATH ${CMAKE_CURRENT_BINARY_DIR}/runner)
file(MAKE_DIRECTORY ${UNITY_RUNNER_PATH})
execute_process(
    COMMAND ruby ${CREATE_RUNNER_RUBY_PATH}/create_runner.rb
            ${CMAKE_CURRENT_SOURCE_DIR}/ut_bsp_adc.c
            ${UNITY_RUNNER_PATH}/ut_bsp_adc_runner.c
    RESULT_VARIABLE runner_result
)

if(NOT runner_result EQUAL 0)
    message(WARNING "Failed to generate test runner for ${targetName}")
endif()

# Create test executable
add_executable(${targetName})

target_sources(${targetName}
    PUBLIC
        ${UNITY_RUNNER_PATH}/ut_bsp_adc_runner.c
    PRIVATE
        ${${targetName}_SOURCES}
)

target_include_directories(${targetName}
    PUBLIC
        ${${targetName}_INCLUDE_DIR}
)

target_link_libraries(${targetName}
    PUBLIC
        bsp_adc       # Links against bsp_adc library which includes all dependencies
        bsp_swtimer   # Explicit link needed for OBJECT library dependencies
)

# Compiler options for coverage and debugging
target_compile_options(${targetName}
    PRIVATE
        -g
        -O0
        -Wall
        -Wshadow
        -fprofile-arcs
        -ftest-coverage
)

# Linker options for coverage
target_link_options(${targetName}
    PRIVATE
        -fprofile-arcs
        --coverage
)

# Register test with CTest
add_test(NAME ctest_${targetName}
    COMMAND ${targetName}
)

unset(DUTName)
unset(targetName)
