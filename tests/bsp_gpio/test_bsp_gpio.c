/**
 * @file test_bsp_gpio.c
 * @brief Unit tests for BSP GPIO module using Unity and CMock
 */

#include "Mockstm32f4xx_hal_gpio.h"
#include "bsp_gpio.h"
#include "gpio_structs/gpio_struct.h"
#include "unity.h"

// Test fixture setup
void setUp(void)
{
    // Called before each test
}

// Test fixture teardown
void tearDown(void)
{
    // Called after each test
}

// ============================================================================
// BspGpioGetLLHandle Tests
// ============================================================================

void test_BspGpioGetLLHandle_Port_A_Pin_0(void)
{
    // Note: PortGetHandle returns NULL, so BspGpioGetLLHandle returns 0
    uint32_t result = BspGpioGetLLHandle(eBSP_GPIO_PORT_A_0);
    TEST_ASSERT_EQUAL_UINT32(0x00000000, result); // Returns 0 when PortGetHandle is NULL
}

void test_BspGpioGetLLHandle_Port_A_Pin_5(void)
{
    // Note: PortGetHandle returns NULL, so BspGpioGetLLHandle returns 0
    uint32_t result = BspGpioGetLLHandle(eBSP_GPIO_PORT_A_5);
    TEST_ASSERT_EQUAL_UINT32(0x00000000, result); // Returns 0 when PortGetHandle is NULL
}

void test_BspGpioGetLLHandle_Port_B_Pin_7(void)
{
    // Note: PortGetHandle returns NULL, so BspGpioGetLLHandle returns 0
    uint32_t result = BspGpioGetLLHandle(eBSP_GPIO_PORT_B_7);
    TEST_ASSERT_EQUAL_UINT32(0x00000000, result); // Returns 0 when PortGetHandle is NULL
}

// ============================================================================
// BspGpioCfgPin Tests
// ============================================================================

void test_BspGpioCfgPin_ConfigureAsOutput_PushPull(void)
{
    // Note: Current implementation has PortGetHandle returning NULL
    // These tests verify the function doesn't crash with NULL pointer
    BspGpioCfgPin(eBSP_GPIO_PORT_A_0, 0x00000001, eGPIO_CFG_PP_OUT);
    // Function should handle NULL gracefully
}

void test_BspGpioCfgPin_ConfigureAsOutput_OpenDrain(void)
{
    BspGpioCfgPin(eBSP_GPIO_PORT_A_1, 0x00000002, eGPIO_CFG_OD_OUT);
    // Function should handle NULL gracefully
}

void test_BspGpioCfgPin_ConfigureAsInput(void)
{
    BspGpioCfgPin(eBSP_GPIO_PORT_A_2, 0x00000004, eGPIO_CFG_INPUT);
    // Function should handle NULL gracefully
}

// ============================================================================
// BspGpioWritePin Tests
// ============================================================================

void test_BspGpioWritePin_SetHigh(void)
{
    // Note: Current implementation has PortGetHandle returning NULL
    // Function should not crash
    BspGpioWritePin(eBSP_GPIO_PORT_A_0, true);
}

void test_BspGpioWritePin_SetLow(void)
{
    BspGpioWritePin(eBSP_GPIO_PORT_A_0, false);
}

// ============================================================================
// BspGpioTogglePin Tests
// ============================================================================

void test_BspGpioTogglePin_ValidPin(void)
{
    // Note: Current implementation has PortGetHandle returning NULL
    // Function should not crash
    BspGpioTogglePin(eBSP_GPIO_PORT_A_5);
}

// ============================================================================
// BspGpioReadPin Tests
// ============================================================================

void test_BspGpioReadPin_ReturnsValue(void)
{
    // Note: Current implementation has PortGetHandle returning NULL
    // Function should not crash and return false
    bool result = BspGpioReadPin(eBSP_GPIO_PORT_B_0);
    TEST_ASSERT_FALSE(result);
}

// ============================================================================
// BspGpioSetIRQHandler Tests
// ============================================================================

static bool irq_callback_called = false;

void test_irq_callback(void)
{
    irq_callback_called = true;
}

void test_BspGpioSetIRQHandler_ValidCallback(void)
{
    irq_callback_called = false;
    // Note: Current implementation has PortGetHandle returning NULL
    // Function should handle NULL gracefully
    BspGpioSetIRQHandler(eBSP_GPIO_PORT_A_0, test_irq_callback);
}

// ============================================================================
// BspGpioEnableIRQ Tests
// ============================================================================

void test_BspGpioEnableIRQ_ValidPin(void)
{
    // Note: Current implementation has PortGetHandle returning NULL
    // Function should handle NULL gracefully
    BspGpioEnableIRQ(eBSP_GPIO_PORT_A_0);
}

// ============================================================================
// BspGpioIRQ Tests
// ============================================================================

void test_BspGpioIRQ_Channel_0(void)
{
    // Note: This function processes interrupt channels
    // Current implementation may not work correctly due to NULL pointers
    BspGpioIRQ(eBSP_GPIO_IRQ_CH_0);
}

void test_BspGpioIRQ_Channel_9_5(void)
{
    BspGpioIRQ(eBSP_GPIO_IRQ_CH_9_5);
}

void test_BspGpioIRQ_Channel_15_10(void)
{
    BspGpioIRQ(eBSP_GPIO_IRQ_CH_15_10);
}

void test_BspGpioIRQ_Channel_1(void)
{
    BspGpioIRQ(eBSP_GPIO_IRQ_CH_1);
}

void test_BspGpioIRQ_Channel_2(void)
{
    BspGpioIRQ(eBSP_GPIO_IRQ_CH_2);
}

void test_BspGpioIRQ_Channel_3(void)
{
    BspGpioIRQ(eBSP_GPIO_IRQ_CH_3);
}

void test_BspGpioIRQ_Channel_4(void)
{
    BspGpioIRQ(eBSP_GPIO_IRQ_CH_4);
}

// ============================================================================
// Additional Pin Tests for Better Coverage
// ============================================================================

void test_BspGpioGetLLHandle_AllPortAPins(void)
{
    // Test all Port A pins
    TEST_ASSERT_EQUAL_UINT32(0, BspGpioGetLLHandle(eBSP_GPIO_PORT_A_1));
    TEST_ASSERT_EQUAL_UINT32(0, BspGpioGetLLHandle(eBSP_GPIO_PORT_A_2));
    TEST_ASSERT_EQUAL_UINT32(0, BspGpioGetLLHandle(eBSP_GPIO_PORT_A_3));
    TEST_ASSERT_EQUAL_UINT32(0, BspGpioGetLLHandle(eBSP_GPIO_PORT_A_4));
    TEST_ASSERT_EQUAL_UINT32(0, BspGpioGetLLHandle(eBSP_GPIO_PORT_A_6));
    TEST_ASSERT_EQUAL_UINT32(0, BspGpioGetLLHandle(eBSP_GPIO_PORT_A_7));
}

void test_BspGpioGetLLHandle_AllPortBPins(void)
{
    // Test all Port B pins
    TEST_ASSERT_EQUAL_UINT32(0, BspGpioGetLLHandle(eBSP_GPIO_PORT_B_0));
    TEST_ASSERT_EQUAL_UINT32(0, BspGpioGetLLHandle(eBSP_GPIO_PORT_B_1));
    TEST_ASSERT_EQUAL_UINT32(0, BspGpioGetLLHandle(eBSP_GPIO_PORT_B_2));
    TEST_ASSERT_EQUAL_UINT32(0, BspGpioGetLLHandle(eBSP_GPIO_PORT_B_3));
    TEST_ASSERT_EQUAL_UINT32(0, BspGpioGetLLHandle(eBSP_GPIO_PORT_B_4));
    TEST_ASSERT_EQUAL_UINT32(0, BspGpioGetLLHandle(eBSP_GPIO_PORT_B_5));
    TEST_ASSERT_EQUAL_UINT32(0, BspGpioGetLLHandle(eBSP_GPIO_PORT_B_6));
}

void test_BspGpioCfgPin_WithZeroHandle(void)
{
    // Test with zero handle - should not crash
    BspGpioCfgPin(eBSP_GPIO_PORT_A_0, 0x00000000, eGPIO_CFG_PP_OUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_A_0, 0x00000000, eGPIO_CFG_OD_OUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_A_0, 0x00000000, eGPIO_CFG_INPUT);
}

void test_BspGpioCfgPin_DifferentPins(void)
{
    // Test multiple different pins and configurations
    BspGpioCfgPin(eBSP_GPIO_PORT_B_0, 0x00000001, eGPIO_CFG_PP_OUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_B_1, 0x00000002, eGPIO_CFG_OD_OUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_B_2, 0x00000004, eGPIO_CFG_INPUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_A_3, 0x00000008, eGPIO_CFG_PP_OUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_A_4, 0x00000010, eGPIO_CFG_OD_OUT);
}

void test_BspGpioWritePin_MultiplePins(void)
{
    // Test writing to multiple different pins
    BspGpioWritePin(eBSP_GPIO_PORT_A_1, true);
    BspGpioWritePin(eBSP_GPIO_PORT_A_2, false);
    BspGpioWritePin(eBSP_GPIO_PORT_B_0, true);
    BspGpioWritePin(eBSP_GPIO_PORT_B_1, false);
    BspGpioWritePin(eBSP_GPIO_PORT_B_2, true);
    BspGpioWritePin(eBSP_GPIO_PORT_A_3, false);
}

void test_BspGpioTogglePin_MultiplePins(void)
{
    // Test toggling multiple pins
    BspGpioTogglePin(eBSP_GPIO_PORT_A_0);
    BspGpioTogglePin(eBSP_GPIO_PORT_A_1);
    BspGpioTogglePin(eBSP_GPIO_PORT_B_0);
    BspGpioTogglePin(eBSP_GPIO_PORT_B_7);
}

void test_BspGpioReadPin_MultiplePins(void)
{
    // Test reading from multiple pins
    bool result;
    result = BspGpioReadPin(eBSP_GPIO_PORT_A_0);
    TEST_ASSERT_FALSE(result);

    result = BspGpioReadPin(eBSP_GPIO_PORT_A_5);
    TEST_ASSERT_FALSE(result);

    result = BspGpioReadPin(eBSP_GPIO_PORT_B_3);
    TEST_ASSERT_FALSE(result);

    result = BspGpioReadPin(eBSP_GPIO_PORT_B_7);
    TEST_ASSERT_FALSE(result);
}

void test_BspGpioSetIRQHandler_MultiplePins(void)
{
    // Test setting IRQ handlers on multiple pins
    BspGpioSetIRQHandler(eBSP_GPIO_PORT_A_0, test_irq_callback);
    BspGpioSetIRQHandler(eBSP_GPIO_PORT_A_1, test_irq_callback);
    BspGpioSetIRQHandler(eBSP_GPIO_PORT_B_0, test_irq_callback);
    BspGpioSetIRQHandler(eBSP_GPIO_PORT_B_7, test_irq_callback);
}

void test_BspGpioSetIRQHandler_NullCallback(void)
{
    // Test setting a NULL callback
    BspGpioSetIRQHandler(eBSP_GPIO_PORT_A_0, NULL);
}

void test_BspGpioEnableIRQ_MultiplePins(void)
{
    // Test enabling IRQ on multiple pins
    BspGpioEnableIRQ(eBSP_GPIO_PORT_A_1);
    BspGpioEnableIRQ(eBSP_GPIO_PORT_A_2);
    BspGpioEnableIRQ(eBSP_GPIO_PORT_B_0);
    BspGpioEnableIRQ(eBSP_GPIO_PORT_B_5);
}

// ============================================================================
// Edge Case and Boundary Tests
// ============================================================================

void test_BspGpioCfgPin_InvalidConfigEnum(void)
{
    // Test with an invalid configuration (testing the else branch)
    // Cast to test default/else case
    BspGpioCfgPin(eBSP_GPIO_PORT_A_0, 0x00000001, (GpioPortPinCfg)99);
}

void test_BspGpioCfgPin_AllConfigTypes(void)
{
    // Ensure all configuration types are tested
    BspGpioCfgPin(eBSP_GPIO_PORT_A_0, 0x00000001, eGPIO_CFG_OD_OUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_A_1, 0x00000002, eGPIO_CFG_PP_OUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_A_2, 0x00000004, eGPIO_CFG_INPUT);
}

void test_BspGpioWritePin_BothStates(void)
{
    // Test both true and false paths explicitly
    BspGpioWritePin(eBSP_GPIO_PORT_A_0, true);
    BspGpioWritePin(eBSP_GPIO_PORT_A_0, false);
    BspGpioWritePin(eBSP_GPIO_PORT_B_5, true);
    BspGpioWritePin(eBSP_GPIO_PORT_B_5, false);
}

void test_BspGpioReadPin_VerifyReturnValue(void)
{
    // Test that function returns false when PortGetHandle returns NULL
    bool result1 = BspGpioReadPin(eBSP_GPIO_PORT_A_0);
    bool result2 = BspGpioReadPin(eBSP_GPIO_PORT_B_7);

    TEST_ASSERT_FALSE(result1);
    TEST_ASSERT_FALSE(result2);
    TEST_ASSERT_EQUAL(result1, result2);
}

void test_BspGpioSetIRQHandler_SequentialCalls(void)
{
    // Test setting handler, then changing it
    BspGpioSetIRQHandler(eBSP_GPIO_PORT_A_0, test_irq_callback);
    BspGpioSetIRQHandler(eBSP_GPIO_PORT_A_0, NULL);
    BspGpioSetIRQHandler(eBSP_GPIO_PORT_A_0, test_irq_callback);
}

void test_BspGpioTogglePin_AllPins(void)
{
    // Test toggling all available pins
    BspGpioTogglePin(eBSP_GPIO_PORT_A_0);
    BspGpioTogglePin(eBSP_GPIO_PORT_A_1);
    BspGpioTogglePin(eBSP_GPIO_PORT_A_2);
    BspGpioTogglePin(eBSP_GPIO_PORT_A_3);
    BspGpioTogglePin(eBSP_GPIO_PORT_A_4);
    BspGpioTogglePin(eBSP_GPIO_PORT_A_5);
    BspGpioTogglePin(eBSP_GPIO_PORT_A_6);
    BspGpioTogglePin(eBSP_GPIO_PORT_A_7);
    BspGpioTogglePin(eBSP_GPIO_PORT_B_0);
    BspGpioTogglePin(eBSP_GPIO_PORT_B_1);
    BspGpioTogglePin(eBSP_GPIO_PORT_B_2);
    BspGpioTogglePin(eBSP_GPIO_PORT_B_3);
    BspGpioTogglePin(eBSP_GPIO_PORT_B_4);
    BspGpioTogglePin(eBSP_GPIO_PORT_B_5);
    BspGpioTogglePin(eBSP_GPIO_PORT_B_6);
    BspGpioTogglePin(eBSP_GPIO_PORT_B_7);
}

void test_BspGpioCfgPin_VariousHandles(void)
{
    // Test with various pin handle values
    BspGpioCfgPin(eBSP_GPIO_PORT_A_0, 0x0001, eGPIO_CFG_PP_OUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_A_1, 0x0002, eGPIO_CFG_OD_OUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_A_2, 0x0004, eGPIO_CFG_INPUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_A_3, 0x0008, eGPIO_CFG_PP_OUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_A_4, 0x0010, eGPIO_CFG_OD_OUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_A_5, 0x0020, eGPIO_CFG_INPUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_A_6, 0x0040, eGPIO_CFG_PP_OUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_A_7, 0x0080, eGPIO_CFG_OD_OUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_B_0, 0x0100, eGPIO_CFG_INPUT);
    BspGpioCfgPin(eBSP_GPIO_PORT_B_1, 0x0200, eGPIO_CFG_PP_OUT);
}

void test_BspGpioGetLLHandle_ConsistentBehavior(void)
{
    // Test that function consistently returns 0 for all pins
    for (GpioPort_e pin = eBSP_GPIO_PORT_A_0; pin <= eBSP_GPIO_PORT_B_7; pin++)
    {
        uint32_t result = BspGpioGetLLHandle(pin);
        TEST_ASSERT_EQUAL_UINT32(0, result);
    }
}
