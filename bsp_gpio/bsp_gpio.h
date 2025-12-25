/*
 * bsp_gpio.h
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

typedef void (*GpioIrqCb_t)(void);

/**
 * Write digital output.
 * @param ePin enum representing the pin
 * @param bSet true for HIGH, false for
 * LOW
 */
void BspGpioWritePin(uint32_t const ePin, bool const bSet);

/**
 * Toggle digital output value
 * @param ePin enum representing the pin
 */
void BspGpioTogglePin(uint32_t const ePin);

/**
 * Read digital input value
 * @param ePin enum representing the pin
 * @return true if HIGH, false if
 * LOW
 */
bool BspGpioReadPin(uint32_t const ePin);
/**
 * Register callback function to be called when corresponding
 * interrupt is triggered
 * @param ePin
 * enum representing the pin
 * @param pCb callback function
 */
void BspGpioSetIRQHandler(uint32_t const ePin, GpioIrqCb_t pCb);

/**
 * Enable external interrupt on the given pin
 * @param ePin enum representing the pin
 */
void BspGpioEnableIRQ(uint32_t const ePin);

#ifdef __cplusplus
}
#endif
