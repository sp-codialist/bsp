# BSP Library Packaging Guide

This document describes how to build and use the BSP library package.

## Building the Package

### Prerequisites
- CMake 3.13 or higher
- ARM GCC toolchain (e.g., arm-none-eabi-gcc 14.3)
- Production build environment (BUILD_TESTING=OFF)

### Build Steps

1. Configure the project with a production preset:
   ```bash
   cmake --preset bsp-user-gnuarm14.3
   ```

2. Build the library:
   ```bash
   cmake --build --preset bsp-user-gnuarm14.3
   ```

3. Generate the package:
   ```bash
   cd build/bsp-user-gnuarm14.3
   cpack
   ```

This creates a tarball: `bsp-<version>-<toolchain>.tar.gz`

Example: `bsp-0.0.0.255-gnuarm14.3.tar.gz`

### Versioning

The package version is determined by:
1. **Git tag** (recommended): Set `GITHUB_BRANCH_bsp` environment variable to git tag (e.g., `v1.0.0`)
2. **PROJECT_VERSION**: Falls back to version in root CMakeLists.txt

To build with a specific version:
```bash
cmake --preset bsp-user-gnuarm14.3 -DGITHUB_BRANCH_bsp=v1.0.0
```

## Package Structure

```
bsp-<version>-<toolchain>/
├── CMakeLists.txt              # Root loader for CPM/FetchContent
├── lib/
│   ├── libbsp.a                # Static library
│   └── cmake/bsp/              # CMake package files
│       ├── bspConfig.cmake
│       ├── bspConfigVersion.cmake
│       └── bspTargets.cmake
└── include/bsp/                # Headers organized by module
    ├── adc/
    ├── can/
    ├── common/
    ├── gpio/
    ├── i2c/
    ├── led/
    ├── pwm/
    ├── rtc/
    ├── spi/
    └── swtimer/
```

## Using the Package

### Method 1: CPM/FetchContent (Recommended)

Add to your project's CMakeLists.txt:

```cmake
include(CPM.cmake)

CPMAddPackage(
  NAME bsp
  URL "https://yourserver.com/bsp-1.0.0-gnuarm14.3.tar.gz"
  URL_HASH SHA256=<hash>
)

# Link against the library
target_link_libraries(your_target PRIVATE bsp::bsp)
```

The package automatically provides:
- `bsp::bsp` target
- `BSP_INCLUDE_DIRS` variable (all include paths)
- `BSP_LIBRARIES` variable (bsp::bsp)

### Method 2: Manual Installation

1. Extract the package:
   ```bash
   tar -xzf bsp-1.0.0-gnuarm14.3.tar.gz
   ```

2. In your CMakeLists.txt:
   ```cmake
   # Add the package to CMake prefix path
   list(APPEND CMAKE_PREFIX_PATH "/path/to/bsp-1.0.0-gnuarm14.3/lib/cmake")

   # Find HAL first (required dependency)
   find_package(cpb 1.0.11 REQUIRED)

   # Find BSP
   find_package(bsp REQUIRED)

   # Link against the library
   target_link_libraries(your_target PRIVATE bsp::bsp)
   ```

### Method 3: Using the Root CMakeLists.txt

For CPM-style usage, include the package directory:

```cmake
add_subdirectory(/path/to/bsp-1.0.0-gnuarm14.3)

target_link_libraries(your_target PRIVATE ${BSP_LIBRARIES})
target_include_directories(your_target PRIVATE ${BSP_INCLUDE_DIRS})
```

## Dependencies

### Required Dependencies

1. **STM32 HAL (cpb package) >= 1.0.11**
   - Must be available before including bsp
   - Download from: https://github.com/yourusername/stm32cubef4
   - The HAL is NOT bundled with BSP to allow version flexibility

2. **Configuration Headers** (user-provided)
   - `bsp_can_config.h` - CAN peripheral configuration
   - Add to your project's include path

Example structure:
```
your_project/
├── include/
│   └── bsp_can_config.h    # Your CAN configuration
└── CMakeLists.txt
```

### Setting up HAL Dependency

```cmake
# Example: Using CPM to fetch both HAL and BSP
CPMAddPackage(
  NAME cpb
  URL "https://yourserver.com/cpb-hal-1.28.3-gnuarm14.3.tar.gz"
  URL_HASH SHA256=<hash>
)

CPMAddPackage(
  NAME bsp
  URL "https://yourserver.com/bsp-1.0.0-gnuarm14.3.tar.gz"
  URL_HASH SHA256=<hash>
)

# Now link both
target_link_libraries(your_firmware PRIVATE cpb::cpb bsp::bsp)
```

## Include Paths

The package provides multiple include directories:

```cmake
${BSP_INCLUDE_DIRS}
# Expands to:
#   <prefix>/include/bsp/adc
#   <prefix>/include/bsp/can
#   <prefix>/include/bsp/common
#   <prefix>/include/bsp/gpio
#   <prefix>/include/bsp/i2c
#   <prefix>/include/bsp/led
#   <prefix>/include/bsp/pwm
#   <prefix>/include/bsp/rtc
#   <prefix>/include/bsp/spi
#   <prefix>/include/bsp/swtimer
```

In your code:
```c
#include "bsp_gpio.h"
#include "bsp_led.h"
#include "bsp_adc.h"
// etc.
```

## Testing

The package excludes test infrastructure. Tests remain in the development repository only.

To run tests during development:
```bash
cmake --preset bsp-test-host
cmake --build --preset bsp-test-host
ctest --preset bsp-test-host
```

## Troubleshooting

### "cpb::cpb target not found"
- Ensure HAL package (cpb) is loaded before BSP
- Check HAL version is >= 1.0.11

### "bsp_can_config.h not found"
- Create this header in your project
- Add your project's include directory before BSP headers

### Wrong toolchain version
- Package name includes toolchain (e.g., `gnuarm14.3`)
- Use a package built with your target toolchain
- Or rebuild from source with your toolchain

## Contributing

Package generation is configured in:
- [cmake/bsp-install.cmake](cmake/bsp-install.cmake) - Install rules
- [cmake/bspConfig.cmake.in](cmake/bspConfig.cmake.in) - Package config template
- [cmake/cpack.cmake](cmake/cpack.cmake) - CPack configuration
- [cmake/bsp-root-CMakeLists.txt.in](cmake/bsp-root-CMakeLists.txt.in) - Root loader template
