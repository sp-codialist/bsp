# add subdirectories for test cases
add_subdirectory (bsp_gpio)

# Coverage target using gcovr
add_custom_target(coverage
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
    COMMAND gcovr -r ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
        --exclude '${CMAKE_SOURCE_DIR}/_deps/.*'
        --exclude '${CMAKE_SOURCE_DIR}/tests/.*'
        --exclude '${CMAKE_BINARY_DIR}/.*runner.*'
        --print-summary
        --html --html-details -o ${CMAKE_BINARY_DIR}/coverage/index.html
    COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated: ${CMAKE_BINARY_DIR}/coverage/index.html"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests and generating coverage report with gcovr"
)
