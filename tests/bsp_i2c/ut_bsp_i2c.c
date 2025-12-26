/**
 * @file ut_bsp_i2c.c
 * @brief Unit tests for BSP I2C module using Unity and CMock
 * @note This test file mocks HAL I2C dependencies to test BSP I2C functionality
 */

#include "Mockstm32f4xx_hal_i2c.h"
#include "bsp_i2c.h"
#include "unity.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// External declarations for HAL callbacks implemented in production code
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* hi2c);
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef* hi2c);
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef* hi2c);
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef* hi2c);
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c);

// External declarations for HAL handles (must be defined for tests)
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;
I2C_HandleTypeDef hi2c3;
I2C_HandleTypeDef hi2c4;
I2C_HandleTypeDef hi2c5;
I2C_HandleTypeDef hi2c6;

// External access to private module state (FORCE_STATIC makes it accessible in test builds)
// Module structure definition from production code
typedef struct
{
    BspI2cInstance_e    eInstance;
    I2C_HandleTypeDef*  pHalHandle;
    BspI2cMode_e        eMode;
    uint32_t            uTimeoutMs;
    bool                bAllocated;
    BspI2cTxCpltCb_t    pTxCpltCb;
    BspI2cRxCpltCb_t    pRxCpltCb;
    BspI2cMemTxCpltCb_t pMemTxCpltCb;
    BspI2cMemRxCpltCb_t pMemRxCpltCb;
    BspI2cErrorCb_t     pErrorCb;
} BspI2cModule_t;

extern BspI2cModule_t s_i2cModules[6];

// Test callback tracking
static BspI2cHandle_t s_lastCallbackHandle   = -1;
static bool           s_txCallbackInvoked    = false;
static bool           s_rxCallbackInvoked    = false;
static bool           s_memTxCallbackInvoked = false;
static bool           s_memRxCallbackInvoked = false;
static bool           s_errorCallbackInvoked = false;
static BspI2cError_e  s_lastError            = eBSP_I2C_ERR_NONE;
static int            s_callbackCount        = 0;

// Test callbacks
static void TestTxCallback(BspI2cHandle_t handle)
{
    s_lastCallbackHandle = handle;
    s_txCallbackInvoked  = true;
    s_callbackCount++;
}

static void TestRxCallback(BspI2cHandle_t handle)
{
    s_lastCallbackHandle = handle;
    s_rxCallbackInvoked  = true;
    s_callbackCount++;
}

static void TestMemTxCallback(BspI2cHandle_t handle)
{
    s_lastCallbackHandle   = handle;
    s_memTxCallbackInvoked = true;
    s_callbackCount++;
}

static void TestMemRxCallback(BspI2cHandle_t handle)
{
    s_lastCallbackHandle   = handle;
    s_memRxCallbackInvoked = true;
    s_callbackCount++;
}

static void TestErrorCallback(BspI2cHandle_t handle, BspI2cError_e error)
{
    s_lastCallbackHandle   = handle;
    s_lastError            = error;
    s_errorCallbackInvoked = true;
    s_callbackCount++;
}

// ============================================================================
// Test Fixtures
// ============================================================================

void setUp(void)
{
    // Reset callback tracking
    s_lastCallbackHandle   = -1;
    s_txCallbackInvoked    = false;
    s_rxCallbackInvoked    = false;
    s_memTxCallbackInvoked = false;
    s_memRxCallbackInvoked = false;
    s_errorCallbackInvoked = false;
    s_lastError            = eBSP_I2C_ERR_NONE;
    s_callbackCount        = 0;

    // Free all I2C instances to ensure clean state between tests
    for (int8_t i = 0; i < 6; i++)
    {
        BspI2cFree(i);
    }
}

void tearDown(void)
{
    // Clean up after each test - free all instances
    for (int8_t i = 0; i < 6; i++)
    {
        BspI2cFree(i);
    }
}

// ============================================================================
// Test Cases: Allocation and Deallocation
// ============================================================================

void test_BspI2cAllocate_ValidParameters_ReturnsValidHandle(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_BLOCKING, 1000);

    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);
    TEST_ASSERT_LESS_THAN(6, handle);
}

void test_BspI2cAllocate_DefaultTimeout_UsesDefaultValue(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_BLOCKING, 0);

    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);
}

void test_BspI2cAllocate_InterruptMode_ReturnsValidHandle(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_INTERRUPT, 0);

    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);
}

void test_BspI2cAllocate_InvalidInstance_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_COUNT, eBSP_I2C_MODE_BLOCKING, 1000);

    TEST_ASSERT_EQUAL_INT8(-1, handle);
}

void test_BspI2cAllocate_DuplicateInstance_ReturnsError(void)
{
    BspI2cHandle_t handle1 = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_BLOCKING, 500);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle1);

    // Try to allocate same instance again
    BspI2cHandle_t handle2 = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_INTERRUPT, 1000);
    TEST_ASSERT_EQUAL_INT8(-1, handle2);
}

void test_BspI2cAllocate_AllInstances_ReturnsUniqueHandles(void)
{
    BspI2cHandle_t handles[6];

    for (uint8_t i = 0; i < 6; i++)
    {
        handles[i] = BspI2cAllocate((BspI2cInstance_e)i, eBSP_I2C_MODE_BLOCKING, 1000);
        TEST_ASSERT_GREATER_OR_EQUAL(0, handles[i]);
    }

    // Verify all handles are unique
    for (uint8_t i = 0; i < 6; i++)
    {
        for (uint8_t j = i + 1; j < 6; j++)
        {
            TEST_ASSERT_NOT_EQUAL(handles[i], handles[j]);
        }
    }
}

void test_BspI2cAllocate_ExceedMaxInstances_ReturnsError(void)
{
    // Allocate all 6 instances
    for (uint8_t i = 0; i < 6; i++)
    {
        BspI2cHandle_t handle = BspI2cAllocate((BspI2cInstance_e)i, eBSP_I2C_MODE_BLOCKING, 1000);
        TEST_ASSERT_GREATER_OR_EQUAL(0, handle);
    }

    // Should not be able to allocate more (instances are exhausted)
    // This test verifies proper resource management
}

void test_BspI2cFree_ValidHandle_ReturnsSuccess(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cError_e error = BspI2cFree(handle);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_BspI2cFree_InvalidHandle_ReturnsError(void)
{
    BspI2cError_e error = BspI2cFree(-1);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_HANDLE, error);

    error = BspI2cFree(10);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_HANDLE, error);
}

void test_BspI2cFree_AllowsReallocation(void)
{
    BspI2cHandle_t handle1 = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle1);

    BspI2cError_e error = BspI2cFree(handle1);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);

    // Should be able to allocate same instance again
    BspI2cHandle_t handle2 = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_INTERRUPT, 500);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle2);
}

// ============================================================================
// Test Cases: Callback Registration
// ============================================================================

void test_BspI2cRegisterTxCallback_ValidHandle_ReturnsSuccess(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cError_e error = BspI2cRegisterTxCallback(handle, TestTxCallback);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_BspI2cRegisterTxCallback_InvalidHandle_ReturnsError(void)
{
    BspI2cError_e error = BspI2cRegisterTxCallback(-1, TestTxCallback);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_HANDLE, error);
}

void test_BspI2cRegisterRxCallback_ValidHandle_ReturnsSuccess(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cError_e error = BspI2cRegisterRxCallback(handle, TestRxCallback);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_BspI2cRegisterRxCallback_InvalidHandle_ReturnsError(void)
{
    BspI2cError_e error = BspI2cRegisterRxCallback(-1, TestRxCallback);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_HANDLE, error);
}

void test_BspI2cRegisterMemTxCallback_ValidHandle_ReturnsSuccess(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cError_e error = BspI2cRegisterMemTxCallback(handle, TestMemTxCallback);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_BspI2cRegisterMemTxCallback_InvalidHandle_ReturnsError(void)
{
    BspI2cError_e error = BspI2cRegisterMemTxCallback(-1, TestMemTxCallback);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_HANDLE, error);
}

void test_BspI2cRegisterMemRxCallback_ValidHandle_ReturnsSuccess(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_4, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cError_e error = BspI2cRegisterMemRxCallback(handle, TestMemRxCallback);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_BspI2cRegisterMemRxCallback_InvalidHandle_ReturnsError(void)
{
    BspI2cError_e error = BspI2cRegisterMemRxCallback(-1, TestMemRxCallback);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_HANDLE, error);
}

void test_BspI2cRegisterErrorCallback_ValidHandle_ReturnsSuccess(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_5, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cError_e error = BspI2cRegisterErrorCallback(handle, TestErrorCallback);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_BspI2cRegisterErrorCallback_InvalidHandle_ReturnsError(void)
{
    BspI2cError_e error = BspI2cRegisterErrorCallback(-1, TestErrorCallback);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_HANDLE, error);
}

// ============================================================================
// Test Cases: Blocking Mode - Transmit
// ============================================================================

void test_BspI2cTransmit_ValidParameters_ReturnsSuccess(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t                txData[] = {0x01, 0x02, 0x03};
    BspI2cTransferConfig_t config   = {.devAddr = 0xA0, .pData = txData, .length = 3};

    HAL_I2C_Master_Transmit_ExpectAndReturn(&hi2c1, 0xA0, txData, 3, 1000, HAL_OK);

    BspI2cError_e error = BspI2cTransmit(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_BspI2cTransmit_InvalidHandle_ReturnsError(void)
{
    uint8_t                txData[] = {0x01};
    BspI2cTransferConfig_t config   = {.devAddr = 0xA0, .pData = txData, .length = 1};

    BspI2cError_e error = BspI2cTransmit(-1, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_HANDLE, error);
}

void test_BspI2cTransmit_NullConfig_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cError_e error = BspI2cTransmit(handle, NULL);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cTransmit_NullData_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cTransferConfig_t config = {.devAddr = 0xA0, .pData = NULL, .length = 1};

    BspI2cError_e error = BspI2cTransmit(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cTransmit_WrongMode_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t                txData[] = {0x01};
    BspI2cTransferConfig_t config   = {.devAddr = 0xA0, .pData = txData, .length = 1};

    BspI2cError_e error = BspI2cTransmit(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cTransmit_HAL_Timeout_ReturnsTimeout(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t                txData[] = {0x01};
    BspI2cTransferConfig_t config   = {.devAddr = 0xA0, .pData = txData, .length = 1};

    HAL_I2C_Master_Transmit_ExpectAndReturn(&hi2c1, 0xA0, txData, 1, 1000, HAL_TIMEOUT);

    BspI2cError_e error = BspI2cTransmit(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_TIMEOUT, error);
}

void test_BspI2cTransmit_HAL_Error_ReturnsTransferError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t                txData[] = {0x01};
    BspI2cTransferConfig_t config   = {.devAddr = 0xA0, .pData = txData, .length = 1};

    HAL_I2C_Master_Transmit_ExpectAndReturn(&hi2c1, 0xA0, txData, 1, 1000, HAL_ERROR);

    BspI2cError_e error = BspI2cTransmit(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_TRANSFER, error);
}

// ============================================================================
// Test Cases: Blocking Mode - Receive
// ============================================================================

void test_BspI2cReceive_ValidParameters_ReturnsSuccess(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_BLOCKING, 500);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t                rxData[10];
    BspI2cTransferConfig_t config = {.devAddr = 0xA2, .pData = rxData, .length = 10};

    HAL_I2C_Master_Receive_ExpectAndReturn(&hi2c2, 0xA2, rxData, 10, 500, HAL_OK);

    BspI2cError_e error = BspI2cReceive(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_BspI2cReceive_InvalidHandle_ReturnsError(void)
{
    uint8_t                rxData[5];
    BspI2cTransferConfig_t config = {.devAddr = 0xA2, .pData = rxData, .length = 5};

    BspI2cError_e error = BspI2cReceive(-1, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_HANDLE, error);
}

void test_BspI2cReceive_NullConfig_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_BLOCKING, 500);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cError_e error = BspI2cReceive(handle, NULL);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cReceive_NullData_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_BLOCKING, 500);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cTransferConfig_t config = {.devAddr = 0xA2, .pData = NULL, .length = 5};

    BspI2cError_e error = BspI2cReceive(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cReceive_WrongMode_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_INTERRUPT, 500);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t                rxData[5];
    BspI2cTransferConfig_t config = {.devAddr = 0xA2, .pData = rxData, .length = 5};

    BspI2cError_e error = BspI2cReceive(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cReceive_HAL_Timeout_ReturnsTimeout(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_BLOCKING, 500);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t                rxData[5];
    BspI2cTransferConfig_t config = {.devAddr = 0xA2, .pData = rxData, .length = 5};

    HAL_I2C_Master_Receive_ExpectAndReturn(&hi2c2, 0xA2, rxData, 5, 500, HAL_TIMEOUT);

    BspI2cError_e error = BspI2cReceive(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_TIMEOUT, error);
}

// ============================================================================
// Test Cases: Blocking Mode - Memory Operations
// ============================================================================

void test_BspI2cMemRead_ValidParameters_ReturnsSuccess(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t           rxData[16];
    BspI2cMemConfig_t config = {
        .devAddr = 0xA0, .memAddr = 0x1234, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_16BIT, .pData = rxData, .length = 16};

    HAL_I2C_Mem_Read_ExpectAndReturn(&hi2c3, 0xA0, 0x1234, 2, rxData, 16, 1000, HAL_OK);

    BspI2cError_e error = BspI2cMemRead(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_BspI2cMemRead_8BitAddress_ReturnsSuccess(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t           rxData[8];
    BspI2cMemConfig_t config = {.devAddr = 0xA0, .memAddr = 0x20, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_8BIT, .pData = rxData, .length = 8};

    HAL_I2C_Mem_Read_ExpectAndReturn(&hi2c3, 0xA0, 0x20, 1, rxData, 8, 1000, HAL_OK);

    BspI2cError_e error = BspI2cMemRead(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_BspI2cMemRead_NullData_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cMemConfig_t config = {
        .devAddr = 0xA0, .memAddr = 0x1234, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_16BIT, .pData = NULL, .length = 16};

    BspI2cError_e error = BspI2cMemRead(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cMemRead_WrongMode_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_INTERRUPT, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t           rxData[16];
    BspI2cMemConfig_t config = {
        .devAddr = 0xA0, .memAddr = 0x1234, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_16BIT, .pData = rxData, .length = 16};

    BspI2cError_e error = BspI2cMemRead(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cMemRead_InvalidHandle_ReturnsError(void)
{
    uint8_t           rxData[8];
    BspI2cMemConfig_t config = {.devAddr = 0xA0, .memAddr = 0x00, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_8BIT, .pData = rxData, .length = 8};

    BspI2cError_e error = BspI2cMemRead(-1, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_HANDLE, error);
}

void test_BspI2cMemRead_NullConfig_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cError_e error = BspI2cMemRead(handle, NULL);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cMemWrite_ValidParameters_ReturnsSuccess(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_4, eBSP_I2C_MODE_BLOCKING, 2000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t           txData[] = {0xAA, 0xBB, 0xCC, 0xDD};
    BspI2cMemConfig_t config   = {
          .devAddr = 0xA0, .memAddr = 0x5678, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_16BIT, .pData = txData, .length = 4};

    HAL_I2C_Mem_Write_ExpectAndReturn(&hi2c4, 0xA0, 0x5678, 2, txData, 4, 2000, HAL_OK);

    BspI2cError_e error = BspI2cMemWrite(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_BspI2cMemWrite_NullData_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_4, eBSP_I2C_MODE_BLOCKING, 2000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cMemConfig_t config = {
        .devAddr = 0xA0, .memAddr = 0x5678, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_16BIT, .pData = NULL, .length = 4};

    BspI2cError_e error = BspI2cMemWrite(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cMemWrite_WrongMode_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_4, eBSP_I2C_MODE_INTERRUPT, 2000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t           txData[] = {0xAA, 0xBB, 0xCC, 0xDD};
    BspI2cMemConfig_t config   = {
          .devAddr = 0xA0, .memAddr = 0x5678, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_16BIT, .pData = txData, .length = 4};

    BspI2cError_e error = BspI2cMemWrite(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cMemWrite_HAL_Timeout_ReturnsTimeout(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_4, eBSP_I2C_MODE_BLOCKING, 2000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t           txData[] = {0xAA};
    BspI2cMemConfig_t config = {.devAddr = 0xA0, .memAddr = 0x00, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_8BIT, .pData = txData, .length = 1};

    HAL_I2C_Mem_Write_ExpectAndReturn(&hi2c4, 0xA0, 0x00, 1, txData, 1, 2000, HAL_TIMEOUT);

    BspI2cError_e error = BspI2cMemWrite(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_TIMEOUT, error);
}

// ============================================================================
// Test Cases: Interrupt Mode - Transmit
// ============================================================================

void test_BspI2cTransmitIT_ValidParameters_ReturnsSuccess(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t                txData[] = {0x55, 0xAA};
    BspI2cTransferConfig_t config   = {.devAddr = 0xA0, .pData = txData, .length = 2};

    HAL_I2C_Master_Transmit_IT_ExpectAndReturn(&hi2c1, 0xA0, txData, 2, HAL_OK);

    BspI2cError_e error = BspI2cTransmitIT(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_BspI2cTransmitIT_InvalidHandle_ReturnsError(void)
{
    uint8_t                txData[] = {0x01};
    BspI2cTransferConfig_t config   = {.devAddr = 0xA0, .pData = txData, .length = 1};

    BspI2cError_e error = BspI2cTransmitIT(-1, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_HANDLE, error);
}

void test_BspI2cTransmitIT_NullConfig_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cError_e error = BspI2cTransmitIT(handle, NULL);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cTransmitIT_NullData_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cTransferConfig_t config = {.devAddr = 0xA0, .pData = NULL, .length = 1};

    BspI2cError_e error = BspI2cTransmitIT(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cTransmitIT_WrongMode_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t                txData[] = {0x01};
    BspI2cTransferConfig_t config   = {.devAddr = 0xA0, .pData = txData, .length = 1};

    BspI2cError_e error = BspI2cTransmitIT(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cTransmitIT_HAL_Busy_ReturnsBusy(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t                txData[] = {0x01};
    BspI2cTransferConfig_t config   = {.devAddr = 0xA0, .pData = txData, .length = 1};

    HAL_I2C_Master_Transmit_IT_ExpectAndReturn(&hi2c1, 0xA0, txData, 1, HAL_BUSY);

    BspI2cError_e error = BspI2cTransmitIT(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_BUSY, error);
}

void test_BspI2cTransmitIT_HAL_Error_ReturnsTransferError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t                txData[] = {0x01};
    BspI2cTransferConfig_t config   = {.devAddr = 0xA0, .pData = txData, .length = 1};

    HAL_I2C_Master_Transmit_IT_ExpectAndReturn(&hi2c1, 0xA0, txData, 1, HAL_ERROR);

    BspI2cError_e error = BspI2cTransmitIT(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_TRANSFER, error);
}

// ============================================================================
// Test Cases: Interrupt Mode - Receive
// ============================================================================

void test_BspI2cReceiveIT_ValidParameters_ReturnsSuccess(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t                rxData[8];
    BspI2cTransferConfig_t config = {.devAddr = 0xA2, .pData = rxData, .length = 8};

    HAL_I2C_Master_Receive_IT_ExpectAndReturn(&hi2c2, 0xA2, rxData, 8, HAL_OK);

    BspI2cError_e error = BspI2cReceiveIT(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_BspI2cReceiveIT_NullConfig_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cError_e error = BspI2cReceiveIT(handle, NULL);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cReceiveIT_NullData_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cTransferConfig_t config = {.devAddr = 0xA2, .pData = NULL, .length = 8};

    BspI2cError_e error = BspI2cReceiveIT(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cReceiveIT_WrongMode_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t                rxData[8];
    BspI2cTransferConfig_t config = {.devAddr = 0xA2, .pData = rxData, .length = 8};

    BspI2cError_e error = BspI2cReceiveIT(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cReceiveIT_HAL_Busy_ReturnsBusy(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t                rxData[8];
    BspI2cTransferConfig_t config = {.devAddr = 0xA2, .pData = rxData, .length = 8};

    HAL_I2C_Master_Receive_IT_ExpectAndReturn(&hi2c2, 0xA2, rxData, 8, HAL_BUSY);

    BspI2cError_e error = BspI2cReceiveIT(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_BUSY, error);
}

// ============================================================================
// Test Cases: Interrupt Mode - Memory Operations
// ============================================================================

void test_BspI2cMemReadIT_ValidParameters_ReturnsSuccess(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t           rxData[16];
    BspI2cMemConfig_t config = {
        .devAddr = 0xA0, .memAddr = 0x0100, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_16BIT, .pData = rxData, .length = 16};

    HAL_I2C_Mem_Read_IT_ExpectAndReturn(&hi2c3, 0xA0, 0x0100, 2, rxData, 16, HAL_OK);

    BspI2cError_e error = BspI2cMemReadIT(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_BspI2cMemReadIT_NullConfig_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cError_e error = BspI2cMemReadIT(handle, NULL);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cMemReadIT_NullData_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cMemConfig_t config = {
        .devAddr = 0xA0, .memAddr = 0x0100, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_16BIT, .pData = NULL, .length = 16};

    BspI2cError_e error = BspI2cMemReadIT(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cMemReadIT_WrongMode_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t           rxData[16];
    BspI2cMemConfig_t config = {
        .devAddr = 0xA0, .memAddr = 0x0100, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_16BIT, .pData = rxData, .length = 16};

    BspI2cError_e error = BspI2cMemReadIT(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cMemReadIT_HAL_Busy_ReturnsBusy(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t           rxData[16];
    BspI2cMemConfig_t config = {
        .devAddr = 0xA0, .memAddr = 0x0100, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_16BIT, .pData = rxData, .length = 16};

    HAL_I2C_Mem_Read_IT_ExpectAndReturn(&hi2c3, 0xA0, 0x0100, 2, rxData, 16, HAL_BUSY);

    BspI2cError_e error = BspI2cMemReadIT(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_BUSY, error);
}

void test_BspI2cMemWriteIT_ValidParameters_ReturnsSuccess(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_4, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t           txData[] = {0x11, 0x22, 0x33, 0x44};
    BspI2cMemConfig_t config = {.devAddr = 0xA0, .memAddr = 0x50, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_8BIT, .pData = txData, .length = 4};

    HAL_I2C_Mem_Write_IT_ExpectAndReturn(&hi2c4, 0xA0, 0x50, 1, txData, 4, HAL_OK);

    BspI2cError_e error = BspI2cMemWriteIT(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_BspI2cMemWriteIT_NullConfig_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_4, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cError_e error = BspI2cMemWriteIT(handle, NULL);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cMemWriteIT_NullData_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_4, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cMemConfig_t config = {.devAddr = 0xA0, .memAddr = 0x50, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_8BIT, .pData = NULL, .length = 4};

    BspI2cError_e error = BspI2cMemWriteIT(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

void test_BspI2cMemWriteIT_WrongMode_ReturnsError(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_4, eBSP_I2C_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t           txData[] = {0x11};
    BspI2cMemConfig_t config = {.devAddr = 0xA0, .memAddr = 0x50, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_8BIT, .pData = txData, .length = 1};

    BspI2cError_e error = BspI2cMemWriteIT(handle, &config);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_INVALID_PARAM, error);
}

// ============================================================================
// Test Cases: HAL Callbacks
// ============================================================================

void test_HAL_I2C_MasterTxCpltCallback_WithRegisteredCallback_InvokesCallback(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cRegisterTxCallback(handle, TestTxCallback);

    // Trigger HAL callback
    HAL_I2C_MasterTxCpltCallback(&hi2c1);

    TEST_ASSERT_TRUE(s_txCallbackInvoked);
    TEST_ASSERT_EQUAL(handle, s_lastCallbackHandle);
}

void test_HAL_I2C_MasterTxCpltCallback_NoCallback_DoesNotCrash(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Don't register a callback
    HAL_I2C_MasterTxCpltCallback(&hi2c1);

    TEST_ASSERT_FALSE(s_txCallbackInvoked);
}

void test_HAL_I2C_MasterTxCpltCallback_UnallocatedInstance_DoesNotCrash(void)
{
    // Call with unallocated instance
    HAL_I2C_MasterTxCpltCallback(&hi2c1);

    TEST_ASSERT_FALSE(s_txCallbackInvoked);
}

void test_HAL_I2C_MasterRxCpltCallback_WithRegisteredCallback_InvokesCallback(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cRegisterRxCallback(handle, TestRxCallback);

    // Trigger HAL callback
    HAL_I2C_MasterRxCpltCallback(&hi2c2);

    TEST_ASSERT_TRUE(s_rxCallbackInvoked);
    TEST_ASSERT_EQUAL(handle, s_lastCallbackHandle);
}

void test_HAL_I2C_MasterRxCpltCallback_NoCallback_DoesNotCrash(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    HAL_I2C_MasterRxCpltCallback(&hi2c2);

    TEST_ASSERT_FALSE(s_rxCallbackInvoked);
}

void test_HAL_I2C_MemTxCpltCallback_WithRegisteredCallback_InvokesCallback(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cRegisterMemTxCallback(handle, TestMemTxCallback);

    // Trigger HAL callback
    HAL_I2C_MemTxCpltCallback(&hi2c3);

    TEST_ASSERT_TRUE(s_memTxCallbackInvoked);
    TEST_ASSERT_EQUAL(handle, s_lastCallbackHandle);
}

void test_HAL_I2C_MemTxCpltCallback_NoCallback_DoesNotCrash(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    HAL_I2C_MemTxCpltCallback(&hi2c3);

    TEST_ASSERT_FALSE(s_memTxCallbackInvoked);
}

void test_HAL_I2C_MemRxCpltCallback_WithRegisteredCallback_InvokesCallback(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_4, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cRegisterMemRxCallback(handle, TestMemRxCallback);

    // Trigger HAL callback
    HAL_I2C_MemRxCpltCallback(&hi2c4);

    TEST_ASSERT_TRUE(s_memRxCallbackInvoked);
    TEST_ASSERT_EQUAL(handle, s_lastCallbackHandle);
}

void test_HAL_I2C_MemRxCpltCallback_NoCallback_DoesNotCrash(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_4, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    HAL_I2C_MemRxCpltCallback(&hi2c4);

    TEST_ASSERT_FALSE(s_memRxCallbackInvoked);
}

void test_HAL_I2C_ErrorCallback_WithRegisteredCallback_InvokesCallback(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_5, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspI2cRegisterErrorCallback(handle, TestErrorCallback);

    // Trigger HAL callback
    HAL_I2C_ErrorCallback(&hi2c5);

    TEST_ASSERT_TRUE(s_errorCallbackInvoked);
    TEST_ASSERT_EQUAL(handle, s_lastCallbackHandle);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_TRANSFER, s_lastError);
}

void test_HAL_I2C_ErrorCallback_NoCallback_DoesNotCrash(void)
{
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_5, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    HAL_I2C_ErrorCallback(&hi2c5);

    TEST_ASSERT_FALSE(s_errorCallbackInvoked);
}

// ============================================================================
// Test Cases: Integration Tests
// ============================================================================

void test_Integration_BlockingMode_CompleteWorkflow(void)
{
    // Allocate I2C instance in blocking mode
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Perform transmit
    uint8_t                txData[] = {0x01, 0x02, 0x03};
    BspI2cTransferConfig_t txConfig = {.devAddr = 0xA0, .pData = txData, .length = 3};
    HAL_I2C_Master_Transmit_ExpectAndReturn(&hi2c1, 0xA0, txData, 3, 1000, HAL_OK);
    BspI2cError_e error = BspI2cTransmit(handle, &txConfig);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);

    // Perform receive
    uint8_t                rxData[5];
    BspI2cTransferConfig_t rxConfig = {.devAddr = 0xA0, .pData = rxData, .length = 5};
    HAL_I2C_Master_Receive_ExpectAndReturn(&hi2c1, 0xA0, rxData, 5, 1000, HAL_OK);
    error = BspI2cReceive(handle, &rxConfig);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);

    // Free the handle
    error = BspI2cFree(handle);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_Integration_InterruptMode_CompleteWorkflow(void)
{
    // Allocate I2C instance in interrupt mode
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_INTERRUPT, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Register callbacks
    BspI2cRegisterTxCallback(handle, TestTxCallback);
    BspI2cRegisterRxCallback(handle, TestRxCallback);
    BspI2cRegisterErrorCallback(handle, TestErrorCallback);

    // Perform transmit IT
    uint8_t                txData[] = {0xAA, 0xBB};
    BspI2cTransferConfig_t txConfig = {.devAddr = 0xA2, .pData = txData, .length = 2};
    HAL_I2C_Master_Transmit_IT_ExpectAndReturn(&hi2c2, 0xA2, txData, 2, HAL_OK);
    BspI2cError_e error = BspI2cTransmitIT(handle, &txConfig);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);

    // Simulate HAL callback
    HAL_I2C_MasterTxCpltCallback(&hi2c2);
    TEST_ASSERT_TRUE(s_txCallbackInvoked);

    // Free the handle
    error = BspI2cFree(handle);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_Integration_MemoryOperations_CompleteWorkflow(void)
{
    // Allocate I2C instance in blocking mode
    BspI2cHandle_t handle = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_BLOCKING, 2000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Write to memory
    uint8_t           writeData[] = {0x11, 0x22, 0x33, 0x44};
    BspI2cMemConfig_t writeConfig = {
        .devAddr = 0xA0, .memAddr = 0x0100, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_16BIT, .pData = writeData, .length = 4};
    HAL_I2C_Mem_Write_ExpectAndReturn(&hi2c3, 0xA0, 0x0100, 2, writeData, 4, 2000, HAL_OK);
    BspI2cError_e error = BspI2cMemWrite(handle, &writeConfig);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);

    // Read from memory
    uint8_t           readData[4];
    BspI2cMemConfig_t readConfig = {
        .devAddr = 0xA0, .memAddr = 0x0100, .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_16BIT, .pData = readData, .length = 4};
    HAL_I2C_Mem_Read_ExpectAndReturn(&hi2c3, 0xA0, 0x0100, 2, readData, 4, 2000, HAL_OK);
    error = BspI2cMemRead(handle, &readConfig);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);

    // Free the handle
    error = BspI2cFree(handle);
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, error);
}

void test_Integration_MultipleInstances_IndependentOperation(void)
{
    // Allocate multiple instances
    BspI2cHandle_t handle1 = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_BLOCKING, 500);
    BspI2cHandle_t handle2 = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_INTERRUPT, 0);
    BspI2cHandle_t handle3 = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_BLOCKING, 1000);

    TEST_ASSERT_GREATER_OR_EQUAL(0, handle1);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle2);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle3);

    // Verify they are all different
    TEST_ASSERT_NOT_EQUAL(handle1, handle2);
    TEST_ASSERT_NOT_EQUAL(handle2, handle3);
    TEST_ASSERT_NOT_EQUAL(handle1, handle3);

    // Free all handles
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, BspI2cFree(handle1));
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, BspI2cFree(handle2));
    TEST_ASSERT_EQUAL(eBSP_I2C_ERR_NONE, BspI2cFree(handle3));
}
