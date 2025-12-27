/**
 * @file ut_bsp_can.c
 * @brief Unit tests for BSP CAN module
 */

#include "Mockstm32f4xx_hal_can.h"
#include "Mockstm32f4xx_hal_gpio.h"
#include "bsp_can.h"
#include "bsp_led.h"
#include "gpio_struct.h"
#include "unity.h"
#include <string.h>

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

/* HAL callback functions defined in production code */
extern void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan);
extern void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef* hcan);
extern void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef* hcan);
extern void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef* hcan);
extern void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef* hcan);
extern void HAL_CAN_ErrorCallback(CAN_HandleTypeDef* hcan);

/* ============================================================================
 * Test Helper Functions
 * ========================================================================== */

/**
 * @brief Dummy RX callback for testing callback registration
 */
static void sDummyRxCallback(BspCanHandle_t handle, const BspCanMessage_t* pMessage)
{
    (void)handle;
    (void)pMessage;
}

/* Test callback trackers */
static bool             s_bRxCallbackInvoked       = false;
static bool             s_bTxCallbackInvoked       = false;
static bool             s_bErrorCallbackInvoked    = false;
static bool             s_bBusStateCallbackInvoked = false;
static BspCanMessage_t  s_tLastRxMessage;
static uint32_t         s_uLastTxId     = 0;
static BspCanError_e    s_eLastError    = eBSP_CAN_ERR_NONE;
static BspCanBusState_e s_eLastBusState = eBSP_CAN_STATE_ERROR_ACTIVE;

static void sTestRxCallback(BspCanHandle_t handle, const BspCanMessage_t* pMessage)
{
    (void)handle;
    s_bRxCallbackInvoked = true;
    if (pMessage)
    {
        s_tLastRxMessage = *pMessage;
    }
}

static void sTestTxCallback(BspCanHandle_t handle, uint32_t uTxId)
{
    (void)handle;
    s_bTxCallbackInvoked = true;
    s_uLastTxId          = uTxId;
}

static void sTestErrorCallback(BspCanHandle_t handle, BspCanError_e eError)
{
    (void)handle;
    s_bErrorCallbackInvoked = true;
    s_eLastError            = eError;
}

static void sTestBusStateCallback(BspCanHandle_t handle, BspCanBusState_e eState)
{
    (void)handle;
    s_bBusStateCallbackInvoked = true;
    s_eLastBusState            = eState;
}

/* ============================================================================
 * Test Fixtures
 * ========================================================================== */

static CAN_HandleTypeDef s_tCanHandle;
static CAN_TypeDef       s_tCan1Instance;
static CAN_TypeDef       s_tCan2Instance;

void setUp(void)
{
    /* Initialize fake CAN instances with zeroed registers */
    memset(&s_tCan1Instance, 0, sizeof(CAN_TypeDef));
    memset(&s_tCan2Instance, 0, sizeof(CAN_TypeDef));

    /* Initialize global HAL CAN handles used by production code */
    hcan1.Instance = &s_tCan1Instance;
    hcan2.Instance = &s_tCan2Instance;

    /* Initialize local test handle */
    s_tCanHandle.Instance = &s_tCan1Instance;

    /* Reset callback trackers */
    s_bRxCallbackInvoked       = false;
    s_bTxCallbackInvoked       = false;
    s_bErrorCallbackInvoked    = false;
    s_bBusStateCallbackInvoked = false;
    s_uLastTxId                = 0;
    s_eLastError               = eBSP_CAN_ERR_NONE;
    s_eLastBusState            = eBSP_CAN_STATE_ERROR_ACTIVE;
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

    BspCanError_e eError = BspCanRegisterRxCallback(hCan, sDummyRxCallback);

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

/* ============================================================================
 * Test Cases - BspCanStart
 * ========================================================================== */

void test_BspCanStart_WithStandardFilter_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    BspCanFilter_t tFilter = {.uFilterId = 0x100, .uFilterMask = 0x7F0, .eIdType = eBSP_CAN_ID_STANDARD, .byFifoAssignment = 0};
    BspCanAddFilter(hCan, &tFilter);

    HAL_CAN_ConfigFilter_ExpectAndReturn(&hcan1, NULL, HAL_OK);
    HAL_CAN_ConfigFilter_IgnoreArg_sFilterConfig();
    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_ExpectAndReturn(&hcan1,
                                                 CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING | CAN_IT_TX_MAILBOX_EMPTY |
                                                     CAN_IT_ERROR | CAN_IT_BUSOFF | CAN_IT_ERROR_PASSIVE,
                                                 HAL_OK);

    BspCanError_e eError = BspCanStart(hCan);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

void test_BspCanStart_WithExtendedFilter_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    BspCanFilter_t tFilter = {.uFilterId = 0x12345678, .uFilterMask = 0x1FFFFFFF, .eIdType = eBSP_CAN_ID_EXTENDED, .byFifoAssignment = 1};
    BspCanAddFilter(hCan, &tFilter);

    HAL_CAN_ConfigFilter_ExpectAndReturn(&hcan1, NULL, HAL_OK);
    HAL_CAN_ConfigFilter_IgnoreArg_sFilterConfig();
    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);

    BspCanError_e eError = BspCanStart(hCan);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

void test_BspCanStart_InvalidHandle_ReturnsError(void)
{
    BspCanError_e eError = BspCanStart(BSP_CAN_INVALID_HANDLE);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}

void test_BspCanStart_AlreadyStarted_ReturnsError(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    BspCanError_e eError = BspCanStart(hCan);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_ALREADY_STARTED, eError);
}

void test_BspCanStart_HAL_ConfigFilter_Fails(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    BspCanFilter_t tFilter = {.uFilterId = 0x100, .uFilterMask = 0x7F0, .eIdType = eBSP_CAN_ID_STANDARD, .byFifoAssignment = 0};
    BspCanAddFilter(hCan, &tFilter);

    HAL_CAN_ConfigFilter_ExpectAndReturn(&hcan1, NULL, HAL_ERROR);
    HAL_CAN_ConfigFilter_IgnoreArg_sFilterConfig();

    BspCanError_e eError = BspCanStart(hCan);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_HAL_ERROR, eError);
}

void test_BspCanStart_HAL_Start_Fails(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_ERROR);

    BspCanError_e eError = BspCanStart(hCan);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_HAL_ERROR, eError);
}

void test_BspCanStart_HAL_ActivateNotification_Fails(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_ERROR);
    HAL_CAN_Stop_ExpectAndReturn(&hcan1, HAL_OK);

    BspCanError_e eError = BspCanStart(hCan);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_HAL_ERROR, eError);
}

/* ============================================================================
 * Test Cases - BspCanStop
 * ========================================================================== */

void test_BspCanStop_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    HAL_CAN_DeactivateNotification_ExpectAndReturn(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING, HAL_OK);
    HAL_CAN_Stop_ExpectAndReturn(&hcan1, HAL_OK);

    BspCanError_e eError = BspCanStop(hCan);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

void test_BspCanStop_InvalidHandle_ReturnsError(void)
{
    BspCanError_e eError = BspCanStop(BSP_CAN_INVALID_HANDLE);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}

void test_BspCanStop_NotStarted_ReturnsError(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    BspCanError_e eError = BspCanStop(hCan);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NOT_STARTED, eError);
}

/* ============================================================================
 * Test Cases - BspCanTransmit Extended
 * ========================================================================== */

void test_BspCanTransmit_StandardFrame_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    BspCanMessage_t tMsg = {.uId        = 0x123,
                            .eIdType    = eBSP_CAN_ID_STANDARD,
                            .eFrameType = eBSP_CAN_FRAME_DATA,
                            .byDataLen  = 8,
                            .aData      = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}};

    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 1);
    HAL_CAN_AddTxMessage_ExpectAndReturn(&hcan1, NULL, NULL, NULL, HAL_OK);
    HAL_CAN_AddTxMessage_IgnoreArg_pHeader();
    HAL_CAN_AddTxMessage_IgnoreArg_aData();
    HAL_CAN_AddTxMessage_IgnoreArg_pTxMailbox();

    BspCanError_e eError = BspCanTransmit(hCan, &tMsg, 0, 0x1234);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

void test_BspCanTransmit_ExtendedFrame_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    BspCanMessage_t tMsg = {.uId        = 0x12345678,
                            .eIdType    = eBSP_CAN_ID_EXTENDED,
                            .eFrameType = eBSP_CAN_FRAME_DATA,
                            .byDataLen  = 4,
                            .aData      = {0xAA, 0xBB, 0xCC, 0xDD}};

    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 1);
    HAL_CAN_AddTxMessage_IgnoreAndReturn(HAL_OK);

    BspCanError_e eError = BspCanTransmit(hCan, &tMsg, 1, 0x5678);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

void test_BspCanTransmit_RemoteFrame_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    BspCanMessage_t tMsg = {.uId = 0x456, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_REMOTE, .byDataLen = 0};

    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 2);
    HAL_CAN_AddTxMessage_IgnoreAndReturn(HAL_OK);

    BspCanError_e eError = BspCanTransmit(hCan, &tMsg, 2, 0x9ABC);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

void test_BspCanTransmit_NotStarted_ReturnsError(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    BspCanMessage_t tMsg = {.uId = 0x123, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 8};

    BspCanError_e eError = BspCanTransmit(hCan, &tMsg, 0, 0x1234);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NOT_STARTED, eError);
}

void test_BspCanTransmit_InvalidPriority_ReturnsError(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    BspCanMessage_t tMsg = {.uId = 0x123, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 8};

    BspCanError_e eError = BspCanTransmit(hCan, &tMsg, 255, 0x1234);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_PARAM, eError);
}

void test_BspCanTransmit_QueuedWhenNoMailbox_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    BspCanMessage_t tMsg = {.uId = 0x123, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 8};

    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);

    BspCanError_e eError = BspCanTransmit(hCan, &tMsg, 0, 0x1234);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

/* ============================================================================
 * Test Cases - BspCanAbortTransmit
 * ========================================================================== */

void test_BspCanAbortTransmit_MessageInQueue_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    BspCanMessage_t tMsg = {.uId = 0x123, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 8};
    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);
    BspCanTransmit(hCan, &tMsg, 0, 0xABCD);

    BspCanError_e eError = BspCanAbortTransmit(hCan, 0xABCD);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

void test_BspCanAbortTransmit_MessageNotFound_ReturnsError(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    BspCanError_e eError = BspCanAbortTransmit(hCan, 0x9999);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_PARAM, eError);
}

void test_BspCanAbortTransmit_InvalidHandle_ReturnsError(void)
{
    BspCanError_e eError = BspCanAbortTransmit(BSP_CAN_INVALID_HANDLE, 0x1234);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}

/* ============================================================================
 * Test Cases - Filter Operations Extended
 * ========================================================================== */

void test_BspCanAddFilter_NullFilter_ReturnsError(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    BspCanError_e eError = BspCanAddFilter(hCan, NULL);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_PARAM, eError);
}

void test_BspCanAddFilter_AfterStart_ReturnsError(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    BspCanFilter_t tFilter = {.uFilterId = 0x100, .uFilterMask = 0x7F0, .eIdType = eBSP_CAN_ID_STANDARD, .byFifoAssignment = 0};

    BspCanError_e eError = BspCanAddFilter(hCan, &tFilter);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_ALREADY_STARTED, eError);
}

void test_BspCanAddFilter_MaxFilters_ReturnsError(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    BspCanFilter_t tFilter = {.uFilterId = 0x100, .uFilterMask = 0x7F0, .eIdType = eBSP_CAN_ID_STANDARD, .byFifoAssignment = 0};

    for (uint8_t i = 0; i < 14; i++)
    {
        BspCanAddFilter(hCan, &tFilter);
    }

    BspCanError_e eError = BspCanAddFilter(hCan, &tFilter);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_FILTER_FULL, eError);
}

/* ============================================================================
 * Test Cases - Callback Registration Extended
 * ========================================================================== */

void test_BspCanRegisterTxCallback_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    BspCanError_e eError = BspCanRegisterTxCallback(hCan, sTestTxCallback);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

void test_BspCanRegisterTxCallback_InvalidHandle_ReturnsError(void)
{
    BspCanError_e eError = BspCanRegisterTxCallback(BSP_CAN_INVALID_HANDLE, sTestTxCallback);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}

void test_BspCanRegisterErrorCallback_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    BspCanError_e eError = BspCanRegisterErrorCallback(hCan, sTestErrorCallback);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

void test_BspCanRegisterErrorCallback_InvalidHandle_ReturnsError(void)
{
    BspCanError_e eError = BspCanRegisterErrorCallback(BSP_CAN_INVALID_HANDLE, sTestErrorCallback);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}

void test_BspCanRegisterBusStateCallback_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    BspCanError_e eError = BspCanRegisterBusStateCallback(hCan, sTestBusStateCallback);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

void test_BspCanRegisterBusStateCallback_InvalidHandle_ReturnsError(void)
{
    BspCanError_e eError = BspCanRegisterBusStateCallback(BSP_CAN_INVALID_HANDLE, sTestBusStateCallback);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}

void test_BspCanRegisterRxCallback_InvalidHandle_ReturnsError(void)
{
    BspCanError_e eError = BspCanRegisterRxCallback(BSP_CAN_INVALID_HANDLE, sTestRxCallback);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}

/* ============================================================================
 * Test Cases - Queue/Buffer Info Extended
 * ========================================================================== */

void test_BspCanGetTxQueueInfo_InvalidHandle_ReturnsError(void)
{
    uint8_t       byUsed, byFree;
    BspCanError_e eError = BspCanGetTxQueueInfo(BSP_CAN_INVALID_HANDLE, &byUsed, &byFree);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}

void test_BspCanGetTxQueueInfo_NullPointers_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    BspCanError_e eError = BspCanGetTxQueueInfo(hCan, NULL, NULL);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

void test_BspCanGetRxBufferInfo_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    uint8_t  byUsed    = 0xFF;
    uint32_t uOverruns = 0xFF;

    BspCanError_e eError = BspCanGetRxBufferInfo(hCan, &byUsed, &uOverruns);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
    TEST_ASSERT_EQUAL(0, byUsed);
}

void test_BspCanGetRxBufferInfo_InvalidHandle_ReturnsError(void)
{
    uint8_t       byUsed;
    uint32_t      uOverruns;
    BspCanError_e eError = BspCanGetRxBufferInfo(BSP_CAN_INVALID_HANDLE, &byUsed, &uOverruns);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}

void test_BspCanGetRxBufferInfo_NullPointers_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    BspCanError_e eError = BspCanGetRxBufferInfo(hCan, NULL, NULL);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

/* ============================================================================
 * Test Cases - Bus State and Error Counters
 * ========================================================================== */

void test_BspCanGetBusState_ErrorActive(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    hcan1.Instance->ESR = 0; // No errors

    BspCanBusState_e eState;
    BspCanError_e    eError = BspCanGetBusState(hCan, &eState);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
    TEST_ASSERT_EQUAL(eBSP_CAN_STATE_ERROR_ACTIVE, eState);
}

void test_BspCanGetBusState_ErrorPassive(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    hcan1.Instance->ESR = CAN_ESR_EPVF;

    BspCanBusState_e eState;
    BspCanError_e    eError = BspCanGetBusState(hCan, &eState);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
    TEST_ASSERT_EQUAL(eBSP_CAN_STATE_ERROR_PASSIVE, eState);
}

void test_BspCanGetBusState_BusOff(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    hcan1.Instance->ESR = CAN_ESR_BOFF;

    BspCanBusState_e eState;
    BspCanError_e    eError = BspCanGetBusState(hCan, &eState);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
    TEST_ASSERT_EQUAL(eBSP_CAN_STATE_BUS_OFF, eState);
}

void test_BspCanGetBusState_InvalidHandle_ReturnsError(void)
{
    BspCanBusState_e eState;
    BspCanError_e    eError = BspCanGetBusState(BSP_CAN_INVALID_HANDLE, &eState);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}

void test_BspCanGetBusState_NullPointer_ReturnsError(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    BspCanError_e eError = BspCanGetBusState(hCan, NULL);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}

void test_BspCanGetErrorCounters_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    hcan1.Instance->ESR = (0x12 << 16) | (0x34 << 24); // TEC=0x12, REC=0x34

    uint8_t       byTxErrors = 0;
    uint8_t       byRxErrors = 0;
    BspCanError_e eError     = BspCanGetErrorCounters(hCan, &byTxErrors, &byRxErrors);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
    TEST_ASSERT_EQUAL(0x12, byTxErrors);
    TEST_ASSERT_EQUAL(0x34, byRxErrors);
}

void test_BspCanGetErrorCounters_InvalidHandle_ReturnsError(void)
{
    uint8_t       byTxErrors, byRxErrors;
    BspCanError_e eError = BspCanGetErrorCounters(BSP_CAN_INVALID_HANDLE, &byTxErrors, &byRxErrors);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}

void test_BspCanGetErrorCounters_NullPointers_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    BspCanError_e eError = BspCanGetErrorCounters(hCan, NULL, NULL);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

/* ============================================================================
 * Test Cases - Statistics
 * ========================================================================== */

#if BSP_CAN_ENABLE_STATISTICS
void test_BspCanGetStatistics_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    BspCanStatistics_t tStats;
    BspCanError_e      eError = BspCanGetStatistics(hCan, &tStats);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
    TEST_ASSERT_EQUAL(0, tStats.uTxCount);
    TEST_ASSERT_EQUAL(0, tStats.uRxCount);
}

void test_BspCanGetStatistics_InvalidHandle_ReturnsError(void)
{
    BspCanStatistics_t tStats;
    BspCanError_e      eError = BspCanGetStatistics(BSP_CAN_INVALID_HANDLE, &tStats);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}

void test_BspCanGetStatistics_NullPointer_ReturnsError(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    BspCanError_e eError = BspCanGetStatistics(hCan, NULL);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_HANDLE, eError);
}
#endif

/* ============================================================================
 * Test Cases - HAL Callbacks
 * ========================================================================== */

/* Note: HAL callbacks are tested implicitly through transmit/receive operations
 * and are internal to the BSP implementation. Direct testing would require
 * exposing internal state. */

/* ============================================================================
 * Test Cases - Multiple Allocations
 * ========================================================================== */

void test_BspCanAllocate_BothInstances_Success(void)
{
    BspCanConfig_t tConfig1 = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanConfig_t tConfig2 = {.eInstance = eBSP_CAN_INSTANCE_2, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};

    BspCanHandle_t hCan1 = BspCanAllocate(&tConfig1, NULL, NULL);
    BspCanHandle_t hCan2 = BspCanAllocate(&tConfig2, NULL, NULL);

    TEST_ASSERT_NOT_EQUAL(BSP_CAN_INVALID_HANDLE, hCan1);
    TEST_ASSERT_NOT_EQUAL(BSP_CAN_INVALID_HANDLE, hCan2);
    TEST_ASSERT_NOT_EQUAL(hCan1, hCan2);
}

void test_BspCanAllocate_AllSlotsUsed_ReturnsInvalid(void)
{
    BspCanConfig_t tConfig1 = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanConfig_t tConfig2 = {.eInstance = eBSP_CAN_INSTANCE_2, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};

    (void)BspCanAllocate(&tConfig1, NULL, NULL);
    (void)BspCanAllocate(&tConfig2, NULL, NULL);
    BspCanHandle_t hCan3 = BspCanAllocate(&tConfig1, NULL, NULL);

    TEST_ASSERT_EQUAL(BSP_CAN_INVALID_HANDLE, hCan3);
}

void test_BspCanFree_ThenReallocate_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};

    BspCanHandle_t hCan1 = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanFree(hCan1);
    BspCanHandle_t hCan2 = BspCanAllocate(&tConfig, NULL, NULL);

    TEST_ASSERT_NOT_EQUAL(BSP_CAN_INVALID_HANDLE, hCan2);
}

/* ============================================================================
 * Test Cases - Edge Cases and Coverage
 * ========================================================================== */

void test_BspCanTransmit_QueueFull_ReturnsError(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    BspCanMessage_t tMsg = {.uId = 0x123, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 8};

    // Fill queue completely - ignore return for mailbox checks
    HAL_CAN_GetTxMailboxesFreeLevel_IgnoreAndReturn(0);

    for (uint8_t i = 0; i < 32; i++)
    {
        BspCanTransmit(hCan, &tMsg, 0, 0x2000 + i);
    }

    // Try to add one more - should fail
    BspCanError_e eError = BspCanTransmit(hCan, &tMsg, 0, 0x9999);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_TX_QUEUE_FULL, eError);
}

void test_BspCanTransmit_DifferentPriorities_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    BspCanMessage_t tMsg = {.uId = 0x123, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 8};

    // Send messages with different priorities
    for (uint8_t prio = 0; prio < 8; prio++)
    {
        HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);
        BspCanError_e eError = BspCanTransmit(hCan, &tMsg, prio, 0x3000 + prio);
        TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
    }
}

void test_BspCanFree_WhileStarted_StopsFirst(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    HAL_CAN_DeactivateNotification_IgnoreAndReturn(HAL_OK);
    HAL_CAN_Stop_ExpectAndReturn(&hcan1, HAL_OK);

    BspCanError_e eError = BspCanFree(hCan);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

/* ============================================================================
 * Test Cases - HAL RX Callbacks
 * ========================================================================== */

void test_HAL_CAN_RxFifo0MsgPendingCallback_StandardID_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanRegisterRxCallback(hCan, sTestRxCallback);

    HAL_CAN_GetRxMessage_ExpectAndReturn(&hcan1, CAN_RX_FIFO0, NULL, NULL, HAL_OK);
    HAL_CAN_GetRxMessage_IgnoreArg_pHeader();
    HAL_CAN_GetRxMessage_IgnoreArg_aData();

    HAL_CAN_RxFifo0MsgPendingCallback(&hcan1);

    TEST_ASSERT_TRUE(s_bRxCallbackInvoked);
}

void test_HAL_CAN_RxFifo0MsgPendingCallback_WithLED(void)
{
    LiveLed_t      led     = {.ePin = eM_LED1};
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, &led);
    BspCanRegisterRxCallback(hCan, sTestRxCallback);

    HAL_CAN_GetRxMessage_ExpectAndReturn(&hcan1, CAN_RX_FIFO0, NULL, NULL, HAL_OK);
    HAL_CAN_GetRxMessage_IgnoreArg_pHeader();
    HAL_CAN_GetRxMessage_IgnoreArg_aData();
    HAL_GPIO_TogglePin_Ignore(); /* LED blink */

    HAL_CAN_RxFifo0MsgPendingCallback(&hcan1);

    TEST_ASSERT_TRUE(s_bRxCallbackInvoked);
}

void test_HAL_CAN_RxFifo0MsgPendingCallback_HAL_GetRxMessage_Fails(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanRegisterRxCallback(hCan, sTestRxCallback);

    HAL_CAN_GetRxMessage_ExpectAndReturn(&hcan1, CAN_RX_FIFO0, NULL, NULL, HAL_ERROR);
    HAL_CAN_GetRxMessage_IgnoreArg_pHeader();
    HAL_CAN_GetRxMessage_IgnoreArg_aData();

    HAL_CAN_RxFifo0MsgPendingCallback(&hcan1);

    TEST_ASSERT_FALSE(s_bRxCallbackInvoked);
}

void test_HAL_CAN_RxFifo0MsgPendingCallback_InvalidHandle(void)
{
    CAN_HandleTypeDef hInvalidCan;
    hInvalidCan.Instance = (CAN_TypeDef*)0x99999999;

    /* Should not crash, just return early */
    HAL_CAN_RxFifo0MsgPendingCallback(&hInvalidCan);
}

void test_HAL_CAN_RxFifo0MsgPendingCallback_NoCallback(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    (void)BspCanAllocate(&tConfig, NULL, NULL);
    /* Don't register callback */

    HAL_CAN_GetRxMessage_ExpectAndReturn(&hcan1, CAN_RX_FIFO0, NULL, NULL, HAL_OK);
    HAL_CAN_GetRxMessage_IgnoreArg_pHeader();
    HAL_CAN_GetRxMessage_IgnoreArg_aData();

    HAL_CAN_RxFifo0MsgPendingCallback(&hcan1);

    TEST_ASSERT_FALSE(s_bRxCallbackInvoked);
}

void test_HAL_CAN_RxFifo1MsgPendingCallback_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanRegisterRxCallback(hCan, sTestRxCallback);

    HAL_CAN_GetRxMessage_ExpectAndReturn(&hcan1, CAN_RX_FIFO1, NULL, NULL, HAL_OK);
    HAL_CAN_GetRxMessage_IgnoreArg_pHeader();
    HAL_CAN_GetRxMessage_IgnoreArg_aData();

    HAL_CAN_RxFifo1MsgPendingCallback(&hcan1);

    TEST_ASSERT_TRUE(s_bRxCallbackInvoked);
}

void test_HAL_CAN_RxFifo1MsgPendingCallback_WithLED(void)
{
    LiveLed_t      led     = {.ePin = eM_LED1};
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, &led);
    BspCanRegisterRxCallback(hCan, sTestRxCallback);

    HAL_CAN_GetRxMessage_ExpectAndReturn(&hcan1, CAN_RX_FIFO1, NULL, NULL, HAL_OK);
    HAL_CAN_GetRxMessage_IgnoreArg_pHeader();
    HAL_CAN_GetRxMessage_IgnoreArg_aData();
    HAL_GPIO_TogglePin_Ignore(); /* LED blink */

    HAL_CAN_RxFifo1MsgPendingCallback(&hcan1);

    TEST_ASSERT_TRUE(s_bRxCallbackInvoked);
}

void test_HAL_CAN_RxFifo1MsgPendingCallback_HAL_GetRxMessage_Fails(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanRegisterRxCallback(hCan, sTestRxCallback);

    HAL_CAN_GetRxMessage_ExpectAndReturn(&hcan1, CAN_RX_FIFO1, NULL, NULL, HAL_ERROR);
    HAL_CAN_GetRxMessage_IgnoreArg_pHeader();
    HAL_CAN_GetRxMessage_IgnoreArg_aData();

    HAL_CAN_RxFifo1MsgPendingCallback(&hcan1);

    TEST_ASSERT_FALSE(s_bRxCallbackInvoked);
}

void test_HAL_CAN_RxFifo1MsgPendingCallback_InvalidHandle(void)
{
    CAN_HandleTypeDef hInvalidCan;
    hInvalidCan.Instance = (CAN_TypeDef*)0x99999999;

    HAL_CAN_RxFifo1MsgPendingCallback(&hInvalidCan);
}

void test_HAL_CAN_RxFifo1MsgPendingCallback_NoCallback(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    (void)BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_GetRxMessage_ExpectAndReturn(&hcan1, CAN_RX_FIFO1, NULL, NULL, HAL_OK);
    HAL_CAN_GetRxMessage_IgnoreArg_pHeader();
    HAL_CAN_GetRxMessage_IgnoreArg_aData();

    HAL_CAN_RxFifo1MsgPendingCallback(&hcan1);

    TEST_ASSERT_FALSE(s_bRxCallbackInvoked);
}

/* ============================================================================
 * Test Cases - HAL TX Callbacks
 * ========================================================================== */

void test_HAL_CAN_TxMailbox0CompleteCallback_WithCallback(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanRegisterTxCallback(hCan, sTestTxCallback);

    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0); /* No free mailboxes, queue empty */

    HAL_CAN_TxMailbox0CompleteCallback(&hcan1);

    TEST_ASSERT_TRUE(s_bTxCallbackInvoked);
}

void test_HAL_CAN_TxMailbox0CompleteCallback_NoCallback(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    (void)BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);

    HAL_CAN_TxMailbox0CompleteCallback(&hcan1);

    TEST_ASSERT_FALSE(s_bTxCallbackInvoked);
}

void test_HAL_CAN_TxMailbox0CompleteCallback_InvalidHandle(void)
{
    CAN_HandleTypeDef hInvalidCan;
    hInvalidCan.Instance = (CAN_TypeDef*)0x99999999;

    HAL_CAN_TxMailbox0CompleteCallback(&hInvalidCan);
}

void test_HAL_CAN_TxMailbox0CompleteCallback_WithQueuedMessage(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    BspCanRegisterTxCallback(hCan, sTestTxCallback);

    /* Queue a message when no mailboxes available */
    BspCanMessage_t tMsg = {.uId = 0x123, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 8};
    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);
    BspCanTransmit(hCan, &tMsg, 0, 0xABCD);

    /* Mailbox complete should dequeue and send next message */
    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 1);
    HAL_CAN_AddTxMessage_IgnoreAndReturn(HAL_OK);

    HAL_CAN_TxMailbox0CompleteCallback(&hcan1);

    TEST_ASSERT_TRUE(s_bTxCallbackInvoked);
}

void test_HAL_CAN_TxMailbox1CompleteCallback_WithCallback(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanRegisterTxCallback(hCan, sTestTxCallback);

    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);

    HAL_CAN_TxMailbox1CompleteCallback(&hcan1);

    TEST_ASSERT_TRUE(s_bTxCallbackInvoked);
}

void test_HAL_CAN_TxMailbox1CompleteCallback_NoCallback(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    (void)BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);

    HAL_CAN_TxMailbox1CompleteCallback(&hcan1);

    TEST_ASSERT_FALSE(s_bTxCallbackInvoked);
}

void test_HAL_CAN_TxMailbox1CompleteCallback_InvalidHandle(void)
{
    CAN_HandleTypeDef hInvalidCan;
    hInvalidCan.Instance = (CAN_TypeDef*)0x99999999;

    HAL_CAN_TxMailbox1CompleteCallback(&hInvalidCan);
}

void test_HAL_CAN_TxMailbox1CompleteCallback_WithQueuedMessage(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    BspCanRegisterTxCallback(hCan, sTestTxCallback);

    BspCanMessage_t tMsg = {.uId = 0x456, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 4};
    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);
    BspCanTransmit(hCan, &tMsg, 1, 0xDEAD);

    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 1);
    HAL_CAN_AddTxMessage_IgnoreAndReturn(HAL_OK);

    HAL_CAN_TxMailbox1CompleteCallback(&hcan1);

    TEST_ASSERT_TRUE(s_bTxCallbackInvoked);
}

void test_HAL_CAN_TxMailbox2CompleteCallback_WithCallback(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanRegisterTxCallback(hCan, sTestTxCallback);

    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);

    HAL_CAN_TxMailbox2CompleteCallback(&hcan1);

    TEST_ASSERT_TRUE(s_bTxCallbackInvoked);
}

void test_HAL_CAN_TxMailbox2CompleteCallback_NoCallback(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    (void)BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);

    HAL_CAN_TxMailbox2CompleteCallback(&hcan1);

    TEST_ASSERT_FALSE(s_bTxCallbackInvoked);
}

void test_HAL_CAN_TxMailbox2CompleteCallback_InvalidHandle(void)
{
    CAN_HandleTypeDef hInvalidCan;
    hInvalidCan.Instance = (CAN_TypeDef*)0x99999999;

    HAL_CAN_TxMailbox2CompleteCallback(&hInvalidCan);
}

void test_HAL_CAN_TxMailbox2CompleteCallback_WithQueuedMessage(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    BspCanRegisterTxCallback(hCan, sTestTxCallback);

    BspCanMessage_t tMsg = {.uId = 0x789, .eIdType = eBSP_CAN_ID_EXTENDED, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 2};
    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);
    BspCanTransmit(hCan, &tMsg, 2, 0xBEEF);

    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 1);
    HAL_CAN_AddTxMessage_IgnoreAndReturn(HAL_OK);

    HAL_CAN_TxMailbox2CompleteCallback(&hcan1);

    TEST_ASSERT_TRUE(s_bTxCallbackInvoked);
}

/* ============================================================================
 * Test Cases - HAL Error Callbacks
 * ========================================================================== */

void test_HAL_CAN_ErrorCallback_BusOff_WithCallbacks(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanRegisterErrorCallback(hCan, sTestErrorCallback);
    BspCanRegisterBusStateCallback(hCan, sTestBusStateCallback);

    HAL_CAN_GetError_ExpectAndReturn(&hcan1, HAL_CAN_ERROR_BOF);

    HAL_CAN_ErrorCallback(&hcan1);

    TEST_ASSERT_TRUE(s_bErrorCallbackInvoked);
    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_BUS_OFF, s_eLastError);
    TEST_ASSERT_TRUE(s_bBusStateCallbackInvoked);
    TEST_ASSERT_EQUAL(eBSP_CAN_STATE_BUS_OFF, s_eLastBusState);
}

void test_HAL_CAN_ErrorCallback_BusOff_NoCallbacks(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    (void)BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_GetError_ExpectAndReturn(&hcan1, HAL_CAN_ERROR_BOF);

    HAL_CAN_ErrorCallback(&hcan1);

    TEST_ASSERT_FALSE(s_bErrorCallbackInvoked);
    TEST_ASSERT_FALSE(s_bBusStateCallbackInvoked);
}

void test_HAL_CAN_ErrorCallback_ErrorPassive_WithCallbacks(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanRegisterErrorCallback(hCan, sTestErrorCallback);
    BspCanRegisterBusStateCallback(hCan, sTestBusStateCallback);

    HAL_CAN_GetError_ExpectAndReturn(&hcan1, HAL_CAN_ERROR_EPV);

    HAL_CAN_ErrorCallback(&hcan1);

    TEST_ASSERT_TRUE(s_bErrorCallbackInvoked);
    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_BUS_PASSIVE, s_eLastError);
    TEST_ASSERT_TRUE(s_bBusStateCallbackInvoked);
    TEST_ASSERT_EQUAL(eBSP_CAN_STATE_ERROR_PASSIVE, s_eLastBusState);
}

void test_HAL_CAN_ErrorCallback_ErrorPassive_NoCallbacks(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    (void)BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_GetError_ExpectAndReturn(&hcan1, HAL_CAN_ERROR_EPV);

    HAL_CAN_ErrorCallback(&hcan1);

    TEST_ASSERT_FALSE(s_bErrorCallbackInvoked);
    TEST_ASSERT_FALSE(s_bBusStateCallbackInvoked);
}

void test_HAL_CAN_ErrorCallback_GenericError_WithCallback(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanRegisterErrorCallback(hCan, sTestErrorCallback);

    HAL_CAN_GetError_ExpectAndReturn(&hcan1, HAL_CAN_ERROR_NONE);

    HAL_CAN_ErrorCallback(&hcan1);

    TEST_ASSERT_TRUE(s_bErrorCallbackInvoked);
    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_HAL_ERROR, s_eLastError);
}

void test_HAL_CAN_ErrorCallback_GenericError_NoCallback(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    (void)BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_GetError_ExpectAndReturn(&hcan1, HAL_CAN_ERROR_NONE);

    HAL_CAN_ErrorCallback(&hcan1);

    TEST_ASSERT_FALSE(s_bErrorCallbackInvoked);
}

void test_HAL_CAN_ErrorCallback_InvalidHandle(void)
{
    CAN_HandleTypeDef hInvalidCan;
    hInvalidCan.Instance = (CAN_TypeDef*)0x99999999;

    HAL_CAN_ErrorCallback(&hInvalidCan);
}

/* ============================================================================
 * Test Cases - Instance 2 Coverage
 * ========================================================================== */

void test_BspCanAllocate_Instance2_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_2, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};

    BspCanHandle_t hCan = BspCanAllocate(&tConfig, NULL, NULL);

    TEST_ASSERT_NOT_EQUAL(BSP_CAN_INVALID_HANDLE, hCan);
}

void test_HAL_CAN_RxFifo0MsgPendingCallback_Instance2(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_2, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanRegisterRxCallback(hCan, sTestRxCallback);

    HAL_CAN_GetRxMessage_ExpectAndReturn(&hcan2, CAN_RX_FIFO0, NULL, NULL, HAL_OK);
    HAL_CAN_GetRxMessage_IgnoreArg_pHeader();
    HAL_CAN_GetRxMessage_IgnoreArg_aData();

    HAL_CAN_RxFifo0MsgPendingCallback(&hcan2);

    TEST_ASSERT_TRUE(s_bRxCallbackInvoked);
}

void test_HAL_CAN_TxMailbox0CompleteCallback_Instance2(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_2, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanRegisterTxCallback(hCan, sTestTxCallback);

    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan2, 0);

    HAL_CAN_TxMailbox0CompleteCallback(&hcan2);

    TEST_ASSERT_TRUE(s_bTxCallbackInvoked);
}

void test_HAL_CAN_ErrorCallback_Instance2(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_2, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanRegisterErrorCallback(hCan, sTestErrorCallback);

    HAL_CAN_GetError_ExpectAndReturn(&hcan2, HAL_CAN_ERROR_BOF);

    HAL_CAN_ErrorCallback(&hcan2);

    TEST_ASSERT_TRUE(s_bErrorCallbackInvoked);
}

/* ============================================================================
 * Test Cases - Additional Edge Cases
 * ========================================================================== */

void test_BspCanStart_NoFilters_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    /* Start without adding any filters */
    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);

    BspCanError_e eError = BspCanStart(hCan);

    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

void test_BspCanTransmit_AllPriorities_Success(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    BspCanMessage_t tMsg = {.uId = 0x123, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 8};

    /* Test all 8 priority levels (0-7) */
    for (uint8_t prio = 0; prio < 8; prio++)
    {
        HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 1);
        HAL_CAN_AddTxMessage_IgnoreAndReturn(HAL_OK);

        BspCanError_e eError = BspCanTransmit(hCan, &tMsg, prio, 0x1000 + prio);
        TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
    }
}

void test_HAL_CAN_TxMailbox0CompleteCallback_WithQueuedMessage_AndTxLED(void)
{
    LiveLed_t       txLed   = {.ePin = eM_LED2};
    BspCanConfig_t  tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t  hCan    = BspCanAllocate(&tConfig, &txLed, NULL);
    BspCanMessage_t tMsg    = {.uId = 0x123, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 8};

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    /* Queue a message when no mailbox available */
    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);
    BspCanTransmit(hCan, &tMsg, 0, 0x1234);

    /* Mailbox complete callback triggers dequeue and sends queued message */
    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 1);
    HAL_CAN_AddTxMessage_IgnoreAndReturn(HAL_OK);
    HAL_GPIO_TogglePin_Ignore(); /* TX LED blink */

    HAL_CAN_TxMailbox0CompleteCallback(&hcan1);
}

void test_HAL_CAN_RxFifo0MsgPendingCallback_ExtendedID(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanRegisterRxCallback(hCan, sTestRxCallback);

    /* Test with extended ID - the production code will parse it internally */
    HAL_CAN_GetRxMessage_ExpectAndReturn(&hcan1, CAN_RX_FIFO0, NULL, NULL, HAL_OK);
    HAL_CAN_GetRxMessage_IgnoreArg_pHeader();
    HAL_CAN_GetRxMessage_IgnoreArg_aData();

    HAL_CAN_RxFifo0MsgPendingCallback(&hcan1);

    TEST_ASSERT_TRUE(s_bRxCallbackInvoked);
}

void test_HAL_CAN_TxMailbox1CompleteCallback_WithQueuedMessage_AndTxLED(void)
{
    LiveLed_t       txLed   = {.ePin = eM_LED2};
    BspCanConfig_t  tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t  hCan    = BspCanAllocate(&tConfig, &txLed, NULL);
    BspCanMessage_t tMsg    = {.uId = 0x123, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 8};

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    /* Queue a message when no mailbox available */
    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);
    BspCanTransmit(hCan, &tMsg, 0, 0x1234);

    /* Mailbox complete callback triggers dequeue and sends queued message with TX LED */
    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 1);
    HAL_CAN_AddTxMessage_IgnoreAndReturn(HAL_OK);
    HAL_GPIO_TogglePin_Ignore(); /* TX LED blink */

    HAL_CAN_TxMailbox1CompleteCallback(&hcan1);
}

void test_HAL_CAN_TxMailbox2CompleteCallback_WithQueuedMessage_AndTxLED(void)
{
    LiveLed_t       txLed   = {.ePin = eM_LED2};
    BspCanConfig_t  tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t  hCan    = BspCanAllocate(&tConfig, &txLed, NULL);
    BspCanMessage_t tMsg    = {.uId = 0x123, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 8};

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    /* Queue a message when no mailbox available */
    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);
    BspCanTransmit(hCan, &tMsg, 0, 0x1234);

    /* Mailbox complete callback triggers dequeue and sends queued message with TX LED */
    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 1);
    HAL_CAN_AddTxMessage_IgnoreAndReturn(HAL_OK);
    HAL_GPIO_TogglePin_Ignore(); /* TX LED blink */

    HAL_CAN_TxMailbox2CompleteCallback(&hcan1);
}

void test_HAL_CAN_RxFifo0MsgPendingCallback_RemoteFrame(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanRegisterRxCallback(hCan, sTestRxCallback);

    /* Test remote frame - tests RTR branch in sParseRxMessage */
    HAL_CAN_GetRxMessage_ExpectAndReturn(&hcan1, CAN_RX_FIFO0, NULL, NULL, HAL_OK);
    HAL_CAN_GetRxMessage_IgnoreArg_pHeader();
    HAL_CAN_GetRxMessage_IgnoreArg_aData();

    HAL_CAN_RxFifo0MsgPendingCallback(&hcan1);

    TEST_ASSERT_TRUE(s_bRxCallbackInvoked);
}

void test_BspCanTransmit_HighPriority_MailboxAssignment(void)
{
    LiveLed_t       txLed   = {.ePin = eM_LED2};
    BspCanConfig_t  tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t  hCan    = BspCanAllocate(&tConfig, &txLed, NULL);
    BspCanMessage_t tMsg    = {.uId = 0x123, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 8};

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    /* Test mailbox assignment variations - AddTxMessage returns different mailboxes */
    /* This tests sMailboxToIndex branches for CAN_TX_MAILBOX1 and CAN_TX_MAILBOX2 */
    for (int i = 0; i < 3; i++)
    {
        HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 1);
        HAL_CAN_AddTxMessage_IgnoreAndReturn(HAL_OK);
        HAL_GPIO_TogglePin_Ignore(); /* TX LED */
        BspCanTransmit(hCan, &tMsg, 0, 0x1000 + i);
    }
}

void test_BspCanAbortTransmit_MessageInMultiplePriorities(void)
{
    BspCanConfig_t  tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t  hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanMessage_t tMsg    = {.uId = 0x100, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 8};

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    /* Queue messages at different priorities */
    for (uint8_t prio = 0; prio < 3; prio++)
    {
        HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);
        BspCanTransmit(hCan, &tMsg, prio, 0x2000 + prio);
    }

    /* Abort middle priority message - tests priority queue search across levels */
    BspCanError_e eError = BspCanAbortTransmit(hCan, 0x2001);
    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

void test_BspCanTransmit_InvalidPriority_UpperBound(void)
{
    BspCanConfig_t  tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t  hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanMessage_t tMsg    = {.uId = 0x123, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 8};

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    /* Test priority = 8 (one above max of 7) */
    BspCanError_e eError = BspCanTransmit(hCan, &tMsg, 8, 0x1234);
    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_PARAM, eError);

    /* Test priority = 255 (max uint8_t) */
    eError = BspCanTransmit(hCan, &tMsg, 255, 0x1235);
    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_PARAM, eError);
}

void test_BspCanAbortTransmit_EmptyQueue_SearchAllPriorities(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    /* Try to abort when queue is empty - tests search through all priority levels */
    BspCanError_e eError = BspCanAbortTransmit(hCan, 0x9999);
    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_INVALID_PARAM, eError);
}

void test_HAL_CAN_TxMailbox_AllThreeMailboxes_WithQueueDrain(void)
{
    LiveLed_t       txLed   = {.ePin = eM_LED2};
    BspCanConfig_t  tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t  hCan    = BspCanAllocate(&tConfig, &txLed, NULL);
    BspCanMessage_t tMsg    = {.uId = 0x456, .eIdType = eBSP_CAN_ID_EXTENDED, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 4};

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);
    BspCanRegisterTxCallback(hCan, sTestTxCallback);

    /* Queue 3 messages when no mailboxes available */
    for (int i = 0; i < 3; i++)
    {
        HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);
        BspCanTransmit(hCan, &tMsg, i, 0x3000 + i);
    }

    /* Test all three mailbox callbacks draining the queue with TX LED */
    /* Mailbox 0 complete */
    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 1);
    HAL_CAN_AddTxMessage_IgnoreAndReturn(HAL_OK);
    HAL_GPIO_TogglePin_Ignore();
    HAL_CAN_TxMailbox0CompleteCallback(&hcan1);
    TEST_ASSERT_TRUE(s_bTxCallbackInvoked);
    s_bTxCallbackInvoked = false;

    /* Mailbox 1 complete */
    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 1);
    HAL_CAN_AddTxMessage_IgnoreAndReturn(HAL_OK);
    HAL_GPIO_TogglePin_Ignore();
    HAL_CAN_TxMailbox1CompleteCallback(&hcan1);
    TEST_ASSERT_TRUE(s_bTxCallbackInvoked);
    s_bTxCallbackInvoked = false;

    /* Mailbox 2 complete */
    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 1);
    HAL_CAN_AddTxMessage_IgnoreAndReturn(HAL_OK);
    HAL_GPIO_TogglePin_Ignore();
    HAL_CAN_TxMailbox2CompleteCallback(&hcan1);
    TEST_ASSERT_TRUE(s_bTxCallbackInvoked);
}

void test_BspCanTransmit_ExtendedID_WithTxLED(void)
{
    LiveLed_t       txLed   = {.ePin = eM_LED1};
    BspCanConfig_t  tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t  hCan    = BspCanAllocate(&tConfig, &txLed, NULL);
    BspCanMessage_t tMsg    = {.uId = 0x1FFFFFFF, .eIdType = eBSP_CAN_ID_EXTENDED, .eFrameType = eBSP_CAN_FRAME_REMOTE, .byDataLen = 0};

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    /* Test extended ID + remote frame combination with TX LED */
    HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 1);
    HAL_CAN_AddTxMessage_IgnoreAndReturn(HAL_OK);
    HAL_GPIO_TogglePin_Ignore(); /* TX LED blink */

    BspCanError_e eError = BspCanTransmit(hCan, &tMsg, 7, 0x4000);
    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}

void test_HAL_CAN_RxFifo1MsgPendingCallback_RemoteFrame(void)
{
    BspCanConfig_t tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanRegisterRxCallback(hCan, sTestRxCallback);

    /* Test remote frame on FIFO1 - ensures both FIFOs tested with remote frames */
    HAL_CAN_GetRxMessage_ExpectAndReturn(&hcan1, CAN_RX_FIFO1, NULL, NULL, HAL_OK);
    HAL_CAN_GetRxMessage_IgnoreArg_pHeader();
    HAL_CAN_GetRxMessage_IgnoreArg_aData();

    HAL_CAN_RxFifo1MsgPendingCallback(&hcan1);

    TEST_ASSERT_TRUE(s_bRxCallbackInvoked);
}

void test_BspCanTransmit_QueuedMessages_AllPriorities(void)
{
    BspCanConfig_t  tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t  hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanMessage_t tMsg    = {.uId = 0x200, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 8};

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    /* Queue one message at each priority level to test all queue branches */
    for (uint8_t prio = 0; prio < 8; prio++)
    {
        HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);
        BspCanError_e eError = BspCanTransmit(hCan, &tMsg, prio, 0x5000 + prio);
        TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
    }

    /* Verify queue info shows all priorities used */
    uint8_t byUsed = 0;
    BspCanGetTxQueueInfo(hCan, &byUsed, NULL);
    TEST_ASSERT_EQUAL(8, byUsed);
}

void test_BspCanAbortTransmit_FromHighestPriority(void)
{
    BspCanConfig_t  tConfig = {.eInstance = eBSP_CAN_INSTANCE_1, .bLoopback = false, .bSilent = false, .bAutoRetransmit = true};
    BspCanHandle_t  hCan    = BspCanAllocate(&tConfig, NULL, NULL);
    BspCanMessage_t tMsg    = {.uId = 0x300, .eIdType = eBSP_CAN_ID_STANDARD, .eFrameType = eBSP_CAN_FRAME_DATA, .byDataLen = 8};

    HAL_CAN_Start_ExpectAndReturn(&hcan1, HAL_OK);
    HAL_CAN_ActivateNotification_IgnoreAndReturn(HAL_OK);
    BspCanStart(hCan);

    /* Queue messages at priority 0 (highest) */
    for (int i = 0; i < 3; i++)
    {
        HAL_CAN_GetTxMailboxesFreeLevel_ExpectAndReturn(&hcan1, 0);
        BspCanTransmit(hCan, &tMsg, 0, 0x6000 + i);
    }

    /* Abort from priority 0 - tests bitmap update branch when queue becomes empty */
    BspCanError_e eError = BspCanAbortTransmit(hCan, 0x6001);
    TEST_ASSERT_EQUAL(eBSP_CAN_ERR_NONE, eError);
}
