# BSP library CPack configuration
# Configures package generation for distribution

# Set CPack generator to tarball (TGZ)
set(CPACK_GENERATOR "TGZ")

# Extract version from GITHUB_BRANCH_bsp variable (format: v1.0.0)
# If not set, use PROJECT_VERSION
if(DEFINED GITHUB_BRANCH_bsp)
    string(REGEX MATCH "v([0-9]+\\.[0-9]+\\.[0-9]+)" BSP_VERSION_MATCH "${GITHUB_BRANCH_bsp}")
    if(BSP_VERSION_MATCH)
        set(BSP_PACKAGE_VERSION "${CMAKE_MATCH_1}")
    else()
        message(WARNING "Could not extract version from GITHUB_BRANCH_bsp: ${GITHUB_BRANCH_bsp}, using PROJECT_VERSION")
        set(BSP_PACKAGE_VERSION "${PROJECT_VERSION}")
    endif()
else()
    message(STATUS "GITHUB_BRANCH_bsp not set, using PROJECT_VERSION for package")
    set(BSP_PACKAGE_VERSION "${PROJECT_VERSION}")
endif()

# Detect compiler and version for package filename
# Format: bsp-<version>-<compiler><version>.tar.gz
# Example: bsp-1.0.0-gnuarm14.3.tar.gz
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    set(COMPILER_ID "gnuarm")
    string(REGEX MATCH "^([0-9]+\\.[0-9]+)" COMPILER_VERSION_SHORT "${CMAKE_C_COMPILER_VERSION}")
    set(TOOLCHAIN_SUFFIX "${COMPILER_ID}${COMPILER_VERSION_SHORT}")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(COMPILER_ID "clang")
    string(REGEX MATCH "^([0-9]+\\.[0-9]+)" COMPILER_VERSION_SHORT "${CMAKE_C_COMPILER_VERSION}")
    set(TOOLCHAIN_SUFFIX "${COMPILER_ID}${COMPILER_VERSION_SHORT}")
elseif(CMAKE_C_COMPILER_ID MATCHES "ARMCC")
    set(COMPILER_ID "armcc")
    string(REGEX MATCH "^([0-9]+\\.[0-9]+)" COMPILER_VERSION_SHORT "${CMAKE_C_COMPILER_VERSION}")
    set(TOOLCHAIN_SUFFIX "${COMPILER_ID}${COMPILER_VERSION_SHORT}")
else()
    set(TOOLCHAIN_SUFFIX "unknown")
endif()

# Set package name with version and toolchain
set(CPACK_PACKAGE_NAME "bsp")
set(CPACK_PACKAGE_VERSION "${BSP_PACKAGE_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "bsp-${BSP_PACKAGE_VERSION}-${TOOLCHAIN_SUFFIX}")

# Package description
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "BSP library for STM32 microcontrollers")
set(CPACK_PACKAGE_VENDOR "Your Organization")

# Set single component for library
set(CPACK_COMPONENTS_ALL library)

# Install the root CMakeLists.txt from template
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/bsp-root-CMakeLists.txt.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt"
    DESTINATION .
    COMPONENT library
)

# Include CPack module to enable packaging
include(CPack)

message(STATUS "CPack configured: ${CPACK_PACKAGE_FILE_NAME}.tar.gz")
