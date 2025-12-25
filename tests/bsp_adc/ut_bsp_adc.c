/**
 * @file ut_bsp_adc.c
 * @brief Unit tests for BSP ADC module using Unity and CMock
 * @note This test file mocks HAL and SWTimer dependencies to test BSP ADC functionality
 */

#include "Mockstm32f4xx_hal.h"
#include "Mockstm32f4xx_hal_adc.h"
#include "Mockstm32f4xx_hal_cortex.h"
#include "bsp_adc.h"
#include "bsp_swtimer.h"
#include "unity.h"
#include <stdbool.h>
#include <stdint.h>

// External declaration for HAL callback implemented in production code
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);

// External declarations for HAL handles (must be defined for tests)
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
ADC_HandleTypeDef hadc3;

// Test callback tracking
static uint16_t      s_lastAdcValue1        = 0;
static uint16_t      s_lastAdcValue2        = 0;
static uint16_t      s_lastAdcValue3        = 0;
static bool          s_callback1Invoked     = false;
static bool          s_callback2Invoked     = false;
static bool          s_callback3Invoked     = false;
static int           s_callbackCount        = 0;
static BspAdcError_e s_lastError            = eBSP_ADC_ERR_NONE;
static bool          s_errorCallbackInvoked = false;

// Test callbacks
static void TestAdcCallback1(uint16_t wValue)
{
    s_lastAdcValue1    = wValue;
    s_callback1Invoked = true;
    s_callbackCount++;
}

static void TestAdcCallback2(uint16_t wValue)
{
    s_lastAdcValue2    = wValue;
    s_callback2Invoked = true;
    s_callbackCount++;
}

static void TestAdcCallback3(uint16_t wValue)
{
    s_lastAdcValue3    = wValue;
    s_callback3Invoked = true;
    s_callbackCount++;
}

static void TestErrorCallback(BspAdcError_e eError)
{
    s_lastError            = eError;
    s_errorCallbackInvoked = true;
}

// ============================================================================
// Test Fixtures
// ============================================================================

// External access to module state for test reset
extern void BspAdcResetModuleForTest(void);

void setUp(void)
{
    // Reset callback tracking
    s_lastAdcValue1        = 0;
    s_lastAdcValue2        = 0;
    s_lastAdcValue3        = 0;
    s_callback1Invoked     = false;
    s_callback2Invoked     = false;
    s_callback3Invoked     = false;
    s_callbackCount        = 0;
    s_lastError            = eBSP_ADC_ERR_NONE;
    s_errorCallbackInvoked = false;

    // Reset module state
    BspAdcResetModuleForTest();
}

void tearDown(void)
{
    // Cleanup
}

// ============================================================================
// Test Cases: Channel Allocation
// ============================================================================

void test_BspAdcAllocateChannel_ValidParameters_ReturnsValidHandle(void)
{
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();

    BspAdcChannelHandle_t handle =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_3Cycles, TestAdcCallback1);

    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);
    TEST_ASSERT_LESS_THAN(16, handle);
}

void test_BspAdcAllocateChannel_InvalidInstance_ReturnsError(void)
{
    BspAdcChannelHandle_t handle =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_COUNT, eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_3Cycles, TestAdcCallback1);

    TEST_ASSERT_EQUAL_INT8(-1, handle);
}

void test_BspAdcAllocateChannel_DuplicateChannel_ReturnsError(void)
{
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle1 =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_5, eBSP_ADC_SampleTime_15Cycles, TestAdcCallback1);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle1);

    // Try to allocate same channel on same ADC - should detect duplicate before calling HAL
    BspAdcChannelHandle_t handle2 =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_5, eBSP_ADC_SampleTime_28Cycles, TestAdcCallback2);

    TEST_ASSERT_EQUAL_INT8(-1, handle2);
}

void test_BspAdcAllocateChannel_SameChannelDifferentADC_ReturnsValidHandles(void)
{
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle1 =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_3, eBSP_ADC_SampleTime_56Cycles, TestAdcCallback1);

    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc2, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle2 =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_2, eBSP_ADC_CHANNEL_3, eBSP_ADC_SampleTime_84Cycles, TestAdcCallback2);

    TEST_ASSERT_GREATER_OR_EQUAL(0, handle1);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle2);
    TEST_ASSERT_NOT_EQUAL(handle1, handle2);
}

void test_BspAdcAllocateChannel_ExhaustAllSlots_17thReturnsError(void)
{
    BspAdcChannelHandle_t handles[16];

    // Allocate all 16 slots
    for (int i = 0; i < 16; i++)
    {
        HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
        HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
        handles[i] = BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_0 + i, eBSP_ADC_SampleTime_3Cycles, NULL);
        TEST_ASSERT_GREATER_OR_EQUAL(0, handles[i]);
    }

    // 17th allocation should fail without calling HAL (no free slots)
    BspAdcChannelHandle_t overflow = BspAdcAllocateChannel(eBSP_ADC_INSTANCE_2, eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_15Cycles, NULL);
    TEST_ASSERT_EQUAL_INT8(-1, overflow);
}

void test_BspAdcAllocateChannel_HALConfigFails_ReturnsError(void)
{
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_ERROR);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();

    BspAdcChannelHandle_t handle =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_3Cycles, TestAdcCallback1);

    TEST_ASSERT_EQUAL_INT8(-1, handle);
}

// ============================================================================
// Test Cases: Free Channel
// ============================================================================

void test_BspAdcFreeChannel_ValidHandle_Succeeds(void)
{
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_7, eBSP_ADC_SampleTime_112Cycles, TestAdcCallback1);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Should not crash
    BspAdcFreeChannel(handle);
}

void test_BspAdcFreeChannel_InvalidHandle_DoesNothing(void)
{
    // Should not crash
    BspAdcFreeChannel(-1);
    BspAdcFreeChannel(16);
    BspAdcFreeChannel(100);
}

void test_BspAdcFreeChannel_AllowsReallocation(void)
{
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle1 =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_10, eBSP_ADC_SampleTime_144Cycles, TestAdcCallback1);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle1);

    // Free the channel
    BspAdcFreeChannel(handle1);

    // Should be able to allocate same channel again
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle2 =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_10, eBSP_ADC_SampleTime_480Cycles, TestAdcCallback2);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle2);
}

// ============================================================================
// Test Cases: Start/Stop
// ============================================================================

void test_BspAdcStart_ValidHandle_Works(void)
{
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_3Cycles, TestAdcCallback1);

    HAL_GetTick_ExpectAndReturn(0);
    BspAdcStart(handle, 500);

    // Timer should be active (tested via integration)
}

void test_BspAdcStart_InvalidHandle_DoesNothing(void)
{
    // Should not crash
    BspAdcStart(-1, 100);
    BspAdcStart(16, 200);
}

void test_BspAdcStart_MultipleChannelsIndependentTimers_Works(void)
{
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle1 =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_15Cycles, TestAdcCallback1);

    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc2, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle2 =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_2, eBSP_ADC_CHANNEL_5, eBSP_ADC_SampleTime_28Cycles, TestAdcCallback2);

    // Start both with different periods
    HAL_GetTick_ExpectAndReturn(0);
    BspAdcStart(handle1, 100);

    HAL_GetTick_ExpectAndReturn(0);
    BspAdcStart(handle2, 500);
}

void test_BspAdcStop_ValidHandle_Works(void)
{
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_3Cycles, TestAdcCallback1);

    HAL_GetTick_ExpectAndReturn(0);
    BspAdcStart(handle, 100);

    BspAdcStop(handle);
}

void test_BspAdcStop_InvalidHandle_DoesNothing(void)
{
    // Should not crash
    BspAdcStop(-1);
    BspAdcStop(16);
}

// ============================================================================
// Test Cases: Error Callback
// ============================================================================

void test_BspAdcRegisterErrorCallback_ValidHandle_Works(void)
{
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_56Cycles, TestAdcCallback1);

    BspAdcRegisterErrorCallback(handle, TestErrorCallback);

    // Callback registered (tested via error scenarios)
}

void test_BspAdcRegisterErrorCallback_InvalidHandle_DoesNothing(void)
{
    // Should not crash
    BspAdcRegisterErrorCallback(-1, TestErrorCallback);
    BspAdcRegisterErrorCallback(16, TestErrorCallback);
}

void test_BspAdcRegisterErrorCallback_NullCallback_Accepted(void)
{
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_84Cycles, TestAdcCallback1);

    BspAdcRegisterErrorCallback(handle, NULL);
    // Should not crash
}

// ============================================================================
// Test Cases: HAL Callback
// ============================================================================

void test_HAL_ADC_ConvCpltCallback_SingleChannel_InvokesCallback(void)
{
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_3Cycles, TestAdcCallback1);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Trigger HAL callback
    HAL_ADC_ConvCpltCallback(&hadc1);

    // Callback should be invoked
    TEST_ASSERT_TRUE(s_callback1Invoked);
}

void test_HAL_ADC_ConvCpltCallback_MultipleChannelsSameADC_InvokesAllCallbacks(void)
{
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle1 =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_15Cycles, TestAdcCallback1);

    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle2 =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_5, eBSP_ADC_SampleTime_28Cycles, TestAdcCallback2);

    TEST_ASSERT_GREATER_OR_EQUAL(0, handle1);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle2);

    // Trigger HAL callback for ADC1
    HAL_ADC_ConvCpltCallback(&hadc1);

    // Both callbacks should be invoked
    TEST_ASSERT_TRUE(s_callback1Invoked);
    TEST_ASSERT_TRUE(s_callback2Invoked);
}

void test_HAL_ADC_ConvCpltCallback_WrongADC_DoesNotInvokeCallback(void)
{
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_3Cycles, TestAdcCallback1);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Call with ADC2 instead of ADC1
    HAL_ADC_ConvCpltCallback(&hadc2);

    // Callback should NOT be invoked
    TEST_ASSERT_FALSE(s_callback1Invoked);
}

void test_HAL_ADC_ConvCpltCallback_NullCallback_DoesNotCrash(void)
{
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle = BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_3, eBSP_ADC_SampleTime_56Cycles, NULL);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Should not crash even with NULL callback
    HAL_ADC_ConvCpltCallback(&hadc1);
}

// ============================================================================
// Test Cases: Integration
// ============================================================================

void test_Integration_SingleChannel_CompleteWorkflow(void)
{
    // Allocate channel on ADC1
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_84Cycles, TestAdcCallback1);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Register error callback
    BspAdcRegisterErrorCallback(handle, TestErrorCallback);

    // Start periodic sampling
    HAL_GetTick_ExpectAndReturn(0);
    BspAdcStart(handle, 1000);

    // Simulate ADC conversion complete
    HAL_ADC_ConvCpltCallback(&hadc1);

    // Verify callback was invoked
    TEST_ASSERT_TRUE(s_callback1Invoked);

    // Stop sampling
    BspAdcStop(handle);

    // Free channel
    BspAdcFreeChannel(handle);
}

void test_Integration_MultipleChannelsDifferentADCs_IndependentOperation(void)
{
    // Allocate channel on ADC1
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle1 =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_2, eBSP_ADC_SampleTime_112Cycles, TestAdcCallback1);

    // Allocate channel on ADC2
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc2, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle2 =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_2, eBSP_ADC_CHANNEL_7, eBSP_ADC_SampleTime_144Cycles, TestAdcCallback2);

    // Allocate channel on ADC3
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc3, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t handle3 =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_3, eBSP_ADC_CHANNEL_12, eBSP_ADC_SampleTime_480Cycles, TestAdcCallback3);

    TEST_ASSERT_GREATER_OR_EQUAL(0, handle1);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle2);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle3);

    // Start all with different periods
    HAL_GetTick_ExpectAndReturn(0);
    BspAdcStart(handle1, 100);

    HAL_GetTick_ExpectAndReturn(0);
    BspAdcStart(handle2, 500);

    HAL_GetTick_ExpectAndReturn(0);
    BspAdcStart(handle3, 2000);

    // Trigger ADC1 - only callback1 should be invoked
    HAL_ADC_ConvCpltCallback(&hadc1);
    TEST_ASSERT_TRUE(s_callback1Invoked);
    TEST_ASSERT_FALSE(s_callback2Invoked);
    TEST_ASSERT_FALSE(s_callback3Invoked);

    // Reset flags
    s_callback1Invoked = false;
    s_callback2Invoked = false;
    s_callback3Invoked = false;

    // Trigger ADC2 - only callback2 should be invoked
    HAL_ADC_ConvCpltCallback(&hadc2);
    TEST_ASSERT_FALSE(s_callback1Invoked);
    TEST_ASSERT_TRUE(s_callback2Invoked);
    TEST_ASSERT_FALSE(s_callback3Invoked);

    // Reset flags
    s_callback1Invoked = false;
    s_callback2Invoked = false;
    s_callback3Invoked = false;

    // Trigger ADC3 - only callback3 should be invoked
    HAL_ADC_ConvCpltCallback(&hadc3);
    TEST_ASSERT_FALSE(s_callback1Invoked);
    TEST_ASSERT_FALSE(s_callback2Invoked);
    TEST_ASSERT_TRUE(s_callback3Invoked);

    // Stop all
    BspAdcStop(handle1);
    BspAdcStop(handle2);
    BspAdcStop(handle3);

    // Free all
    BspAdcFreeChannel(handle1);
    BspAdcFreeChannel(handle2);
    BspAdcFreeChannel(handle3);
}

void test_Integration_AllocateFreeReallocate_Works(void)
{
    // Allocate all 16 slots
    BspAdcChannelHandle_t handles[16];
    for (int i = 0; i < 16; i++)
    {
        HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
        HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
        handles[i] = BspAdcAllocateChannel(eBSP_ADC_INSTANCE_1, eBSP_ADC_CHANNEL_0 + i, eBSP_ADC_SampleTime_3Cycles, TestAdcCallback1);
        TEST_ASSERT_GREATER_OR_EQUAL(0, handles[i]);
    }

    // All slots full - next allocation should fail without calling HAL
    BspAdcChannelHandle_t overflow =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_2, eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_15Cycles, TestAdcCallback2);
    TEST_ASSERT_EQUAL_INT8(-1, overflow);

    // Free a few slots
    BspAdcFreeChannel(handles[5]);
    BspAdcFreeChannel(handles[10]);
    BspAdcFreeChannel(handles[15]);

    // Should be able to allocate again
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc2, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t newHandle1 =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_2, eBSP_ADC_CHANNEL_3, eBSP_ADC_SampleTime_28Cycles, TestAdcCallback2);
    TEST_ASSERT_GREATER_OR_EQUAL(0, newHandle1);

    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc3, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcChannelHandle_t newHandle2 =
        BspAdcAllocateChannel(eBSP_ADC_INSTANCE_3, eBSP_ADC_CHANNEL_8, eBSP_ADC_SampleTime_56Cycles, TestAdcCallback3);
    TEST_ASSERT_GREATER_OR_EQUAL(0, newHandle2);
}
