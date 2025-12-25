/*
 * bsp_spi.h
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
 * SPI handle type.
 * Valid handles are >= 0, -1 indicates an invalid handle or error.
 */
typedef int8_t BspSpiHandle_t;

/**
 * SPI peripheral instance enumeration.
 * Identifies which hardware SPI peripheral to use.
 */
typedef enum
{
    eBSP_SPI_INSTANCE_1 = 0u, /**< SPI1 peripheral */
    eBSP_SPI_INSTANCE_2,      /**< SPI2 peripheral */
    eBSP_SPI_INSTANCE_3,      /**< SPI3 peripheral */
    eBSP_SPI_INSTANCE_4,      /**< SPI4 peripheral */
    eBSP_SPI_INSTANCE_5,      /**< SPI5 peripheral */
    eBSP_SPI_INSTANCE_6,      /**< SPI6 peripheral */
    eBSP_SPI_INSTANCE_COUNT
} BspSpiInstance_e;

/**
 * SPI operation mode enumeration.
 * Determines whether the SPI operates in blocking or DMA mode.
 */
typedef enum
{
    eBSP_SPI_MODE_BLOCKING = 0u, /**< Blocking mode with timeout */
    eBSP_SPI_MODE_DMA            /**< DMA mode with callbacks */
} BspSpiMode_e;

/**
 * SPI error enumeration.
 * Error codes returned by SPI operations.
 */
typedef enum
{
    eBSP_SPI_ERR_NONE = 0,       /**< No error */
    eBSP_SPI_ERR_INVALID_PARAM,  /**< Invalid parameter */
    eBSP_SPI_ERR_INVALID_HANDLE, /**< Invalid or unallocated handle */
    eBSP_SPI_ERR_BUSY,           /**< SPI peripheral is busy */
    eBSP_SPI_ERR_TIMEOUT,        /**< Operation timed out */
    eBSP_SPI_ERR_TRANSFER,       /**< Transfer error */
    eBSP_SPI_ERR_NO_RESOURCE     /**< No available SPI module slots */
} BspSpiError_e;

/**
 * Callback type for SPI transmit completion.
 * Called when a DMA transmit operation completes successfully.
 *
 * @param handle The SPI handle that completed transmission
 */
typedef void (*BspSpiTxCpltCb_t)(BspSpiHandle_t handle);

/**
 * Callback type for SPI receive completion.
 * Called when a DMA receive operation completes successfully.
 *
 * @param handle The SPI handle that completed reception
 */
typedef void (*BspSpiRxCpltCb_t)(BspSpiHandle_t handle);

/**
 * Callback type for SPI transmit/receive completion.
 * Called when a DMA transmit-receive operation completes successfully.
 *
 * @param handle The SPI handle that completed the operation
 */
typedef void (*BspSpiTxRxCpltCb_t)(BspSpiHandle_t handle);

/**
 * Callback type for SPI error notification.
 * Called when an error occurs during a DMA operation.
 *
 * @param handle The SPI handle that encountered an error
 * @param error The error code
 */
typedef void (*BspSpiErrorCb_t)(BspSpiHandle_t handle, BspSpiError_e error);

/* --- Public Functions --- */

/**
 * Allocates an SPI module instance.
 *
 * @param eInstance The SPI peripheral instance to allocate
 * @param eMode The operation mode (blocking or DMA)
 * @param uTimeoutMs Timeout in milliseconds for blocking mode operations (0 = use default)
 * @return Handle to the allocated SPI instance (>= 0), or -1 on error
 */
BspSpiHandle_t BspSpiAllocate(BspSpiInstance_e eInstance, BspSpiMode_e eMode, uint32_t uTimeoutMs);

/**
 * Frees a previously allocated SPI module instance.
 *
 * @param handle The SPI handle to free
 * @return Error code indicating success or failure
 */
BspSpiError_e BspSpiFree(BspSpiHandle_t handle);

/**
 * Registers a transmit completion callback for DMA mode.
 *
 * @param handle The SPI handle
 * @param pCb Callback function to invoke on transmit completion
 * @return Error code indicating success or failure
 */
BspSpiError_e BspSpiRegisterTxCallback(BspSpiHandle_t handle, BspSpiTxCpltCb_t pCb);

/**
 * Registers a receive completion callback for DMA mode.
 *
 * @param handle The SPI handle
 * @param pCb Callback function to invoke on receive completion
 * @return Error code indicating success or failure
 */
BspSpiError_e BspSpiRegisterRxCallback(BspSpiHandle_t handle, BspSpiRxCpltCb_t pCb);

/**
 * Registers a transmit-receive completion callback for DMA mode.
 *
 * @param handle The SPI handle
 * @param pCb Callback function to invoke on transmit-receive completion
 * @return Error code indicating success or failure
 */
BspSpiError_e BspSpiRegisterTxRxCallback(BspSpiHandle_t handle, BspSpiTxRxCpltCb_t pCb);

/**
 * Registers an error callback for DMA mode.
 *
 * @param handle The SPI handle
 * @param pCb Callback function to invoke on error
 * @return Error code indicating success or failure
 */
BspSpiError_e BspSpiRegisterErrorCallback(BspSpiHandle_t handle, BspSpiErrorCb_t pCb);

/* --- Blocking Mode Functions --- */

/**
 * Transmits data in blocking mode.
 * Note: Caller is responsible for chip select (CS) control.
 *
 * @param handle The SPI handle
 * @param pTxData Pointer to the data to transmit
 * @param uLength Length of the data in bytes
 * @return Error code indicating success or failure
 */
BspSpiError_e BspSpiTransmit(BspSpiHandle_t handle, const uint8_t* pTxData, uint32_t uLength);

/**
 * Receives data in blocking mode.
 * Note: Caller is responsible for chip select (CS) control.
 *
 * @param handle The SPI handle
 * @param pRxData Pointer to the buffer to store received data
 * @param uLength Length of the data to receive in bytes
 * @return Error code indicating success or failure
 */
BspSpiError_e BspSpiReceive(BspSpiHandle_t handle, uint8_t* pRxData, uint32_t uLength);

/**
 * Transmits and receives data simultaneously in blocking mode (full-duplex).
 * Note: Caller is responsible for chip select (CS) control.
 *
 * @param handle The SPI handle
 * @param pTxData Pointer to the data to transmit
 * @param pRxData Pointer to the buffer to store received data
 * @param uLength Length of the data in bytes
 * @return Error code indicating success or failure
 */
BspSpiError_e BspSpiTransmitReceive(BspSpiHandle_t handle, const uint8_t* pTxData, uint8_t* pRxData, uint32_t uLength);

/* --- DMA Mode Functions --- */

/**
 * Transmits data using DMA.
 * Completion is signaled via the registered transmit callback.
 * Note: Caller is responsible for chip select (CS) control.
 *
 * @param handle The SPI handle
 * @param pTxData Pointer to the data to transmit (must remain valid until callback)
 * @param uLength Length of the data in bytes
 * @return Error code indicating success or failure
 */
BspSpiError_e BspSpiTransmitDMA(BspSpiHandle_t handle, const uint8_t* pTxData, uint32_t uLength);

/**
 * Receives data using DMA.
 * Completion is signaled via the registered receive callback.
 * Note: Caller is responsible for chip select (CS) control.
 *
 * @param handle The SPI handle
 * @param pRxData Pointer to the buffer to store received data (must remain valid until callback)
 * @param uLength Length of the data to receive in bytes
 * @return Error code indicating success or failure
 */
BspSpiError_e BspSpiReceiveDMA(BspSpiHandle_t handle, uint8_t* pRxData, uint32_t uLength);

/**
 * Transmits and receives data simultaneously using DMA (full-duplex).
 * Completion is signaled via the registered transmit-receive callback.
 * Note: Caller is responsible for chip select (CS) control.
 *
 * @param handle The SPI handle
 * @param pTxData Pointer to the data to transmit (must remain valid until callback)
 * @param pRxData Pointer to the buffer to store received data (must remain valid until callback)
 * @param uLength Length of the data in bytes
 * @return Error code indicating success or failure
 */
BspSpiError_e BspSpiTransmitReceiveDMA(BspSpiHandle_t handle, const uint8_t* pTxData, uint8_t* pRxData, uint32_t uLength);

#ifdef __cplusplus
}
#endif
