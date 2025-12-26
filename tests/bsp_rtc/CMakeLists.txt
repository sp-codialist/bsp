cmake_minimum_required(VERSION 3.21)

# Test target name
set(DUTName bsp_rtc)
set(targetName test_${DUTName})

project(${targetName})

# Set CREATE_RUNNER_RUBY_PATH for runner generation script
set(CREATE_RUNNER_RUBY_PATH ${CMAKE_SOURCE_DIR}/tests/cmake CACHE PATH "Path to ruby scripts")

# Test source files
set(${targetName}_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ut_bsp_rtc.c
)

# Test include directories
set(${targetName}_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../${DUTName}
    ${CMAKE_BINARY_DIR}/tests/mock_stm32_hal
)

# Generate Unity test runner
set(UNITY_RUNNER_PATH ${CMAKE_CURRENT_BINARY_DIR}/runner)
file(MAKE_DIRECTORY ${UNITY_RUNNER_PATH})
execute_process(
    COMMAND ruby ${CREATE_RUNNER_RUBY_PATH}/create_runner.rb
            ${CMAKE_CURRENT_SOURCE_DIR}/ut_bsp_rtc.c
            ${UNITY_RUNNER_PATH}/ut_bsp_rtc_runner.c
    RESULT_VARIABLE runner_result
)

if(NOT runner_result EQUAL 0)
    message(WARNING "Failed to generate test runner for ${targetName}")
endif()

# Create test executable
add_executable(${targetName})

target_sources(${targetName}
    PUBLIC
        ${UNITY_RUNNER_PATH}/ut_bsp_rtc_runner.c
    PRIVATE
        ${${targetName}_SOURCES}
)

target_include_directories(${targetName}
    PUBLIC
        ${${targetName}_INCLUDE_DIR}
)

target_compile_definitions(${targetName}
    PRIVATE
        UNITY_UNIT_TESTS
        # No specific device needed for host tests
)

target_link_libraries(${targetName}
    PRIVATE
        unity
        cmock
        mock_stm32_hal
        bsp_rtc
        bsp_common
)

target_compile_options(${targetName}
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        --coverage
        -fprofile-arcs
        -ftest-coverage
)

target_link_options(${targetName}
    PRIVATE
        --coverage
)

# Add test to CTest
add_test(
    NAME ${targetName}
    COMMAND ${targetName}
)

# Generate coverage report
add_custom_target(${targetName}_coverage
    COMMAND gcovr
        --root ${CMAKE_SOURCE_DIR}
        --filter '${CMAKE_SOURCE_DIR}/bsp_rtc/.*'
        --exclude-unreachable-branches
        --exclude-throw-branches
        --print-summary
        --txt ${CMAKE_BINARY_DIR}/${targetName}_coverage.txt
    DEPENDS ${targetName}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating coverage report for ${targetName}"
)
