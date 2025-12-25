# Building the BSP Library

Build instructions and CMake configuration.

## Prerequisites

- CMake 3.12+ (4.0+ recommended)
- ARM GCC toolchain 14.3 (for production builds)
- Ninja build system (recommended)

## Quick Start

### Production Build

```bash
# Configure with ARM GCC toolchain
cmake --preset bsp-gnuarm14.3

# Build
cmake --build build/bsp-gnuarm14.3
```

### Unit Tests

```bash
# Configure tests (no ARM toolchain needed)
cmake --preset bsp-test-host

# Build and run tests
cmake --build build/bsp-test-host
ctest --preset bsp-test-host
```

## CMake Presets

Available build configurations:

- **bsp-gnuarm14.3**: ARM GCC 14.3 production build
- **bsp-atfe21.1**: ARM Clang LLVM 21.1 production build
- **bsp-test-host**: Unit tests (host machine, no ARM toolchain)

## Build Outputs

### Production Build
- Static libraries: `libbsp_gpio.a`, `libbsp_led.a`, `libbsp_swtimer.a`
- Compile database: `compile_commands.json`

### Test Build
- Test executables: `test_bsp_gpio`, `test_bsp_led`, `test_bsp_swtimer`
- Coverage data: `.gcda` and `.gcno` files
- Coverage reports: `coverage/` directory

## Configuration Options

### Build Type
- **Debug** (default): Full debugging symbols, no optimization
- Release builds not currently supported

### Target Device
- **STM32F429xx** (default)
- Configured via CMake presets

## Dependencies

Dependencies are automatically fetched via CPM:

1. **cmake_scripts** (v1.0.7.4)
   - ARM toolchain configurations
   - Build utilities

2. **cpu_precompiled_hal** (v1.28.3)
   - Pre-compiled STM32CubeF4 HAL
   - CMSIS libraries

3. **Unity** (v2.6.0) - Unit testing framework
4. **CMock** (v2.5.3) - Mock generation

## Clean Build

```bash
# Remove build directory
rm -rf build

# Reconfigure and build
cmake --preset bsp-gnuarm14.3
cmake --build build/bsp-gnuarm14.3
```

## Advanced Configuration

### Custom Toolchain Path

Set environment variable before configuring:

```bash
export ARM_TOOLCHAIN_PATH=/path/to/gcc-arm-none-eabi
cmake --preset bsp-gnuarm14.3
```

### Coverage Reports

```bash
cd build/bsp-test-host

# HTML report
gcovr -r ../.. --html-details coverage/index.html

# Terminal summary
gcovr -r ../.. --print-summary

# Filter specific module
gcovr -r ../.. --filter '.*bsp_led\.c$'
```

## Troubleshooting

### CMake configuration fails
- Check CMake version: `cmake --version`
- Verify ARM toolchain in PATH
- Check internet connection (for dependencies)

### Build errors
- Clean build directory: `rm -rf build`
- Verify STM32_DEVICE is set correctly
- Check toolchain compatibility

### Tests fail to link
- Ensure `BUILD_TESTING` is ON
- Verify mock generation succeeded
- Check CMake output for errors

## IDE Integration

### VS Code
- Use `compile_commands.json` for IntelliSense
- Install CMake Tools extension
- Select preset from status bar

### CLion
- Import CMake project
- Select preset from CMake settings
- Configure toolchain in settings

## See Also

- [Testing Guide](testing.md) - Running and writing tests
- [BSP GPIO](bsp_gpio.md) - GPIO module
- [BSP LED](bsp_led.md) - LED module
