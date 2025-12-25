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
static bool          s_callback1Invoked     = false;
static bool          s_callback2Invoked     = false;
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

static void TestErrorCallback(BspAdcError_e eError)
{
    s_lastError            = eError;
    s_errorCallbackInvoked = true;
}

// ============================================================================
// Test Fixtures
// ============================================================================

void setUp(void)
{
    // Reset callback tracking
    s_lastAdcValue1        = 0;
    s_lastAdcValue2        = 0;
    s_callback1Invoked     = false;
    s_callback2Invoked     = false;
    s_callbackCount        = 0;
    s_lastError            = eBSP_ADC_ERR_NONE;
    s_errorCallbackInvoked = false;

    // Setup mock ADC handles
    hadc1.Init.NbrOfConversion = 3;
    hadc2.Init.NbrOfConversion = 2;
    hadc3.Init.NbrOfConversion = 4;
}

void tearDown(void)
{
    // Cleanup
}

// ============================================================================
// Test Cases: Initialization
// ============================================================================

void test_BspAdcInit_InvalidInstance_ReturnsFalse(void)
{
    bool result = BspAdcInit(eBSP_ADC_INSTANCE_COUNT);
    TEST_ASSERT_FALSE(result);
}

void test_BspAdcInit_ValidInstance_ADC1_ReturnsTrue(void)
{
    bool result = BspAdcInit(eBSP_ADC_INSTANCE_1);

    TEST_ASSERT_TRUE(result);
}

void test_BspAdcInit_ValidInstance_ADC2_ReturnsTrue(void)
{
    bool result = BspAdcInit(eBSP_ADC_INSTANCE_2);

    TEST_ASSERT_TRUE(result);
}

void test_BspAdcInit_ValidInstance_ADC3_ReturnsTrue(void)
{
    bool result = BspAdcInit(eBSP_ADC_INSTANCE_3);

    TEST_ASSERT_TRUE(result);
}

// ============================================================================
// Test Cases: Channel Registration
// ============================================================================

void test_BspAdcRegisterChannel_FirstChannel_Success(void)
{
    BspAdcInit(eBSP_ADC_INSTANCE_1);

    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();

    bool result = BspAdcRegisterChannel(eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_3Cycles, TestAdcCallback1);

    TEST_ASSERT_TRUE(result);
}

void test_BspAdcRegisterChannel_MultipleChannels_AllSucceed(void)
{
    BspAdcInit(eBSP_ADC_INSTANCE_1);

    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcRegisterChannel(eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_15Cycles, TestAdcCallback1);

    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcRegisterChannel(eBSP_ADC_CHANNEL_1, eBSP_ADC_SampleTime_28Cycles, TestAdcCallback2);

    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    bool result = BspAdcRegisterChannel(eBSP_ADC_CHANNEL_2, eBSP_ADC_SampleTime_56Cycles, NULL);

    TEST_ASSERT_TRUE(result);
}

void test_BspAdcRegisterChannel_ExceedsMaxChannels_ReturnsFalse(void)
{
    BspAdcInit(eBSP_ADC_INSTANCE_1);

    // Register up to max (3 channels for ADC1)
    for (int i = 0; i < 3; i++)
    {
        HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
        HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
        BspAdcRegisterChannel(eBSP_ADC_CHANNEL_0 + i, eBSP_ADC_SampleTime_3Cycles, NULL);
    }

    // Try to register one more - should fail
    bool result = BspAdcRegisterChannel(eBSP_ADC_CHANNEL_3, eBSP_ADC_SampleTime_3Cycles, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_BspAdcRegisterChannel_HALConfigFails_ReturnsFalse(void)
{
    BspAdcInit(eBSP_ADC_INSTANCE_1);

    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_ERROR);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();

    bool result = BspAdcRegisterChannel(eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_3Cycles, TestAdcCallback1);

    TEST_ASSERT_FALSE(result);
}

// ============================================================================
// Test Cases: Start/Stop
// ============================================================================

void test_BspAdcStart_AllChannelsRegistered_StartsTimer(void)
{
    BspAdcInit(eBSP_ADC_INSTANCE_1);

    // Register all 3 channels
    for (int i = 0; i < 3; i++)
    {
        HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
        HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
        BspAdcRegisterChannel(i, eBSP_ADC_SampleTime_3Cycles, TestAdcCallback1);
    }

    BspAdcStart(500);

    // Timer should now be active (tested via integration)
}

void test_BspAdcStart_NotAllChannelsRegistered_CallsErrorCallback(void)
{
    BspAdcInit(eBSP_ADC_INSTANCE_1);
    BspAdcRegisterErrorCallback(TestErrorCallback);

    // Register only 2 of 3 channels
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcRegisterChannel(eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_3Cycles, NULL);

    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcRegisterChannel(eBSP_ADC_CHANNEL_1, eBSP_ADC_SampleTime_3Cycles, NULL);

    BspAdcStart(100);

    TEST_ASSERT_TRUE(s_errorCallbackInvoked);
    TEST_ASSERT_EQUAL(eBSP_ADC_ERR_CONFIGURATION, s_lastError);
}

void test_BspAdcStop_Works(void)
{
    BspAdcInit(eBSP_ADC_INSTANCE_1);

    // Register channels and start
    for (int i = 0; i < 3; i++)
    {
        HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
        HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
        BspAdcRegisterChannel(i, eBSP_ADC_SampleTime_3Cycles, NULL);
    }
    BspAdcStart(100);

    // Stop should work without error
    BspAdcStop();
}

// ============================================================================
// Test Cases: Error Callback
// ============================================================================

void test_BspAdcRegisterErrorCallback_Works(void)
{
    BspAdcRegisterErrorCallback(TestErrorCallback);

    // Callback should be registered (tested via error scenarios)
}

void test_BspAdcRegisterErrorCallback_NullCallback_Accepted(void)
{
    BspAdcRegisterErrorCallback(NULL);

    // Should not crash
}

// ============================================================================
// Test Cases: HAL Callback
// ============================================================================

void test_HAL_ADC_ConvCpltCallback_WithADC1_CallsCallbacks(void)
{
    BspAdcInit(eBSP_ADC_INSTANCE_1);

    // Register 2 channels with callbacks
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcRegisterChannel(eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_3Cycles, TestAdcCallback1);

    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcRegisterChannel(eBSP_ADC_CHANNEL_1, eBSP_ADC_SampleTime_3Cycles, TestAdcCallback2);

    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcRegisterChannel(eBSP_ADC_CHANNEL_2, eBSP_ADC_SampleTime_3Cycles, NULL);

    // Need to simulate the DMA result data getting filled
    // Since we can't access internal arrays, we trigger a callback and see if it works

    HAL_ADC_ConvCpltCallback(&hadc1);

    // Callbacks should have been invoked
    TEST_ASSERT_TRUE(s_callback1Invoked);
    TEST_ASSERT_TRUE(s_callback2Invoked);
}

void test_HAL_ADC_ConvCpltCallback_WrongADC_DoesNothing(void)
{
    BspAdcInit(eBSP_ADC_INSTANCE_1);

    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    BspAdcRegisterChannel(eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_3Cycles, TestAdcCallback1);

    // Call with ADC2 instead of ADC1
    HAL_ADC_ConvCpltCallback(&hadc2);

    // Callback should NOT be invoked since wrong ADC
    TEST_ASSERT_FALSE(s_callback1Invoked);
}

// ============================================================================
// Test Cases: Integration
// ============================================================================

void test_Integration_ADC1_CompleteWorkflow(void)
{
    // Initialize ADC1
    bool initResult = BspAdcInit(eBSP_ADC_INSTANCE_1);
    TEST_ASSERT_TRUE(initResult);

    // Register error callback
    BspAdcRegisterErrorCallback(TestErrorCallback);

    // Register channels
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    bool reg1 = BspAdcRegisterChannel(eBSP_ADC_CHANNEL_0, eBSP_ADC_SampleTime_84Cycles, TestAdcCallback1);
    TEST_ASSERT_TRUE(reg1);

    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    bool reg2 = BspAdcRegisterChannel(eBSP_ADC_CHANNEL_5, eBSP_ADC_SampleTime_112Cycles, TestAdcCallback2);
    TEST_ASSERT_TRUE(reg2);

    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc1, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    bool reg3 = BspAdcRegisterChannel(eBSP_ADC_CHANNEL_10, eBSP_ADC_SampleTime_144Cycles, NULL);
    TEST_ASSERT_TRUE(reg3);

    // Start periodic sampling
    BspAdcStart(1000);

    // Simulate ADC conversion complete
    HAL_ADC_ConvCpltCallback(&hadc1);

    // Verify callbacks were invoked
    TEST_ASSERT_TRUE(s_callback1Invoked);
    TEST_ASSERT_TRUE(s_callback2Invoked);

    // Stop sampling
    BspAdcStop();
}

void test_Integration_ADC2_CompleteWorkflow(void)
{
    // Initialize ADC2
    bool initResult = BspAdcInit(eBSP_ADC_INSTANCE_2);
    TEST_ASSERT_TRUE(initResult);

    // Register 2 channels for ADC2
    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc2, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    bool reg1 = BspAdcRegisterChannel(eBSP_ADC_CHANNEL_1, eBSP_ADC_SampleTime_56Cycles, TestAdcCallback1);
    TEST_ASSERT_TRUE(reg1);

    HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc2, NULL, HAL_OK);
    HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
    bool reg2 = BspAdcRegisterChannel(eBSP_ADC_CHANNEL_2, eBSP_ADC_SampleTime_84Cycles, TestAdcCallback2);
    TEST_ASSERT_TRUE(reg2);

    // Start and trigger conversion
    BspAdcStart(500);
    HAL_ADC_ConvCpltCallback(&hadc2);

    TEST_ASSERT_TRUE(s_callback1Invoked);
    TEST_ASSERT_TRUE(s_callback2Invoked);

    BspAdcStop();
}

void test_Integration_ADC3_CompleteWorkflow(void)
{
    // Initialize ADC3
    bool initResult = BspAdcInit(eBSP_ADC_INSTANCE_3);
    TEST_ASSERT_TRUE(initResult);

    // Register all 4 channels for ADC3
    for (int i = 0; i < 4; i++)
    {
        HAL_ADC_ConfigChannel_ExpectAndReturn(&hadc3, NULL, HAL_OK);
        HAL_ADC_ConfigChannel_IgnoreArg_sConfig();
        BspAdcRegisterChannel(i, eBSP_ADC_SampleTime_480Cycles, (i < 2) ? TestAdcCallback1 : NULL);
    }

    BspAdcStart(2000);
    HAL_ADC_ConvCpltCallback(&hadc3);

    BspAdcStop();
}
