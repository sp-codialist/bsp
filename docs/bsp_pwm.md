# BSP PWM Module

Hardware-abstracted PWM (Pulse Width Modulation) driver for STM32F4 timer-based PWM generation.

## Features

- Support for 12 timer peripherals (TIM1-5, TIM8-14)
- Up to 16 simultaneously active PWM channels
- Dynamic frequency configuration (1 kHz - 1000 kHz)
- Real-time duty cycle adjustment (0-100%)
- Runtime prescaler modification
- Frequency conflict detection
- Error callback notifications
- 98.6% test coverage (47 tests)

## API Reference

### Channel Management

- `BspPwmAllocateChannel(eTimer, eChannel, wFrequencyKhz)` - Allocate and configure PWM channel
- `BspPwmFreeChannel(handle)` - Free allocated PWM channel

**Timers**: `eBSP_PWM_TIMER_1` through `eBSP_PWM_TIMER_14` (not all timers have 4 channels)

**Channels**: `eBSP_PWM_CHANNEL_1` through `eBSP_PWM_CHANNEL_4`

**Frequency Range**: 1 kHz to 1000 kHz (1 - 1000 in kHz units)

### PWM Control

- `BspPwmStart(handle)` - Start PWM generation on a channel
- `BspPwmStop(handle)` - Stop PWM generation on a channel
- `BspPwmStartAll()` - Start all allocated PWM channels
- `BspPwmStopAll()` - Stop all allocated PWM channels

### Signal Configuration

- `BspPwmSetDutyCycle(handle, wDutyCyclePpt)` - Set duty cycle (0-1000 parts per thousand)
- `BspPwmSetPrescaler(eTimer, wPrescaler)` - Update timer prescaler (affects all channels on timer)

### Error Handling

- `BspPwmRegisterErrorCallback(handle, callback)` - Register error notification callback

## Error Codes

- `eBSP_PWM_ERR_NONE` - No error
- `eBSP_PWM_ERR_INVALID_HANDLE` - Invalid or freed handle
- `eBSP_PWM_ERR_INVALID_PARAM` - Invalid parameter provided
- `eBSP_PWM_ERR_NO_RESOURCE` - No free channel slots available
- `eBSP_PWM_ERR_FREQUENCY_CONFLICT` - Frequency conflict on same timer
- `eBSP_PWM_ERR_TIMER_RUNNING` - Cannot change prescaler while timer is running
- `eBSP_PWM_ERR_HAL_ERROR` - HAL layer error

## Usage Examples

### Basic PWM Generation

```c
#include "bsp_pwm.h"

// Generate 10 kHz PWM signal at 50% duty cycle on TIM2 CH1
BspPwmHandle_t pwm = BspPwmAllocateChannel(eBSP_PWM_TIMER_2, eBSP_PWM_CHANNEL_1, 10);

if (pwm >= 0) {
    // Set 50% duty cycle (500 parts per thousand)
    BspPwmSetDutyCycle(pwm, 500);

    // Start PWM generation
    BspPwmError_e err = BspPwmStart(pwm);

    if (err == eBSP_PWM_ERR_NONE) {
        // PWM is now running
    }
}
```

### Motor Speed Control

```c
#include "bsp_pwm.h"

static BspPwmHandle_t motorPwm;

void motorInit(void) {
    // Allocate 20 kHz PWM for motor control
    motorPwm = BspPwmAllocateChannel(eBSP_PWM_TIMER_3, eBSP_PWM_CHANNEL_2, 20);

    if (motorPwm >= 0) {
        // Start at 0% duty cycle (motor stopped)
        BspPwmSetDutyCycle(motorPwm, 0);
        BspPwmStart(motorPwm);
    }
}

void setMotorSpeed(uint8_t speedPercent) {
    if (speedPercent > 100) {
        speedPercent = 100;
    }

    // Convert percent to parts per thousand
    uint16_t dutyCyclePpt = (speedPercent * 10);
    BspPwmSetDutyCycle(motorPwm, dutyCyclePpt);
}

void motorStop(void) {
    BspPwmStop(motorPwm);
    BspPwmFreeChannel(motorPwm);
}
```

### Multi-Channel LED Dimming

```c
#include "bsp_pwm.h"

static BspPwmHandle_t redLed, greenLed, blueLed;

void rgbLedInit(void) {
    // All on same timer (TIM4) for synchronized operation
    redLed   = BspPwmAllocateChannel(eBSP_PWM_TIMER_4, eBSP_PWM_CHANNEL_1, 1);  // 1 kHz
    greenLed = BspPwmAllocateChannel(eBSP_PWM_TIMER_4, eBSP_PWM_CHANNEL_2, 1);
    blueLed  = BspPwmAllocateChannel(eBSP_PWM_TIMER_4, eBSP_PWM_CHANNEL_3, 1);

    // Start all channels
    BspPwmStartAll();
}

void setRgbColor(uint8_t red, uint8_t green, uint8_t blue) {
    // Scale 0-255 to 0-1000 parts per thousand
    BspPwmSetDutyCycle(redLed,   (red   * 1000) / 255);
    BspPwmSetDutyCycle(greenLed, (green * 1000) / 255);
    BspPwmSetDutyCycle(blueLed,  (blue  * 1000) / 255);
}

void fadeToColor(uint8_t targetRed, uint8_t targetGreen, uint8_t targetBlue) {
    // Smooth transition over 100 steps
    for (int i = 0; i <= 100; i++) {
        setRgbColor(
            (targetRed   * i) / 100,
            (targetGreen * i) / 100,
            (targetBlue  * i) / 100
        );
        HAL_Delay(10);  // 10ms per step = 1 second total
    }
}
```

### Servo Control

```c
#include "bsp_pwm.h"

static BspPwmHandle_t servo;

void servoInit(void) {
    // Standard servo: 50 Hz (20ms period), 1-2ms pulse width
    // We'll use higher frequency and scale duty cycle
    servo = BspPwmAllocateChannel(eBSP_PWM_TIMER_5, eBSP_PWM_CHANNEL_1, 50);

    if (servo >= 0) {
        servoSetAngle(90);  // Center position
        BspPwmStart(servo);
    }
}

void servoSetAngle(uint8_t degrees) {
    if (degrees > 180) {
        degrees = 180;
    }

    // Map 0-180 degrees to 1-2ms pulse width
    // At 50Hz (20ms period), 1ms = 5%, 2ms = 10%
    // So 0° = 50 ppt, 180° = 100 ppt
    uint16_t pulsePpt = 50 + ((degrees * 50) / 180);

    BspPwmSetDutyCycle(servo, pulsePpt);
}
```

### Error Handling with Callbacks

```c
#include "bsp_pwm.h"

static BspPwmHandle_t pwm1, pwm2;

void pwmErrorHandler(BspPwmHandle_t handle, BspPwmError_e error) {
    switch (error) {
        case eBSP_PWM_ERR_FREQUENCY_CONFLICT:
            // Another channel on same timer has different frequency
            // This is a warning - operation continues
            printf("Warning: Frequency conflict on handle %d\n", handle);
            break;

        case eBSP_PWM_ERR_HAL_ERROR:
            // Hardware error occurred
            printf("Error: HAL error on handle %d\n", handle);
            break;

        default:
            break;
    }
}

void setupWithErrorHandling(void) {
    // Allocate first channel
    pwm1 = BspPwmAllocateChannel(eBSP_PWM_TIMER_2, eBSP_PWM_CHANNEL_1, 10);
    BspPwmRegisterErrorCallback(pwm1, pwmErrorHandler);

    // Try to allocate second channel with different frequency
    // This will trigger frequency conflict callback on pwm1
    pwm2 = BspPwmAllocateChannel(eBSP_PWM_TIMER_2, eBSP_PWM_CHANNEL_2, 20);
    // Callback will be called on pwm1 (existing channel)

    BspPwmStart(pwm1);
    BspPwmStart(pwm2);
}
```

### Dynamic Prescaler Adjustment

```c
#include "bsp_pwm.h"

void adjustTimerResolution(void) {
    // Allocate channel
    BspPwmHandle_t pwm = BspPwmAllocateChannel(eBSP_PWM_TIMER_2, eBSP_PWM_CHANNEL_1, 100);

    // Must stop timer before changing prescaler
    BspPwmStop(pwm);

    // Change prescaler to increase resolution
    // Default APB1 prescaler is 83 (84MHz / 84 = 1MHz tick)
    // Lower prescaler = higher resolution, lower max ARR
    BspPwmError_e err = BspPwmSetPrescaler(eBSP_PWM_TIMER_2, 41);

    if (err == eBSP_PWM_ERR_NONE) {
        // Prescaler updated, ARR recalculated for all channels on TIM2
        BspPwmStart(pwm);
    }
}
```

### Complete Lifecycle Example

```c
#include "bsp_pwm.h"

void pwmCompleteExample(void) {
    // Step 1: Allocate channel
    BspPwmHandle_t pwm = BspPwmAllocateChannel(
        eBSP_PWM_TIMER_3,      // Timer 3
        eBSP_PWM_CHANNEL_4,    // Channel 4
        25                      // 25 kHz
    );

    if (pwm < 0) {
        // Allocation failed
        return;
    }

    // Step 2: Configure initial duty cycle
    BspPwmSetDutyCycle(pwm, 250);  // 25%

    // Step 3: Start PWM
    BspPwmError_e err = BspPwmStart(pwm);
    if (err != eBSP_PWM_ERR_NONE) {
        BspPwmFreeChannel(pwm);
        return;
    }

    // Step 4: Adjust duty cycle during operation
    HAL_Delay(1000);
    BspPwmSetDutyCycle(pwm, 750);  // 75%

    // Step 5: Stop PWM
    HAL_Delay(1000);
    BspPwmStop(pwm);

    // Step 6: Free resources
    BspPwmFreeChannel(pwm);
}
```

## Important Notes

### Timer Channel Availability

- **APB1 Timers** (TIM2-5, TIM12-14): Typically 42 MHz base clock
- **APB2 Timers** (TIM1, TIM8-11): Typically 84 MHz base clock

Not all timers support all 4 channels. Check your STM32F4 variant datasheet.

### Frequency Conflicts

Multiple channels on the **same timer** must use the **same frequency**. If you allocate a channel with a different frequency on a timer that already has channels allocated, a frequency conflict warning will be issued via the error callback, but allocation will succeed.

The last allocated frequency will be used for all channels on that timer.

### Duty Cycle Units

Duty cycle is specified in **parts per thousand (ppt)** for fine-grained control:
- `0` = 0% (always low)
- `500` = 50% (square wave)
- `1000` = 100% (always high)

This provides 0.1% resolution.

### Resource Limits

Maximum **16 simultaneous PWM channels** across all timers. Use `BspPwmFreeChannel()` to release unused channels.

### Thread Safety

This module is **not thread-safe**. If used in multi-threaded environment (RTOS), protect API calls with mutexes.

### Prescaler Changes

`BspPwmSetPrescaler()` can only be called when **no channels on that timer are running**. Stop all channels first using `BspPwmStop()` or `BspPwmStopAll()`.

## Timer Characteristics

| Timer   | Bus  | Base Clock* | Channels | Advanced Features |
|---------|------|-------------|----------|-------------------|
| TIM1    | APB2 | 168 MHz     | 4        | Yes (complementary outputs) |
| TIM2    | APB1 | 84 MHz      | 4        | No                |
| TIM3    | APB1 | 84 MHz      | 4        | No                |
| TIM4    | APB1 | 84 MHz      | 4        | No                |
| TIM5    | APB1 | 84 MHz      | 4        | No                |
| TIM8    | APB2 | 168 MHz     | 4        | Yes (complementary outputs) |
| TIM9    | APB2 | 168 MHz     | 2        | No                |
| TIM10   | APB2 | 168 MHz     | 1        | No                |
| TIM11   | APB2 | 168 MHz     | 1        | No                |
| TIM12   | APB1 | 84 MHz      | 2        | No                |
| TIM13   | APB1 | 84 MHz      | 1        | No                |
| TIM14   | APB1 | 84 MHz      | 1        | No                |

*Base clocks shown are typical for 168 MHz system clock with standard prescalers. Actual values depend on your clock configuration.

## Performance Characteristics

- **Channel Allocation**: O(n) where n = number of allocated channels
- **Duty Cycle Update**: O(1) direct register write
- **Start/Stop**: O(1) HAL call
- **Memory Footprint**: ~2 KB code, ~256 bytes RAM (16 channels)

## See Also

- [bsp_gpio.md](bsp_gpio.md) - GPIO pin configuration for PWM outputs
- [bsp_led.md](bsp_led.md) - High-level LED control with patterns
- STM32F4 Reference Manual - Timer chapter for detailed hardware behavior
