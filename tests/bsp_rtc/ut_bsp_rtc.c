/**
 * @file ut_bsp_rtc.c
 * @brief Unit tests for BSP RTC module using Unity and CMock
 * @note This test file mocks HAL RTC layer functions to test BSP RTC functionality
 */

#include "bsp_rtc.h"

#include "Mockstm32f4xx_hal_rtc.h"
#include "unity.h"

#include <string.h>

/* ============================================================================
 * External HAL Handle Declaration
 * ========================================================================== */

// External RTC handle that production code expects
RTC_HandleTypeDef hrtc;

/* ============================================================================
 * External Declarations for FORCE_STATIC Functions and Variables
 * ========================================================================== */

// Externals for testing internal functions (FORCE_STATIC)
extern bool     s_bInitialized;
extern bool     sIsLeapYear(uint16_t wYear);
extern bool     sIsValidDateTime(const BspRtcDateTime_t* pDateTime);
extern uint32_t sDateTimeToUnix(const BspRtcDateTime_t* pDateTime);
extern void     sUnixToDateTime(uint32_t uUnixTime, BspRtcDateTime_t* pDateTime);

/* ============================================================================
 * Stub Functions for HAL Mocks
 * ========================================================================== */

static HAL_StatusTypeDef stub_HAL_RTC_GetTime(RTC_HandleTypeDef* hrtc, RTC_TimeTypeDef* sTime, uint32_t Format, int cmock_num_calls)
{
    (void)hrtc;
    (void)Format;
    (void)cmock_num_calls;

    // Return a valid time (2024-01-01 12:30:45)
    sTime->Hours   = 12;
    sTime->Minutes = 30;
    sTime->Seconds = 45;
    return HAL_OK;
}

static HAL_StatusTypeDef stub_HAL_RTC_GetDate(RTC_HandleTypeDef* hrtc, RTC_DateTypeDef* sDate, uint32_t Format, int cmock_num_calls)
{
    (void)hrtc;
    (void)Format;
    (void)cmock_num_calls;

    // Return a valid date (2024-01-01) - Year is offset from 2000
    sDate->Year  = 24; // 2024
    sDate->Month = 1;
    sDate->Date  = 1;
    return HAL_OK;
}

/* ============================================================================
 * Test Fixtures
 * ========================================================================== */

void setUp(void)
{
    // Reset module state before each test
    s_bInitialized = false;
    memset(&hrtc, 0, sizeof(hrtc));
}

void tearDown(void)
{
    // Clean up after each test (if needed)
}

/* ============================================================================
 * BspRtcInit Tests
 * ========================================================================== */

void test_BspRtcInit_FirstCall_ReturnsSuccess(void)
{
    // Act
    BspRtcError_e result = BspRtcInit();

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_NONE, result);
    TEST_ASSERT_TRUE(s_bInitialized);
}

void test_BspRtcInit_MultipleCalls_ReturnsSuccess(void)
{
    // Act
    BspRtcError_e result1 = BspRtcInit();
    BspRtcError_e result2 = BspRtcInit();

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_NONE, result1);
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_NONE, result2);
}

/* ============================================================================
 * BspRtcSetDateTime Tests
 * ========================================================================== */

void test_BspRtcSetDateTime_NotInitialized_ReturnsError(void)
{
    // Arrange
    BspRtcDateTime_t dateTime = {.wYear = 2025, .byMonth = 12, .byDay = 26, .byHour = 14, .byMinute = 30, .bySecond = 45};

    // Act
    BspRtcError_e result = BspRtcSetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_NOT_INIT, result);
}

void test_BspRtcSetDateTime_NullPointer_ReturnsError(void)
{
    // Arrange
    BspRtcInit();

    // Act
    BspRtcError_e result = BspRtcSetDateTime(NULL);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_INVALID_PARAM, result);
}

void test_BspRtcSetDateTime_ValidDateTime_ReturnsSuccess(void)
{
    // Arrange
    BspRtcInit();
    BspRtcDateTime_t dateTime = {.wYear = 2025, .byMonth = 12, .byDay = 26, .byHour = 14, .byMinute = 30, .bySecond = 45};

    HAL_RTC_SetTime_ExpectAndReturn(&hrtc, NULL, RTC_FORMAT_BIN, HAL_OK);
    HAL_RTC_SetTime_IgnoreArg_sTime();

    HAL_RTC_SetDate_ExpectAndReturn(&hrtc, NULL, RTC_FORMAT_BIN, HAL_OK);
    HAL_RTC_SetDate_IgnoreArg_sDate();

    // Act
    BspRtcError_e result = BspRtcSetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_NONE, result);
}

void test_BspRtcSetDateTime_HAL_SetTimeFails_ReturnsError(void)
{
    // Arrange
    BspRtcInit();
    BspRtcDateTime_t dateTime = {.wYear = 2025, .byMonth = 12, .byDay = 26, .byHour = 14, .byMinute = 30, .bySecond = 45};

    HAL_RTC_SetTime_ExpectAndReturn(&hrtc, NULL, RTC_FORMAT_BIN, HAL_ERROR);
    HAL_RTC_SetTime_IgnoreArg_sTime();

    // Act
    BspRtcError_e result = BspRtcSetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_HAL_ERROR, result);
}

void test_BspRtcSetDateTime_HAL_SetDateFails_ReturnsError(void)
{
    // Arrange
    BspRtcInit();
    BspRtcDateTime_t dateTime = {.wYear = 2025, .byMonth = 12, .byDay = 26, .byHour = 14, .byMinute = 30, .bySecond = 45};

    HAL_RTC_SetTime_ExpectAndReturn(&hrtc, NULL, RTC_FORMAT_BIN, HAL_OK);
    HAL_RTC_SetTime_IgnoreArg_sTime();

    HAL_RTC_SetDate_ExpectAndReturn(&hrtc, NULL, RTC_FORMAT_BIN, HAL_ERROR);
    HAL_RTC_SetDate_IgnoreArg_sDate();

    // Act
    BspRtcError_e result = BspRtcSetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_HAL_ERROR, result);
}

void test_BspRtcSetDateTime_YearBefore2000_ReturnsError(void)
{
    // Arrange
    BspRtcInit();
    BspRtcDateTime_t dateTime = {.wYear = 1999, .byMonth = 12, .byDay = 31, .byHour = 23, .byMinute = 59, .bySecond = 59};

    // Act
    BspRtcError_e result = BspRtcSetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_INVALID_PARAM, result);
}

void test_BspRtcSetDateTime_InvalidMonth_ReturnsError(void)
{
    // Arrange
    BspRtcInit();
    BspRtcDateTime_t dateTime = {.wYear    = 2025,
                                 .byMonth  = 13, // Invalid month
                                 .byDay    = 1,
                                 .byHour   = 0,
                                 .byMinute = 0,
                                 .bySecond = 0};

    // Act
    BspRtcError_e result = BspRtcSetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_INVALID_PARAM, result);
}

void test_BspRtcSetDateTime_InvalidDay_ReturnsError(void)
{
    // Arrange
    BspRtcInit();
    BspRtcDateTime_t dateTime = {.wYear    = 2025,
                                 .byMonth  = 2,
                                 .byDay    = 29, // Not a leap year
                                 .byHour   = 0,
                                 .byMinute = 0,
                                 .bySecond = 0};

    // Act
    BspRtcError_e result = BspRtcSetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_INVALID_PARAM, result);
}

void test_BspRtcSetDateTime_LeapYearFeb29_ReturnsSuccess(void)
{
    // Arrange
    BspRtcInit();
    BspRtcDateTime_t dateTime = {.wYear    = 2024, // Leap year
                                 .byMonth  = 2,
                                 .byDay    = 29,
                                 .byHour   = 0,
                                 .byMinute = 0,
                                 .bySecond = 0};

    HAL_RTC_SetTime_ExpectAndReturn(&hrtc, NULL, RTC_FORMAT_BIN, HAL_OK);
    HAL_RTC_SetTime_IgnoreArg_sTime();

    HAL_RTC_SetDate_ExpectAndReturn(&hrtc, NULL, RTC_FORMAT_BIN, HAL_OK);
    HAL_RTC_SetDate_IgnoreArg_sDate();

    // Act
    BspRtcError_e result = BspRtcSetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_NONE, result);
}

void test_BspRtcSetDateTime_InvalidHour_ReturnsError(void)
{
    // Arrange
    BspRtcInit();
    BspRtcDateTime_t dateTime = {.wYear    = 2025,
                                 .byMonth  = 12,
                                 .byDay    = 26,
                                 .byHour   = 24, // Invalid hour
                                 .byMinute = 0,
                                 .bySecond = 0};

    // Act
    BspRtcError_e result = BspRtcSetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_INVALID_PARAM, result);
}

/* ============================================================================
 * BspRtcGetDateTime Tests
 * ========================================================================== */

void test_BspRtcGetDateTime_NotInitialized_ReturnsError(void)
{
    // Arrange
    BspRtcDateTime_t dateTime;

    // Act
    BspRtcError_e result = BspRtcGetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_NOT_INIT, result);
}

void test_BspRtcGetDateTime_NullPointer_ReturnsError(void)
{
    // Arrange
    BspRtcInit();

    // Act
    BspRtcError_e result = BspRtcGetDateTime(NULL);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_INVALID_PARAM, result);
}

void test_BspRtcGetDateTime_ValidData_ReturnsSuccess(void)
{
    // Arrange
    BspRtcInit();
    BspRtcDateTime_t dateTime;

    HAL_RTC_GetTime_Stub(stub_HAL_RTC_GetTime);
    HAL_RTC_GetDate_Stub(stub_HAL_RTC_GetDate);

    // Act
    BspRtcError_e result = BspRtcGetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_NONE, result);
    TEST_ASSERT_EQUAL(2024, dateTime.wYear);
    TEST_ASSERT_EQUAL(1, dateTime.byMonth);
    TEST_ASSERT_EQUAL(1, dateTime.byDay);
    TEST_ASSERT_EQUAL(12, dateTime.byHour);
    TEST_ASSERT_EQUAL(30, dateTime.byMinute);
    TEST_ASSERT_EQUAL(45, dateTime.bySecond);
}

void test_BspRtcGetDateTime_HAL_GetTimeFails_ReturnsError(void)
{
    // Arrange
    BspRtcInit();
    BspRtcDateTime_t dateTime;

    HAL_RTC_GetTime_IgnoreAndReturn(HAL_ERROR);

    // Act
    BspRtcError_e result = BspRtcGetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_HAL_ERROR, result);
}

void test_BspRtcGetDateTime_HAL_GetDateFails_ReturnsError(void)
{
    // Arrange
    BspRtcInit();
    BspRtcDateTime_t dateTime;

    HAL_RTC_GetTime_IgnoreAndReturn(HAL_OK);
    HAL_RTC_GetDate_IgnoreAndReturn(HAL_ERROR);

    // Act
    BspRtcError_e result = BspRtcGetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_HAL_ERROR, result);
}

/* ============================================================================
 * BspRtcSetUnixTime Tests
 * ========================================================================== */

void test_BspRtcSetUnixTime_Epoch_ReturnsError(void)
{
    // Arrange
    BspRtcInit();
    uint32_t unixTime = 0; // Jan 1, 1970 00:00:00 (before 2000, should fail)

    // Act
    BspRtcError_e result = BspRtcSetUnixTime(unixTime);

    // Assert - Epoch is before 2000, so validation should fail
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_INVALID_PARAM, result);
}

void test_BspRtcSetUnixTime_KnownDate_ReturnsSuccess(void)
{
    // Arrange
    BspRtcInit();
    // Dec 26, 2025 14:30:45 UTC = 1766826645
    uint32_t unixTime = 1766826645;

    HAL_RTC_SetTime_ExpectAndReturn(&hrtc, NULL, RTC_FORMAT_BIN, HAL_OK);
    HAL_RTC_SetTime_IgnoreArg_sTime();

    HAL_RTC_SetDate_ExpectAndReturn(&hrtc, NULL, RTC_FORMAT_BIN, HAL_OK);
    HAL_RTC_SetDate_IgnoreArg_sDate();

    // Act
    BspRtcError_e result = BspRtcSetUnixTime(unixTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_NONE, result);
}

/* ============================================================================
 * BspRtcGetUnixTime Tests
 * ========================================================================== */

void test_BspRtcGetUnixTime_NotInitialized_ReturnsError(void)
{
    // Arrange
    uint32_t unixTime;

    // Act
    BspRtcError_e result = BspRtcGetUnixTime(&unixTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_NOT_INIT, result);
}

void test_BspRtcGetUnixTime_NullPointer_ReturnsError(void)
{
    // Arrange
    BspRtcInit();

    // Act
    BspRtcError_e result = BspRtcGetUnixTime(NULL);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_INVALID_PARAM, result);
}

void test_BspRtcGetUnixTime_ValidData_ReturnsSuccess(void)
{
    // Arrange
    BspRtcInit();
    uint32_t unixTime;

    HAL_RTC_GetTime_Stub(stub_HAL_RTC_GetTime);
    HAL_RTC_GetDate_Stub(stub_HAL_RTC_GetDate);

    // Act
    BspRtcError_e result = BspRtcGetUnixTime(&unixTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_NONE, result);
    // Unix time for 2024-01-01 12:30:45 UTC
    TEST_ASSERT_EQUAL(1704112245, unixTime);
}

void test_BspRtcGetUnixTime_HAL_Fails_ReturnsError(void)
{
    // Arrange
    BspRtcInit();
    uint32_t unixTime;

    HAL_RTC_GetTime_IgnoreAndReturn(HAL_ERROR);

    // Act
    BspRtcError_e result = BspRtcGetUnixTime(&unixTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_HAL_ERROR, result);
}

/* ============================================================================
 * sIsLeapYear Internal Function Tests
 * ========================================================================== */

void test_sIsLeapYear_2024_ReturnsTrue(void)
{
    // Act
    bool result = sIsLeapYear(2024);

    // Assert
    TEST_ASSERT_TRUE(result);
}

void test_sIsLeapYear_2025_ReturnsFalse(void)
{
    // Act
    bool result = sIsLeapYear(2025);

    // Assert
    TEST_ASSERT_FALSE(result);
}

void test_sIsLeapYear_2000_ReturnsTrue(void)
{
    // Act (divisible by 400)
    bool result = sIsLeapYear(2000);

    // Assert
    TEST_ASSERT_TRUE(result);
}

void test_sIsLeapYear_1900_ReturnsFalse(void)
{
    // Act (divisible by 100 but not 400)
    bool result = sIsLeapYear(1900);

    // Assert
    TEST_ASSERT_FALSE(result);
}

/* ============================================================================
 * sDateTimeToUnix and sUnixToDateTime Round-Trip Tests
 * ========================================================================== */

void test_DateTimeToUnix_Epoch_ReturnsZero(void)
{
    // Arrange
    BspRtcDateTime_t dateTime = {.wYear = 1970, .byMonth = 1, .byDay = 1, .byHour = 0, .byMinute = 0, .bySecond = 0};

    // Act
    uint32_t result = sDateTimeToUnix(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(0, result);
}

void test_DateTimeToUnix_Y2K_ReturnsCorrectValue(void)
{
    // Arrange
    BspRtcDateTime_t dateTime = {.wYear = 2000, .byMonth = 1, .byDay = 1, .byHour = 0, .byMinute = 0, .bySecond = 0};

    // Act
    uint32_t result = sDateTimeToUnix(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(946684800, result);
}

void test_UnixToDateTime_Epoch_ReturnsCorrectDateTime(void)
{
    // Arrange
    BspRtcDateTime_t dateTime;
    uint32_t         unixTime = 0;

    // Act
    sUnixToDateTime(unixTime, &dateTime);

    // Assert
    TEST_ASSERT_EQUAL(1970, dateTime.wYear);
    TEST_ASSERT_EQUAL(1, dateTime.byMonth);
    TEST_ASSERT_EQUAL(1, dateTime.byDay);
    TEST_ASSERT_EQUAL(0, dateTime.byHour);
    TEST_ASSERT_EQUAL(0, dateTime.byMinute);
    TEST_ASSERT_EQUAL(0, dateTime.bySecond);
}

void test_UnixToDateTime_Y2K_ReturnsCorrectDateTime(void)
{
    // Arrange
    BspRtcDateTime_t dateTime;
    uint32_t         unixTime = 946684800;

    // Act
    sUnixToDateTime(unixTime, &dateTime);

    // Assert
    TEST_ASSERT_EQUAL(2000, dateTime.wYear);
    TEST_ASSERT_EQUAL(1, dateTime.byMonth);
    TEST_ASSERT_EQUAL(1, dateTime.byDay);
    TEST_ASSERT_EQUAL(0, dateTime.byHour);
    TEST_ASSERT_EQUAL(0, dateTime.byMinute);
    TEST_ASSERT_EQUAL(0, dateTime.bySecond);
}

void test_UnixConversion_RoundTrip_PreservesDateTime(void)
{
    // Arrange
    BspRtcDateTime_t original = {.wYear = 2025, .byMonth = 12, .byDay = 26, .byHour = 14, .byMinute = 30, .bySecond = 45};
    BspRtcDateTime_t converted;

    // Act
    uint32_t unixTime = sDateTimeToUnix(&original);
    sUnixToDateTime(unixTime, &converted);

    // Assert
    TEST_ASSERT_EQUAL(original.wYear, converted.wYear);
    TEST_ASSERT_EQUAL(original.byMonth, converted.byMonth);
    TEST_ASSERT_EQUAL(original.byDay, converted.byDay);
    TEST_ASSERT_EQUAL(original.byHour, converted.byHour);
    TEST_ASSERT_EQUAL(original.byMinute, converted.byMinute);
    TEST_ASSERT_EQUAL(original.bySecond, converted.bySecond);
}

void test_UnixConversion_LeapYear_HandlesCorrectly(void)
{
    // Arrange - Feb 29, 2024 (leap year)
    BspRtcDateTime_t original = {.wYear = 2024, .byMonth = 2, .byDay = 29, .byHour = 12, .byMinute = 0, .bySecond = 0};
    BspRtcDateTime_t converted;

    // Act
    uint32_t unixTime = sDateTimeToUnix(&original);
    sUnixToDateTime(unixTime, &converted);

    // Assert
    TEST_ASSERT_EQUAL(original.wYear, converted.wYear);
    TEST_ASSERT_EQUAL(original.byMonth, converted.byMonth);
    TEST_ASSERT_EQUAL(original.byDay, converted.byDay);
}

void test_UnixConversion_LeapYearAfterFeb_AddsExtraDay(void)
{
    // Arrange - March 1, 2024 (leap year, after Feb)
    BspRtcDateTime_t original = {.wYear = 2024, .byMonth = 3, .byDay = 1, .byHour = 0, .byMinute = 0, .bySecond = 0};
    BspRtcDateTime_t converted;

    // Act
    uint32_t unixTime = sDateTimeToUnix(&original);
    sUnixToDateTime(unixTime, &converted);

    // Assert - Verify round-trip conversion works correctly
    TEST_ASSERT_EQUAL(original.wYear, converted.wYear);
    TEST_ASSERT_EQUAL(original.byMonth, converted.byMonth);
    TEST_ASSERT_EQUAL(original.byDay, converted.byDay);
}

/* ============================================================================
 * Edge Case Tests
 * ========================================================================== */

void test_BspRtcSetDateTime_MaxValues_ReturnsSuccess(void)
{
    // Arrange
    BspRtcInit();
    BspRtcDateTime_t dateTime = {.wYear = 2099, .byMonth = 12, .byDay = 31, .byHour = 23, .byMinute = 59, .bySecond = 59};

    HAL_RTC_SetTime_ExpectAndReturn(&hrtc, NULL, RTC_FORMAT_BIN, HAL_OK);
    HAL_RTC_SetTime_IgnoreArg_sTime();

    HAL_RTC_SetDate_ExpectAndReturn(&hrtc, NULL, RTC_FORMAT_BIN, HAL_OK);
    HAL_RTC_SetDate_IgnoreArg_sDate();

    // Act
    BspRtcError_e result = BspRtcSetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_NONE, result);
}

void test_BspRtcSetDateTime_MinValues_ReturnsSuccess(void)
{
    // Arrange
    BspRtcInit();
    BspRtcDateTime_t dateTime = {.wYear = 2000, .byMonth = 1, .byDay = 1, .byHour = 0, .byMinute = 0, .bySecond = 0};

    HAL_RTC_SetTime_ExpectAndReturn(&hrtc, NULL, RTC_FORMAT_BIN, HAL_OK);
    HAL_RTC_SetTime_IgnoreArg_sTime();

    HAL_RTC_SetDate_ExpectAndReturn(&hrtc, NULL, RTC_FORMAT_BIN, HAL_OK);
    HAL_RTC_SetDate_IgnoreArg_sDate();

    // Act
    BspRtcError_e result = BspRtcSetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_NONE, result);
}

void test_BspRtcSetDateTime_MonthZero_ReturnsError(void)
{
    // Arrange
    BspRtcInit();
    BspRtcDateTime_t dateTime = {.wYear = 2025, .byMonth = 0, .byDay = 1, .byHour = 0, .byMinute = 0, .bySecond = 0};

    // Act
    BspRtcError_e result = BspRtcSetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_INVALID_PARAM, result);
}

void test_BspRtcSetDateTime_DayZero_ReturnsError(void)
{
    // Arrange
    BspRtcInit();
    BspRtcDateTime_t dateTime = {.wYear = 2025, .byMonth = 1, .byDay = 0, .byHour = 0, .byMinute = 0, .bySecond = 0};

    // Act
    BspRtcError_e result = BspRtcSetDateTime(&dateTime);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_RTC_ERR_INVALID_PARAM, result);
}
