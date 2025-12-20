/*
 * bsp_led.c
 *
 *  Created on: May 12, 2025
 *      Author: IlicAleksander
 */

#include "cpu_specific.h"
#include "timer.h"
#include "bsp_gpio.h"
#include "bsp_led.h"

#define LED_CNT                     4u
#define LED_DBLINK_TOGGLE_CNT       3u
#define LED_UPDATE_PERIOD_50MS      20u
#define LED_TIMER_PERIOD_MS         50u
#define LED_ONE_BLINK_HALF_PRD_50MS 4u
#define LED_ONE_BLINK_TOGGLE_CNT    2u

// --- Local Types ---

/**
 * Structure representing a single LED and its runtime state.
 */
typedef struct
{
    GpioPort_e ePin;                   /**< GPIO pin associated with the LED */
    bool bState;                       /**< Current output state */
    uint16_t wUpdPeriod;               /**< Main blink period (in timer ticks) */
    uint16_t wUpdPeriodDoubleBlink;    /**< Double-blink interval (in timer ticks) */
    uint16_t wCnt;                     /**< Counter for main blink period */
    bool bDoubleBlink;                 /**< Double-blink active flag */
    uint16_t wDoubleBlinkCnt;          /**< Counter for double-blink */
    uint8_t byDoubleBlinkToggleCnt;    /**< Toggle count for double-blink */
    uint16_t wUpdPeriodNew;            /**< New main period (pending update) */
    uint16_t wUpdPeriodDoubleBlinkNew; /**< New double-blink period (pending update) */
    volatile bool bUpdatePending;      /**< Flag indicating a pending period update */
    volatile bool bOneBlink;           /**< One-blink active flag */
    uint16_t wOneBlinkCnt;             /**< Counter for one-blink */
    uint8_t byOneBlinkToggleCnt;       /**< Toggle count for one-blink */
} LiveLed_t;

// --- Local Variables ---

static LiveLed_t s_aLeds[LED_CNT];
static bool s_bInitialized = false;
static TimerModuleType s_hLiveLedTimer;
static uint16_t s_wUpdateCnt = 0u;

// --- Prototypes ---

/**
 * Timer callback for periodic LED updates.
 */
static void sLedTimerTick(void);

/**
 * Updates the double-blink and one-blink state for a given LED.
 *
 * @param pLed Pointer to the LED structure to update
 */
static void sLedUpdateDoubleBlink(LiveLed_t *pLed);

/**
 * Updates the regular blink state for a given LED.
 *
 * @param pLed Pointer to the LED structure to update
 */
static void sLedUpdateState(LiveLed_t *pLed);

/**
 * Applies any pending period updates for a given LED.
 *
 * @param pLed Pointer to the LED structure to update
 */
static void sLedUpdatePeriode(LiveLed_t *pLed);

// --- Public functions ---

bool LedInit(void)
{
    bool bSucc = true;
    if(!s_bInitialized)
    {
        for(uint8_t byI = 0u; byI < LED_CNT; byI++)
        {
            s_aLeds[byI].bState = true;
        }
        bSucc = TimerRegisterModule(&s_hLiveLedTimer, &sLedTimerTick);
        if(bSucc)
        {
            s_bInitialized = true;
        }
    }
    return bSucc;
}

bool LedRegister(GpioPort_e ePin, LedHandle_t *pHandle)
{
    bool bSucc = false;
    static uint8_t s_byLedCnt = 0u;
    if(s_byLedCnt < LED_CNT)
    {
        s_aLeds[s_byLedCnt].ePin = ePin;
        *pHandle = s_byLedCnt;
        s_byLedCnt++;
        bSucc = true;
    }
    else
    {
        *pHandle = LED_INVL_HANDLE;
    }
    return bSucc;
}

void LedStart(void)
{
    if(s_bInitialized)
    {
        s_wUpdateCnt = LED_UPDATE_PERIOD_50MS;
        TimerStart(s_hLiveLedTimer, LED_TIMER_PERIOD_MS, TIMER_PERIODIC);
    }
}

void LedSetPeriode(LedHandle_t hLed, uint16_t wHalfPrdMs, uint16_t wDoubleBlinkIntervalMs)
{
    if(hLed < LED_CNT)
    {
        LiveLed_t *pLed = &s_aLeds[hLed];
        if((wHalfPrdMs == LED_ON) || (wHalfPrdMs == LED_OFF))
        {
            pLed->wUpdPeriod = wHalfPrdMs;
            pLed->wUpdPeriodDoubleBlink = 0u;
            const bool bIsLedOn = (wHalfPrdMs == LED_ON);
            BspGpioWritePin(pLed->ePin, bIsLedOn);
        }
        else
        {
            pLed->wUpdPeriodNew = wHalfPrdMs / LED_TIMER_PERIOD_MS;
            pLed->wUpdPeriodDoubleBlinkNew = wDoubleBlinkIntervalMs / LED_TIMER_PERIOD_MS;
            // Ensures that update is done in IRQ after all values are set
            __DMB();
            pLed->bUpdatePending = true;
        }
    }
}

void LedBlink(LedHandle_t hLed)
{
    if(hLed < LED_CNT)
    {
        LiveLed_t *pLed = &s_aLeds[hLed];
        const bool bOneBlink = pLed->bOneBlink;
        if(!bOneBlink)
        {
            BspGpioTogglePin(pLed->ePin);
            pLed->wOneBlinkCnt = 0u;
            pLed->byOneBlinkToggleCnt = 1u;
            pLed->bOneBlink = true;
        }
    }
}

// --- Local Functions ---

static void sLedUpdateDoubleBlink(LiveLed_t *pLed)
{
    const bool bOneBlink = pLed->bOneBlink;
    if(bOneBlink)
    {
        pLed->wOneBlinkCnt++;
        if(pLed->wOneBlinkCnt == LED_ONE_BLINK_HALF_PRD_50MS)
        {
            pLed->wOneBlinkCnt = 0u;
            if(pLed->byOneBlinkToggleCnt == LED_ONE_BLINK_TOGGLE_CNT)
            {
                pLed->byOneBlinkToggleCnt = 0u;
                pLed->bOneBlink = false;
            }
            else
            {
                pLed->byOneBlinkToggleCnt++;
                BspGpioTogglePin(pLed->ePin);
            }
        }
    }
    else if(pLed->bDoubleBlink)
    {
        pLed->wDoubleBlinkCnt++;
        if(pLed->wDoubleBlinkCnt == pLed->wUpdPeriodDoubleBlink)
        {
            BspGpioTogglePin(pLed->ePin);
            pLed->byDoubleBlinkToggleCnt++;
            if(pLed->byDoubleBlinkToggleCnt == LED_DBLINK_TOGGLE_CNT)
            {
                pLed->bDoubleBlink = false;
            }
            pLed->wDoubleBlinkCnt = 0u;
        }
    }
    else
    {
        // nothing to do
    }
}

static void sLedUpdateState(LiveLed_t *pLed)
{
    if((pLed->wUpdPeriod != LED_ON) && (pLed->wUpdPeriod != LED_OFF))
    {
        pLed->wCnt++;
        if(pLed->wCnt >= pLed->wUpdPeriod)
        {
            pLed->bState = !pLed->bState;
            BspGpioWritePin(pLed->ePin, pLed->bState);
            pLed->wCnt = 0u;
            // If LED transitioned Low-> High and double blink configure
            // set double blink flag
            if((pLed->wUpdPeriodDoubleBlink > 0u) && pLed->bState)
            {
                pLed->bDoubleBlink = true;
                // wDoubleBlinkCnt will overflow to 0 in the same IRQ
                pLed->wDoubleBlinkCnt = UINT16_MAX;
                pLed->byDoubleBlinkToggleCnt = 0;
            }
            else
            {
                // this line is reached when wUpdPeriodBlink >= wUpdPeriod / 4
                pLed->bDoubleBlink = false;
            }
        }
    }
}

static void sLedUpdatePeriode(LiveLed_t *pLed)
{
    if(pLed->bUpdatePending)
    {
        pLed->wUpdPeriod = pLed->wUpdPeriodNew;
        pLed->wUpdPeriodDoubleBlink = pLed->wUpdPeriodDoubleBlinkNew;
        pLed->wCnt = 0u;
        pLed->wDoubleBlinkCnt = 0u;
        pLed->bDoubleBlink = false;
        pLed->byDoubleBlinkToggleCnt = 0u;
        pLed->bUpdatePending = false;
        // Ensure LED is in correct state if blinking is off
        if((pLed->wUpdPeriod == LED_ON) || (pLed->wUpdPeriod == LED_OFF))
        {
            const bool bIsLedOn = (pLed->wUpdPeriod == LED_ON);
            BspGpioWritePin(pLed->ePin, bIsLedOn);
        }
    }
}

// --- Callback Functions ---

static void sLedTimerTick(void)
{
    bool bUpdateValues = false;
    s_wUpdateCnt++;
    if(s_wUpdateCnt >= LED_UPDATE_PERIOD_50MS)
    {
        bUpdateValues = true;
        s_wUpdateCnt = 0u;
    }
    for(uint8_t byI = 0u; byI < LED_CNT; byI++)
    {
        LiveLed_t *pLed = &s_aLeds[byI];
        sLedUpdateState(pLed);
        sLedUpdateDoubleBlink(pLed);
        if(bUpdateValues)
        {
            sLedUpdatePeriode(pLed);
        }
    }
}
