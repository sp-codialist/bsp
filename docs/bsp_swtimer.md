# BSP Software Timer Module

Periodic timer functionality with callback support.

## Features

- Non-blocking software timers based on HAL_SYSTICK
- Periodic and one-shot modes
- Multiple timer registration (up to 16)
- Timer expiration with rollover handling
- 100% test coverage

## API Reference

- `SWTimerInit(timer)` - Initialize timer
- `SWTimerStart(timer)` - Start timer
- `SWTimerStop(timer)` - Stop timer
- `SWTimerProcess(timer)` - Process timer (called periodically)
- `SWTimerIsActive(timer)` - Check if timer is running

## Usage Example

```c
#include "bsp_swtimer.h"

SWTimerModule myTimer;

void timerCallback(void) {
    // Called when timer expires
}

void setup(void) {
    myTimer.interval = 1000;  // 1000ms
    myTimer.periodic = true;
    myTimer.pCallbackFunction = timerCallback;

    SWTimerInit(&myTimer);
    SWTimerStart(&myTimer);
}

// In SysTick handler
void HAL_SYSTICK_Callback(void) {
    SWTimerProcess(&myTimer);
}
```

## Timer Configuration

```c
typedef struct {
    uint32_t interval;              // Timer interval in ms
    bool periodic;                  // true = periodic, false = one-shot
    bool active;                    // Timer running state
    uint32_t expiration;            // Expiration tick value
    SWTimerCallbackFunction callback; // Callback function
} SWTimerModule;
```

## Implementation Notes

- Timers are based on HAL_GetTick() (typically 1ms resolution)
- Handles tick counter rollover automatically
- Callbacks should be kept short
- Maximum 16 timers can be registered globally

## See Also

- [BSP LED](bsp_led.md) - Uses software timers
- [Testing Guide](testing.md)
