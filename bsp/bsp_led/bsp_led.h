/*
 * bsp_led.h
 *
 *  Created on: May 12, 2025
 *      Author: IlicAleksander
 */

#ifndef BSP_LED_H_
#define BSP_LED_H_

#include "cpu_specific.h"
#include <stdbool.h>
#include <stdint.h>

#define LED_ON          0xFFFFu
#define LED_OFF         0u
#define LED_INVL_HANDLE 0xFFu

typedef uint8_t LedHandle_t;

/**
 * Initializes Alive LED control module.
 * @return true on success
 */
bool LedInit(void);

/**
 * Registers LED.
 * @param ePin pin enum.
 * @param pHandle The assigned LED handle.
 * @return true on success
 */
bool LedRegister(GpioPort_e ePin, LedHandle_t* pHandle);

/**
 * Starts Timer for blinking LEDs. When the timer is started, LEDs will start blinking if
 * configured to do so. LEDs can be set on or off before this function is called; Blinking
 * frequency can be set as well, but takes effect after this function is called.
 */
void LedStart(void);

/**
 * Starts blinking LED periodically, or set to constantly on or off.
 *
 * @param hLed hLed handle of LED that should blink.
 * @param wHalfPrdMs blinking half-period in mili-seconds. (LED_ON or LED_OFF to set a constant state)
 * @param wDoubleBlinkIntervalMs if larger than zero, LED will blink twice within one period. This
 * parameter sets for how long LED will be ON during the two short blinks. wDoubleBlinkIntervalMs should
 * be <= wHalfPrdMs / 4
 */
void LedSetPeriode(LedHandle_t hLed, uint16_t wHalfPrdMs, uint16_t wDoubleBlinkIntervalMs);

/**
 * Blinks LED once, with fixed, pre-defined period. If called
 * when a blink is in progress, the call is ignored.
 *
 * @details If LED was OFF, it would be set ON and then OFF. If it was
 * ON the opposite action is performed.
 *
 * @param hLed handle of LED that should blink.
 */
void LedBlink(LedHandle_t hLed);

#endif /* BSP_LED_H_ */
