/**
 * @file ut_bsp_can.c
 * @brief Unit tests for BSP CAN module
 */

#include "Mockstm32f4xx_hal_can.h"
#include "bsp_can.h"
#include "bsp_led.h"
#include "gpio_structs/gpio_struct.h"
#include "unity.h"

/* ============================================================================
 * Test Stubs and Mocks
 * ========================================================================== */

/* Stub for HAL_GetTick - required by production code */
uint32_t HAL_GetTick(void)
{
    static uint32_t tick = 0;
    return tick++;
}

/* Stub CAN handles - required by production code */
CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;

/* Mock GPIO port for testing */
static GPIO_TypeDef mock_GPIOA;

/* Stub gpio_pins array - required by bsp_led/bsp_gpio dependencies */
const gpio_t gpio_pins[eGPIO_COUNT] = {
    [eM_LED1] = {&mock_GPIOA, GPIO_PIN_0},
    [eM_LED2] = {&mock_GPIOA, GPIO_PIN_1},
    /* Remaining pins default to {NULL, 0} */
};

/* ============================================================================
 * Test Fixtures
 * ========================================================================== */

static CAN_HandleTypeDef s_tCanHandle;

void setUp(void)
{
    /* Initialize global HAL CAN handles used by production code */
    hcan1.Instance = (CAN_TypeDef*)0x40006400U; /* Mock CAN1 address */
    hcan2.Instance = (CAN_TypeDef*)0x40006800U; /* Mock CAN2 address */

    /* Initialize local test handle */
    s_tCanHandle.Instance = (CAN_TypeDef*)0x40006400U; /* Mock CAN1 address */
}

void tearDown(void)
{
    /* Ignore HAL calls during cleanup */
    HAL_CAN_Stop_IgnoreAndReturn(HAL_OK);
    HAL_CAN_DeactivateNotification_IgnoreAndReturn(HAL_OK);

    /* Cleanup all allocated module handles to ensure clean state between tests */
    for (int8_t i = 0; i < 2; i++) /* BSP_CAN_MAX_INSTANCES = 2 */
    {
        BspCanFree((BspCanHandle_t)i);
    }
}

/* ============================================================================
 * Test Cases - Module Allocation
 * ========================================================================== */

/**
 * @brief Test successful CAN module allocation.
 */
void test_BspCanAllocate_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};

    BspCanHandle_t hCan = BspCanAllocate(&tConfig, NULL, NULL);

    TEST_ASSERT_NOT_EQUAL(BSP_CAN_INVALID_HANDLE, hCan);
    TEST_ASSERT_GREATER_OR_EQUAL(0, hCan);
}

/**
 * @brief Test allocation with NULL config returns invalid handle.
 */
void test_BspCanAllocate_NullConfig_ReturnsInvalid(void)
{
    BspCanHandle_t hCan = BspCanAllocate(NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(BSP_CAN_INVALID_HANDLE, hCan);
}

/**
 * @brief Test allocation with NULL HAL handle returns invalid handle.
 */
void test_BspCanAllocate_InvalidInstance_ReturnsInvalid(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_COUNT, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};

    BspCanHandle_t hCan = BspCanAllocate(&tConfig, NULL, NULL);

    TEST_ASSERT_EQUAL(BSP_CAN_INVALID_HANDLE, hCan);
}

/* ============================================================================
 * Test Cases - Filter Configuration
 * ========================================================================== */

/**
 * @brief Test adding filter before start succeeds.
 */
void test_BspCanAddFilter_BeforeStart_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};

    BspCanHandle_t hCan = BspCanAllocate(&tConfig, NULL, NULL);
    TEST_ASSERT_NOT_EQUAL(BSP_CAN_INVALID_HANDLE, hCan);

    BspCanFilter_t tFilter = {.uFilterId = 0x100, .uFilterMask = 0x7F0, .eIdType = eBSP_CAN_ID_STANDARD, .byFifoAssignment = 0};

    BspCanError_e eError = BspCanAddFilter(hCan, &tFilter);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

/**
 * @brief Test adding filter with invalid handle returns error.
 */
void test_BspCanAddFilter_InvalidHandle_ReturnsError(void)
{
    BspCanFilter_t tFilter = {.uFilterId = 0x100, .uFilterMask = 0x7F0, .eIdType = eBSP_CAN_ID_STANDARD, .byFifoAssignment = 0};

    BspCanError_e eError = BspCanAddFilter(BSP_CAN_INVALID_HANDLE, &tFilter);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}

/* ============================================================================
 * Test Cases - Transmit Queue
 * ========================================================================== */

/**
 * @brief Test transmit with invalid handle returns error.
 */
void test_BspCanTransmit_InvalidHandle_ReturnsError(void)
{
    BspCanMessage_t tMsg = {.uId        = 0x123,
                            .eIdType    = eBSP_CAN_ID_STANDARD,
                            .eFrameType = eBSP_CAN_FRAME_DATA,
                            .byDataLen  = 8,
                            .aData      = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}};

    BspCanError_e eError = BspCanTransmit(BSP_CAN_INVALID_HANDLE, &tMsg, 0, 0x1234);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}

/**
 * @brief Test transmit with NULL message returns error.
 */
void test_BspCanTransmit_NullMessage_ReturnsError(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};

    BspCanHandle_t hCan = BspCanAllocate(&tConfig, NULL, NULL);
    TEST_ASSERT_NOT_EQUAL(BSP_CAN_INVALID_HANDLE, hCan);

    /* Start CAN */
    HAL_CAN_ConfigFilter_IgnoreAndReturn(HAL_OK);
    HAL_CAN_Start_ExpectAndReturn(&s_tCanHandle, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);

    BspCanError_e eStartError = BspCanStart(hCan);
    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eStartError);

    /* Try transmit with NULL message */
    BspCanError_e eError = BspCanTransmit(hCan, NULL, 0, 0x1234);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_PARAM, eError);
}

/* ============================================================================
 * Test Cases - Callback Registration
 * ========================================================================== */

/**
 * @brief Test RX callback registration.
 */
void test_BspCanRegisterRxCallback_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};

    BspCanHandle_t hCan = BspCanAllocate(&tConfig, NULL, NULL);
    TEST_ASSERT_NOT_EQUAL(BSP_CAN_INVALID_HANDLE, hCan);

    /* Dummy callback function */
    void rxCallback(BspCanHandle_t handle, const BspCanMessage_t* pMessage)
    {
        (void)handle;
        (void)pMessage;
    }

    BspCanError_e eError = BspCanRegisterRxCallback(hCan, rxCallback);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

/* ============================================================================
 * Test Cases - Queue Info
 * ========================================================================== */

/**
 * @brief Test getting TX queue info after allocation.
 */
void test_BspCanGetTxQueueInfo_AfterAllocation_ReturnsEmpty(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};

    BspCanHandle_t hCan = BspCanAllocate(&tConfig, NULL, NULL);
    TEST_ASSERT_NOT_EQUAL(BSP_CAN_INVALID_HANDLE, hCan);

    uint8_t byUsed = 0xFF;
    uint8_t byFree = 0;

    BspCanError_e eError = BspCanGetTxQueueInfo(hCan, &byUsed, &byFree);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
    TEST_ASSERT_EQUAL(0, byUsed);
    TEST_ASSERT_EQUAL(BSP_CAN_TX_QUEUE_DEPTH, byFree);
}

/* ============================================================================
 * Test Cases - Module Free
 * ========================================================================== */

/**
 * @brief Test freeing a valid module.
 */
void test_BspCanFree_ValidHandle_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};

    BspCanHandle_t hCan = BspCanAllocate(&tConfig, NULL, NULL);
    TEST_ASSERT_NOT_EQUAL(BSP_CAN_INVALID_HANDLE, hCan);

    BspCanError_e eError = BspCanFree(hCan);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

/**
 * @brief Test freeing invalid handle returns error.
 */
void test_BspCanFree_InvalidHandle_ReturnsError(void)
{
    BspCanError_e eError = BspCanFree(BSP_CAN_INVALID_HANDLE);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}
