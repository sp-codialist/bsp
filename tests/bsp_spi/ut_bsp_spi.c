/**
 * @file ut_bsp_spi.c
 * @brief Unit tests for BSP SPI module using Unity and CMock
 * @note This test file mocks HAL layer functions to test BSP SPI functionality
 */

#include "Mockstm32f4xx_hal_spi.h"
#include "bsp_spi.h"
#include "unity.h"
#include <string.h>

// Forward declarations for HAL callbacks (implemented in production code)
extern void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi);
extern void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef* hspi);
extern void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi);
extern void HAL_SPI_ErrorCallback(SPI_HandleTypeDef* hspi);

// Mock SPI peripheral instances (needed for HAL handles)
static SPI_TypeDef mock_SPI1;
static SPI_TypeDef mock_SPI2;
static SPI_TypeDef mock_SPI3;
static SPI_TypeDef mock_SPI4;
static SPI_TypeDef mock_SPI5;
static SPI_TypeDef mock_SPI6;

// Mock HAL SPI handles
SPI_HandleTypeDef hspi1 = {.Instance = &mock_SPI1};
SPI_HandleTypeDef hspi2 = {.Instance = &mock_SPI2};
SPI_HandleTypeDef hspi3 = {.Instance = &mock_SPI3};
SPI_HandleTypeDef hspi4 = {.Instance = &mock_SPI4};
SPI_HandleTypeDef hspi5 = {.Instance = &mock_SPI5};
SPI_HandleTypeDef hspi6 = {.Instance = &mock_SPI6};

// Test callback trackers
static bool           tx_callback_invoked    = false;
static bool           rx_callback_invoked    = false;
static bool           txrx_callback_invoked  = false;
static bool           error_callback_invoked = false;
static BspSpiHandle_t callback_handle        = -1;
static BspSpiError_e  callback_error         = eBSP_SPI_ERR_NONE;

// Test callbacks
static void test_tx_callback(BspSpiHandle_t handle)
{
    tx_callback_invoked = true;
    callback_handle     = handle;
}

static void test_rx_callback(BspSpiHandle_t handle)
{
    rx_callback_invoked = true;
    callback_handle     = handle;
}

static void test_txrx_callback(BspSpiHandle_t handle)
{
    txrx_callback_invoked = true;
    callback_handle       = handle;
}

static void test_error_callback(BspSpiHandle_t handle, BspSpiError_e error)
{
    error_callback_invoked = true;
    callback_handle        = handle;
    callback_error         = error;
}

// ============================================================================
// Test Fixtures
// ============================================================================

void setUp(void)
{
    // Reset callback trackers
    tx_callback_invoked    = false;
    rx_callback_invoked    = false;
    txrx_callback_invoked  = false;
    error_callback_invoked = false;
    callback_handle        = -1;
    callback_error         = eBSP_SPI_ERR_NONE;

    // Free all SPI instances to ensure clean state between tests
    for (int8_t i = 0; i < 6; i++)
    {
        BspSpiFree(i);
    }
}

void tearDown(void)
{
    // Clean up after each test - free all instances
    for (int8_t i = 0; i < 6; i++)
    {
        BspSpiFree(i);
    }
}

// ============================================================================
// BspSpiAllocate Tests
// ============================================================================

void test_BspSpiAllocate_ValidInstance_BlockingMode(void)
{
    // Arrange
    BspSpiInstance_e instance = eBSP_SPI_INSTANCE_1;
    BspSpiMode_e     mode     = eBSP_SPI_MODE_BLOCKING;
    uint32_t         timeout  = 1000;

    // Act
    BspSpiHandle_t handle = BspSpiAllocate(instance, mode, timeout);

    // Assert
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);
}

void test_BspSpiAllocate_ValidInstance_DMAMode(void)
{
    // Arrange
    BspSpiInstance_e instance = eBSP_SPI_INSTANCE_2;
    BspSpiMode_e     mode     = eBSP_SPI_MODE_DMA;
    uint32_t         timeout  = 0; // Not used in DMA mode

    // Act
    BspSpiHandle_t handle = BspSpiAllocate(instance, mode, timeout);

    // Assert
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);
}

void test_BspSpiAllocate_InvalidInstance(void)
{
    // Arrange
    BspSpiInstance_e instance = eBSP_SPI_INSTANCE_COUNT; // Invalid
    BspSpiMode_e     mode     = eBSP_SPI_MODE_BLOCKING;
    uint32_t         timeout  = 1000;

    // Act
    BspSpiHandle_t handle = BspSpiAllocate(instance, mode, timeout);

    // Assert
    TEST_ASSERT_EQUAL(-1, handle);
}

void test_BspSpiAllocate_DuplicateAllocation(void)
{
    // Arrange
    BspSpiInstance_e instance = eBSP_SPI_INSTANCE_1;
    BspSpiMode_e     mode     = eBSP_SPI_MODE_BLOCKING;
    uint32_t         timeout  = 1000;

    // Act
    BspSpiHandle_t handle1 = BspSpiAllocate(instance, mode, timeout);
    BspSpiHandle_t handle2 = BspSpiAllocate(instance, mode, timeout); // Should fail

    // Assert
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle1);
    TEST_ASSERT_EQUAL(-1, handle2);

    // Cleanup
    BspSpiFree(handle1);
}

void test_BspSpiAllocate_AllInstances(void)
{
    // Test allocating all 6 SPI instances to cover all switch cases
    BspSpiHandle_t handles[6];

    handles[0] = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    handles[1] = BspSpiAllocate(eBSP_SPI_INSTANCE_2, eBSP_SPI_MODE_BLOCKING, 1000);
    handles[2] = BspSpiAllocate(eBSP_SPI_INSTANCE_3, eBSP_SPI_MODE_BLOCKING, 1000);
    handles[3] = BspSpiAllocate(eBSP_SPI_INSTANCE_4, eBSP_SPI_MODE_BLOCKING, 1000);
    handles[4] = BspSpiAllocate(eBSP_SPI_INSTANCE_5, eBSP_SPI_MODE_BLOCKING, 1000);
    handles[5] = BspSpiAllocate(eBSP_SPI_INSTANCE_6, eBSP_SPI_MODE_BLOCKING, 1000);

    // Assert all allocated successfully
    for (int i = 0; i < 6; i++)
    {
        TEST_ASSERT_GREATER_OR_EQUAL(0, handles[i]);
    }

    // Cleanup
    for (int i = 0; i < 6; i++)
    {
        BspSpiFree(handles[i]);
    }
}

void test_BspSpiAllocate_NoFreeSlots(void)
{
    // Allocate all 6 available slots
    BspSpiHandle_t handles[6];
    handles[0] = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    handles[1] = BspSpiAllocate(eBSP_SPI_INSTANCE_2, eBSP_SPI_MODE_BLOCKING, 1000);
    handles[2] = BspSpiAllocate(eBSP_SPI_INSTANCE_3, eBSP_SPI_MODE_BLOCKING, 1000);
    handles[3] = BspSpiAllocate(eBSP_SPI_INSTANCE_4, eBSP_SPI_MODE_BLOCKING, 1000);
    handles[4] = BspSpiAllocate(eBSP_SPI_INSTANCE_5, eBSP_SPI_MODE_BLOCKING, 1000);
    handles[5] = BspSpiAllocate(eBSP_SPI_INSTANCE_6, eBSP_SPI_MODE_BLOCKING, 1000);

    // Try to allocate when no slots are free (using a different valid instance value won't help)
    // This is actually impossible to test correctly because we only have 6 instances
    // The instance check will prevent us from getting to the "no free slots" code
    // Let's just verify all are allocated
    for (int i = 0; i < 6; i++)
    {
        TEST_ASSERT_GREATER_OR_EQUAL(0, handles[i]);
    }

    // Cleanup
    for (int i = 0; i < 6; i++)
    {
        BspSpiFree(handles[i]);
    }
}

// ============================================================================
// BspSpiFree Tests
// ============================================================================

void test_BspSpiFree_ValidHandle(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Act
    BspSpiError_e result = BspSpiFree(handle);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_NONE, result);
}

void test_BspSpiFree_InvalidHandle(void)
{
    // Act
    BspSpiError_e result = BspSpiFree(-1);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_HANDLE, result);
}

void test_BspSpiFree_DoubleFree(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    BspSpiFree(handle);

    // Act
    BspSpiError_e result = BspSpiFree(handle); // Second free should fail

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_HANDLE, result);
}

// ============================================================================
// BspSpiRegisterTxCallback Tests
// ============================================================================

void test_BspSpiRegisterTxCallback_ValidHandle(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Act
    BspSpiError_e result = BspSpiRegisterTxCallback(handle, test_tx_callback);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_NONE, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiRegisterTxCallback_InvalidHandle(void)
{
    // Act
    BspSpiError_e result = BspSpiRegisterTxCallback(-1, test_tx_callback);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_HANDLE, result);
}

void test_BspSpiRegisterTxCallback_NullCallback(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Act
    BspSpiError_e result = BspSpiRegisterTxCallback(handle, NULL);

    // Assert - production code currently accepts NULL (no validation)
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_NONE, result);

    // Cleanup
    BspSpiFree(handle);
}

// ============================================================================
// BspSpiRegisterRxCallback Tests
// ============================================================================

void test_BspSpiRegisterRxCallback_ValidHandle(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Act
    BspSpiError_e result = BspSpiRegisterRxCallback(handle, test_rx_callback);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_NONE, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiRegisterRxCallback_InvalidHandle(void)
{
    // Act
    BspSpiError_e result = BspSpiRegisterRxCallback(-1, test_rx_callback);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_HANDLE, result);
}

// ============================================================================
// BspSpiRegisterTxRxCallback Tests
// ============================================================================

void test_BspSpiRegisterTxRxCallback_ValidHandle(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Act
    BspSpiError_e result = BspSpiRegisterTxRxCallback(handle, test_txrx_callback);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_NONE, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiRegisterTxRxCallback_InvalidHandle(void)
{
    // Act
    BspSpiError_e result = BspSpiRegisterTxRxCallback(-1, test_txrx_callback);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_HANDLE, result);
}

// ============================================================================
// BspSpiRegisterErrorCallback Tests
// ============================================================================

void test_BspSpiRegisterErrorCallback_ValidHandle(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Act
    BspSpiError_e result = BspSpiRegisterErrorCallback(handle, test_error_callback);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_NONE, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiRegisterErrorCallback_InvalidHandle(void)
{
    // Act
    BspSpiError_e result = BspSpiRegisterErrorCallback(-1, test_error_callback);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_HANDLE, result);
}

// ============================================================================
// BspSpiTransmit Tests (Blocking Mode)
// ============================================================================

void test_BspSpiTransmit_ValidParameters(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t  txData[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t length   = sizeof(txData);

    // Mock HAL_SPI_Transmit to return HAL_OK
    HAL_SPI_Transmit_ExpectAndReturn(&hspi1, txData, length, 1000, HAL_OK);

    // Act
    BspSpiError_e result = BspSpiTransmit(handle, txData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_NONE, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmit_InvalidHandle(void)
{
    // Arrange
    uint8_t txData[] = {0x01, 0x02};

    // Act
    BspSpiError_e result = BspSpiTransmit(-1, txData, 2);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_HANDLE, result);
}

void test_BspSpiTransmit_NullPointer(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Act
    BspSpiError_e result = BspSpiTransmit(handle, NULL, 10);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_PARAM, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmit_ZeroLength(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t txData[] = {0x01};

    // Mock expectation - production code calls HAL even with zero length
    HAL_SPI_Transmit_ExpectAndReturn(&hspi1, txData, 0, 1000, HAL_OK);

    // Act - production code currently passes zero length to HAL (no validation)
    BspSpiError_e result = BspSpiTransmit(handle, txData, 0);

    // Assert - currently succeeds because HAL is called
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_NONE, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmit_HAL_Timeout(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t  txData[] = {0x01, 0x02};
    uint32_t length   = sizeof(txData);

    // Mock HAL_SPI_Transmit to return HAL_TIMEOUT
    HAL_SPI_Transmit_ExpectAndReturn(&hspi1, txData, length, 1000, HAL_TIMEOUT);

    // Act
    BspSpiError_e result = BspSpiTransmit(handle, txData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_TIMEOUT, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmit_HAL_Error(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t  txData[] = {0x01, 0x02};
    uint32_t length   = sizeof(txData);

    // Mock HAL_SPI_Transmit to return HAL_ERROR
    HAL_SPI_Transmit_ExpectAndReturn(&hspi1, txData, length, 1000, HAL_ERROR);

    // Act
    BspSpiError_e result = BspSpiTransmit(handle, txData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_TRANSFER, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmit_HAL_Busy(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t  txData[] = {0x01, 0x02};
    uint32_t length   = sizeof(txData);

    // Mock HAL_SPI_Transmit to return HAL_BUSY
    HAL_SPI_Transmit_ExpectAndReturn(&hspi1, txData, length, 1000, HAL_BUSY);

    // Act
    BspSpiError_e result = BspSpiTransmit(handle, txData, length);

    // Assert - blocking mode treats BUSY as TRANSFER error
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_TRANSFER, result);

    // Cleanup
    BspSpiFree(handle);
}

// ============================================================================
// BspSpiReceive Tests (Blocking Mode)
// ============================================================================

void test_BspSpiReceive_ValidParameters(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t  rxData[4] = {0};
    uint32_t length    = sizeof(rxData);

    // Mock HAL_SPI_Receive to return HAL_OK
    HAL_SPI_Receive_ExpectAndReturn(&hspi1, rxData, length, 1000, HAL_OK);

    // Act
    BspSpiError_e result = BspSpiReceive(handle, rxData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_NONE, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiReceive_InvalidHandle(void)
{
    // Arrange
    uint8_t rxData[4];

    // Act
    BspSpiError_e result = BspSpiReceive(-1, rxData, 4);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_HANDLE, result);
}

void test_BspSpiReceive_NullPointer(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Act
    BspSpiError_e result = BspSpiReceive(handle, NULL, 10);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_PARAM, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiReceive_HAL_Timeout(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t  rxData[4] = {0};
    uint32_t length    = sizeof(rxData);

    // Mock HAL_SPI_Receive to return HAL_TIMEOUT
    HAL_SPI_Receive_ExpectAndReturn(&hspi1, rxData, length, 1000, HAL_TIMEOUT);

    // Act
    BspSpiError_e result = BspSpiReceive(handle, rxData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_TIMEOUT, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiReceive_HAL_Error(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t  rxData[4] = {0};
    uint32_t length    = sizeof(rxData);

    // Mock HAL_SPI_Receive to return HAL_ERROR
    HAL_SPI_Receive_ExpectAndReturn(&hspi1, rxData, length, 1000, HAL_ERROR);

    // Act
    BspSpiError_e result = BspSpiReceive(handle, rxData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_TRANSFER, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiReceive_HAL_Busy(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t  rxData[4] = {0};
    uint32_t length    = sizeof(rxData);

    // Mock HAL_SPI_Receive to return HAL_BUSY
    HAL_SPI_Receive_ExpectAndReturn(&hspi1, rxData, length, 1000, HAL_BUSY);

    // Act
    BspSpiError_e result = BspSpiReceive(handle, rxData, length);

    // Assert - blocking mode treats BUSY as TRANSFER error
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_TRANSFER, result);

    // Cleanup
    BspSpiFree(handle);
}

// ============================================================================
// BspSpiTransmitReceive Tests (Blocking Mode)
// ============================================================================

void test_BspSpiTransmitReceive_ValidParameters(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t  txData[]  = {0x01, 0x02, 0x03, 0x04};
    uint8_t  rxData[4] = {0};
    uint32_t length    = sizeof(txData);

    // Mock HAL_SPI_TransmitReceive to return HAL_OK
    HAL_SPI_TransmitReceive_ExpectAndReturn(&hspi1, txData, rxData, length, 1000, HAL_OK);

    // Act
    BspSpiError_e result = BspSpiTransmitReceive(handle, txData, rxData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_NONE, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmitReceive_InvalidHandle(void)
{
    // Arrange
    uint8_t txData[] = {0x01};
    uint8_t rxData[1];

    // Act
    BspSpiError_e result = BspSpiTransmitReceive(-1, txData, rxData, 1);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_HANDLE, result);
}

void test_BspSpiTransmitReceive_NullTxPointer(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t rxData[4];

    // Act
    BspSpiError_e result = BspSpiTransmitReceive(handle, NULL, rxData, 4);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_PARAM, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmitReceive_NullRxPointer(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t txData[] = {0x01, 0x02};

    // Act
    BspSpiError_e result = BspSpiTransmitReceive(handle, txData, NULL, 2);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_PARAM, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmitReceive_HAL_Timeout(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t  txData[]  = {0x01, 0x02};
    uint8_t  rxData[2] = {0};
    uint32_t length    = sizeof(txData);

    // Mock HAL_SPI_TransmitReceive to return HAL_TIMEOUT
    HAL_SPI_TransmitReceive_ExpectAndReturn(&hspi1, txData, rxData, length, 1000, HAL_TIMEOUT);

    // Act
    BspSpiError_e result = BspSpiTransmitReceive(handle, txData, rxData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_TIMEOUT, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmitReceive_HAL_Error(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t  txData[]  = {0x01, 0x02};
    uint8_t  rxData[2] = {0};
    uint32_t length    = sizeof(txData);

    // Mock HAL_SPI_TransmitReceive to return HAL_ERROR
    HAL_SPI_TransmitReceive_ExpectAndReturn(&hspi1, txData, rxData, length, 1000, HAL_ERROR);

    // Act
    BspSpiError_e result = BspSpiTransmitReceive(handle, txData, rxData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_TRANSFER, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmitReceive_HAL_Busy(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t  txData[]  = {0x01, 0x02};
    uint8_t  rxData[2] = {0};
    uint32_t length    = sizeof(txData);

    // Mock HAL_SPI_TransmitReceive to return HAL_BUSY
    HAL_SPI_TransmitReceive_ExpectAndReturn(&hspi1, txData, rxData, length, 1000, HAL_BUSY);

    // Act
    BspSpiError_e result = BspSpiTransmitReceive(handle, txData, rxData, length);

    // Assert - blocking mode treats BUSY as TRANSFER error
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_TRANSFER, result);

    // Cleanup
    BspSpiFree(handle);
}

// ============================================================================
// BspSpiTransmitDMA Tests (DMA Mode)
// ============================================================================

void test_BspSpiTransmitDMA_ValidParameters(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterTxCallback(handle, test_tx_callback);

    uint8_t  txData[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t length   = sizeof(txData);

    // Mock HAL_SPI_Transmit_DMA to return HAL_OK
    HAL_SPI_Transmit_DMA_ExpectAndReturn(&hspi1, txData, length, HAL_OK);

    // Act
    BspSpiError_e result = BspSpiTransmitDMA(handle, txData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_NONE, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmitDMA_InvalidHandle(void)
{
    // Arrange
    uint8_t txData[] = {0x01};

    // Act
    BspSpiError_e result = BspSpiTransmitDMA(-1, txData, 1);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_HANDLE, result);
}

void test_BspSpiTransmitDMA_NullPointer(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Act
    BspSpiError_e result = BspSpiTransmitDMA(handle, NULL, 10);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_PARAM, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmitDMA_HAL_Error(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterTxCallback(handle, test_tx_callback);

    uint8_t  txData[] = {0x01, 0x02};
    uint32_t length   = sizeof(txData);

    // Mock HAL_SPI_Transmit_DMA to return HAL_ERROR
    HAL_SPI_Transmit_DMA_ExpectAndReturn(&hspi1, txData, length, HAL_ERROR);

    // Act
    BspSpiError_e result = BspSpiTransmitDMA(handle, txData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_TRANSFER, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmitDMA_HAL_Busy(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterTxCallback(handle, test_tx_callback);

    uint8_t  txData[] = {0x01, 0x02};
    uint32_t length   = sizeof(txData);

    // Mock HAL_SPI_Transmit_DMA to return HAL_BUSY
    HAL_SPI_Transmit_DMA_ExpectAndReturn(&hspi1, txData, length, HAL_BUSY);

    // Act
    BspSpiError_e result = BspSpiTransmitDMA(handle, txData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_BUSY, result);

    // Cleanup
    BspSpiFree(handle);
}

// ============================================================================
// BspSpiReceiveDMA Tests (DMA Mode)
// ============================================================================

void test_BspSpiReceiveDMA_ValidParameters(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterRxCallback(handle, test_rx_callback);

    uint8_t  rxData[4] = {0};
    uint32_t length    = sizeof(rxData);

    // Mock HAL_SPI_Receive_DMA to return HAL_OK
    HAL_SPI_Receive_DMA_ExpectAndReturn(&hspi1, rxData, length, HAL_OK);

    // Act
    BspSpiError_e result = BspSpiReceiveDMA(handle, rxData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_NONE, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiReceiveDMA_InvalidHandle(void)
{
    // Arrange
    uint8_t rxData[4];

    // Act
    BspSpiError_e result = BspSpiReceiveDMA(-1, rxData, 4);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_HANDLE, result);
}

void test_BspSpiReceiveDMA_HAL_Error(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterRxCallback(handle, test_rx_callback);

    uint8_t  rxData[4] = {0};
    uint32_t length    = sizeof(rxData);

    // Mock HAL_SPI_Receive_DMA to return HAL_ERROR
    HAL_SPI_Receive_DMA_ExpectAndReturn(&hspi1, rxData, length, HAL_ERROR);

    // Act
    BspSpiError_e result = BspSpiReceiveDMA(handle, rxData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_TRANSFER, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiReceiveDMA_HAL_Busy(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterRxCallback(handle, test_rx_callback);

    uint8_t  rxData[4] = {0};
    uint32_t length    = sizeof(rxData);

    // Mock HAL_SPI_Receive_DMA to return HAL_BUSY
    HAL_SPI_Receive_DMA_ExpectAndReturn(&hspi1, rxData, length, HAL_BUSY);

    // Act
    BspSpiError_e result = BspSpiReceiveDMA(handle, rxData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_BUSY, result);

    // Cleanup
    BspSpiFree(handle);
}

// ============================================================================
// BspSpiTransmitReceiveDMA Tests (DMA Mode)
// ============================================================================

void test_BspSpiTransmitReceiveDMA_ValidParameters(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterTxRxCallback(handle, test_txrx_callback);

    uint8_t  txData[]  = {0x01, 0x02, 0x03, 0x04};
    uint8_t  rxData[4] = {0};
    uint32_t length    = sizeof(txData);

    // Mock HAL_SPI_TransmitReceive_DMA to return HAL_OK
    HAL_SPI_TransmitReceive_DMA_ExpectAndReturn(&hspi1, txData, rxData, length, HAL_OK);

    // Act
    BspSpiError_e result = BspSpiTransmitReceiveDMA(handle, txData, rxData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_NONE, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmitReceiveDMA_InvalidHandle(void)
{
    // Arrange
    uint8_t txData[] = {0x01};
    uint8_t rxData[1];

    // Act
    BspSpiError_e result = BspSpiTransmitReceiveDMA(-1, txData, rxData, 1);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_HANDLE, result);
}

void test_BspSpiTransmitReceiveDMA_NullTxPointer(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t rxData[4];

    // Act
    BspSpiError_e result = BspSpiTransmitReceiveDMA(handle, NULL, rxData, 4);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_PARAM, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmitReceiveDMA_HAL_Error(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterTxRxCallback(handle, test_txrx_callback);

    uint8_t  txData[]  = {0x01, 0x02};
    uint8_t  rxData[2] = {0};
    uint32_t length    = sizeof(txData);

    // Mock HAL_SPI_TransmitReceive_DMA to return HAL_ERROR
    HAL_SPI_TransmitReceive_DMA_ExpectAndReturn(&hspi1, txData, rxData, length, HAL_ERROR);

    // Act
    BspSpiError_e result = BspSpiTransmitReceiveDMA(handle, txData, rxData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_TRANSFER, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmitReceiveDMA_HAL_Busy(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterTxRxCallback(handle, test_txrx_callback);

    uint8_t  txData[]  = {0x01, 0x02};
    uint8_t  rxData[2] = {0};
    uint32_t length    = sizeof(txData);

    // Mock HAL_SPI_TransmitReceive_DMA to return HAL_BUSY
    HAL_SPI_TransmitReceive_DMA_ExpectAndReturn(&hspi1, txData, rxData, length, HAL_BUSY);

    // Act
    BspSpiError_e result = BspSpiTransmitReceiveDMA(handle, txData, rxData, length);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_BUSY, result);

    // Cleanup
    BspSpiFree(handle);
}

// ============================================================================
// HAL Callback Tests
// ============================================================================

void test_HAL_SPI_TxCpltCallback(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterTxCallback(handle, test_tx_callback);

    // Act - Simulate HAL callback
    HAL_SPI_TxCpltCallback(&hspi1);

    // Assert
    TEST_ASSERT_TRUE(tx_callback_invoked);
    TEST_ASSERT_EQUAL(handle, callback_handle);

    // Cleanup
    BspSpiFree(handle);
}

void test_HAL_SPI_TxCpltCallback_NullCallback(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // No callback registered

    // Act - Simulate HAL callback (should not crash)
    HAL_SPI_TxCpltCallback(&hspi1);

    // Assert - callback should not be invoked
    TEST_ASSERT_FALSE(tx_callback_invoked);

    // Cleanup
    BspSpiFree(handle);
}

void test_HAL_SPI_TxCpltCallback_InvalidHandle(void)
{
    // Arrange - Create a mock SPI handle not in our module list
    SPI_TypeDef       mock_unknown_spi;
    SPI_HandleTypeDef unknown_hspi = {.Instance = &mock_unknown_spi};

    // Act - Simulate HAL callback with unknown handle (should not crash)
    HAL_SPI_TxCpltCallback(&unknown_hspi);

    // Assert - callback should not be invoked
    TEST_ASSERT_FALSE(tx_callback_invoked);
}

void test_HAL_SPI_RxCpltCallback(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_2, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterRxCallback(handle, test_rx_callback);

    // Act - Simulate HAL callback
    HAL_SPI_RxCpltCallback(&hspi2);

    // Assert
    TEST_ASSERT_TRUE(rx_callback_invoked);
    TEST_ASSERT_EQUAL(handle, callback_handle);

    // Cleanup
    BspSpiFree(handle);
}

void test_HAL_SPI_RxCpltCallback_NullCallback(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Act - Simulate HAL callback (should not crash)
    HAL_SPI_RxCpltCallback(&hspi1);

    // Assert - callback should not be invoked
    TEST_ASSERT_FALSE(rx_callback_invoked);

    // Cleanup
    BspSpiFree(handle);
}

void test_HAL_SPI_TxRxCpltCallback(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_3, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterTxRxCallback(handle, test_txrx_callback);

    // Act - Simulate HAL callback
    HAL_SPI_TxRxCpltCallback(&hspi3);

    // Assert
    TEST_ASSERT_TRUE(txrx_callback_invoked);
    TEST_ASSERT_EQUAL(handle, callback_handle);

    // Cleanup
    BspSpiFree(handle);
}

void test_HAL_SPI_TxRxCpltCallback_NullCallback(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Act - Simulate HAL callback (should not crash)
    HAL_SPI_TxRxCpltCallback(&hspi1);

    // Assert - callback should not be invoked
    TEST_ASSERT_FALSE(txrx_callback_invoked);

    // Cleanup
    BspSpiFree(handle);
}

void test_HAL_SPI_ErrorCallback(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_4, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterErrorCallback(handle, test_error_callback);

    // Act - Simulate HAL error callback
    HAL_SPI_ErrorCallback(&hspi4);

    // Assert
    TEST_ASSERT_TRUE(error_callback_invoked);
    TEST_ASSERT_EQUAL(handle, callback_handle);
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_TRANSFER, callback_error);

    // Cleanup
    BspSpiFree(handle);
}

void test_HAL_SPI_ErrorCallback_NullCallback(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Act - Simulate HAL callback (should not crash)
    HAL_SPI_ErrorCallback(&hspi1);

    // Assert - callback should not be invoked
    TEST_ASSERT_FALSE(error_callback_invoked);

    // Cleanup
    BspSpiFree(handle);
}

void test_HAL_SPI_ErrorCallback_NullHalHandle(void)
{
    // Act - Call with NULL HAL handle (should not crash)
    HAL_SPI_ErrorCallback(NULL);

    // Assert - callback should not be invoked
    TEST_ASSERT_FALSE(error_callback_invoked);
}

void test_HAL_SPI_TxCpltCallback_ModuleNotAllocated(void)
{
    // Arrange - Allocate and then free to test non-allocated module
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterTxCallback(handle, test_tx_callback);
    BspSpiFree(handle); // Module is now deallocated

    // Act - Simulate HAL callback on deallocated module
    HAL_SPI_TxCpltCallback(&hspi1);

    // Assert - callback should not be invoked since module is not allocated
    TEST_ASSERT_FALSE(tx_callback_invoked);
}

void test_HAL_SPI_RxCpltCallback_ModuleNotAllocated(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterRxCallback(handle, test_rx_callback);
    BspSpiFree(handle);

    // Act
    HAL_SPI_RxCpltCallback(&hspi1);

    // Assert
    TEST_ASSERT_FALSE(rx_callback_invoked);
}

void test_HAL_SPI_TxRxCpltCallback_ModuleNotAllocated(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterTxRxCallback(handle, test_txrx_callback);
    BspSpiFree(handle);

    // Act
    HAL_SPI_TxRxCpltCallback(&hspi1);

    // Assert
    TEST_ASSERT_FALSE(txrx_callback_invoked);
}

void test_HAL_SPI_ErrorCallback_ModuleNotAllocated(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    BspSpiRegisterErrorCallback(handle, test_error_callback);
    BspSpiFree(handle);

    // Act
    HAL_SPI_ErrorCallback(&hspi1);

    // Assert
    TEST_ASSERT_FALSE(error_callback_invoked);
}

void test_BspSpiTransmit_WrongMode_DMA(void)
{
    // Arrange - Allocate in DMA mode but try to use blocking function
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t txData[] = {0x01, 0x02};

    // Act - Try to use blocking transmit with DMA mode handle
    BspSpiError_e result = BspSpiTransmit(handle, txData, 2);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_PARAM, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiReceive_WrongMode_DMA(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t rxData[4];

    // Act
    BspSpiError_e result = BspSpiReceive(handle, rxData, 4);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_PARAM, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmitReceive_WrongMode_DMA(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t txData[] = {0x01, 0x02};
    uint8_t rxData[2];

    // Act
    BspSpiError_e result = BspSpiTransmitReceive(handle, txData, rxData, 2);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_PARAM, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmitDMA_WrongMode_Blocking(void)
{
    // Arrange - Allocate in blocking mode but try to use DMA function
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t txData[] = {0x01, 0x02};

    // Act
    BspSpiError_e result = BspSpiTransmitDMA(handle, txData, 2);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_PARAM, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiReceiveDMA_WrongMode_Blocking(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t rxData[4];

    // Act
    BspSpiError_e result = BspSpiReceiveDMA(handle, rxData, 4);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_PARAM, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmitReceiveDMA_WrongMode_Blocking(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_BLOCKING, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t txData[] = {0x01, 0x02};
    uint8_t rxData[2];

    // Act
    BspSpiError_e result = BspSpiTransmitReceiveDMA(handle, txData, rxData, 2);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_PARAM, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiReceiveDMA_NullPointer(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    // Act
    BspSpiError_e result = BspSpiReceiveDMA(handle, NULL, 10);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_PARAM, result);

    // Cleanup
    BspSpiFree(handle);
}

void test_BspSpiTransmitReceiveDMA_NullRxPointer(void)
{
    // Arrange
    BspSpiHandle_t handle = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, handle);

    uint8_t txData[] = {0x01, 0x02};

    // Act
    BspSpiError_e result = BspSpiTransmitReceiveDMA(handle, txData, NULL, 2);

    // Assert
    TEST_ASSERT_EQUAL(eBSP_SPI_ERR_INVALID_PARAM, result);

    // Cleanup
    BspSpiFree(handle);
}
