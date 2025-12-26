/**
 * @file    bsp_rtc.c
 * @brief   Implementation of BSP RTC driver for STM32 hardware real-time clock
 * @details Provides hardware abstraction for RTC operations with UTC time support.
 */

#include "bsp_rtc.h"

#include <string.h>

#include "stm32f4xx_hal.h"

/* --- External Variables --- */

// External HAL RTC handle (must be defined in application)
extern RTC_HandleTypeDef hrtc;

/* --- Constants --- */

#define EPOCH_YEAR           1970u
#define BASE_YEAR            2000u
#define MONTHS_PER_YEAR      12u
#define DAYS_PER_COMMON_YEAR 365u
#define HOURS_PER_DAY        24u
#define MINUTES_PER_HOUR     60u
#define SECONDS_PER_MINUTE   60u

/* --- Local Variables --- */

/**
 * Initialization flag to track if RTC module is ready.
 */
static bool s_bInitialized = false;

/**
 * Days in each month for a non-leap year.
 */
static const uint8_t s_abyDaysInMonth[MONTHS_PER_YEAR] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/* --- Local Function Prototypes --- */

/**
 * @brief Checks if a given year is a leap year.
 * @param wYear Full year (e.g., 2024)
 * @return true if leap year, false otherwise
 */
static bool sIsLeapYear(uint16_t wYear);

/**
 * @brief Validates date and time values.
 * @param pDateTime Pointer to date/time structure
 * @return true if valid, false otherwise
 */
static bool sIsValidDateTime(const BspRtcDateTime_t* pDateTime);

/**
 * @brief Converts date/time to Unix timestamp.
 * @param pDateTime Pointer to date/time structure
 * @return Unix timestamp (seconds since epoch)
 */
static uint32_t sDateTimeToUnix(const BspRtcDateTime_t* pDateTime);

/**
 * @brief Converts Unix timestamp to date/time.
 * @param uUnixTime Unix timestamp
 * @param pDateTime Pointer to date/time structure to fill
 */
static void sUnixToDateTime(uint32_t uUnixTime, BspRtcDateTime_t* pDateTime);

/* --- Public Functions --- */

BspRtcError_e BspRtcInit(void)
{
    s_bInitialized = true;
    return eBSP_RTC_ERR_NONE;
}

BspRtcError_e BspRtcSetDateTime(const BspRtcDateTime_t* pDateTime)
{
    if (!s_bInitialized)
    {
        return eBSP_RTC_ERR_NOT_INIT;
    }

    if ((pDateTime == NULL) || !sIsValidDateTime(pDateTime))
    {
        return eBSP_RTC_ERR_INVALID_PARAM;
    }

    RTC_DateTypeDef tDate = {0};
    RTC_TimeTypeDef tTime = {0};

    // Convert year to 2-digit format (RTC stores year as offset from 2000)
    tDate.Year    = (uint8_t)(pDateTime->wYear - BASE_YEAR);
    tDate.Month   = pDateTime->byMonth;
    tDate.Date    = pDateTime->byDay;
    tDate.WeekDay = RTC_WEEKDAY_MONDAY; // Not used, set to default

    tTime.Hours          = pDateTime->byHour;
    tTime.Minutes        = pDateTime->byMinute;
    tTime.Seconds        = pDateTime->bySecond;
    tTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    tTime.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime(&hrtc, &tTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        return eBSP_RTC_ERR_HAL_ERROR;
    }

    if (HAL_RTC_SetDate(&hrtc, &tDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        return eBSP_RTC_ERR_HAL_ERROR;
    }

    return eBSP_RTC_ERR_NONE;
}

BspRtcError_e BspRtcGetDateTime(BspRtcDateTime_t* pDateTime)
{
    if (!s_bInitialized)
    {
        return eBSP_RTC_ERR_NOT_INIT;
    }

    if (pDateTime == NULL)
    {
        return eBSP_RTC_ERR_INVALID_PARAM;
    }

    RTC_DateTypeDef tDate = {0};
    RTC_TimeTypeDef tTime = {0};

    // Must read time before date (STM32 RTC requirement)
    // Time read freezes shadow registers, date read unfreezes them
    if (HAL_RTC_GetTime(&hrtc, &tTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        return eBSP_RTC_ERR_HAL_ERROR;
    }

    if (HAL_RTC_GetDate(&hrtc, &tDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        return eBSP_RTC_ERR_HAL_ERROR;
    }

    pDateTime->wYear    = (uint16_t)(tDate.Year + BASE_YEAR);
    pDateTime->byMonth  = tDate.Month;
    pDateTime->byDay    = tDate.Date;
    pDateTime->byHour   = tTime.Hours;
    pDateTime->byMinute = tTime.Minutes;
    pDateTime->bySecond = tTime.Seconds;

    return eBSP_RTC_ERR_NONE;
}

BspRtcError_e BspRtcSetUnixTime(uint32_t uUnixTime)
{
    BspRtcDateTime_t tDateTime;
    sUnixToDateTime(uUnixTime, &tDateTime);
    return BspRtcSetDateTime(&tDateTime);
}

BspRtcError_e BspRtcGetUnixTime(uint32_t* pUnixTime)
{
    if (!s_bInitialized)
    {
        return eBSP_RTC_ERR_NOT_INIT;
    }

    if (pUnixTime == NULL)
    {
        return eBSP_RTC_ERR_INVALID_PARAM;
    }

    BspRtcDateTime_t tDateTime;
    BspRtcError_e    eError = BspRtcGetDateTime(&tDateTime);

    if (eError == eBSP_RTC_ERR_NONE)
    {
        *pUnixTime = sDateTimeToUnix(&tDateTime);
    }

    return eError;
}

/* --- Local Functions --- */

static bool sIsLeapYear(uint16_t wYear)
{
    // Leap year if divisible by 4, not by 100 unless also by 400
    const bool bIsDivisibleBy4      = ((wYear % 4u) == 0u);
    const bool bIsNotDivisibleBy100 = ((wYear % 100u) != 0u);
    const bool bIsDivisibleBy400    = ((wYear % 400u) == 0u);

    return (bIsDivisibleBy4 && (bIsNotDivisibleBy100 || bIsDivisibleBy400));
}

static bool sIsValidDateTime(const BspRtcDateTime_t* pDateTime)
{
    // Check year range
    if (pDateTime->wYear < BASE_YEAR)
    {
        return false;
    }

    // Check month range
    if ((pDateTime->byMonth < 1u) || (pDateTime->byMonth > MONTHS_PER_YEAR))
    {
        return false;
    }

    // Check day range
    uint8_t byMaxDay = s_abyDaysInMonth[pDateTime->byMonth - 1u];
    if ((pDateTime->byMonth == 2u) && sIsLeapYear(pDateTime->wYear))
    {
        byMaxDay = 29u;
    }

    if ((pDateTime->byDay < 1u) || (pDateTime->byDay > byMaxDay))
    {
        return false;
    }

    // Check time ranges
    if ((pDateTime->byHour >= HOURS_PER_DAY) || (pDateTime->byMinute >= MINUTES_PER_HOUR) || (pDateTime->bySecond >= SECONDS_PER_MINUTE))
    {
        return false;
    }

    return true;
}

static uint32_t sDateTimeToUnix(const BspRtcDateTime_t* pDateTime)
{
    const uint16_t wYear            = pDateTime->wYear;
    const uint32_t uYearsSinceEpoch = (uint32_t)(wYear - EPOCH_YEAR);

    // Days accumulated at start of each month (non-leap year)
    const uint16_t awDaysByMonth[MONTHS_PER_YEAR] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    // Calculate leap years since epoch (excluding current year)
    // Leap year every 4 years, except century years unless divisible by 400
    const uint32_t uLeapYears = (((wYear - 1u - 1968u) / 4u) - ((wYear - 1u - 1900u) / 100u) + ((wYear - 1u - 1600u) / 400u));

    // Calculate total days
    uint32_t uDays = (uYearsSinceEpoch * DAYS_PER_COMMON_YEAR) + uLeapYears;
    uDays += awDaysByMonth[pDateTime->byMonth - 1u];
    uDays += pDateTime->byDay - 1u;

    // Add one day if current year is leap and month > February
    if ((pDateTime->byMonth > 2u) && sIsLeapYear(wYear))
    {
        uDays++;
    }

    // Convert to seconds
    const uint32_t uSeconds =
        (((uDays * HOURS_PER_DAY + pDateTime->byHour) * MINUTES_PER_HOUR + pDateTime->byMinute) * SECONDS_PER_MINUTE + pDateTime->bySecond);

    return uSeconds;
}

static void sUnixToDateTime(uint32_t uUnixTime, BspRtcDateTime_t* pDateTime)
{
    // Algorithm to convert Unix timestamp to date/time
    uint32_t uDays          = uUnixTime / (HOURS_PER_DAY * MINUTES_PER_HOUR * SECONDS_PER_MINUTE);
    uint32_t uRemainingSecs = uUnixTime % (HOURS_PER_DAY * MINUTES_PER_HOUR * SECONDS_PER_MINUTE);

    // Calculate time components
    pDateTime->byHour = (uint8_t)(uRemainingSecs / (MINUTES_PER_HOUR * SECONDS_PER_MINUTE));
    uRemainingSecs %= (MINUTES_PER_HOUR * SECONDS_PER_MINUTE);
    pDateTime->byMinute = (uint8_t)(uRemainingSecs / SECONDS_PER_MINUTE);
    pDateTime->bySecond = (uint8_t)(uRemainingSecs % SECONDS_PER_MINUTE);

    // Calculate year (approximate, then refine)
    uint16_t wYear = EPOCH_YEAR;
    while (true)
    {
        const uint16_t wDaysInYear = sIsLeapYear(wYear) ? 366u : 365u;
        if (uDays < wDaysInYear)
        {
            break;
        }
        uDays -= wDaysInYear;
        wYear++;
    }
    pDateTime->wYear = wYear;

    // Calculate month and day
    uint8_t byMonth = 1u;
    while (byMonth <= MONTHS_PER_YEAR)
    {
        uint8_t byDaysInMonth = s_abyDaysInMonth[byMonth - 1u];
        if ((byMonth == 2u) && sIsLeapYear(wYear))
        {
            byDaysInMonth = 29u;
        }

        if (uDays < byDaysInMonth)
        {
            break;
        }

        uDays -= byDaysInMonth;
        byMonth++;
    }

    pDateTime->byMonth = byMonth;
    pDateTime->byDay   = (uint8_t)(uDays + 1u);
}
