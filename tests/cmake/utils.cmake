include(CMakePrintHelpers)

# Some generated mocks need to be patched in order to prevent compilation errors
# This is possible since some sources have a not self-contained include policy
# in their header files.
function(patch_mocks mock_dir)
    message(STATUS "Patch mocked files for project: ${PROJECT_NAME}")
    file(GLOB MOCKS_TO_PATCH  ${PATCHES_PATH}/*.patch)
    foreach(patch_file IN LISTS MOCKS_TO_PATCH)
        message(STATUS "Applying patch file: ${patch_file} to mocks in ${mock_dir}")
        execute_process(
            COMMAND patch --forward --directory=${mock_dir}
            INPUT_FILE ${patch_file}
            RESULT_VARIABLE patch_result
            OUTPUT_VARIABLE patch_output
            ERROR_VARIABLE patch_error
        )
        if(patch_result AND NOT patch_result EQUAL 0 AND NOT patch_result EQUAL 1)
            message(WARNING "Failed to apply patch ${patch_file}: exit code ${patch_result}")
            message(WARNING "Output: ${patch_output}")
            message(WARNING "Error: ${patch_error}")
        else()
            message(STATUS "Successfully applied patch ${patch_file} (exit code: ${patch_result})")
        endif()
    endforeach()
endfunction()

# Patch source files before generating mocks to prevent compilation errors
# This is useful for removing problematic functions/macros from headers before CMock processes them
function(patch_src src_file patch_file)
    if(NOT EXISTS ${src_file})
        message(WARNING "Source file does not exist: ${src_file}")
        return()
    endif()
    if(NOT EXISTS ${patch_file})
        message(WARNING "Patch file does not exist: ${patch_file}")
        return()
    endif()

    # Silently apply patch
    execute_process(
        COMMAND patch --forward --silent --reject-file=- ${src_file} ${patch_file}
        RESULT_VARIABLE patch_result
        OUTPUT_QUIET
        ERROR_QUIET
    )

    # patch returns 0 if successful, 1 if already applied (which is OK), >1 for real errors
    if(patch_result GREATER 1)
        message(WARNING "Failed to apply patch ${patch_file} to ${src_file} (exit code: ${patch_result})")
    endif()
endfunction()
