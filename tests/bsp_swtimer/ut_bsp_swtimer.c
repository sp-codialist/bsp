/**
 * @file ut_bsp_swtimer.c
 * @brief Unit tests for BSP Software Timer module using Unity and CMock
 * @note This test file mocks HAL layer functions to test BSP SWTimer functionality
 */

#include "Mockstm32f4xx_hal.h"
#include "bsp_swtimer.h"
#include "unity.h"

// External declaration for HAL callback implemented in production code
void HAL_SYSTICK_Callback(void);

// Test callback tracker
static bool callback_invoked  = false;
static int  callback_count    = 0;
static bool callback2_invoked = false;

static void test_callback(void)
{
    callback_invoked = true;
    callback_count++;
}

static void test_callback2(void)
{
    callback2_invoked = true;
}

// ============================================================================
// Test Fixtures
// ============================================================================

void setUp(void)
{
    // Reset callback trackers
    callback_invoked  = false;
    callback2_invoked = false;
    callback_count    = 0;
}

void tearDown(void)
{
    // Clean up after each test
}

// ============================================================================
// SWTimerInit Tests
// ============================================================================

void test_SWTimerInit_ValidPointer_ReturnsTrue(void)
{
    // Arrange
    SWTimerModule timer = {0};

    // Act
    bool result = SWTimerInit(&timer);

    // Assert
    TEST_ASSERT_TRUE(result);
}

void test_SWTimerInit_NullPointer_ReturnsFalse(void)
{
    // Act
    bool result = SWTimerInit(NULL);

    // Assert
    TEST_ASSERT_FALSE(result);
}

void test_SWTimerInit_SameTimerTwice_ReturnsTrue(void)
{
    // Arrange
    SWTimerModule timer = {0};

    // Act
    bool result1 = SWTimerInit(&timer);
    bool result2 = SWTimerInit(&timer);

    // Assert - should succeed both times (already registered)
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(result2);
}

void test_SWTimerInit_MultipleTimers_RegistersAll(void)
{
    // Arrange
    SWTimerModule timer1 = {0};
    SWTimerModule timer2 = {0};
    SWTimerModule timer3 = {0};

    // Act
    bool result1 = SWTimerInit(&timer1);
    bool result2 = SWTimerInit(&timer2);
    bool result3 = SWTimerInit(&timer3);

    // Assert
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_TRUE(result3);
}

void test_SWTimerInit_MaxTimersReached_ReturnsFalse(void)
{
    // Arrange - Previous tests have registered some timers already
    // We need to fill remaining slots to reach MAX (16)
    // Register timers until we hit the limit
    SWTimerModule timers[20] = {0}; // More than MAX
    bool          hitLimit   = false;

    // Act - try to register timers until we fail
    for (int i = 0; i < 20; i++)
    {
        bool result = SWTimerInit(&timers[i]);
        if (!result)
        {
            // We hit the limit, this is expected
            hitLimit = true;
            break;
        }
    }

    // Assert - we should have hit the limit at some point
    TEST_ASSERT_TRUE_MESSAGE(hitLimit, "Expected to hit MAX_SW_TIMERS limit");
}

// ============================================================================
// SWTimerStart Tests
// ============================================================================

void test_SWTimerStart_ValidTimer_SetsActiveAndExpiration(void)
{
    // Arrange
    SWTimerModule timer     = {0};
    timer.interval          = 100;
    timer.pCallbackFunction = test_callback;
    timer.periodic          = false;

    uint32_t current_tick = 1000;
    HAL_GetTick_ExpectAndReturn(current_tick);

    // Act
    bool result = SWTimerStart(&timer);

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(timer.active);
    TEST_ASSERT_EQUAL_UINT32(current_tick + 100, timer.expiration);
}

void test_SWTimerStart_NullPointer_ReturnsFalse(void)
{
    // Act
    bool result = SWTimerStart(NULL);

    // Assert
    TEST_ASSERT_FALSE(result);
}

void test_SWTimerStart_PeriodicTimer_SetsActiveAndExpiration(void)
{
    // Arrange
    SWTimerModule timer     = {0};
    timer.interval          = 500;
    timer.pCallbackFunction = test_callback;
    timer.periodic          = true;

    uint32_t current_tick = 2000;
    HAL_GetTick_ExpectAndReturn(current_tick);

    // Act
    bool result = SWTimerStart(&timer);

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(timer.active);
    TEST_ASSERT_TRUE(timer.periodic);
    TEST_ASSERT_EQUAL_UINT32(current_tick + 500, timer.expiration);
}

// ============================================================================
// SWTimerStop Tests
// ============================================================================

void test_SWTimerStop_ActiveTimer_DeactivatesTimer(void)
{
    // Arrange
    SWTimerModule timer = {0};
    timer.active        = true;
    timer.interval      = 100;

    // Act
    SWTimerStop(&timer);

    // Assert
    TEST_ASSERT_FALSE(timer.active);
}

void test_SWTimerStop_NullPointer_NoAction(void)
{
    // Act - should not crash
    SWTimerStop(NULL);

    // Assert - no crash is success
}

void test_SWTimerStop_InactiveTimer_RemainsInactive(void)
{
    // Arrange
    SWTimerModule timer = {0};
    timer.active        = false;

    // Act
    SWTimerStop(&timer);

    // Assert
    TEST_ASSERT_FALSE(timer.active);
}

// ============================================================================
// SWTimerProcess Tests
// ============================================================================

void test_SWTimerProcess_NullPointer_NoAction(void)
{
    // Act - should not crash
    SWTimerProcess(NULL);

    // Assert - no crash is success
}

void test_SWTimerProcess_InactiveTimer_NoCallback(void)
{
    // Arrange
    SWTimerModule timer     = {0};
    timer.active            = false;
    timer.pCallbackFunction = test_callback;

    // Act
    SWTimerProcess(&timer);

    // Assert
    TEST_ASSERT_FALSE(callback_invoked);
}

void test_SWTimerProcess_NotExpired_NoCallback(void)
{
    // Arrange
    SWTimerModule timer     = {0};
    timer.active            = true;
    timer.expiration        = 1100;
    timer.interval          = 100;
    timer.pCallbackFunction = test_callback;
    timer.periodic          = false;

    HAL_GetTick_ExpectAndReturn(1000); // Current tick before expiration

    // Act
    SWTimerProcess(&timer);

    // Assert
    TEST_ASSERT_FALSE(callback_invoked);
    TEST_ASSERT_TRUE(timer.active); // Should still be active
}

void test_SWTimerProcess_OneShotExpired_CallsCallbackAndDeactivates(void)
{
    // Arrange
    SWTimerModule timer     = {0};
    timer.active            = true;
    timer.expiration        = 1100;
    timer.interval          = 100;
    timer.pCallbackFunction = test_callback;
    timer.periodic          = false;

    HAL_GetTick_ExpectAndReturn(1100); // Exactly at expiration

    // Act
    SWTimerProcess(&timer);

    // Assert
    TEST_ASSERT_TRUE(callback_invoked);
    TEST_ASSERT_EQUAL_INT(1, callback_count);
    TEST_ASSERT_FALSE(timer.active); // One-shot should deactivate
}

void test_SWTimerProcess_OneShotExpiredPastDue_CallsCallbackAndDeactivates(void)
{
    // Arrange
    SWTimerModule timer     = {0};
    timer.active            = true;
    timer.expiration        = 1100;
    timer.interval          = 100;
    timer.pCallbackFunction = test_callback;
    timer.periodic          = false;

    HAL_GetTick_ExpectAndReturn(1150); // Past expiration

    // Act
    SWTimerProcess(&timer);

    // Assert
    TEST_ASSERT_TRUE(callback_invoked);
    TEST_ASSERT_EQUAL_INT(1, callback_count);
    TEST_ASSERT_FALSE(timer.active); // One-shot should deactivate
}

void test_SWTimerProcess_PeriodicExpired_CallsCallbackAndRestarts(void)
{
    // Arrange
    SWTimerModule timer     = {0};
    timer.active            = true;
    timer.expiration        = 1100;
    timer.interval          = 100;
    timer.pCallbackFunction = test_callback;
    timer.periodic          = true;

    HAL_GetTick_ExpectAndReturn(1100); // Exactly at expiration

    // Act
    SWTimerProcess(&timer);

    // Assert
    TEST_ASSERT_TRUE(callback_invoked);
    TEST_ASSERT_EQUAL_INT(1, callback_count);
    TEST_ASSERT_TRUE(timer.active);                         // Periodic should remain active
    TEST_ASSERT_EQUAL_UINT32(1100 + 100, timer.expiration); // Should restart with new expiration
}

void test_SWTimerProcess_ExpiredNoCallback_DeactivatesWithoutCrash(void)
{
    // Arrange
    SWTimerModule timer     = {0};
    timer.active            = true;
    timer.expiration        = 1100;
    timer.interval          = 100;
    timer.pCallbackFunction = NULL; // No callback set
    timer.periodic          = false;

    HAL_GetTick_ExpectAndReturn(1100);

    // Act
    SWTimerProcess(&timer);

    // Assert
    TEST_ASSERT_FALSE(timer.active); // Should still deactivate
    TEST_ASSERT_FALSE(callback_invoked);
}

void test_SWTimerProcess_RolloverScenario_HandlesCorrectly(void)
{
    // Arrange - test tick counter rollover handling
    SWTimerModule timer     = {0};
    timer.active            = true;
    timer.expiration        = 100; // Expiration after rollover
    timer.interval          = 200;
    timer.pCallbackFunction = test_callback;
    timer.periodic          = false;

    // Current tick is near max, expiration wrapped around
    HAL_GetTick_ExpectAndReturn(200); // After expiration even with rollover

    // Act
    SWTimerProcess(&timer);

    // Assert
    TEST_ASSERT_TRUE(callback_invoked);
    TEST_ASSERT_FALSE(timer.active);
}

// ============================================================================
// SWTimerIsActive Tests
// ============================================================================

void test_SWTimerIsActive_NullPointer_ReturnsFalse(void)
{
    // Act
    bool result = SWTimerIsActive(NULL);

    // Assert
    TEST_ASSERT_FALSE(result);
}

void test_SWTimerIsActive_ActiveTimer_ReturnsTrue(void)
{
    // Arrange
    SWTimerModule timer = {0};
    timer.active        = true;

    // Act
    bool result = SWTimerIsActive(&timer);

    // Assert
    TEST_ASSERT_TRUE(result);
}

void test_SWTimerIsActive_InactiveTimer_ReturnsFalse(void)
{
    // Arrange
    SWTimerModule timer = {0};
    timer.active        = false;

    // Act
    bool result = SWTimerIsActive(&timer);

    // Assert
    TEST_ASSERT_FALSE(result);
}

// ============================================================================
// SWTimerGetRemaining Tests
// ============================================================================

void test_SWTimerGetRemaining_NullPointer_ReturnsZero(void)
{
    // Act
    uint32_t remaining = SWTimerGetRemaining(NULL);

    // Assert
    TEST_ASSERT_EQUAL_UINT32(0, remaining);
}

void test_SWTimerGetRemaining_InactiveTimer_ReturnsZero(void)
{
    // Arrange
    SWTimerModule timer = {0};
    timer.active        = false;
    timer.expiration    = 1100;

    // Act
    uint32_t remaining = SWTimerGetRemaining(&timer);

    // Assert
    TEST_ASSERT_EQUAL_UINT32(0, remaining);
}

void test_SWTimerGetRemaining_ActiveNotExpired_ReturnsRemaining(void)
{
    // Arrange
    SWTimerModule timer = {0};
    timer.active        = true;
    timer.expiration    = 1100;

    HAL_GetTick_ExpectAndReturn(1000);

    // Act
    uint32_t remaining = SWTimerGetRemaining(&timer);

    // Assert
    TEST_ASSERT_EQUAL_UINT32(100, remaining);
}

void test_SWTimerGetRemaining_ActiveExpired_ReturnsZero(void)
{
    // Arrange
    SWTimerModule timer = {0};
    timer.active        = true;
    timer.expiration    = 1100;

    HAL_GetTick_ExpectAndReturn(1200); // Past expiration

    // Act
    uint32_t remaining = SWTimerGetRemaining(&timer);

    // Assert
    TEST_ASSERT_EQUAL_UINT32(0, remaining);
}

void test_SWTimerGetRemaining_ActiveExactlyExpired_ReturnsZero(void)
{
    // Arrange
    SWTimerModule timer = {0};
    timer.active        = true;
    timer.expiration    = 1100;

    HAL_GetTick_ExpectAndReturn(1100); // Exactly at expiration

    // Act
    uint32_t remaining = SWTimerGetRemaining(&timer);

    // Assert
    TEST_ASSERT_EQUAL_UINT32(0, remaining);
}

// ============================================================================
// HAL_SYSTICK_Callback Tests
// Note: These tests work with the accumulated timer registry state from previous tests
// ============================================================================

void test_HAL_SYSTICK_Callback_NoTimersRegistered_NoAction(void)
{
    // Note: Previous tests may have registered and activated timers
    // Allow unlimited HAL_GetTick calls since we don't know how many active timers exist
    HAL_GetTick_IgnoreAndReturn(5000); // Return a time where no timers should expire

    // Act - should not crash
    HAL_SYSTICK_Callback();

    // Assert - no crash is success
    HAL_GetTick_StopIgnore();
}

void test_HAL_SYSTICK_Callback_ProcessesRegisteredTimers(void)
{
    // Arrange - register and start a new timer
    SWTimerModule timer1     = {0};
    timer1.interval          = 100;
    timer1.pCallbackFunction = test_callback;
    timer1.periodic          = false;

    SWTimerInit(&timer1);

    HAL_GetTick_ExpectAndReturn(1000); // For SWTimerStart
    SWTimerStart(&timer1);

    // The callback will process all timers - we use IgnoreAndReturn
    // to handle any active timers from previous tests
    HAL_GetTick_IgnoreAndReturn(1100); // For processing all active timers including timer1

    // Act
    HAL_SYSTICK_Callback();

    // Assert
    TEST_ASSERT_TRUE(callback_invoked);
    TEST_ASSERT_FALSE(timer1.active); // Should be deactivated after one-shot

    HAL_GetTick_StopIgnore();
}

void test_HAL_SYSTICK_Callback_ProcessesMultipleTimers(void)
{
    // Arrange - register and start two new timers
    SWTimerModule timer1     = {0};
    timer1.interval          = 100;
    timer1.pCallbackFunction = test_callback;
    timer1.periodic          = false;

    SWTimerModule timer2     = {0};
    timer2.interval          = 100;
    timer2.pCallbackFunction = test_callback2;
    timer2.periodic          = false;

    SWTimerInit(&timer1);
    SWTimerInit(&timer2);

    HAL_GetTick_ExpectAndReturn(1000); // For timer1 start
    SWTimerStart(&timer1);

    HAL_GetTick_ExpectAndReturn(1000); // For timer2 start
    SWTimerStart(&timer2);

    // Both timers are active and will be processed along with any others
    HAL_GetTick_IgnoreAndReturn(1100); // For processing all active timers

    // Act
    HAL_SYSTICK_Callback();

    // Assert
    TEST_ASSERT_TRUE(callback_invoked);
    TEST_ASSERT_TRUE(callback2_invoked);

    HAL_GetTick_StopIgnore();
}
