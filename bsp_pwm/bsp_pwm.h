/**
 * @file    bsp_pwm.h
 * @brief   BSP PWM driver interface for STM32 timer-based PWM generation
 *
 * This module provides a hardware abstraction layer for PWM signal generation
 * using STM32 hardware timers. It supports multiple timers and channels with
 * configurable frequency and duty cycle control.
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

/* --- Type Definitions --- */

/**
 * @brief Maximum number of simultaneously allocated PWM channels.
 *
 * Based on STM32F4 supporting up to 14 timers with 4 channels each,
 * though typically only 4 timers × 4 channels = 16 are commonly used.
 */
#define BSP_PWM_MAX_CHANNELS (16u)

/**
 * @brief PWM channel handle type.
 *
 * Valid handles are >= 0. A value of -1 indicates an invalid handle or error.
 */
typedef int8_t BspPwmHandle_t;

/**
 * @brief PWM timer instance enumeration.
 *
 * Identifies which hardware timer to use for PWM generation.
 * Not all timers may be available depending on hardware configuration.
 */
typedef enum
{
    eBSP_PWM_TIMER_1 = 0u, /**< Timer 1 (APB2, Advanced) */
    eBSP_PWM_TIMER_2,      /**< Timer 2 (APB1, General Purpose) */
    eBSP_PWM_TIMER_3,      /**< Timer 3 (APB1, General Purpose) */
    eBSP_PWM_TIMER_4,      /**< Timer 4 (APB1, General Purpose) */
    eBSP_PWM_TIMER_5,      /**< Timer 5 (APB1, General Purpose) */
    eBSP_PWM_TIMER_8,      /**< Timer 8 (APB2, Advanced) */
    eBSP_PWM_TIMER_9,      /**< Timer 9 (APB2, General Purpose) */
    eBSP_PWM_TIMER_10,     /**< Timer 10 (APB2, General Purpose) */
    eBSP_PWM_TIMER_11,     /**< Timer 11 (APB2, General Purpose) */
    eBSP_PWM_TIMER_12,     /**< Timer 12 (APB1, General Purpose) */
    eBSP_PWM_TIMER_13,     /**< Timer 13 (APB1, General Purpose) */
    eBSP_PWM_TIMER_14,     /**< Timer 14 (APB1, General Purpose) */
    eBSP_PWM_TIMER_COUNT   /**< Number of supported timers */
} BspPwmTimer_e;

/**
 * @brief PWM channel enumeration.
 *
 * Identifies which channel of a timer to use for PWM output.
 */
typedef enum
{
    eBSP_PWM_CHANNEL_1 = 0u, /**< Timer channel 1 */
    eBSP_PWM_CHANNEL_2,      /**< Timer channel 2 */
    eBSP_PWM_CHANNEL_3,      /**< Timer channel 3 */
    eBSP_PWM_CHANNEL_4,      /**< Timer channel 4 */
    eBSP_PWM_CHANNEL_COUNT   /**< Number of channels per timer */
} BspPwmChannel_e;

/**
 * @brief PWM error codes.
 */
typedef enum
{
    eBSP_PWM_ERR_NONE = 0,           /**< No error */
    eBSP_PWM_ERR_INVALID_HANDLE,     /**< Invalid or unallocated handle */
    eBSP_PWM_ERR_INVALID_PARAM,      /**< Invalid parameter provided */
    eBSP_PWM_ERR_NO_RESOURCE,        /**< No free channel slots available */
    eBSP_PWM_ERR_FREQUENCY_CONFLICT, /**< Frequency conflict on same timer */
    eBSP_PWM_ERR_TIMER_RUNNING,      /**< Timer is running, cannot change prescaler */
    eBSP_PWM_ERR_HAL_ERROR           /**< HAL layer error */
} BspPwmError_e;

/**
 * @brief Error callback function type for PWM operations.
 *
 * @param[in] handle  Handle of the PWM channel that encountered an error
 * @param[in] eError  Error code
 */
typedef void (*BspPwmErrorCb_t)(BspPwmHandle_t handle, BspPwmError_e eError);

/* --- Public Functions --- */

/**
 * @brief Allocates a PWM channel with specified timer, channel, and frequency.
 *
 * This function finds a free channel slot, configures the specified timer and
 * channel for PWM generation at the requested frequency. If multiple channels
 * on the same timer are allocated with different frequencies, a frequency
 * conflict warning will be issued via the error callback.
 *
 * @param[in] eTimer        Timer instance to use
 * @param[in] eChannel      Timer channel to use (1-4)
 * @param[in] wFrequencyKhz PWM frequency in kHz
 *
 * @return Handle to the allocated channel (>= 0) on success, -1 on failure
 *
 * @note The timer must be initialized and configured for PWM mode via CubeMX
 *       or equivalent initialization code before calling this function.
 */
BspPwmHandle_t BspPwmAllocateChannel(BspPwmTimer_e eTimer, BspPwmChannel_e eChannel, uint16_t wFrequencyKhz);

/**
 * @brief Frees a previously allocated PWM channel.
 *
 * Stops the PWM output if running and releases the channel resource.
 *
 * @param[in] handle  Handle of the channel to free
 *
 * @return Error code
 * @retval eBSP_PWM_ERR_NONE           Success
 * @retval eBSP_PWM_ERR_INVALID_HANDLE Invalid handle
 */
BspPwmError_e BspPwmFreeChannel(BspPwmHandle_t handle);

/**
 * @brief Sets the prescaler value for a specific timer.
 *
 * Allows configuring the timer prescaler to achieve desired frequency ranges.
 * This function can only be called when the timer is not running.
 *
 * @param[in] eTimer      Timer instance
 * @param[in] wPrescaler  Prescaler value (PSC register value, actual divisor is PSC+1)
 *
 * @return Error code
 * @retval eBSP_PWM_ERR_NONE          Success
 * @retval eBSP_PWM_ERR_INVALID_PARAM Invalid timer
 * @retval eBSP_PWM_ERR_TIMER_RUNNING Timer has active channels
 */
BspPwmError_e BspPwmSetPrescaler(BspPwmTimer_e eTimer, uint16_t wPrescaler);

/**
 * @brief Starts PWM generation on a specific channel.
 *
 * @param[in] handle  Handle of the channel to start
 *
 * @return Error code
 * @retval eBSP_PWM_ERR_NONE           Success
 * @retval eBSP_PWM_ERR_INVALID_HANDLE Invalid handle
 * @retval eBSP_PWM_ERR_HAL_ERROR      HAL layer error
 */
BspPwmError_e BspPwmStart(BspPwmHandle_t handle);

/**
 * @brief Starts PWM generation on all allocated channels.
 *
 * @return Error code
 * @retval eBSP_PWM_ERR_NONE      Success
 * @retval eBSP_PWM_ERR_HAL_ERROR HAL layer error on one or more channels
 */
BspPwmError_e BspPwmStartAll(void);

/**
 * @brief Stops PWM generation on a specific channel.
 *
 * @param[in] handle  Handle of the channel to stop
 *
 * @return Error code
 * @retval eBSP_PWM_ERR_NONE           Success
 * @retval eBSP_PWM_ERR_INVALID_HANDLE Invalid handle
 * @retval eBSP_PWM_ERR_HAL_ERROR      HAL layer error
 */
BspPwmError_e BspPwmStop(BspPwmHandle_t handle);

/**
 * @brief Stops PWM generation on all allocated channels.
 *
 * @return Error code
 * @retval eBSP_PWM_ERR_NONE      Success
 * @retval eBSP_PWM_ERR_HAL_ERROR HAL layer error on one or more channels
 */
BspPwmError_e BspPwmStopAll(void);

/**
 * @brief Sets the duty cycle for a PWM channel.
 *
 * @param[in] handle         Handle of the channel
 * @param[in] wDutyCyclePpt  Duty cycle in parts per thousand (0-1000, where 1000 = 100%)
 *
 * @return Error code
 * @retval eBSP_PWM_ERR_NONE           Success
 * @retval eBSP_PWM_ERR_INVALID_HANDLE Invalid handle
 * @retval eBSP_PWM_ERR_INVALID_PARAM  Duty cycle > 1000
 */
BspPwmError_e BspPwmSetDutyCycle(BspPwmHandle_t handle, uint16_t wDutyCyclePpt);

/**
 * @brief Registers an error callback for a specific PWM channel.
 *
 * The callback will be invoked when errors occur during PWM operations
 * on the specified channel.
 *
 * @param[in] handle  Handle of the channel
 * @param[in] pErrCb  Pointer to error callback function (NULL to unregister)
 *
 * @return Error code
 * @retval eBSP_PWM_ERR_NONE           Success
 * @retval eBSP_PWM_ERR_INVALID_HANDLE Invalid handle
 */
BspPwmError_e BspPwmRegisterErrorCallback(BspPwmHandle_t handle, BspPwmErrorCb_t pErrCb);

#ifdef __cplusplus
}
#endif
