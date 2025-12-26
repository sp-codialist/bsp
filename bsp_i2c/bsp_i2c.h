/*
 * bsp_i2c.h
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

/* --- Type Definitions --- */

/**
 * I2C handle type.
 * Valid handles are >= 0, -1 indicates an invalid handle or error.
 */
typedef int8_t BspI2cHandle_t;

/**
 * I2C peripheral instance enumeration.
 * Identifies which hardware I2C peripheral to use.
 */
typedef enum
{
    eBSP_I2C_INSTANCE_1 = 0u, /**< I2C1 peripheral */
    eBSP_I2C_INSTANCE_2,      /**< I2C2 peripheral */
    eBSP_I2C_INSTANCE_3,      /**< I2C3 peripheral */
    eBSP_I2C_INSTANCE_4,      /**< I2C4 peripheral */
    eBSP_I2C_INSTANCE_5,      /**< I2C5 peripheral */
    eBSP_I2C_INSTANCE_6,      /**< I2C6 peripheral */
    eBSP_I2C_INSTANCE_COUNT
} BspI2cInstance_e;

/**
 * I2C operation mode enumeration.
 * Determines whether the I2C operates in blocking or interrupt mode.
 */
typedef enum
{
    eBSP_I2C_MODE_BLOCKING = 0u, /**< Blocking mode with timeout */
    eBSP_I2C_MODE_INTERRUPT      /**< Interrupt mode with callbacks */
} BspI2cMode_e;

/**
 * I2C error enumeration.
 * Error codes returned by I2C operations.
 */
typedef enum
{
    eBSP_I2C_ERR_NONE = 0,       /**< No error */
    eBSP_I2C_ERR_INVALID_PARAM,  /**< Invalid parameter */
    eBSP_I2C_ERR_INVALID_HANDLE, /**< Invalid or unallocated handle */
    eBSP_I2C_ERR_BUSY,           /**< I2C peripheral is busy */
    eBSP_I2C_ERR_TIMEOUT,        /**< Operation timed out */
    eBSP_I2C_ERR_TRANSFER,       /**< Transfer error */
    eBSP_I2C_ERR_NO_RESOURCE     /**< No available I2C module slots */
} BspI2cError_e;

/**
 * I2C memory address size enumeration.
 * Specifies the size of the memory address for I2C memory operations.
 */
typedef enum
{
    eBSP_I2C_MEM_ADDR_SIZE_8BIT  = 1u, /**< 8-bit memory address */
    eBSP_I2C_MEM_ADDR_SIZE_16BIT = 2u  /**< 16-bit memory address */
} BspI2cMemAddrSize_e;

/**
 * I2C transfer configuration structure.
 * Used for basic I2C transmit and receive operations.
 *
 * Example usage:
 * @code
 * BspI2cTransferConfig_t config = {
 *     .devAddr = 0xA0,
 *     .pData = txBuffer,
 *     .length = 10
 * };
 * @endcode
 */
typedef struct
{
    uint8_t  devAddr; /**< I2C device address */
    uint8_t* pData;   /**< Pointer to data buffer */
    uint16_t length;  /**< Number of bytes to transfer */
} BspI2cTransferConfig_t;

/**
 * I2C memory transfer configuration structure.
 * Used for I2C memory read and write operations.
 *
 * Example usage:
 * @code
 * BspI2cMemConfig_t config = {
 *     .devAddr = 0xA0,
 *     .memAddr = 0x1234,
 *     .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_16BIT,
 *     .pData = dataBuffer,
 *     .length = 16
 * };
 * @endcode
 */
typedef struct
{
    uint8_t             devAddr;     /**< I2C device address */
    uint16_t            memAddr;     /**< Memory address within the I2C device */
    BspI2cMemAddrSize_e memAddrSize; /**< Size of the memory address */
    uint8_t*            pData;       /**< Pointer to data buffer */
    uint16_t            length;      /**< Number of bytes to transfer */
} BspI2cMemConfig_t;

/**
 * Callback type for I2C transmit completion.
 * Called when an interrupt mode transmit operation completes successfully.
 *
 * @param handle The I2C handle that completed transmission
 */
typedef void (*BspI2cTxCpltCb_t)(BspI2cHandle_t handle);

/**
 * Callback type for I2C receive completion.
 * Called when an interrupt mode receive operation completes successfully.
 *
 * @param handle The I2C handle that completed reception
 */
typedef void (*BspI2cRxCpltCb_t)(BspI2cHandle_t handle);

/**
 * Callback type for I2C memory transmit completion.
 * Called when an interrupt mode memory write operation completes successfully.
 *
 * @param handle The I2C handle that completed the memory write
 */
typedef void (*BspI2cMemTxCpltCb_t)(BspI2cHandle_t handle);

/**
 * Callback type for I2C memory receive completion.
 * Called when an interrupt mode memory read operation completes successfully.
 *
 * @param handle The I2C handle that completed the memory read
 */
typedef void (*BspI2cMemRxCpltCb_t)(BspI2cHandle_t handle);

/**
 * Callback type for I2C error notification.
 * Called when an error occurs during an interrupt mode operation.
 *
 * @param handle The I2C handle that encountered an error
 * @param error The error code
 */
typedef void (*BspI2cErrorCb_t)(BspI2cHandle_t handle, BspI2cError_e error);

/* --- Public Functions --- */

/**
 * Allocates an I2C module instance.
 *
 * @param eInstance The I2C peripheral instance to allocate
 * @param eMode The operation mode (blocking or interrupt)
 * @param uTimeoutMs Timeout in milliseconds for blocking mode operations (0 = use default)
 * @return Handle to the allocated I2C instance (>= 0), or -1 on error
 */
BspI2cHandle_t BspI2cAllocate(BspI2cInstance_e eInstance, BspI2cMode_e eMode, uint32_t uTimeoutMs);

/**
 * Frees a previously allocated I2C module instance.
 *
 * @param handle The I2C handle to free
 * @return Error code indicating success or failure
 */
BspI2cError_e BspI2cFree(BspI2cHandle_t handle);

/**
 * Registers a transmit completion callback for interrupt mode.
 *
 * @param handle The I2C handle
 * @param pCb Callback function to invoke on transmit completion
 * @return Error code indicating success or failure
 */
BspI2cError_e BspI2cRegisterTxCallback(BspI2cHandle_t handle, BspI2cTxCpltCb_t pCb);

/**
 * Registers a receive completion callback for interrupt mode.
 *
 * @param handle The I2C handle
 * @param pCb Callback function to invoke on receive completion
 * @return Error code indicating success or failure
 */
BspI2cError_e BspI2cRegisterRxCallback(BspI2cHandle_t handle, BspI2cRxCpltCb_t pCb);

/**
 * Registers a memory transmit completion callback for interrupt mode.
 *
 * @param handle The I2C handle
 * @param pCb Callback function to invoke on memory write completion
 * @return Error code indicating success or failure
 */
BspI2cError_e BspI2cRegisterMemTxCallback(BspI2cHandle_t handle, BspI2cMemTxCpltCb_t pCb);

/**
 * Registers a memory receive completion callback for interrupt mode.
 *
 * @param handle The I2C handle
 * @param pCb Callback function to invoke on memory read completion
 * @return Error code indicating success or failure
 */
BspI2cError_e BspI2cRegisterMemRxCallback(BspI2cHandle_t handle, BspI2cMemRxCpltCb_t pCb);

/**
 * Registers an error callback for interrupt mode.
 *
 * @param handle The I2C handle
 * @param pCb Callback function to invoke on error
 * @return Error code indicating success or failure
 */
BspI2cError_e BspI2cRegisterErrorCallback(BspI2cHandle_t handle, BspI2cErrorCb_t pCb);

/* --- Blocking Mode Functions --- */

/**
 * Transmits data in blocking mode.
 *
 * @param handle The I2C handle
 * @param pConfig Pointer to the transfer configuration
 * @return Error code indicating success or failure
 */
BspI2cError_e BspI2cTransmit(BspI2cHandle_t handle, const BspI2cTransferConfig_t* pConfig);

/**
 * Receives data in blocking mode.
 *
 * @param handle The I2C handle
 * @param pConfig Pointer to the transfer configuration
 * @return Error code indicating success or failure
 */
BspI2cError_e BspI2cReceive(BspI2cHandle_t handle, const BspI2cTransferConfig_t* pConfig);

/**
 * Reads data from an I2C memory device in blocking mode.
 *
 * @param handle The I2C handle
 * @param pConfig Pointer to the memory transfer configuration
 * @return Error code indicating success or failure
 */
BspI2cError_e BspI2cMemRead(BspI2cHandle_t handle, const BspI2cMemConfig_t* pConfig);

/**
 * Writes data to an I2C memory device in blocking mode.
 *
 * @param handle The I2C handle
 * @param pConfig Pointer to the memory transfer configuration
 * @return Error code indicating success or failure
 */
BspI2cError_e BspI2cMemWrite(BspI2cHandle_t handle, const BspI2cMemConfig_t* pConfig);

/* --- Interrupt Mode Functions --- */

/**
 * Transmits data using interrupt mode.
 * Completion is signaled via the registered transmit callback.
 *
 * @param handle The I2C handle
 * @param pConfig Pointer to the transfer configuration (must remain valid until callback)
 * @return Error code indicating success or failure
 */
BspI2cError_e BspI2cTransmitIT(BspI2cHandle_t handle, const BspI2cTransferConfig_t* pConfig);

/**
 * Receives data using interrupt mode.
 * Completion is signaled via the registered receive callback.
 *
 * @param handle The I2C handle
 * @param pConfig Pointer to the transfer configuration (must remain valid until callback)
 * @return Error code indicating success or failure
 */
BspI2cError_e BspI2cReceiveIT(BspI2cHandle_t handle, const BspI2cTransferConfig_t* pConfig);

/**
 * Reads data from an I2C memory device using interrupt mode.
 * Completion is signaled via the registered memory receive callback.
 *
 * @param handle The I2C handle
 * @param pConfig Pointer to the memory transfer configuration (must remain valid until callback)
 * @return Error code indicating success or failure
 */
BspI2cError_e BspI2cMemReadIT(BspI2cHandle_t handle, const BspI2cMemConfig_t* pConfig);

/**
 * Writes data to an I2C memory device using interrupt mode.
 * Completion is signaled via the registered memory transmit callback.
 *
 * @param handle The I2C handle
 * @param pConfig Pointer to the memory transfer configuration (must remain valid until callback)
 * @return Error code indicating success or failure
 */
BspI2cError_e BspI2cMemWriteIT(BspI2cHandle_t handle, const BspI2cMemConfig_t* pConfig);

#ifdef __cplusplus
}
#endif
