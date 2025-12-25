/*
 * bsp_adc.h
 *
 *  Created on: Dec 25, 2025
 *      Author: GitHub Copilot
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

#define BSP_ADC_MAX_CHANNELS 16u

/**
 * ADC error codes
 */
typedef enum
{
    eBSP_ADC_ERR_NONE = 0,      /**< No error */
    eBSP_ADC_ERR_CONFIGURATION, /**< Channel configuration mismatch */
    eBSP_ADC_ERR_CONVERSION,    /**< ADC conversion failed */
    eBSP_ADC_ERR_INVALID_PARAM  /**< Invalid parameter */
} BspAdcError_e;

/**
 * Callback function type for receiving ADC conversion results.
 * @param wValue The 12-bit ADC conversion result
 */
typedef void (*BspAdcValueCb_t)(uint16_t wValue);

/**
 * Callback function type for handling ADC errors.
 * @param eError The error status code
 */
typedef void (*BspAdcErrorCb_t)(BspAdcError_e eError);

/**
 * ADC channel enumeration
 */
typedef enum
{
    eBSP_ADC_CHANNEL_0 = 0u,
    eBSP_ADC_CHANNEL_1,
    eBSP_ADC_CHANNEL_2,
    eBSP_ADC_CHANNEL_3,
    eBSP_ADC_CHANNEL_4,
    eBSP_ADC_CHANNEL_5,
    eBSP_ADC_CHANNEL_6,
    eBSP_ADC_CHANNEL_7,
    eBSP_ADC_CHANNEL_8,
    eBSP_ADC_CHANNEL_9,
    eBSP_ADC_CHANNEL_10,
    eBSP_ADC_CHANNEL_11,
    eBSP_ADC_CHANNEL_12,
    eBSP_ADC_CHANNEL_13,
    eBSP_ADC_CHANNEL_14,
    eBSP_ADC_CHANNEL_15
} BspAdcChannel_e;

/**
 * ADC sample time enumeration
 */
typedef enum
{
    eBSP_ADC_SampleTime_3Cycles = 0u,
    eBSP_ADC_SampleTime_15Cycles,
    eBSP_ADC_SampleTime_28Cycles,
    eBSP_ADC_SampleTime_56Cycles,
    eBSP_ADC_SampleTime_84Cycles,
    eBSP_ADC_SampleTime_112Cycles,
    eBSP_ADC_SampleTime_144Cycles,
    eBSP_ADC_SampleTime_480Cycles
} BspAdcSampleTime_e;

/**
 * @brief Initialize ADC module.
 * @param pAdc Opaque pointer to ADC handle (ADC_HandleTypeDef*)
 * @return true if initialization successful, false otherwise
 */
bool BspAdcInit(void* pAdc);

/**
 * @brief Register an ADC channel with callback.
 * @param eChannel ADC channel to register
 * @param eSampleTime Sample time for this channel
 * @param pValueCallback Callback to receive conversion result
 * @return true if registration successful, false otherwise
 */
bool BspAdcRegisterChannel(BspAdcChannel_e eChannel, BspAdcSampleTime_e eSampleTime, BspAdcValueCb_t pValueCallback);

/**
 * @brief Start periodic ADC sampling.
 * All registered channels must match HAL configuration before calling this.
 * @param uPeriodMs Sampling period in milliseconds
 */
void BspAdcStart(uint32_t uPeriodMs);

/**
 * @brief Stop ADC sampling.
 */
void BspAdcStop(void);

/**
 * @brief Register error callback.
 * @param pErrCb Callback for error handling
 */
void BspAdcRegisterErrorCallback(BspAdcErrorCb_t pErrCb);

#ifdef __cplusplus
}
#endif
