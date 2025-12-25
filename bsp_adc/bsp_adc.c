/*
 * bsp_adc.c
 *
 *  Created on: Dec 25, 2025
 *      Author: GitHub Copilot
 */

#include "bsp_adc.h"
#include "bsp_compiler_attributes.h"
#include "bsp_swtimer.h"
#include "stm32f4xx_hal.h"

#include <stddef.h>

// --- External HAL Handle Declarations ---

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;

// --- Local Types ---

/**
 * @brief ADC module structure to hold all ADC-related data
 */
typedef struct
{
    ADC_HandleTypeDef* pAdcHandle;                        /**< Pointer to HAL ADC handle */
    BspAdcValueCb_t    aCallbacks[BSP_ADC_MAX_CHANNELS];  /**< Callback functions for each channel */
    uint32_t           aResultData[BSP_ADC_MAX_CHANNELS]; /**< ADC conversion results */
    SWTimerModule      timer;                             /**< Software timer for periodic sampling */
    BspAdcErrorCb_t    pErrorCallback;                    /**< Error callback */
    uint32_t           uChannelCount;                     /**< Number of registered channels */
    uint32_t           uMaxChannelCount;                  /**< Maximum channels from HAL config */
    bool               bTimerInitialized;                 /**< Timer initialization flag */
} BspAdcModule_t;

// --- Local Variables ---

FORCE_STATIC BspAdcModule_t s_adcModule = {0};

// --- Local Function Prototypes ---

/**
 * @brief Converts ADC channel enum to STM32 HAL ADC channel value.
 * @param eChannel BSP ADC channel enum
 * @param pStmChannel Pointer to store STM32 HAL channel value
 * @return true if conversion successful, false otherwise
 */
FORCE_STATIC bool BspAdcGetStmChannelValue(BspAdcChannel_e eChannel, uint32_t* pStmChannel);

/**
 * @brief Converts ADC sample time enum to STM32 HAL sample time value.
 * @param eSampleTime BSP ADC sample time enum
 * @param pStmSampleTime Pointer to store STM32 HAL sample time value
 * @return true if conversion successful, false otherwise
 */
FORCE_STATIC bool BspAdcGetStmSampleTimeValue(BspAdcSampleTime_e eSampleTime, uint32_t* pStmSampleTime);

/**
 * @brief Timer callback to trigger ADC DMA conversion.
 */
FORCE_STATIC void BspAdcTimerCallback(void);

/**
 * @brief Starts ADC DMA conversion.
 */
FORCE_STATIC void BspAdcStartReadDma(void);

// --- Public API Implementation ---

bool BspAdcInit(BspAdcInstance_e eAdcInstance)
{
    bool result = false;

    do
    {
        // Map ADC instance enum to HAL handle
        FORCE_STATIC ADC_HandleTypeDef* const aAdcHandles[eBSP_ADC_INSTANCE_COUNT] = {
            [eBSP_ADC_INSTANCE_1] = &hadc1, [eBSP_ADC_INSTANCE_2] = &hadc2, [eBSP_ADC_INSTANCE_3] = &hadc3};

        if (eAdcInstance >= eBSP_ADC_INSTANCE_COUNT)
        {
            break;
        }

        s_adcModule.pAdcHandle = aAdcHandles[eAdcInstance];
        if (s_adcModule.pAdcHandle == NULL)
        {
            break;
        }

        s_adcModule.uMaxChannelCount = s_adcModule.pAdcHandle->Init.NbrOfConversion;

        // Initialize timer
        s_adcModule.timer.interval          = 100u; // Default, will be overridden by BspAdcStart()
        s_adcModule.timer.pCallbackFunction = &BspAdcTimerCallback;
        s_adcModule.timer.periodic          = true;
        s_adcModule.timer.active            = false;

        s_adcModule.bTimerInitialized = SWTimerInit(&s_adcModule.timer);
        result                        = s_adcModule.bTimerInitialized;

    } while (false);

    return result;
}

bool BspAdcRegisterChannel(BspAdcChannel_e eChannel, BspAdcSampleTime_e eSampleTime, BspAdcValueCb_t pValueCallback)
{
    bool result = false;

    do
    {
        if (s_adcModule.uChannelCount >= s_adcModule.uMaxChannelCount)
        {
            break;
        }

        if (s_adcModule.uChannelCount >= BSP_ADC_MAX_CHANNELS)
        {
            break;
        }

        s_adcModule.aCallbacks[s_adcModule.uChannelCount] = pValueCallback;
        s_adcModule.uChannelCount++;

        uint32_t uStmChannel    = 0u;
        uint32_t uStmSampleTime = 0u;

        result = BspAdcGetStmChannelValue(eChannel, &uStmChannel);
        result = BspAdcGetStmSampleTimeValue(eSampleTime, &uStmSampleTime) && result;

        if (result)
        {
            ADC_ChannelConfTypeDef sConfig = {0};
            sConfig.Channel                = uStmChannel;
            sConfig.Rank                   = s_adcModule.uChannelCount;
            sConfig.SamplingTime           = uStmSampleTime;
            sConfig.Offset                 = 0u;

            HAL_StatusTypeDef status = HAL_ADC_ConfigChannel(s_adcModule.pAdcHandle, &sConfig);
            result                   = (status == HAL_OK);
        }

    } while (false);

    return result;
}

void BspAdcStart(uint32_t uPeriodMs)
{
    do
    {
        if (!s_adcModule.bTimerInitialized)
        {
            break;
        }

        if (s_adcModule.uChannelCount != s_adcModule.uMaxChannelCount)
        {
            // Not all channels configured in HAL were registered
            if (s_adcModule.pErrorCallback != NULL)
            {
                s_adcModule.pErrorCallback(eBSP_ADC_ERR_CONFIGURATION);
            }
            break;
        }

        s_adcModule.timer.interval = uPeriodMs;
        SWTimerStart(&s_adcModule.timer);

    } while (false);
}

void BspAdcStop(void)
{
    SWTimerStop(&s_adcModule.timer);
}

void BspAdcRegisterErrorCallback(BspAdcErrorCb_t pErrCb)
{
    s_adcModule.pErrorCallback = pErrCb;
}

// --- Local Function Implementation ---

FORCE_STATIC void BspAdcTimerCallback(void)
{
    BspAdcStartReadDma();
}

FORCE_STATIC void BspAdcStartReadDma(void)
{
    do
    {
        if (s_adcModule.pAdcHandle == NULL)
        {
            break;
        }

        HAL_StatusTypeDef status = HAL_ADC_Start_DMA(s_adcModule.pAdcHandle, s_adcModule.aResultData, s_adcModule.uChannelCount);

        if (status != HAL_OK)
        {
            if (s_adcModule.pErrorCallback != NULL)
            {
                s_adcModule.pErrorCallback(eBSP_ADC_ERR_CONVERSION);
            }
        }

    } while (false);
}

FORCE_STATIC bool BspAdcGetStmChannelValue(BspAdcChannel_e eChannel, uint32_t* pStmChannel)
{
    bool result = true;

    do
    {
        if (pStmChannel == NULL)
        {
            result = false;
            break;
        }

        switch (eChannel)
        {
            case eBSP_ADC_CHANNEL_0:
                *pStmChannel = ADC_CHANNEL_0;
                break;
            case eBSP_ADC_CHANNEL_1:
                *pStmChannel = ADC_CHANNEL_1;
                break;
            case eBSP_ADC_CHANNEL_2:
                *pStmChannel = ADC_CHANNEL_2;
                break;
            case eBSP_ADC_CHANNEL_3:
                *pStmChannel = ADC_CHANNEL_3;
                break;
            case eBSP_ADC_CHANNEL_4:
                *pStmChannel = ADC_CHANNEL_4;
                break;
            case eBSP_ADC_CHANNEL_5:
                *pStmChannel = ADC_CHANNEL_5;
                break;
            case eBSP_ADC_CHANNEL_6:
                *pStmChannel = ADC_CHANNEL_6;
                break;
            case eBSP_ADC_CHANNEL_7:
                *pStmChannel = ADC_CHANNEL_7;
                break;
            case eBSP_ADC_CHANNEL_8:
                *pStmChannel = ADC_CHANNEL_8;
                break;
            case eBSP_ADC_CHANNEL_9:
                *pStmChannel = ADC_CHANNEL_9;
                break;
            case eBSP_ADC_CHANNEL_10:
                *pStmChannel = ADC_CHANNEL_10;
                break;
            case eBSP_ADC_CHANNEL_11:
                *pStmChannel = ADC_CHANNEL_11;
                break;
            case eBSP_ADC_CHANNEL_12:
                *pStmChannel = ADC_CHANNEL_12;
                break;
            case eBSP_ADC_CHANNEL_13:
                *pStmChannel = ADC_CHANNEL_13;
                break;
            case eBSP_ADC_CHANNEL_14:
                *pStmChannel = ADC_CHANNEL_14;
                break;
            case eBSP_ADC_CHANNEL_15:
                *pStmChannel = ADC_CHANNEL_15;
                break;
            default:
                result = false;
                break;
        }

    } while (false);

    return result;
}

FORCE_STATIC bool BspAdcGetStmSampleTimeValue(BspAdcSampleTime_e eSampleTime, uint32_t* pStmSampleTime)
{
    bool result = true;

    do
    {
        if (pStmSampleTime == NULL)
        {
            result = false;
            break;
        }

        switch (eSampleTime)
        {
            case eBSP_ADC_SampleTime_3Cycles:
                *pStmSampleTime = ADC_SAMPLETIME_3CYCLES;
                break;
            case eBSP_ADC_SampleTime_15Cycles:
                *pStmSampleTime = ADC_SAMPLETIME_15CYCLES;
                break;
            case eBSP_ADC_SampleTime_28Cycles:
                *pStmSampleTime = ADC_SAMPLETIME_28CYCLES;
                break;
            case eBSP_ADC_SampleTime_56Cycles:
                *pStmSampleTime = ADC_SAMPLETIME_56CYCLES;
                break;
            case eBSP_ADC_SampleTime_84Cycles:
                *pStmSampleTime = ADC_SAMPLETIME_84CYCLES;
                break;
            case eBSP_ADC_SampleTime_112Cycles:
                *pStmSampleTime = ADC_SAMPLETIME_112CYCLES;
                break;
            case eBSP_ADC_SampleTime_144Cycles:
                *pStmSampleTime = ADC_SAMPLETIME_144CYCLES;
                break;
            case eBSP_ADC_SampleTime_480Cycles:
                *pStmSampleTime = ADC_SAMPLETIME_480CYCLES;
                break;
            default:
                result = false;
                break;
        }

    } while (false);

    return result;
}

// --- HAL Callback Implementation ---

/**
 * @brief HAL ADC conversion complete callback.
 * Called from ADC IRQ when DMA transfer completes.
 * @param hadc ADC handle
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    (void)hadc;

    // Deliver results to user callbacks
    for (uint8_t i = 0u; i < s_adcModule.uChannelCount; i++)
    {
        if (s_adcModule.aCallbacks[i] != NULL)
        {
            s_adcModule.aCallbacks[i]((uint16_t)s_adcModule.aResultData[i]);
        }
    }
}
