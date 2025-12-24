/**
 * @file ut_bsp_gpio.c
 * @brief Unit tests for BSP GPIO module using Unity and CMock
 * @note This test file mocks HAL layer functions to test BSP GPIO functionality
 */

#include "Mockstm32f4xx_hal_cortex.h"
#include "Mockstm32f4xx_hal_gpio.h"
#include "bsp_gpio.h"
#include "gpio_structs/gpio_struct.h"
#include "unity.h"

// Compile-time check that test version of gpio_struct.h is being used
#ifndef GPIO_STRUCT_TEST_VERSION
    #error "Production gpio_struct.h is being used instead of test version!"
#endif

// Verify eGPIO_COUNT is defined correctly
#if eGPIO_COUNT != 6
    #error "eGPIO_COUNT is not 6!"
#endif

// External declaration for HAL callback implemented in production code
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

// Mock GPIO port addresses for testing
static GPIO_TypeDef mock_GPIOA;
static GPIO_TypeDef mock_GPIOB;
static GPIO_TypeDef mock_GPIOC;

// Define test GPIO pins array (mirrors what would be in gpio_struct.c)
const gpio_t gpio_pins[eGPIO_COUNT] = {
    [0] = {&mock_GPIOA, GPIO_PIN_0},  // eGPIO index 0
    [1] = {&mock_GPIOA, GPIO_PIN_1},  // eGPIO index 1
    [2] = {&mock_GPIOA, GPIO_PIN_5},  // eGPIO index 2
    [3] = {&mock_GPIOB, GPIO_PIN_7},  // eGPIO index 3
    [4] = {&mock_GPIOC, GPIO_PIN_13}, // eGPIO index 4
    [5] = {NULL, GPIO_PIN_0},         // eGPIO index 5 - NULL port for testing
};

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

    // Clear all registered callbacks in production code
    for (uint32_t i = 0; i < eGPIO_COUNT; i++)
    {
        BspGpioSetIRQHandler(i, NULL);
    }
}

void tearDown(void)
{
    // Clean up after each test
}

// ============================================================================
// BspGpioWritePin Tests
// ============================================================================

void test_BspGpioWritePin_SetHigh_ValidPin(void)
{
    // Arrange
    uint32_t pin_index = 0;

    // Expect HAL function to be called with GPIO_PIN_SET
    HAL_GPIO_WritePin_Expect(&mock_GPIOA, GPIO_PIN_0, GPIO_PIN_SET);

    // Act
    BspGpioWritePin(pin_index, true);

    // Assert - handled by CMock expectations
}

void test_BspGpioWritePin_SetLow_ValidPin(void)
{
    // Arrange
    uint32_t pin_index = 1;

    // Expect HAL function to be called with GPIO_PIN_RESET
    HAL_GPIO_WritePin_Expect(&mock_GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

    // Act
    BspGpioWritePin(pin_index, false);

    // Assert - handled by CMock expectations
}

void test_BspGpioWritePin_InvalidIndex_OutOfBounds(void)
{
    // Arrange
    uint32_t invalid_index = eGPIO_COUNT + 1;

    // Expect NO HAL function calls
    // CMock will fail if HAL_GPIO_WritePin is called

    // Act
    BspGpioWritePin(invalid_index, true);

    // Assert - no HAL function should be called
}

void test_BspGpioWritePin_NullPort_NoAction(void)
{
    // Arrange
    uint32_t null_port_index = 5; // Index with NULL port

    // Expect NO HAL function calls

    // Act
    BspGpioWritePin(null_port_index, true);

    // Assert - no HAL function should be called
}

void test_BspGpioWritePin_MultipleValidPins(void)
{
    // Test multiple pins in sequence
    HAL_GPIO_WritePin_Expect(&mock_GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    BspGpioWritePin(0, true);

    HAL_GPIO_WritePin_Expect(&mock_GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
    BspGpioWritePin(3, false);

    HAL_GPIO_WritePin_Expect(&mock_GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    BspGpioWritePin(4, true);
}

// ============================================================================
// BspGpioTogglePin Tests
// ============================================================================

void test_BspGpioTogglePin_ValidPin(void)
{
    // Arrange
    uint32_t pin_index = 2;

    // Expect HAL toggle function
    HAL_GPIO_TogglePin_Expect(&mock_GPIOA, GPIO_PIN_5);

    // Act
    BspGpioTogglePin(pin_index);

    // Assert - handled by CMock expectations
}

void test_BspGpioTogglePin_InvalidIndex_OutOfBounds(void)
{
    // Arrange
    uint32_t invalid_index = eGPIO_COUNT;

    // Expect NO HAL function calls

    // Act
    BspGpioTogglePin(invalid_index);

    // Assert - no HAL function should be called
}

void test_BspGpioTogglePin_NullPort_NoAction(void)
{
    // Arrange
    uint32_t null_port_index = 5;

    // Expect NO HAL function calls

    // Act
    BspGpioTogglePin(null_port_index);

    // Assert - no HAL function should be called
}

void test_BspGpioTogglePin_MultiplePins(void)
{
    // Test toggling multiple pins
    HAL_GPIO_TogglePin_Expect(&mock_GPIOA, GPIO_PIN_0);
    BspGpioTogglePin(0);

    HAL_GPIO_TogglePin_Expect(&mock_GPIOA, GPIO_PIN_1);
    BspGpioTogglePin(1);

    HAL_GPIO_TogglePin_Expect(&mock_GPIOB, GPIO_PIN_7);
    BspGpioTogglePin(3);
}

// ============================================================================
// BspGpioReadPin Tests
// ============================================================================

void test_BspGpioReadPin_ReturnsHigh(void)
{
    // Arrange
    uint32_t pin_index = 0;

    // Expect HAL function and return GPIO_PIN_SET
    HAL_GPIO_ReadPin_ExpectAndReturn(&mock_GPIOA, GPIO_PIN_0, GPIO_PIN_SET);

    // Act
    bool result = BspGpioReadPin(pin_index);

    // Assert
    TEST_ASSERT_TRUE(result);
}

void test_BspGpioReadPin_ReturnsLow(void)
{
    // Arrange
    uint32_t pin_index = 1;

    // Expect HAL function and return GPIO_PIN_RESET
    HAL_GPIO_ReadPin_ExpectAndReturn(&mock_GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

    // Act
    bool result = BspGpioReadPin(pin_index);

    // Assert
    TEST_ASSERT_FALSE(result);
}

void test_BspGpioReadPin_InvalidIndex_ReturnsFalse(void)
{
    // Arrange
    uint32_t invalid_index = eGPIO_COUNT + 5;

    // Expect NO HAL function calls

    // Act
    bool result = BspGpioReadPin(invalid_index);

    // Assert
    TEST_ASSERT_FALSE(result);
}

void test_BspGpioReadPin_NullPort_ReturnsFalse(void)
{
    // Arrange
    uint32_t null_port_index = 5;

    // Expect NO HAL function calls

    // Act
    bool result = BspGpioReadPin(null_port_index);

    // Assert
    TEST_ASSERT_FALSE(result);
}

void test_BspGpioReadPin_MultiplePins_DifferentStates(void)
{
    // Test reading multiple pins with different states
    HAL_GPIO_ReadPin_ExpectAndReturn(&mock_GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    TEST_ASSERT_TRUE(BspGpioReadPin(0));

    HAL_GPIO_ReadPin_ExpectAndReturn(&mock_GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    TEST_ASSERT_FALSE(BspGpioReadPin(1));

    HAL_GPIO_ReadPin_ExpectAndReturn(&mock_GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
    TEST_ASSERT_TRUE(BspGpioReadPin(3));
}

// ============================================================================
// BspGpioSetIRQHandler Tests
// ============================================================================

void test_BspGpioSetIRQHandler_ValidPin_ValidCallback(void)
{
    // Arrange
    uint32_t pin_index = 0;

    // Act
    BspGpioSetIRQHandler(pin_index, test_callback);

    // Assert - callback is stored internally (verified by callback test later)
    // No HAL calls expected for this function
}

void test_BspGpioSetIRQHandler_ValidPin_NullCallback(void)
{
    // Arrange
    uint32_t pin_index = 1;

    // Act - should accept NULL callback
    BspGpioSetIRQHandler(pin_index, NULL);

    // Assert - no crash, no HAL calls
}

void test_BspGpioSetIRQHandler_InvalidIndex_NoAction(void)
{
    // Arrange
    uint32_t invalid_index = eGPIO_COUNT;

    // Act
    BspGpioSetIRQHandler(invalid_index, test_callback);

    // Assert - function should return early, no crash
}

void test_BspGpioSetIRQHandler_NullPort_NoAction(void)
{
    // Arrange
    uint32_t null_port_index = 5;

    // Act
    BspGpioSetIRQHandler(null_port_index, test_callback);

    // Assert - function should return early, no crash
}

void test_BspGpioSetIRQHandler_OverwriteCallback(void)
{
    // Arrange
    uint32_t pin_index = 2;

    // Act - set callback twice
    BspGpioSetIRQHandler(pin_index, test_callback);
    BspGpioSetIRQHandler(pin_index, NULL);

    // Assert - second call should overwrite first
}

// ============================================================================
// BspGpioEnableIRQ Tests
// ============================================================================

void test_BspGpioEnableIRQ_Pin0_EnablesEXTI0(void)
{
    // Arrange
    uint32_t pin_index = 0; // GPIO_PIN_0

    // Expect NVIC enable for EXTI0
    HAL_NVIC_EnableIRQ_Expect(EXTI0_IRQn);

    // Act
    BspGpioEnableIRQ(pin_index);

    // Assert - handled by CMock expectations
}

void test_BspGpioEnableIRQ_Pin1_EnablesEXTI1(void)
{
    // Arrange
    uint32_t pin_index = 1; // GPIO_PIN_1

    // Expect NVIC enable for EXTI1
    HAL_NVIC_EnableIRQ_Expect(EXTI1_IRQn);

    // Act
    BspGpioEnableIRQ(pin_index);

    // Assert - handled by CMock expectations
}

void test_BspGpioEnableIRQ_Pin5_EnablesEXTI9_5(void)
{
    // Arrange
    uint32_t pin_index = 2; // GPIO_PIN_5

    // Expect NVIC enable for EXTI9_5
    HAL_NVIC_EnableIRQ_Expect(EXTI9_5_IRQn);

    // Act
    BspGpioEnableIRQ(pin_index);

    // Assert - handled by CMock expectations
}

void test_BspGpioEnableIRQ_Pin7_EnablesEXTI9_5(void)
{
    // Arrange
    uint32_t pin_index = 3; // GPIO_PIN_7

    // Expect NVIC enable for EXTI9_5
    HAL_NVIC_EnableIRQ_Expect(EXTI9_5_IRQn);

    // Act
    BspGpioEnableIRQ(pin_index);

    // Assert - handled by CMock expectations
}

void test_BspGpioEnableIRQ_Pin13_EnablesEXTI15_10(void)
{
    // Arrange
    uint32_t pin_index = 4; // GPIO_PIN_13

    // Expect NVIC enable for EXTI15_10
    HAL_NVIC_EnableIRQ_Expect(EXTI15_10_IRQn);

    // Act
    BspGpioEnableIRQ(pin_index);

    // Assert - handled by CMock expectations
}

void test_BspGpioEnableIRQ_InvalidIndex_NoAction(void)
{
    // Arrange
    uint32_t invalid_index = eGPIO_COUNT + 1;

    // Expect NO NVIC enable calls

    // Act
    BspGpioEnableIRQ(invalid_index);

    // Assert - no HAL function should be called
}

// ============================================================================
// HAL_GPIO_EXTI_Callback Tests
// ============================================================================

void test_HAL_GPIO_EXTI_Callback_ValidPin_CallbackRegistered(void)
{
    // Arrange
    uint32_t pin_index = 0;
    BspGpioSetIRQHandler(pin_index, test_callback);

    // Act
    HAL_GPIO_EXTI_Callback(GPIO_PIN_0);

    // Assert
    TEST_ASSERT_TRUE(callback_invoked);
    TEST_ASSERT_EQUAL(1, callback_count);
}

void test_HAL_GPIO_EXTI_Callback_ValidPin_NoCallbackRegistered(void)
{
    // Arrange - no callback registered for pin

    // Act
    HAL_GPIO_EXTI_Callback(GPIO_PIN_0);

    // Assert - should not crash
    TEST_ASSERT_FALSE(callback_invoked);
    TEST_ASSERT_EQUAL(0, callback_count);
}

void test_HAL_GPIO_EXTI_Callback_MultiplePins_DifferentCallbacks(void)
{
    // Arrange
    callback2_invoked = false;

    BspGpioSetIRQHandler(0, test_callback);
    BspGpioSetIRQHandler(1, test_callback2);

    // Act - trigger first pin
    HAL_GPIO_EXTI_Callback(GPIO_PIN_0);

    // Assert
    TEST_ASSERT_TRUE(callback_invoked);
    TEST_ASSERT_FALSE(callback2_invoked);

    // Act - trigger second pin
    HAL_GPIO_EXTI_Callback(GPIO_PIN_1);

    // Assert
    TEST_ASSERT_TRUE(callback2_invoked);
}

void test_HAL_GPIO_EXTI_Callback_UnknownPin_NoAction(void)
{
    // Arrange
    uint16_t unknown_pin = 0xFFFF; // Pin not in gpio_pins array

    // Act
    HAL_GPIO_EXTI_Callback(unknown_pin);

    // Assert - should not crash
    TEST_ASSERT_FALSE(callback_invoked);
}

void test_HAL_GPIO_EXTI_Callback_CallbackInvokedMultipleTimes(void)
{
    // Arrange
    BspGpioSetIRQHandler(0, test_callback);

    // Act - trigger callback multiple times
    HAL_GPIO_EXTI_Callback(GPIO_PIN_0);
    HAL_GPIO_EXTI_Callback(GPIO_PIN_0);
    HAL_GPIO_EXTI_Callback(GPIO_PIN_0);

    // Assert
    TEST_ASSERT_TRUE(callback_invoked);
    TEST_ASSERT_EQUAL(3, callback_count);
}

// ============================================================================
// Edge Cases and Boundary Tests
// ============================================================================

void test_BoundaryConditions_IndexAtLimit(void)
{
    // Test with index at eGPIO_COUNT boundary
    uint32_t boundary_index = eGPIO_COUNT;

    // All functions should handle this gracefully
    BspGpioWritePin(boundary_index, true);
    BspGpioTogglePin(boundary_index);
    bool result = BspGpioReadPin(boundary_index);
    TEST_ASSERT_FALSE(result);
    BspGpioSetIRQHandler(boundary_index, test_callback);
    BspGpioEnableIRQ(boundary_index);

    // No crashes expected
}

void test_BoundaryConditions_IndexJustBelowLimit(void)
{
    // Create a test pin at the last valid index
    // Note: In real implementation, this would use actual gpio_pins[eGPIO_COUNT-1]
    // For this test, we use index 4 which is within bounds
    uint32_t last_valid_index = 4;

    // Should work normally
    HAL_GPIO_WritePin_Expect(&mock_GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    BspGpioWritePin(last_valid_index, true);
}

void test_Integration_CompleteWorkflow(void)
{
    // Arrange - simulate a complete GPIO workflow
    uint32_t pin_index = 0;

    // Setup callback
    BspGpioSetIRQHandler(pin_index, test_callback);

    // Enable IRQ
    HAL_NVIC_EnableIRQ_Expect(EXTI0_IRQn);
    BspGpioEnableIRQ(pin_index);

    // Write pin high
    HAL_GPIO_WritePin_Expect(&mock_GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    BspGpioWritePin(pin_index, true);

    // Read pin state
    HAL_GPIO_ReadPin_ExpectAndReturn(&mock_GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    TEST_ASSERT_TRUE(BspGpioReadPin(pin_index));

    // Toggle pin
    HAL_GPIO_TogglePin_Expect(&mock_GPIOA, GPIO_PIN_0);
    BspGpioTogglePin(pin_index);

    // Trigger interrupt
    HAL_GPIO_EXTI_Callback(GPIO_PIN_0);
    TEST_ASSERT_TRUE(callback_invoked);
}

void test_StressTest_RapidSuccessiveOperations(void)
{
    // Test rapid successive operations on same pin
    uint32_t pin_index = 1;

    for (int i = 0; i < 5; i++)
    {
        HAL_GPIO_WritePin_Expect(&mock_GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
        BspGpioWritePin(pin_index, true);

        HAL_GPIO_TogglePin_Expect(&mock_GPIOA, GPIO_PIN_1);
        BspGpioTogglePin(pin_index);

        HAL_GPIO_ReadPin_ExpectAndReturn(&mock_GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
        BspGpioReadPin(pin_index);
    }
}
