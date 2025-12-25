/*
 * bsp_adc.c
 *
 *  Created on: Dec 25, 2025
 *      Author: GitHub Copilot
 */

#include "bsp_adc.h"
#include "bsp_swtimer.h"
#include "stm32f4xx_hal.h"

#include <stddef.h>

// --- Local Variables ---

static ADC_HandleTypeDef* s_pAdc                                 = NULL;
static uint32_t           s_uAdcChannelCnt                       = 0u;
static uint32_t           s_uAdcMaxChannelCnt                    = 0u;
static BspAdcValueCb_t    s_aAdcCallbacks[BSP_ADC_MAX_CHANNELS]  = {NULL};
static uint32_t           s_aAdcResultData[BSP_ADC_MAX_CHANNELS] = {0};
static SWTimerModule      s_adcTimer;
static BspAdcErrorCb_t    s_pAdcErrorCallback = NULL;
static bool               s_bTimerInitialized = false;

// --- Local Function Prototypes ---

/**
 * @brief Converts ADC channel enum to STM32 HAL ADC channel value.
 * @param eChannel BSP ADC channel enum
 * @param pStmChannel Pointer to store STM32 HAL channel value
 * @return true if conversion successful, false otherwise
 */
static bool BspAdcGetStmChannelValue(BspAdcChannel_e eChannel, uint32_t* pStmChannel);

/**
 * @brief Converts ADC sample time enum to STM32 HAL sample time value.
 * @param eSampleTime BSP ADC sample time enum
 * @param pStmSampleTime Pointer to store STM32 HAL sample time value
 * @return true if conversion successful, false otherwise
 */
static bool BspAdcGetStmSampleTimeValue(BspAdcSampleTime_e eSampleTime, uint32_t* pStmSampleTime);

/**
 * @brief Timer callback to trigger ADC DMA conversion.
 */
static void BspAdcTimerCallback(void);

/**
 * @brief Starts ADC DMA conversion.
 */
static void BspAdcStartReadDma(void);

// --- Public API Implementation ---

bool BspAdcInit(void* pAdc)
{
    bool result = false;

    do
    {
        if (pAdc == NULL)
        {
            break;
        }

        s_pAdc              = (ADC_HandleTypeDef*)pAdc;
        s_uAdcMaxChannelCnt = s_pAdc->Init.NbrOfConversion;

        // Initialize timer
        s_adcTimer.interval          = 100u; // Default, will be overridden by BspAdcStart()
        s_adcTimer.pCallbackFunction = &BspAdcTimerCallback;
        s_adcTimer.periodic          = true;
        s_adcTimer.active            = false;

        s_bTimerInitialized = SWTimerInit(&s_adcTimer);
        result              = s_bTimerInitialized;

    } while (false);

    return result;
}

bool BspAdcRegisterChannel(BspAdcChannel_e eChannel, BspAdcSampleTime_e eSampleTime, BspAdcValueCb_t pValueCallback)
{
    bool result = false;

    do
    {
        if (s_uAdcChannelCnt >= s_uAdcMaxChannelCnt)
        {
            break;
        }

        if (s_uAdcChannelCnt >= BSP_ADC_MAX_CHANNELS)
        {
            break;
        }

        s_aAdcCallbacks[s_uAdcChannelCnt] = pValueCallback;
        s_uAdcChannelCnt++;

        uint32_t uStmChannel    = 0u;
        uint32_t uStmSampleTime = 0u;

        result = BspAdcGetStmChannelValue(eChannel, &uStmChannel);
        result = BspAdcGetStmSampleTimeValue(eSampleTime, &uStmSampleTime) && result;

        if (result)
        {
            ADC_ChannelConfTypeDef sConfig = {0};
            sConfig.Channel                = uStmChannel;
            sConfig.Rank                   = s_uAdcChannelCnt;
            sConfig.SamplingTime           = uStmSampleTime;
            sConfig.Offset                 = 0u;

            HAL_StatusTypeDef status = HAL_ADC_ConfigChannel(s_pAdc, &sConfig);
            result                   = (status == HAL_OK);
        }

    } while (false);

    return result;
}

void BspAdcStart(uint32_t uPeriodMs)
{
    do
    {
        if (!s_bTimerInitialized)
        {
            break;
        }

        if (s_uAdcChannelCnt != s_uAdcMaxChannelCnt)
        {
            // Not all channels configured in HAL were registered
            if (s_pAdcErrorCallback != NULL)
            {
                s_pAdcErrorCallback(eBSP_ADC_ERR_CONFIGURATION);
            }
            break;
        }

        s_adcTimer.interval = uPeriodMs;
        SWTimerStart(&s_adcTimer);

    } while (false);
}

void BspAdcStop(void)
{
    SWTimerStop(&s_adcTimer);
}

void BspAdcRegisterErrorCallback(BspAdcErrorCb_t pErrCb)
{
    s_pAdcErrorCallback = pErrCb;
}

// --- Local Function Implementation ---

static void BspAdcTimerCallback(void)
{
    BspAdcStartReadDma();
}

static void BspAdcStartReadDma(void)
{
    do
    {
        if (s_pAdc == NULL)
        {
            break;
        }

        HAL_StatusTypeDef status = HAL_ADC_Start_DMA(s_pAdc, s_aAdcResultData, s_uAdcChannelCnt);

        if (status != HAL_OK)
        {
            if (s_pAdcErrorCallback != NULL)
            {
                s_pAdcErrorCallback(eBSP_ADC_ERR_CONVERSION);
            }
        }

    } while (false);
}

static bool BspAdcGetStmChannelValue(BspAdcChannel_e eChannel, uint32_t* pStmChannel)
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

static bool BspAdcGetStmSampleTimeValue(BspAdcSampleTime_e eSampleTime, uint32_t* pStmSampleTime)
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
    for (uint8_t i = 0u; i < s_uAdcChannelCnt; i++)
    {
        if (s_aAdcCallbacks[i] != NULL)
        {
            s_aAdcCallbacks[i]((uint16_t)s_aAdcResultData[i]);
        }
    }
}
