/**
 * @file bsp_can.h
 * @brief Efficient, event-driven CAN BSP module for STM32
 *
 * This module provides a complete CAN communication interface with:
 * - Event-driven architecture with direct ISR callbacks
 * - O(1) priority-based TX queue using bitmap indexing
 * - Lock-free RX circular buffer for thread-safe ISR operation
 * - Optional LED feedback for TX/RX activity
 * - Comprehensive error handling and statistics
 * - Self-contained implementation with no external firmware dependencies
 *
 * @note All callbacks execute in ISR context. Keep execution time <5µs.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "bsp_can_config.h"
#include "bsp_led.h"
#include <stdbool.h>
#include <stdint.h>

/* ============================================================================
 * Constants and Limits
 * ========================================================================== */

/** Maximum CAN data payload length */
static const uint8_t BSP_CAN_MAX_DATA_LEN = 8u;

/* ============================================================================
 * Type Definitions
 * ========================================================================== */

/**
 * @brief CAN module handle type.
 *
 * Handles are allocated by BspCanAllocate() and must be used for all
 * subsequent API calls. Valid handles are >= 0.
 */
typedef int8_t BspCanHandle_t;

/** Invalid handle constant */
static const BspCanHandle_t BSP_CAN_INVALID_HANDLE = -1;

/**
 * @brief CAN peripheral instance enumeration.
 */
typedef enum
{
    eBSP_CAN_INSTANCE_1 = 0u, /**< CAN1 peripheral */
    eBSP_CAN_INSTANCE_2 = 1u, /**< CAN2 peripheral */
    eBSP_CAN_INSTANCE_COUNT
} BspCanInstance_e;

/**
 * @brief CAN frame type.
 */
typedef enum
{
    eBSP_CAN_FRAME_DATA   = 0u, /**< Data frame */
    eBSP_CAN_FRAME_REMOTE = 1u  /**< Remote frame (RTR) */
} BspCanFrameType_e;

/**
 * @brief CAN ID type.
 */
typedef enum
{
    eBSP_CAN_ID_STANDARD = 0u, /**< 11-bit standard ID */
    eBSP_CAN_ID_EXTENDED = 1u  /**< 29-bit extended ID */
} BspCanIdType_e;

/**
 * @brief CAN error codes.
 */
typedef enum
{
    eBSP_CAN_ERR_NONE = 0,        /**< No error */
    eBSP_CAN_ERR_INVALID_PARAM,   /**< Invalid parameter passed to function */
    eBSP_CAN_ERR_INVALID_HANDLE,  /**< Invalid or unallocated handle */
    eBSP_CAN_ERR_NOT_STARTED,     /**< CAN not started yet */
    eBSP_CAN_ERR_ALREADY_STARTED, /**< CAN already started */
    eBSP_CAN_ERR_TX_QUEUE_FULL,   /**< TX queue at capacity */
    eBSP_CAN_ERR_NO_RESOURCE,     /**< No available module slots */
    eBSP_CAN_ERR_FILTER_FULL,     /**< Filter bank full */
    eBSP_CAN_ERR_HAL_ERROR,       /**< STM32 HAL error occurred */
    eBSP_CAN_ERR_BUS_OFF,         /**< CAN bus in bus-off state */
    eBSP_CAN_ERR_BUS_PASSIVE,     /**< CAN bus in error passive state */
    eBSP_CAN_ERR_RX_OVERRUN       /**< RX buffer overrun occurred */
} BspCanError_e;

/**
 * @brief CAN bus state enumeration.
 */
typedef enum
{
    eBSP_CAN_STATE_ERROR_ACTIVE = 0u, /**< Error active (normal operation) */
    eBSP_CAN_STATE_ERROR_PASSIVE,     /**< Error passive (degraded) */
    eBSP_CAN_STATE_BUS_OFF            /**< Bus off (requires restart) */
} BspCanBusState_e;

/**
 * @brief CAN message structure.
 */
typedef struct
{
    uint32_t          uId;        /**< CAN identifier (11 or 29 bit) */
    BspCanIdType_e    eIdType;    /**< Standard or extended ID */
    BspCanFrameType_e eFrameType; /**< Data or remote frame */
    uint8_t           byDataLen;  /**< Data length (0-8 bytes) */
    uint8_t           aData[8];   /**< Payload data (up to 8 bytes) */
    uint32_t          uTimestamp; /**< Message timestamp (HAL_GetTick) */
} BspCanMessage_t;

/**
 * @brief CAN filter configuration.
 *
 * Uses ID/mask mode for flexible filtering. Set mask bits to 1 for
 * ID bits that must match, 0 for don't care.
 */
typedef struct
{
    uint32_t       uFilterId;        /**< Filter ID value */
    uint32_t       uFilterMask;      /**< Filter mask (1=must match, 0=don't care) */
    BspCanIdType_e eIdType;          /**< Standard or extended ID filter */
    uint8_t        byFifoAssignment; /**< 0=FIFO0, 1=FIFO1 */
} BspCanFilter_t;

/**
 * @brief CAN initialization configuration.
 */
typedef struct
{
    BspCanInstance_e eInstance;       /**< CAN peripheral instance */
    bool             bLoopback;       /**< Enable loopback mode (testing) */
    bool             bSilent;         /**< Enable silent mode (monitoring) */
    bool             bAutoRetransmit; /**< Auto-retransmit on error */
} BspCanConfig_t;

#if BSP_CAN_ENABLE_STATISTICS
/**
 * @brief CAN statistics structure.
 */
typedef struct
{
    uint32_t uTxCount;      /**< Total messages transmitted */
    uint32_t uRxCount;      /**< Total messages received */
    uint32_t uErrorCount;   /**< Total error events */
    uint32_t uOverrunCount; /**< RX buffer overruns */
} BspCanStatistics_t;
#endif

/* ============================================================================
 * Callback Type Definitions
 * ========================================================================== */

/**
 * @brief RX message received callback.
 *
 * Called from ISR context when message received and passed through filters.
 * Message data is valid only during callback execution.
 *
 * @warning Executes in ISR context. Keep execution time <5µs.
 *          No blocking calls, no dynamic allocation.
 *
 * @param handle     CAN module handle
 * @param pMessage   Pointer to received message (valid only during callback)
 */
typedef void (*BspCanRxCallback_t)(BspCanHandle_t handle, const BspCanMessage_t* pMessage);

/**
 * @brief TX completion callback.
 *
 * Called from ISR context when message successfully transmitted on CAN bus.
 *
 * @warning Executes in ISR context. Keep execution time <5µs.
 *
 * @param handle     CAN module handle
 * @param uTxId      User-defined TX ID (from BspCanTransmit call)
 */
typedef void (*BspCanTxCallback_t)(BspCanHandle_t handle, uint32_t uTxId);

/**
 * @brief Error callback.
 *
 * Called from ISR context on CAN errors (bus-off, passive, overrun, etc).
 *
 * @warning Executes in ISR context. Keep execution time <5µs.
 *
 * @param handle     CAN module handle
 * @param eError     Error code
 */
typedef void (*BspCanErrorCallback_t)(BspCanHandle_t handle, BspCanError_e eError);

/**
 * @brief Bus state change callback.
 *
 * Called when CAN bus state transitions (active/passive/bus-off).
 *
 * @warning Executes in ISR context. Keep execution time <5µs.
 *
 * @param handle     CAN module handle
 * @param eState     New bus state
 */
typedef void (*BspCanBusStateCallback_t)(BspCanHandle_t handle, BspCanBusState_e eState);

/* ============================================================================
 * Initialization and Configuration API
 * ========================================================================== */

/**
 * @brief Allocate and initialize CAN module instance.
 *
 * Allocates a CAN module handle and initializes internal data structures.
 * The CAN peripheral is NOT started until BspCanStart() is called.
 *
 * @param pConfig    Pointer to configuration structure
 * @param pTxLed     Pointer to TX LED (NULL if not used)
 * @param pRxLed     Pointer to RX LED (NULL if not used)
 * @return           CAN handle (>= 0) on success, BSP_CAN_INVALID_HANDLE on error
 *
 * @note The STM32 HAL CAN peripheral must be configured via CubeMX before calling.
 * @note If LED pointers are not NULL, LedBlink() is called on each TX/RX event.
 */
BspCanHandle_t BspCanAllocate(const BspCanConfig_t* pConfig, LiveLed_t* pTxLed, LiveLed_t* pRxLed);

/**
 * @brief Free a CAN module instance.
 *
 * Stops CAN operation if started, releases resources, and frees the handle.
 *
 * @param handle     CAN module handle
 * @return           Error code
 */
BspCanError_e BspCanFree(BspCanHandle_t handle);

/**
 * @brief Add hardware filter to CAN instance.
 *
 * Configures hardware filter banks. Must be called BEFORE BspCanStart().
 * Supports up to BSP_CAN_MAX_FILTERS filters per instance.
 * Filters are activated atomically when BspCanStart() is called.
 *
 * @param handle     CAN module handle
 * @param pFilter    Pointer to filter configuration
 * @return           Error code
 *
 * @note Filters are not active until BspCanStart() is called.
 * @note Returns eBSP_CAN_ERR_FILTER_FULL if maximum filters exceeded.
 */
BspCanError_e BspCanAddFilter(BspCanHandle_t handle, const BspCanFilter_t* pFilter);

/**
 * @brief Start CAN communication.
 *
 * Activates all configured filters, enables CAN peripheral and interrupts,
 * and begins operation.
 *
 * @param handle     CAN module handle
 * @return           Error code
 */
BspCanError_e BspCanStart(BspCanHandle_t handle);

/**
 * @brief Stop CAN communication.
 *
 * Disables CAN peripheral and interrupts. Pending TX messages are aborted.
 *
 * @param handle     CAN module handle
 * @return           Error code
 */
BspCanError_e BspCanStop(BspCanHandle_t handle);

/* ============================================================================
 * Transmit API
 * ========================================================================== */

/**
 * @brief Transmit CAN message with priority.
 *
 * Queues message for transmission. Messages are sent in priority order:
 * - Primary: byPriority parameter (0 = highest priority in queue)
 * - Secondary: FIFO order within same priority level
 * - Final: CAN bus arbitration by CAN ID (lower ID wins)
 *
 * @param handle     CAN module handle
 * @param pMessage   Pointer to message to transmit
 * @param byPriority Priority level (0 to BSP_CAN_PRIORITY_LEVELS-1, 0=highest)
 * @param uTxId      User-defined TX ID (returned in TX completion callback)
 * @return           Error code
 *
 * @note Non-blocking. Returns eBSP_CAN_ERR_TX_QUEUE_FULL if queue is full.
 * @note Priority affects queue scheduling only. CAN bus arbitration uses CAN ID.
 * @note TX completion callback is invoked with uTxId when transmission complete.
 */
BspCanError_e BspCanTransmit(BspCanHandle_t handle, const BspCanMessage_t* pMessage, uint8_t byPriority, uint32_t uTxId);

/**
 * @brief Abort pending TX message.
 *
 * Attempts to remove message from TX queue before transmission.
 * Cannot abort messages already in hardware TX mailboxes.
 *
 * @param handle     CAN module handle
 * @param uTxId      TX ID to abort (matches uTxId from BspCanTransmit)
 * @return           eBSP_CAN_ERR_NONE if found and aborted, error code otherwise
 *
 * @note Only aborts queued messages, not messages in HW mailboxes.
 */
BspCanError_e BspCanAbortTransmit(BspCanHandle_t handle, uint32_t uTxId);

/**
 * @brief Get TX queue occupancy information.
 *
 * @param handle     CAN module handle
 * @param pUsed      Pointer to store used slots count
 * @param pFree      Pointer to store free slots count
 * @return           Error code
 */
BspCanError_e BspCanGetTxQueueInfo(BspCanHandle_t handle, uint8_t* pUsed, uint8_t* pFree);

/* ============================================================================
 * Receive API
 * ========================================================================== */

/**
 * @brief Register RX message callback.
 *
 * Callback is invoked from ISR context for each received message that
 * passes configured filters.
 *
 * @param handle     CAN module handle
 * @param pCallback  Callback function pointer (NULL to unregister)
 * @return           Error code
 */
BspCanError_e BspCanRegisterRxCallback(BspCanHandle_t handle, BspCanRxCallback_t pCallback);

/**
 * @brief Get RX buffer occupancy information.
 *
 * @param handle     CAN module handle
 * @param pUsed      Pointer to store used buffer count
 * @param pOverruns  Pointer to store cumulative overrun count
 * @return           Error code
 */
BspCanError_e BspCanGetRxBufferInfo(BspCanHandle_t handle, uint8_t* pUsed, uint32_t* pOverruns);

/* ============================================================================
 * Callback Registration and Error Handling API
 * ========================================================================== */

/**
 * @brief Register TX completion callback.
 *
 * @param handle     CAN module handle
 * @param pCallback  Callback function pointer (NULL to unregister)
 * @return           Error code
 */
BspCanError_e BspCanRegisterTxCallback(BspCanHandle_t handle, BspCanTxCallback_t pCallback);

/**
 * @brief Register error callback.
 *
 * Callback is invoked for all error events (bus-off, passive, overrun, etc).
 *
 * @param handle     CAN module handle
 * @param pCallback  Callback function pointer (NULL to unregister)
 * @return           Error code
 */
BspCanError_e BspCanRegisterErrorCallback(BspCanHandle_t handle, BspCanErrorCallback_t pCallback);

/**
 * @brief Register bus state change callback.
 *
 * Callback is invoked when CAN bus transitions between states.
 *
 * @param handle     CAN module handle
 * @param pCallback  Callback function pointer (NULL to unregister)
 * @return           Error code
 */
BspCanError_e BspCanRegisterBusStateCallback(BspCanHandle_t handle, BspCanBusStateCallback_t pCallback);

/**
 * @brief Get current bus state.
 *
 * @param handle     CAN module handle
 * @param pState     Pointer to store bus state
 * @return           Error code
 */
BspCanError_e BspCanGetBusState(BspCanHandle_t handle, BspCanBusState_e* pState);

/**
 * @brief Get error counters from CAN peripheral.
 *
 * @param handle     CAN module handle
 * @param pTxErrors  Pointer to store TX error counter (0-255)
 * @param pRxErrors  Pointer to store RX error counter (0-255)
 * @return           Error code
 */
BspCanError_e BspCanGetErrorCounters(BspCanHandle_t handle, uint8_t* pTxErrors, uint8_t* pRxErrors);

#if BSP_CAN_ENABLE_STATISTICS
/**
 * @brief Get statistics counters.
 *
 * @param handle     CAN module handle
 * @param pStats     Pointer to statistics structure
 * @return           Error code
 */
BspCanError_e BspCanGetStatistics(BspCanHandle_t handle, BspCanStatistics_t* pStats);
#endif

#ifdef __cplusplus
}
#endif
