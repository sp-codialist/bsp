# BSP LED Module

High-level LED control with advanced blinking patterns.

## Features

- Multiple LED management (up to 16)
- Timer-based periodic updates (50ms intervals)
- Configurable blinking patterns
- Double-blink mode support
- One-shot blink functionality
- 100% test coverage (31 tests)

## API Reference

- `LedInit(led)` - Initialize LED
- `LedStart()` - Start LED timer
- `LedSetPeriod(led, halfPeriodMs, doubleBlinkMs)` - Configure blinking
- `LedBlink(led)` - Trigger one-shot blink

## Blinking Modes

### Constant State

```c
LedSetPeriod(&led, 0xFFFF, 0);  // Always ON (LED_ON)
LedSetPeriod(&led, 0, 0);       // Always OFF (LED_OFF)
```

### Regular Blinking

```c
LedSetPeriod(&led, 500, 0);  // Blink every 500ms
```

### Double-Blink

```c
LedSetPeriod(&led, 1000, 100);  // Two short blinks within 1s period
```

### One-Shot Blink

```c
LedBlink(&led);  // Blink once, regardless of current pattern
```

## Usage Example

```c
#include "bsp_led.h"

LiveLed_t statusLed;

void setup(void) {
    statusLed.ePin = eM_LED1;
    LedInit(&statusLed);

    LedStart();  // Start timer (call once)

    // Blink at 1Hz
    LedSetPeriod(&statusLed, 500, 0);
}

void indicate_error(void) {
    // Fast double-blink pattern
    LedSetPeriod(&statusLed, 200, 50);
}

void acknowledge_event(void) {
    // Quick single blink
    LedBlink(&statusLed);
}
```

## LED Structure

```c
typedef struct {
    uint32_t ePin;              // GPIO pin
    bool     bState;            // Current state
    uint16_t wUpdPeriod;        // Main blink period
    uint16_t wUpdPeriodDoubleBlink; // Double-blink interval
    // ... internal state
} LiveLed_t;
```

## Timing Configuration

Default timing parameters (in `bsp_led.c`):

```c
#define LED_TIMER_PERIOD_MS    50u  // Update frequency
#define LED_UPDATE_PERIOD_50MS 20u  // Period unit (20*50ms = 1s)
```

## Implementation Notes

- Period values are in milliseconds
- Updates applied every 1 second (20 * 50ms)
- Uses `bsp_swtimer` for periodic processing
- All LEDs share one software timer

## See Also

- [BSP GPIO](bsp_gpio.md) - Underlying GPIO control
- [BSP Software Timer](bsp_swtimer.md) - Timer implementation
- [Testing Guide](testing.md)
