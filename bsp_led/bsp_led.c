/*
 * bsp_led.c
 *
 *  Created on: May 12, 2025
 *      Author: IlicAleksander
 */
#include <stddef.h>

#include "bsp_led.h"
#include "stm32f4xx_hal.h"

#define LED_MAX_COUNT 16u
#define LED_ON        0xFFFFu
#define LED_OFF       0u

// --- Module Variables ---

static LiveLed_t*    s_pRegisteredLeds[LED_MAX_COUNT] = {NULL};
static uint8_t       s_byRegisteredCount              = 0u;
static SWTimerModule s_timer;
static bool          s_bTimerInitialized = false;

// --- Private Function Prototypes ---

static void ProcessAllLeds(void);
static void ProcessLedBlink(LiveLed_t* const pLed);
static void ProcessLedOneBlink(LiveLed_t* const pLed);
static void ProcessLedDoubleBlink(LiveLed_t* const pLed);
static void ApplyPendingUpdate(LiveLed_t* const pLed);

// --- Public API Implementation ---

bool LedInit(LiveLed_t* const pLed)
{
    bool result = false;

    do
    {
        if (pLed == NULL)
        {
            break;
        }

        // Check if already registered
        for (uint8_t i = 0u; i < s_byRegisteredCount; i++)
        {
            if (s_pRegisteredLeds[i] == pLed)
            {
                result = true;
                break;
            }
        }

        if (result)
        {
            break;
        }

        // Check capacity
        if (s_byRegisteredCount >= LED_MAX_COUNT)
        {
            break;
        }

        // Initialize LED fields
        pLed->bState                   = false;
        pLed->wUpdPeriod               = LED_OFF;
        pLed->wUpdPeriodDoubleBlink    = 0u;
        pLed->wCnt                     = 0u;
        pLed->bDoubleBlink             = false;
        pLed->wDoubleBlinkCnt          = 0u;
        pLed->byDoubleBlinkToggleCnt   = 0u;
        pLed->wUpdPeriodNew            = 0u;
        pLed->wUpdPeriodDoubleBlinkNew = 0u;
        pLed->bUpdatePending           = false;
        pLed->bOneBlink                = false;
        pLed->wOneBlinkCnt             = 0u;
        pLed->byOneBlinkToggleCnt      = 0u;

        // Register LED
        s_pRegisteredLeds[s_byRegisteredCount] = pLed;
        s_byRegisteredCount++;

        // Initialize timer on first registration
        if (!s_bTimerInitialized)
        {
            s_timer.interval          = LED_TIMER_PERIOD_MS;
            s_timer.pCallbackFunction = &ProcessAllLeds;
            s_timer.periodic          = true;
            s_timer.active            = false;

            result = SWTimerInit(&s_timer);
            if (result)
            {
                s_bTimerInitialized = true;
            }
        }
        else
        {
            result = true;
        }
    } while (false);

    return result;
}

void LedStart(void)
{
    do
    {
        if (!s_bTimerInitialized)
        {
            break;
        }

        SWTimerStart(&s_timer);
    } while (false);
}

void LedSetPeriod(LiveLed_t* const pLed, uint16_t wHalfPrdMs, uint16_t wDoubleBlinkIntervalMs)
{
    do
    {
        if (pLed == NULL)
        {
            break;
        }

        // Handle constant on/off states
        if ((wHalfPrdMs == LED_ON) || (wHalfPrdMs == LED_OFF))
        {
            pLed->wUpdPeriod            = wHalfPrdMs;
            pLed->wUpdPeriodDoubleBlink = 0u;
            BspGpioWritePin(pLed->ePin, (wHalfPrdMs == LED_ON));
            break;
        }

        // Prepare new period values (convert ms to timer ticks)
        pLed->wUpdPeriodNew            = wHalfPrdMs / LED_TIMER_PERIOD_MS;
        pLed->wUpdPeriodDoubleBlinkNew = wDoubleBlinkIntervalMs / LED_TIMER_PERIOD_MS;

        // Memory barrier to ensure all values written before flag set
        __DMB();
        pLed->bUpdatePending = true;
    } while (false);
}

void LedBlink(LiveLed_t* const pLed)
{
    do
    {
        if (pLed == NULL)
        {
            break;
        }

        if (pLed->bOneBlink)
        {
            break; // Already blinking
        }

        BspGpioTogglePin(pLed->ePin);
        pLed->wOneBlinkCnt        = 0u;
        pLed->byOneBlinkToggleCnt = 1u;
        pLed->bOneBlink           = true;
    } while (false);
}

// --- Private Function Implementations ---

static void ProcessAllLeds(void)
{
    static uint16_t updateCounter = 0u;
    bool            bApplyUpdates = false;

    updateCounter++;
    if (updateCounter >= LED_UPDATE_PERIOD_50MS)
    {
        bApplyUpdates = true;
        updateCounter = 0u;
    }

    for (uint8_t i = 0u; i < s_byRegisteredCount; i++)
    {
        LiveLed_t* const pLed = s_pRegisteredLeds[i];
        if (pLed != NULL)
        {
            ProcessLedBlink(pLed);
            ProcessLedOneBlink(pLed);
            ProcessLedDoubleBlink(pLed);

            if (bApplyUpdates)
            {
                ApplyPendingUpdate(pLed);
            }
        }
    }
}

static void ProcessLedBlink(LiveLed_t* const pLed)
{
    do
    {
        if ((pLed->wUpdPeriod == LED_ON) || (pLed->wUpdPeriod == LED_OFF))
        {
            break;
        }

        pLed->wCnt++;

        if (pLed->wCnt < pLed->wUpdPeriod)
        {
            break;
        }

        // Toggle LED state
        pLed->bState = !pLed->bState;
        BspGpioWritePin(pLed->ePin, pLed->bState);
        pLed->wCnt = 0u;

        // Activate double-blink if configured and LED just turned on
        if ((pLed->wUpdPeriodDoubleBlink > 0u) && pLed->bState)
        {
            pLed->bDoubleBlink           = true;
            pLed->wDoubleBlinkCnt        = 0u;
            pLed->byDoubleBlinkToggleCnt = 0u;
        }
        else
        {
            pLed->bDoubleBlink = false;
        }
    } while (false);
}

static void ProcessLedOneBlink(LiveLed_t* const pLed)
{
    do
    {
        if (!pLed->bOneBlink)
        {
            break;
        }

        pLed->wOneBlinkCnt++;

        if (pLed->wOneBlinkCnt < LED_ONE_BLINK_HALF_PRD_50MS)
        {
            break;
        }

        pLed->wOneBlinkCnt = 0u;

        if (pLed->byOneBlinkToggleCnt >= LED_ONE_BLINK_TOGGLE_CNT)
        {
            pLed->byOneBlinkToggleCnt = 0u;
            pLed->bOneBlink           = false;
            break;
        }

        pLed->byOneBlinkToggleCnt++;
        BspGpioTogglePin(pLed->ePin);
    } while (false);
}

static void ProcessLedDoubleBlink(LiveLed_t* const pLed)
{
    do
    {
        if (!pLed->bDoubleBlink)
        {
            break;
        }

        pLed->wDoubleBlinkCnt++;

        if (pLed->wDoubleBlinkCnt < pLed->wUpdPeriodDoubleBlink)
        {
            break;
        }

        BspGpioTogglePin(pLed->ePin);
        pLed->wDoubleBlinkCnt = 0u;
        pLed->byDoubleBlinkToggleCnt++;

        if (pLed->byDoubleBlinkToggleCnt >= LED_DBLINK_TOGGLE_CNT)
        {
            pLed->bDoubleBlink = false;
        }
    } while (false);
}

static void ApplyPendingUpdate(LiveLed_t* const pLed)
{
    do
    {
        if (!pLed->bUpdatePending)
        {
            break;
        }

        pLed->wUpdPeriod             = pLed->wUpdPeriodNew;
        pLed->wUpdPeriodDoubleBlink  = pLed->wUpdPeriodDoubleBlinkNew;
        pLed->wCnt                   = 0u;
        pLed->wDoubleBlinkCnt        = 0u;
        pLed->bDoubleBlink           = false;
        pLed->byDoubleBlinkToggleCnt = 0u;
        pLed->bUpdatePending         = false;

        // Apply constant state if configured
        if ((pLed->wUpdPeriod == LED_ON) || (pLed->wUpdPeriod == LED_OFF))
        {
            BspGpioWritePin(pLed->ePin, (pLed->wUpdPeriod == LED_ON));
        }
    } while (false);
}
