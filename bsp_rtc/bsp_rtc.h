/**
 * @file    bsp_rtc.h
 * @brief   BSP RTC driver interface for STM32 hardware real-time clock
 * @details Provides date/time management with UTC-based operations.
 *          No local time or daylight saving time support.
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
 * @brief RTC error codes.
 */
typedef enum
{
    eBSP_RTC_ERR_NONE = 0,      /**< No error */
    eBSP_RTC_ERR_INVALID_PARAM, /**< Invalid parameter provided */
    eBSP_RTC_ERR_HAL_ERROR,     /**< HAL layer error */
    eBSP_RTC_ERR_NOT_INIT       /**< RTC not initialized */
} BspRtcError_e;

/**
 * @brief Date and time structure.
 * @details All values are in UTC (no local time or DST).
 */
typedef struct
{
    uint16_t wYear;    /**< Full year (e.g., 2025) */
    uint8_t  byMonth;  /**< Month (1-12) */
    uint8_t  byDay;    /**< Day of month (1-31) */
    uint8_t  byHour;   /**< Hour (0-23) */
    uint8_t  byMinute; /**< Minute (0-59) */
    uint8_t  bySecond; /**< Second (0-59) */
} BspRtcDateTime_t;

/* --- Public Functions --- */

/**
 * @brief Initializes the RTC module.
 * @details Must be called once during system initialization before any other RTC functions.
 *          The HAL RTC peripheral must already be configured (clock source, prescalers).
 * @return eBSP_RTC_ERR_NONE on success, error code otherwise
 */
BspRtcError_e BspRtcInit(void);

/**
 * @brief Sets the current date and time.
 * @details Sets UTC date and time in hardware RTC.
 * @param[in] pDateTime Pointer to date/time structure
 * @return eBSP_RTC_ERR_NONE on success, error code otherwise
 * @note Year must be >= 2000. Valid ranges: Month (1-12), Day (1-31),
 *       Hour (0-23), Minute (0-59), Second (0-59)
 */
BspRtcError_e BspRtcSetDateTime(const BspRtcDateTime_t* pDateTime);

/**
 * @brief Gets the current date and time.
 * @details Reads UTC date and time from hardware RTC.
 * @param[out] pDateTime Pointer to buffer for date/time structure
 * @return eBSP_RTC_ERR_NONE on success, error code otherwise
 */
BspRtcError_e BspRtcGetDateTime(BspRtcDateTime_t* pDateTime);

/**
 * @brief Sets date and time from Unix timestamp.
 * @details Converts Unix timestamp (seconds since Jan 1, 1970 00:00:00 UTC)
 *          to date/time and sets the RTC.
 * @param[in] uUnixTime Unix timestamp in seconds
 * @return eBSP_RTC_ERR_NONE on success, error code otherwise
 */
BspRtcError_e BspRtcSetUnixTime(uint32_t uUnixTime);

/**
 * @brief Gets current time as Unix timestamp.
 * @details Reads RTC and converts to seconds since Jan 1, 1970 00:00:00 UTC.
 * @param[out] pUnixTime Pointer to buffer for Unix timestamp
 * @return eBSP_RTC_ERR_NONE on success, error code otherwise
 */
BspRtcError_e BspRtcGetUnixTime(uint32_t* pUnixTime);

#ifdef __cplusplus
}
#endif
