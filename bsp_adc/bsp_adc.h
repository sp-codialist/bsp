/*
 * bsp_adc.h
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
 * ADC instance enumeration
 */
typedef enum
{
    eBSP_ADC_INSTANCE_1 = 0u,
    eBSP_ADC_INSTANCE_2,
    eBSP_ADC_INSTANCE_3,
    eBSP_ADC_INSTANCE_COUNT
} BspAdcInstance_e;

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
 * ADC channel handle type
 * Valid handles are 0-15, -1 indicates invalid/error
 */
typedef int8_t BspAdcChannelHandle_t;

/**
 * @brief Allocate and initialize an ADC channel instance.
 * Each instance operates independently with its own timer and callback.
 * Prevents duplicate allocation of same channel on same ADC instance.
 * @param eAdcInstance ADC peripheral instance (ADC1, ADC2, ADC3)
 * @param eChannel Physical ADC channel number (0-15)
 * @param eSampleTime Sample time for this channel
 * @param pValueCallback Callback to receive conversion result
 * @return Channel handle (0-15) on success, -1 on failure (no free slots or duplicate)
 */
BspAdcChannelHandle_t BspAdcAllocateChannel(BspAdcInstance_e eAdcInstance, BspAdcChannel_e eChannel, BspAdcSampleTime_e eSampleTime,
                                            BspAdcValueCb_t pValueCallback);

/**
 * @brief Free an allocated ADC channel instance.
 * Stops the timer and releases the channel for reuse.
 * @param handle Channel handle to free (0-15)
 */
void BspAdcFreeChannel(BspAdcChannelHandle_t handle);

/**
 * @brief Start periodic ADC sampling for a specific channel.
 * @param handle Channel handle (0-15)
 * @param uPeriodMs Sampling period in milliseconds
 */
void BspAdcStart(BspAdcChannelHandle_t handle, uint32_t uPeriodMs);

/**
 * @brief Stop ADC sampling for a specific channel.
 * @param handle Channel handle (0-15)
 */
void BspAdcStop(BspAdcChannelHandle_t handle);

/**
 * @brief Register error callback for a specific channel.
 * @param handle Channel handle (0-15)
 * @param pErrCb Callback for DMA error handling
 */
void BspAdcRegisterErrorCallback(BspAdcChannelHandle_t handle, BspAdcErrorCb_t pErrCb);

#ifdef __cplusplus
}
#endif
