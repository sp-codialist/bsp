/**
 * @file    bsp_pwm.c
 * @brief   BSP PWM driver implementation for STM32 timer-based PWM generation
 */

#include "bsp_pwm.h"
#include "bsp_compiler_attributes.h"

#include <string.h>

/* Include STM32 HAL headers */
#include "stm32f4xx_hal.h"

/* --- Constants --- */

#define PWM_CCR_CALC_DIV (1000u)

/* Default prescaler values for 1MHz timer tick */
#define PWM_DEFAULT_PRESCALER_APB1 (83u)  /**< 84MHz / (83+1) = 1MHz */
#define PWM_DEFAULT_PRESCALER_APB2 (167u) /**< 168MHz / (167+1) = 1MHz */

/* --- External HAL Timer Handles --- */

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim5;
extern TIM_HandleTypeDef htim8;
extern TIM_HandleTypeDef htim9;
extern TIM_HandleTypeDef htim10;
extern TIM_HandleTypeDef htim11;
extern TIM_HandleTypeDef htim12;
extern TIM_HandleTypeDef htim13;
extern TIM_HandleTypeDef htim14;

/* --- Local Types --- */

/**
 * @brief PWM channel data structure.
 */
typedef struct
{
    TIM_HandleTypeDef* pTimer;         /**< Pointer to HAL timer handle */
    BspPwmTimer_e      eTimer;         /**< Timer instance identifier */
    BspPwmChannel_e    eChannel;       /**< Channel identifier */
    uint16_t           wFrequencyKhz;  /**< Configured frequency in kHz */
    uint16_t           wArr;           /**< Timer auto-reload value (ARR) */
    BspPwmErrorCb_t    pErrorCallback; /**< Per-channel error callback */
    bool               bAllocated;     /**< Allocation status flag */
} BspPwmChannel_t;

/* --- Local Variables --- */

/**
 * @brief Array of PWM channel structures.
 */
FORCE_STATIC BspPwmChannel_t s_aPwmChannels[BSP_PWM_MAX_CHANNELS] = {0};

/**
 * @brief Lookup table mapping timer enum to HAL timer handle pointer.
 */
FORCE_STATIC TIM_HandleTypeDef* const s_apTimerHandles[eBSP_PWM_TIMER_COUNT] = {
    &htim1,  /* eBSP_PWM_TIMER_1 */
    &htim2,  /* eBSP_PWM_TIMER_2 */
    &htim3,  /* eBSP_PWM_TIMER_3 */
    &htim4,  /* eBSP_PWM_TIMER_4 */
    &htim5,  /* eBSP_PWM_TIMER_5 */
    &htim8,  /* eBSP_PWM_TIMER_8 */
    &htim9,  /* eBSP_PWM_TIMER_9 */
    &htim10, /* eBSP_PWM_TIMER_10 */
    &htim11, /* eBSP_PWM_TIMER_11 */
    &htim12, /* eBSP_PWM_TIMER_12 */
    &htim13, /* eBSP_PWM_TIMER_13 */
    &htim14  /* eBSP_PWM_TIMER_14 */
};

/**
 * @brief Lookup table indicating which timers are on APB2 (true) vs APB1 (false).
 *
 * STM32F4: TIM1, TIM8-11 are on APB2, others on APB1.
 */
FORCE_STATIC const bool s_abTimerIsApb2[eBSP_PWM_TIMER_COUNT] = {
    true,  /* eBSP_PWM_TIMER_1 - APB2 */
    false, /* eBSP_PWM_TIMER_2 - APB1 */
    false, /* eBSP_PWM_TIMER_3 - APB1 */
    false, /* eBSP_PWM_TIMER_4 - APB1 */
    false, /* eBSP_PWM_TIMER_5 - APB1 */
    true,  /* eBSP_PWM_TIMER_8 - APB2 */
    true,  /* eBSP_PWM_TIMER_9 - APB2 */
    true,  /* eBSP_PWM_TIMER_10 - APB2 */
    true,  /* eBSP_PWM_TIMER_11 - APB2 */
    false, /* eBSP_PWM_TIMER_12 - APB1 */
    false, /* eBSP_PWM_TIMER_13 - APB1 */
    false  /* eBSP_PWM_TIMER_14 - APB1 */
};

/**
 * @brief Per-timer prescaler values.
 *
 * Initialized with defaults for 1MHz timer tick base.
 */
FORCE_STATIC uint16_t s_awTimerPrescalers[eBSP_PWM_TIMER_COUNT] = {
    PWM_DEFAULT_PRESCALER_APB2, /* eBSP_PWM_TIMER_1 */
    PWM_DEFAULT_PRESCALER_APB1, /* eBSP_PWM_TIMER_2 */
    PWM_DEFAULT_PRESCALER_APB1, /* eBSP_PWM_TIMER_3 */
    PWM_DEFAULT_PRESCALER_APB1, /* eBSP_PWM_TIMER_4 */
    PWM_DEFAULT_PRESCALER_APB1, /* eBSP_PWM_TIMER_5 */
    PWM_DEFAULT_PRESCALER_APB2, /* eBSP_PWM_TIMER_8 */
    PWM_DEFAULT_PRESCALER_APB2, /* eBSP_PWM_TIMER_9 */
    PWM_DEFAULT_PRESCALER_APB2, /* eBSP_PWM_TIMER_10 */
    PWM_DEFAULT_PRESCALER_APB2, /* eBSP_PWM_TIMER_11 */
    PWM_DEFAULT_PRESCALER_APB1, /* eBSP_PWM_TIMER_12 */
    PWM_DEFAULT_PRESCALER_APB1, /* eBSP_PWM_TIMER_13 */
    PWM_DEFAULT_PRESCALER_APB1  /* eBSP_PWM_TIMER_14 */
};

/**
 * @brief Per-timer running state flags.
 *
 * Tracks whether a timer has any active PWM channels.
 */
FORCE_STATIC bool s_abTimerRunning[eBSP_PWM_TIMER_COUNT] = {false};

/* --- Private Function Prototypes --- */

/**
 * @brief Validates a PWM handle and returns pointer to channel structure.
 *
 * @param[in] handle  Handle to validate
 *
 * @return Pointer to channel structure if valid, NULL otherwise
 */
FORCE_STATIC BspPwmChannel_t* sBspPwmValidateHandle(BspPwmHandle_t handle);

/**
 * @brief Converts BSP channel enum to HAL timer channel constant.
 *
 * @param[in] eChannel  BSP channel enum
 *
 * @return HAL timer channel constant (TIM_CHANNEL_1-4) or 0 on error
 */
FORCE_STATIC uint32_t sBspPwmGetHalChannel(BspPwmChannel_e eChannel);

/**
 * @brief Gets the actual timer clock frequency from RCC configuration.
 *
 * @param[in] eTimer  Timer instance
 *
 * @return Timer clock frequency in Hz, or 0 on error
 */
FORCE_STATIC uint32_t sBspPwmGetTimerClock(BspPwmTimer_e eTimer);

/**
 * @brief Calculates ARR value from frequency in kHz.
 *
 * @param[in] eTimer        Timer instance
 * @param[in] wFrequencyKhz Desired frequency in kHz
 *
 * @return Calculated ARR value
 */
FORCE_STATIC uint16_t sBspPwmCalculateArr(BspPwmTimer_e eTimer, uint16_t wFrequencyKhz);

/**
 * @brief Checks for frequency conflicts on the same timer.
 *
 * Since all channels on a timer share the same ARR register, they must
 * operate at the same frequency. This function checks if any allocated
 * channels on the specified timer have a different frequency.
 *
 * @param[in] eTimer        Timer instance
 * @param[in] wFrequencyKhz Frequency to check
 *
 * @return true if conflict detected, false otherwise
 */
FORCE_STATIC bool sBspPwmCheckFrequencyConflict(BspPwmTimer_e eTimer, uint16_t wFrequencyKhz);

/**
 * @brief Checks if a timer has any running channels.
 *
 * @param[in] eTimer  Timer instance
 *
 * @return true if timer has running channels, false otherwise
 */
FORCE_STATIC bool sBspPwmIsTimerRunning(BspPwmTimer_e eTimer);

/**
 * @brief Sets the CCR register for a PWM channel.
 *
 * @param[in] pChannel  Pointer to channel structure
 * @param[in] uCcr      CCR value to set
 */
FORCE_STATIC void sBspPwmSetCcr(const BspPwmChannel_t* pChannel, uint32_t uCcr);

/**
 * @brief Invokes the error callback for a channel if registered.
 *
 * @param[in] handle  Channel handle
 * @param[in] eError  Error code
 */
FORCE_STATIC void sBspPwmCallErrorCallback(BspPwmHandle_t handle, BspPwmError_e eError);

FORCE_STATIC void sBspPwmCallErrorCallback(BspPwmHandle_t handle, BspPwmError_e eError);

/* --- Public Functions --- */

BspPwmHandle_t BspPwmAllocateChannel(BspPwmTimer_e eTimer, BspPwmChannel_e eChannel, uint16_t wFrequencyKhz)
{
    BspPwmHandle_t handle = -1;

    do
    {
        /* Validate parameters */
        if (eTimer >= eBSP_PWM_TIMER_COUNT)
        {
            break;
        }

        if (eChannel >= eBSP_PWM_CHANNEL_COUNT)
        {
            break;
        }

        if (wFrequencyKhz == 0u)
        {
            break;
        }

        /* Find a free channel slot */
        for (uint8_t i = 0u; i < BSP_PWM_MAX_CHANNELS; i++)
        {
            if (!s_aPwmChannels[i].bAllocated)
            {
                /* Calculate ARR from frequency */
                const uint16_t wArr = sBspPwmCalculateArr(eTimer, wFrequencyKhz);

                if (wArr == 0u)
                {
                    break;
                }

                /* Check for frequency conflict on same timer */
                if (sBspPwmCheckFrequencyConflict(eTimer, wFrequencyKhz))
                {
                    /* Issue warning via callback but continue allocation */
                    sBspPwmCallErrorCallback((BspPwmHandle_t)i, eBSP_PWM_ERR_FREQUENCY_CONFLICT);
                }

                /* Allocate the channel */
                s_aPwmChannels[i].pTimer         = s_apTimerHandles[eTimer];
                s_aPwmChannels[i].eTimer         = eTimer;
                s_aPwmChannels[i].eChannel       = eChannel;
                s_aPwmChannels[i].wFrequencyKhz  = wFrequencyKhz;
                s_aPwmChannels[i].wArr           = wArr;
                s_aPwmChannels[i].pErrorCallback = NULL;
                s_aPwmChannels[i].bAllocated     = true;

                /* Configure timer ARR and prescaler */
                TIM_HandleTypeDef* pTimer = s_apTimerHandles[eTimer];
                pTimer->Instance->ARR     = wArr;
                pTimer->Instance->PSC     = s_awTimerPrescalers[eTimer];

                /* Initialize duty cycle to 0% */
                sBspPwmSetCcr(&s_aPwmChannels[i], 0u);

                handle = (BspPwmHandle_t)i;
                break;
            }
        }
    } while (false);

    return handle;
}

BspPwmError_e BspPwmFreeChannel(BspPwmHandle_t handle)
{
    BspPwmError_e eError = eBSP_PWM_ERR_INVALID_HANDLE;

    do
    {
        BspPwmChannel_t* pChannel = sBspPwmValidateHandle(handle);

        if (pChannel == NULL)
        {
            break;
        }

        /* Stop the channel if running */
        (void)BspPwmStop(handle);

        /* Clear the allocation */
        memset(pChannel, 0, sizeof(BspPwmChannel_t));

        eError = eBSP_PWM_ERR_NONE;
    } while (false);

    return eError;
}

BspPwmError_e BspPwmSetPrescaler(BspPwmTimer_e eTimer, uint16_t wPrescaler)
{
    BspPwmError_e eError = eBSP_PWM_ERR_INVALID_PARAM;

    do
    {
        if (eTimer >= eBSP_PWM_TIMER_COUNT)
        {
            break;
        }

        /* Check if timer is running */
        if (sBspPwmIsTimerRunning(eTimer))
        {
            eError = eBSP_PWM_ERR_TIMER_RUNNING;
            break;
        }

        /* Update prescaler */
        s_awTimerPrescalers[eTimer] = wPrescaler;

        /* Recalculate ARR for all channels on this timer */
        for (uint8_t i = 0u; i < BSP_PWM_MAX_CHANNELS; i++)
        {
            if (s_aPwmChannels[i].bAllocated && s_aPwmChannels[i].eTimer == eTimer)
            {
                const uint16_t wArr                     = sBspPwmCalculateArr(eTimer, s_aPwmChannels[i].wFrequencyKhz);
                s_aPwmChannels[i].wArr                  = wArr;
                s_aPwmChannels[i].pTimer->Instance->ARR = wArr;
                s_aPwmChannels[i].pTimer->Instance->PSC = wPrescaler;
            }
        }

        eError = eBSP_PWM_ERR_NONE;
    } while (false);

    return eError;
}

BspPwmError_e BspPwmStart(BspPwmHandle_t handle)
{
    BspPwmError_e eError = eBSP_PWM_ERR_INVALID_HANDLE;

    do
    {
        BspPwmChannel_t* pChannel = sBspPwmValidateHandle(handle);

        if (pChannel == NULL)
        {
            break;
        }

        const uint32_t uHalChannel = sBspPwmGetHalChannel(pChannel->eChannel);

        if (uHalChannel == 0u)
        {
            eError = eBSP_PWM_ERR_INVALID_PARAM;
            break;
        }

        /* Start PWM generation */
        const HAL_StatusTypeDef tStatus = HAL_TIM_PWM_Start(pChannel->pTimer, uHalChannel);

        if (tStatus != HAL_OK)
        {
            eError = eBSP_PWM_ERR_HAL_ERROR;
            sBspPwmCallErrorCallback(handle, eError);
            break;
        }

        /* Mark timer as running */
        s_abTimerRunning[pChannel->eTimer] = true;

        eError = eBSP_PWM_ERR_NONE;
    } while (false);

    return eError;
}

BspPwmError_e BspPwmStartAll(void)
{
    BspPwmError_e eError = eBSP_PWM_ERR_NONE;

    for (uint8_t i = 0u; i < BSP_PWM_MAX_CHANNELS; i++)
    {
        if (s_aPwmChannels[i].bAllocated)
        {
            const BspPwmError_e eStartError = BspPwmStart((BspPwmHandle_t)i);

            if (eStartError != eBSP_PWM_ERR_NONE)
            {
                eError = eStartError;
                /* Continue starting other channels despite error */
            }
        }
    }

    return eError;
}

BspPwmError_e BspPwmStop(BspPwmHandle_t handle)
{
    BspPwmError_e eError = eBSP_PWM_ERR_INVALID_HANDLE;

    do
    {
        BspPwmChannel_t* pChannel = sBspPwmValidateHandle(handle);

        if (pChannel == NULL)
        {
            break;
        }

        const uint32_t uHalChannel = sBspPwmGetHalChannel(pChannel->eChannel);

        if (uHalChannel == 0u)
        {
            eError = eBSP_PWM_ERR_INVALID_PARAM;
            break;
        }

        /* Stop PWM generation */
        const HAL_StatusTypeDef tStatus = HAL_TIM_PWM_Stop(pChannel->pTimer, uHalChannel);

        if (tStatus != HAL_OK)
        {
            eError = eBSP_PWM_ERR_HAL_ERROR;
            sBspPwmCallErrorCallback(handle, eError);
            break;
        }

        /* Update running state if no other channels on this timer are active */
        if (!sBspPwmIsTimerRunning(pChannel->eTimer))
        {
            s_abTimerRunning[pChannel->eTimer] = false;
        }

        eError = eBSP_PWM_ERR_NONE;
    } while (false);

    return eError;
}

BspPwmError_e BspPwmStopAll(void)
{
    BspPwmError_e eError = eBSP_PWM_ERR_NONE;

    for (uint8_t i = 0u; i < BSP_PWM_MAX_CHANNELS; i++)
    {
        if (s_aPwmChannels[i].bAllocated)
        {
            const BspPwmError_e eStopError = BspPwmStop((BspPwmHandle_t)i);

            if (eStopError != eBSP_PWM_ERR_NONE)
            {
                eError = eStopError;
                /* Continue stopping other channels despite error */
            }
        }
    }

    /* Clear all running flags */
    memset(s_abTimerRunning, 0, sizeof(s_abTimerRunning));

    return eError;
}

BspPwmError_e BspPwmSetDutyCycle(BspPwmHandle_t handle, uint16_t wDutyCyclePpt)
{
    BspPwmError_e eError = eBSP_PWM_ERR_INVALID_HANDLE;

    do
    {
        BspPwmChannel_t* pChannel = sBspPwmValidateHandle(handle);

        if (pChannel == NULL)
        {
            break;
        }

        if (wDutyCyclePpt > 1000u)
        {
            eError = eBSP_PWM_ERR_INVALID_PARAM;
            sBspPwmCallErrorCallback(handle, eError);
            break;
        }

        /* Calculate CCR value
         * CCR = DutyCycleRatio * (ARR + 1)
         * ARR+1 because counter starts at 0
         */
        const uint32_t uDutyCyclePpt    = (uint32_t)wDutyCyclePpt;
        const uint32_t uAutoReloadValue = (uint32_t)pChannel->wArr;
        const uint32_t uCcr             = (uDutyCyclePpt * (uAutoReloadValue + 1u)) / PWM_CCR_CALC_DIV;

        sBspPwmSetCcr(pChannel, uCcr);

        eError = eBSP_PWM_ERR_NONE;
    } while (false);

    return eError;
}

BspPwmError_e BspPwmRegisterErrorCallback(BspPwmHandle_t handle, BspPwmErrorCb_t pErrCb)
{
    BspPwmError_e eError = eBSP_PWM_ERR_INVALID_HANDLE;

    do
    {
        BspPwmChannel_t* pChannel = sBspPwmValidateHandle(handle);

        if (pChannel == NULL)
        {
            break;
        }

        pChannel->pErrorCallback = pErrCb;

        eError = eBSP_PWM_ERR_NONE;
    } while (false);

    return eError;
}

/* --- Private Functions --- */

FORCE_STATIC BspPwmChannel_t* sBspPwmValidateHandle(BspPwmHandle_t handle)
{
    if (handle < 0 || handle >= (int8_t)BSP_PWM_MAX_CHANNELS)
    {
        return NULL;
    }

    if (!s_aPwmChannels[handle].bAllocated)
    {
        return NULL;
    }

    return &s_aPwmChannels[handle];
}

FORCE_STATIC uint32_t sBspPwmGetHalChannel(BspPwmChannel_e eChannel)
{
    uint32_t uHalChannel = 0u;

    switch (eChannel)
    {
        case eBSP_PWM_CHANNEL_1:
            uHalChannel = TIM_CHANNEL_1;
            break;
        case eBSP_PWM_CHANNEL_2:
            uHalChannel = TIM_CHANNEL_2;
            break;
        case eBSP_PWM_CHANNEL_3:
            uHalChannel = TIM_CHANNEL_3;
            break;
        case eBSP_PWM_CHANNEL_4:
            uHalChannel = TIM_CHANNEL_4;
            break;
        default:
            uHalChannel = 0u;
            break;
    }

    return uHalChannel;
}

FORCE_STATIC uint32_t sBspPwmGetTimerClock(BspPwmTimer_e eTimer)
{
    uint32_t uTimerClock = 0u;

    do
    {
        if (eTimer >= eBSP_PWM_TIMER_COUNT)
        {
            break;
        }

        /* Get APB clock frequency */
        uint32_t uApbClock;
        if (s_abTimerIsApb2[eTimer])
        {
            uApbClock = HAL_RCC_GetPCLK2Freq();
        }
        else
        {
            uApbClock = HAL_RCC_GetPCLK1Freq();
        }

        /* Get RCC clock configuration to determine APB prescaler */
        RCC_ClkInitTypeDef tClkConfig;
        uint32_t           uFlashLatency;
        HAL_RCC_GetClockConfig(&tClkConfig, &uFlashLatency);

        /* Determine APB prescaler */
        uint32_t uApbPrescaler;
        if (s_abTimerIsApb2[eTimer])
        {
            uApbPrescaler = tClkConfig.APB2CLKDivider;
        }
        else
        {
            uApbPrescaler = tClkConfig.APB1CLKDivider;
        }

        /* Timer clock is 2x APB clock when APB prescaler != 1, else 1x */
        if (uApbPrescaler == RCC_HCLK_DIV1)
        {
            uTimerClock = uApbClock;
        }
        else
        {
            uTimerClock = uApbClock * 2u;
        }
    } while (false);

    return uTimerClock;
}

FORCE_STATIC uint16_t sBspPwmCalculateArr(BspPwmTimer_e eTimer, uint16_t wFrequencyKhz)
{
    uint16_t wArr = 0u;

    do
    {
        if (eTimer >= eBSP_PWM_TIMER_COUNT)
        {
            break;
        }

        if (wFrequencyKhz == 0u)
        {
            break;
        }

        const uint32_t uTimerClock = sBspPwmGetTimerClock(eTimer);

        if (uTimerClock == 0u)
        {
            break;
        }

        const uint16_t wPrescaler = s_awTimerPrescalers[eTimer];

        /* Calculate ARR
         * Timer tick frequency = TimerClock / (Prescaler + 1)
         * PWM frequency = Timer tick frequency / (ARR + 1)
         * Therefore: ARR = (TimerClock / (Prescaler + 1) / FrequencyHz) - 1
         *
         * FrequencyKhz is in kHz, so multiply by 1000 for Hz
         */
        const uint32_t uTimerTickFreq = uTimerClock / ((uint32_t)wPrescaler + 1u);
        const uint32_t uFrequencyHz   = (uint32_t)wFrequencyKhz * 1000u;
        const uint32_t uArrLong       = (uTimerTickFreq / uFrequencyHz);

        if (uArrLong == 0u || uArrLong > 65535u)
        {
            break;
        }

        wArr = (uint16_t)(uArrLong - 1u);
    } while (false);

    return wArr;
}

FORCE_STATIC bool sBspPwmCheckFrequencyConflict(BspPwmTimer_e eTimer, uint16_t wFrequencyKhz)
{
    bool bConflict = false;

    for (uint8_t i = 0u; i < BSP_PWM_MAX_CHANNELS; i++)
    {
        if (s_aPwmChannels[i].bAllocated && s_aPwmChannels[i].eTimer == eTimer && s_aPwmChannels[i].wFrequencyKhz != wFrequencyKhz)
        {
            bConflict = true;
            break;
        }
    }

    return bConflict;
}

FORCE_STATIC bool sBspPwmIsTimerRunning(BspPwmTimer_e eTimer)
{
    bool bRunning = false;

    /* Check if any allocated channels on this timer exist */
    for (uint8_t i = 0u; i < BSP_PWM_MAX_CHANNELS; i++)
    {
        if (s_aPwmChannels[i].bAllocated && s_aPwmChannels[i].eTimer == eTimer)
        {
            /* At least one channel exists on this timer */
            bRunning = true;
            break;
        }
    }

    /* If no channels exist, timer is not running */
    if (!bRunning)
    {
        return false;
    }

    /* Return the tracked running state */
    return (eTimer < eBSP_PWM_TIMER_COUNT) ? s_abTimerRunning[eTimer] : false;
}

FORCE_STATIC void sBspPwmSetCcr(const BspPwmChannel_t* pChannel, uint32_t uCcr)
{
    if (pChannel == NULL)
    {
        return;
    }

    switch (pChannel->eChannel)
    {
        case eBSP_PWM_CHANNEL_1:
            pChannel->pTimer->Instance->CCR1 = uCcr;
            break;
        case eBSP_PWM_CHANNEL_2:
            pChannel->pTimer->Instance->CCR2 = uCcr;
            break;
        case eBSP_PWM_CHANNEL_3:
            pChannel->pTimer->Instance->CCR3 = uCcr;
            break;
        case eBSP_PWM_CHANNEL_4:
            pChannel->pTimer->Instance->CCR4 = uCcr;
            break;
        default:
            /* Invalid channel, invoke error callback if available */
            break;
    }
}

FORCE_STATIC void sBspPwmCallErrorCallback(BspPwmHandle_t handle, BspPwmError_e eError)
{
    BspPwmChannel_t* pChannel = sBspPwmValidateHandle(handle);

    if (pChannel != NULL && pChannel->pErrorCallback != NULL)
    {
        pChannel->pErrorCallback(handle, eError);
    }
}
