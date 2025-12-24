/*
 * bsp_led.h
 *
 *  Created on: May 12, 2025
 *      Author: IlicAleksander
 */
#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

#include "bsp_gpio.h"
#include "bsp_swtimer.h"
#define LED_DBLINK_TOGGLE_CNT       3u
#define LED_UPDATE_PERIOD_50MS      20u
#define LED_TIMER_PERIOD_MS         50u
#define LED_ONE_BLINK_HALF_PRD_50MS 4u
#define LED_ONE_BLINK_TOGGLE_CNT    2u

/**
 * Structure representing a single LED and its runtime state.
 */
typedef struct
{
    uint32_t      ePin;                     /**< GPIO pin associated with the LED */
    bool          bState;                   /**< Current output state */
    uint16_t      wUpdPeriod;               /**< Main blink period (in timer ticks) */
    uint16_t      wUpdPeriodDoubleBlink;    /**< Double-blink interval (in timer ticks) */
    uint16_t      wCnt;                     /**< Counter for main blink period */
    bool          bDoubleBlink;             /**< Double-blink active flag */
    uint16_t      wDoubleBlinkCnt;          /**< Counter for double-blink */
    uint8_t       byDoubleBlinkToggleCnt;   /**< Toggle count for double-blink */
    uint16_t      wUpdPeriodNew;            /**< New main period (pending update) */
    uint16_t      wUpdPeriodDoubleBlinkNew; /**< New double-blink period (pending update) */
    volatile bool bUpdatePending;           /**< Flag indicating a pending period update */
    volatile bool bOneBlink;                /**< One-blink active flag */
    uint16_t      wOneBlinkCnt;             /**< Counter for one-blink */
    uint8_t       byOneBlinkToggleCnt;      /**< Toggle count for one-blink */
} LiveLed_t;
/**
 * Initializes Alive LED control module.
 * @return true on success
 */
bool LedInit(LiveLed_t* const pLed);

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
void LedSetPeriod(LiveLed_t* const pLed, uint16_t wHalfPrdMs, uint16_t wDoubleBlinkIntervalMs);

/**
 * Blinks LED once, with fixed, pre-defined period. If called
 * when a blink is in progress, the call is ignored.
 *
 * @details If LED was OFF, it would be set ON and then OFF. If it was
 * ON the opposite action is performed.
 *
 * @param hLed handle of LED that should blink.
 */
void LedBlink(LiveLed_t* const pLed);

#ifdef __cplusplus
}
#endif
