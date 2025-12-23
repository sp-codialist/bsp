# BSP - Board Support Package

A modular Board Support Package (BSP) library for STM32F4 microcontrollers with CMake integration and dependency management via CPM.

![BSP Architecture](docs/bsp.svg)

## Overview

This BSP library provides hardware abstraction layers for GPIO and LED control on STM32F4 series microcontrollers (specifically STM32F429xx). The project is built using modern CMake practices with CPM (CMake Package Manager) for dependency management and supports multiple toolchain configurations.

## Features

- **GPIO Module** (`bsp_gpio`): Low-level GPIO control with interrupt support
- **LED Module** (`bsp_led`): High-level LED control with blinking patterns and timer-based updates
- **CMake Integration**: Modern CMake build system with presets
- **Dependency Management**: Automated dependency fetching via CPM
- **Toolchain Support**: Multiple ARM toolchain configurations
- **STM32CubeF4 HAL**: Pre-compiled HAL integration for efficient builds

## Project Structure

```
bsp/
├── bsp/                          # Board Support Package modules
│   └── bsp_led/                  # LED control module
│       ├── bsp_led.c
│       └── bsp_led.h
├── bsp_gpio/                     # GPIO control module
│   ├── bsp_gpio.c
│   ├── bsp_gpio.h
│   └── CMakeLists.txt
├── cmake/                        # CMake configuration files
│   ├── CPM.cmake                 # CMake Package Manager
│   ├── deps.hal.cmake           # HAL dependencies
│   └── deps.toolchain.cmake     # Toolchain dependencies
├── CMakePresets/                 # CMake preset configurations
│   ├── BspStaticLibPreset.json
│   ├── HiddenPresets.json
│   └── VersionPresets.json
├── docs/                         # Documentation and diagrams
│   └── bsp.svg                   # Architecture diagram
├── _deps/                        # External dependencies (auto-generated)
│   ├── cmake_scripts-src/       # Build scripts and toolchain configs
│   └── cpu_precompiled_hal-src/ # Pre-compiled STM32 HAL
└── CMakeLists.txt               # Main CMake configuration
```

## Architecture

The BSP library consists of two main components:

### 1. GPIO Module (`bsp_gpio`)

Provides low-level GPIO control functionality:

#### Key Features:
- Pin configuration (Input, Push-Pull Output, Open-Drain Output)
- Digital I/O operations (read, write, toggle)
- External interrupt management with callback registration
- Hardware abstraction for GPIO ports A and B (pins 0-7)

#### API Functions:
- `BspGpioGetLLHandle()` - Get low-level handle for pin configuration
- `BspGpioCfgPin()` - Configure pin mode (input/output type)
- `BspGpioWritePin()` - Write digital output value
- `BspGpioTogglePin()` - Toggle digital output
- `BspGpioReadPin()` - Read digital input value
- `BspGpioSetIRQHandler()` - Register interrupt callback
- `BspGpioEnableIRQ()` - Enable external interrupt on pin
- `BspGpioIRQ()` - Interrupt service routine handler

#### Supported GPIO Ports:
```c
eBSP_GPIO_PORT_A_0 to eBSP_GPIO_PORT_A_7  // Port A, pins 0-7
eBSP_GPIO_PORT_B_0 to eBSP_GPIO_PORT_B_7  // Port B, pins 0-7
```

#### Pin Configuration Options:
- `eGPIO_CFG_OD_OUT` - Open-drain output
- `eGPIO_CFG_PP_OUT` - Push-pull output
- `eGPIO_CFG_INPUT` - Input mode

### 2. LED Module (`bsp_led`)

Provides high-level LED control with advanced blinking patterns:

#### Key Features:
- Multiple LED management (up to 4 LEDs)
- Timer-based periodic updates (50ms intervals)
- Configurable blinking patterns
- Double-blink mode support
- One-shot blink functionality

#### API Functions:
- `LedInit()` - Initialize LED control module
- `LedRegister()` - Register LED with GPIO pin
- `LedStart()` - Start timer for LED blinking
- `LedSetPeriode()` - Configure blinking period and pattern
- `LedBlink()` - Trigger one-shot blink

#### Blinking Modes:
1. **Constant State**: Set LED permanently ON or OFF
   ```c
   LedSetPeriode(hLed, LED_ON, 0);   // Always ON
   LedSetPeriode(hLed, LED_OFF, 0);  // Always OFF
   ```

2. **Regular Blinking**: Periodic on/off pattern
   ```c
   LedSetPeriode(hLed, 500, 0);  // Blink every 500ms
   ```

3. **Double-Blink**: Two short blinks within one period
   ```c
   LedSetPeriode(hLed, 500, 100);  // Double blink pattern
   ```

4. **One-Shot Blink**: Single state transition
   ```c
   LedBlink(hLed);  // Blink once
   ```

## Unit Testing

The BSP library includes comprehensive unit tests using Unity and CMock frameworks. Tests are built to run on the host machine for rapid development and CI/CD integration.

![Unit Test Architecture](docs/bsp-test-host.svg)

### Running Unit Tests

1. **Configure the unit test build:**
   ```bash
   cmake --preset bsp-test-host
   ```

2. **Build the tests:**
   ```bash
   cmake --build build/bsp-test-host
   ```

3. **Run the tests:**
   ```bash
   ctest --preset bsp-test-host
   ```

## Dependencies

### External Dependencies (Auto-fetched by CPM)

1. **cmake_scripts**
   - Repository: [kodezine/cmake_scripts](https://github.com/kodezine/cmake_scripts)
   - Purpose: ARM toolchain configurations and build utilities
   - Includes:
     - ARM GCC toolchain support
     - ARM Clang toolchain support
     - CMSIS frameworks (v5, v6, DSP)
     - Unity/CMock testing frameworks
     - Segger RTT utilities

2. **cpu_precompiled_hal**
   - Source: Pre-compiled STM32CubeF4 HAL (v1.28.3)
   - Target: ARM GNU toolchain 14.3
   - Purpose: STM32F4 HAL drivers and CMSIS
   - Device: STM32F429xx

### Build Dependencies
- CMake 3.12 or higher (4.0+ recommended)
- ARM GCC toolchain 14.3 (or compatible ARM toolchain)
- Ninja build system (recommended)

## Building the Project

### Prerequisites

1. Install CMake (version 4.0.0 or later recommended)
2. Install ARM GCC toolchain (arm-none-eabi-gcc 14.3)
3. Install Ninja build system (optional but recommended)

### Build Instructions

1. **Configure the project using CMake presets:**
   ```bash
   cmake --preset bsp-gnuarm14.3
   ```

2. **Build the library:**
   ```bash
   cmake --build build/bsp-gnuarm14.3
   ```

3. **Clean build:**
   ```bash
   rm -rf build
   cmake --preset bsp-gnuarm14.3
   cmake --build build/bsp-gnuarm14.3
   ```

### CMake Presets

The project includes several CMake presets for different toolchain configurations:

- **bsp-gnuarm14.3**: Build with ARM GCC 14.3 toolchain
- **bsp-atfe21.1**: Build with ARM Clang LLVM 21.1 toolchain (ATFE)
- Build type: Debug (default, only supported mode)
- Target device: STM32F429xx
- Generator: Ninja

### Build Outputs

- **Static Library**: `libbsp_gpio.a` - GPIO module static library
- **Compile Database**: `compile_commands.json` - For IDE/editor integration

## Usage Example

### GPIO Control Example

```c
#include "bsp_gpio.h"

// Configure GPIO pin as output
GpioPort_e ledPin = eBSP_GPIO_PORT_A_5;
uint32_t pinHandle = BspGpioGetLLHandle(ledPin);
BspGpioCfgPin(ledPin, pinHandle, eGPIO_CFG_PP_OUT);

// Turn LED on
BspGpioWritePin(ledPin, true);

// Toggle LED
BspGpioTogglePin(ledPin);

// Read input
bool state = BspGpioReadPin(eBSP_GPIO_PORT_B_0);
```

### LED Control Example

```c
#include "bsp_led.h"

// Initialize LED module
LedInit();

// Register LED
LedHandle_t hLed;
LedRegister(eBSP_GPIO_PORT_A_5, &hLed);

// Start LED timer
LedStart();

// Blink LED at 1Hz (500ms half-period)
LedSetPeriode(hLed, 500, 0);

// Double-blink pattern
LedSetPeriode(hLed, 1000, 100);

// One-shot blink
LedBlink(hLed);
```

### GPIO Interrupt Example

```c
#include "bsp_gpio.h"

// Interrupt callback
void buttonCallback(void)
{
    // Handle button press
}

// Configure pin as input with interrupt
GpioPort_e buttonPin = eBSP_GPIO_PORT_B_3;
uint32_t pinHandle = BspGpioGetLLHandle(buttonPin);
BspGpioCfgPin(buttonPin, pinHandle, eGPIO_CFG_INPUT);

// Register callback and enable interrupt
BspGpioSetIRQHandler(buttonPin, buttonCallback);
BspGpioEnableIRQ(buttonPin);
```

## Hardware Support

### Target Platform
- **Microcontroller**: STM32F4 series
- **Specific Device**: STM32F429xx
- **Core**: ARM Cortex-M4F
- **HAL Version**: STM32CubeF4 v1.28.3

### Supported GPIO Pins
- **Port A**: Pins 0-7 (PA0-PA7)
- **Port B**: Pins 0-7 (PB0-PB7)

### Interrupt Channels
- EXTI0 to EXTI4 (individual channels)
- EXTI9_5 (shared channel for pins 5-9)
- EXTI15_10 (shared channel for pins 10-15)

## Configuration

### Device Configuration
The target STM32 device is configured in CMake presets:
```json
{
  "STM32_DEVICE": "STM32F429xx"
}
```

### Build Configuration
- **Build Type**: Debug only (Release builds not currently supported)
- **Language Standards**: C and C++ enabled
- **Generator**: Ninja (recommended)

## Development

### Adding New GPIO Pins
To extend support for additional GPIO ports:

1. Add new enums to `GpioPort_e` in [bsp_gpio/bsp_gpio.h](bsp_gpio/bsp_gpio.h)
2. Update the port mapping in [bsp_gpio/bsp_gpio.c](bsp_gpio/bsp_gpio.c)
3. Update interrupt channel mappings if needed

### Customizing LED Behavior
LED timing parameters can be adjusted in [bsp/bsp_led/bsp_led.c](bsp/bsp_led/bsp_led.c):
```c
#define LED_CNT                     4u    // Max number of LEDs
#define LED_TIMER_PERIOD_MS         50u   // Update frequency
#define LED_UPDATE_PERIOD_50MS      20u   // Period unit
```

## Version Information

- **Project Version**: 0.0.0.255
- **CMake Version**: 3.12+ required, 4.0.0+ recommended
- **Author**: IlicAleksander
- **License**: (See project license file)

## Troubleshooting

### Common Issues

1. **CMake configuration fails**
   - Ensure CMake version is 3.12 or higher
   - Verify ARM toolchain is in PATH
   - Check internet connectivity (for dependency downloads)

2. **Build errors**
   - Verify STM32_DEVICE is correctly defined
   - Ensure all dependencies are fetched
   - Clean build directory and reconfigure

3. **Toolchain not found**
   - Install ARM GCC toolchain
   - Update PATH environment variable
   - Check CMake preset configuration

## Contributing

When contributing to this repository:
1. Maintain consistent code style
2. Update documentation for API changes
3. Test changes on target hardware
4. Update version numbers appropriately

## See Also

- [STM32CubeF4 Documentation](https://www.st.com/en/embedded-software/stm32cubef4.html)
- [ARM CMSIS Documentation](https://arm-software.github.io/CMSIS_6/)
- [CMake Documentation](https://cmake.org/documentation/)
- [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake)

---

**Last Updated**: December 2025
