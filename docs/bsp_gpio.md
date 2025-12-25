# BSP GPIO Module

Low-level GPIO control with interrupt support for STM32F4 microcontrollers.

## Features

- Pin configuration (Input, Push-Pull Output, Open-Drain Output)
- Digital I/O operations (read, write, toggle)
- External interrupt management with callbacks
- 97% test coverage

## API Reference

### Configuration

- `BspGpioGetLLHandle(ePin)` - Get HAL pin handle
- `BspGpioCfgPin(ePin, uPin, eCfg)` - Configure pin mode

**Modes**: `eGPIO_CFG_INPUT`, `eGPIO_CFG_PP_OUT`, `eGPIO_CFG_OD_OUT`

### Digital I/O

- `BspGpioWritePin(ePin, bState)` - Write output
- `BspGpioTogglePin(ePin)` - Toggle output
- `BspGpioReadPin(ePin)` - Read input

### Interrupts

- `BspGpioSetIRQHandler(ePin, callback)` - Register interrupt callback
- `BspGpioEnableIRQ(ePin)` - Enable interrupt
- `BspGpioIRQ()` - Dispatch interrupts (call from ISR)

## Usage Examples

### Basic Output

```c
uint32_t led = eM_LED1;
uint32_t handle = BspGpioGetLLHandle(led);
BspGpioCfgPin(led, handle, eGPIO_CFG_PP_OUT);
BspGpioWritePin(led, true);
BspGpioTogglePin(led);
```

### Interrupt

```c
void buttonCallback(void) { /* Handle press */ }

uint32_t btn = eM_BUTTON;
uint32_t handle = BspGpioGetLLHandle(btn);
BspGpioCfgPin(btn, handle, eGPIO_CFG_INPUT);
BspGpioSetIRQHandler(btn, buttonCallback);
BspGpioEnableIRQ(btn);
```

## Hardware Support

- **Ports**: A, B
- **Pins**: 0-15
- **EXTI**: 0-4 (dedicated), 9_5, 15_10 (shared)

## See Also

- [BSP LED](bsp_led.md) - High-level LED control
- [Common Utilities](bsp_common.md)
