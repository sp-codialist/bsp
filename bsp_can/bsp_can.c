/**
 * @file bsp_can.c
 * @brief CAN BSP module implementation with O(1) priority queue
 *
 * Implementation of event-driven CAN driver with bitmap-based priority queue
 * and lock-free RX circular buffer.
 */

#include "bsp_can.h"
#include "stm32f4xx_hal.h"
#include <stddef.h>
#include <string.h>

/* ============================================================================
 * External HAL Handle Declarations
 * ========================================================================== */

/* These handles are defined by CubeMX in main.c or can_config.c */
extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

/**
 * @brief Lookup table mapping instance enum to HAL handles
 */
static CAN_HandleTypeDef* const s_apHalHandles[BSP_CAN_MAX_INSTANCES] = {
    &hcan1, /* eBSP_CAN_INSTANCE_1 = 0 */
    &hcan2  /* eBSP_CAN_INSTANCE_2 = 1 */
};

/* ============================================================================
 * Private Constants
 * ========================================================================== */

/** Number of hardware TX mailboxes in STM32 CAN peripheral */
#define CAN_HW_MAILBOX_COUNT (3u)

/** Capacity per priority level (equal distribution) */
static const uint8_t CAN_QUEUE_CAPACITY_PER_PRIORITY = (BSP_CAN_TX_QUEUE_DEPTH / BSP_CAN_PRIORITY_LEVELS);

/* ============================================================================
 * Private Type Definitions (all private structures in .c file)
 * ========================================================================== */

/**
 * @brief TX queue entry.
 */
typedef struct
{
    BspCanMessage_t tMessage;   /**< CAN message */
    uint32_t        uTxId;      /**< User TX ID */
    uint8_t         byPriority; /**< Priority level */
    bool            bInUse;     /**< Entry allocated flag */
} BspCanTxEntry_t;

/**
 * @brief Circular buffer for one priority level.
 */
typedef struct
{
    uint8_t byHead;                                /**< Read index */
    uint8_t byTail;                                /**< Write index */
    uint8_t byCount;                               /**< Number of entries */
    uint8_t aEntryIndices[BSP_CAN_TX_QUEUE_DEPTH]; /**< Indices into entry pool */
} BspCanPriorityQueue_t;

/**
 * @brief TX queue manager (per CAN instance).
 */
typedef struct
{
    BspCanPriorityQueue_t aQueues[BSP_CAN_PRIORITY_LEVELS]; /**< One queue per priority */
    BspCanTxEntry_t       aEntries[BSP_CAN_TX_QUEUE_DEPTH]; /**< Shared entry pool */
    uint8_t               byPriorityBitmap;                 /**< Bitmap of non-empty queues */
    uint8_t               byTotalUsed;                      /**< Total entries in use */
} BspCanTxQueueManager_t;

/**
 * @brief RX circular buffer entry.
 */
typedef struct
{
    BspCanMessage_t tMessage; /**< Received message */
} BspCanRxEntry_t;

/**
 * @brief RX buffer manager (lock-free single producer/consumer).
 */
typedef struct
{
    BspCanRxEntry_t  aEntries[BSP_CAN_RX_BUFFER_DEPTH]; /**< Circular buffer */
    volatile uint8_t byWriteIndex;                      /**< ISR write index (volatile) */
    uint8_t          byReadIndex;                       /**< User read index */
#if BSP_CAN_ENABLE_STATISTICS
    uint32_t uOverrunCount; /**< Cumulative overrun counter */
#endif
} BspCanRxBuffer_t;

/**
 * @brief Mailbox tracking structure.
 */
typedef struct
{
    volatile bool bActive; /**< Mailbox contains pending message */
    uint32_t      uTxId;   /**< User TX ID of message in mailbox */
} BspCanMailbox_t;

/**
 * @brief CAN module instance structure.
 */
typedef struct
{
    /* Configuration */
    BspCanConfig_t     tConfig;
    CAN_HandleTypeDef* pHalHandle; /**< Cast from config's void* */
    bool               bAllocated;
    bool               bStarted;

    /* TX Queue */
    BspCanTxQueueManager_t tTxQueue;

    /* RX Buffer */
    BspCanRxBuffer_t tRxBuffer;

    /* Filters */
    BspCanFilter_t aFilters[BSP_CAN_MAX_FILTERS];
    uint8_t        byFilterCount;

    /* Mailbox Tracking */
    BspCanMailbox_t aMailboxes[CAN_HW_MAILBOX_COUNT];

    /* LED Handles */
    LiveLed_t* pTxLed;
    LiveLed_t* pRxLed;

    /* Callbacks */
    BspCanRxCallback_t       pRxCallback;
    BspCanTxCallback_t       pTxCallback;
    BspCanErrorCallback_t    pErrorCallback;
    BspCanBusStateCallback_t pBusStateCallback;

#if BSP_CAN_ENABLE_STATISTICS
    /* Statistics */
    uint32_t uTxCount;
    uint32_t uRxCount;
    uint32_t uErrorCount;
#endif
} BspCanModule_t;

/* ============================================================================
 * Private Global Variables
 * ========================================================================== */

/** Module instance array */
static BspCanModule_t s_aModules[BSP_CAN_MAX_INSTANCES] = {0};

/* ============================================================================
 * Private Helper Functions - TX Queue Management (O(1) operations)
 * ========================================================================== */

/**
 * @brief Initialize TX queue manager.
 */
static void sTxQueueInit(BspCanTxQueueManager_t* pQueue)
{
    memset(pQueue, 0, sizeof(BspCanTxQueueManager_t));

    /* Initialize each priority queue */
    for (uint8_t i = 0u; i < BSP_CAN_PRIORITY_LEVELS; i++)
    {
        pQueue->aQueues[i].byHead  = 0u;
        pQueue->aQueues[i].byTail  = 0u;
        pQueue->aQueues[i].byCount = 0u;
    }
}

/**
 * @brief Allocate a TX entry from the pool.
 * @return Pointer to entry, or NULL if pool full.
 */
static BspCanTxEntry_t* sTxQueueAllocateEntry(BspCanTxQueueManager_t* pQueue)
{
    if (pQueue->byTotalUsed >= BSP_CAN_TX_QUEUE_DEPTH)
    {
        return NULL;
    }

    /* Find free entry */
    for (uint8_t i = 0u; i < BSP_CAN_TX_QUEUE_DEPTH; i++)
    {
        if (!pQueue->aEntries[i].bInUse)
        {
            pQueue->aEntries[i].bInUse = true;
            pQueue->byTotalUsed++;
            return &pQueue->aEntries[i];
        }
    }

    return NULL;
}

/**
 * @brief Enqueue entry into priority level. O(1) operation.
 * @return true on success, false if priority queue full.
 */
static bool sTxQueueEnqueue(BspCanTxQueueManager_t* pQueue, uint8_t byEntryIndex, uint8_t byPriority)
{
    if (byPriority >= BSP_CAN_PRIORITY_LEVELS)
    {
        return false;
    }

    BspCanPriorityQueue_t* pPrioQueue = &pQueue->aQueues[byPriority];

    /* Check if priority queue has space */
    if (pPrioQueue->byCount >= CAN_QUEUE_CAPACITY_PER_PRIORITY)
    {
        return false;
    }

    /* Add to tail */
    pPrioQueue->aEntryIndices[pPrioQueue->byTail] = byEntryIndex;
    pPrioQueue->byTail                            = (pPrioQueue->byTail + 1u) % CAN_QUEUE_CAPACITY_PER_PRIORITY;
    pPrioQueue->byCount++;

    /* Update bitmap */
    pQueue->byPriorityBitmap |= (1u << byPriority);

    return true;
}

/**
 * @brief Dequeue highest priority entry. O(1) operation using __builtin_ctz.
 * @return Entry index, or 0xFF if queue empty.
 */
static uint8_t sTxQueueDequeue(BspCanTxQueueManager_t* pQueue)
{
    /* Check if any queue has entries */
    if (pQueue->byPriorityBitmap == 0u)
    {
        return 0xFFu;
    }

    /* Find highest priority (lowest bit set) using count trailing zeros */
    uint8_t byPriority = (uint8_t)__builtin_ctz(pQueue->byPriorityBitmap);

    BspCanPriorityQueue_t* pPrioQueue = &pQueue->aQueues[byPriority];

    /* Get entry from head */
    uint8_t byEntryIndex = pPrioQueue->aEntryIndices[pPrioQueue->byHead];
    pPrioQueue->byHead   = (pPrioQueue->byHead + 1u) % CAN_QUEUE_CAPACITY_PER_PRIORITY;
    pPrioQueue->byCount--;

    /* Update bitmap if queue now empty */
    if (pPrioQueue->byCount == 0u)
    {
        pQueue->byPriorityBitmap &= ~(1u << byPriority);
    }

    return byEntryIndex;
}

/**
 * @brief Free a TX entry back to pool.
 */
static void sTxQueueFreeEntry(BspCanTxQueueManager_t* pQueue, uint8_t byEntryIndex)
{
    if (byEntryIndex < BSP_CAN_TX_QUEUE_DEPTH)
    {
        pQueue->aEntries[byEntryIndex].bInUse = false;
        pQueue->byTotalUsed--;
    }
}

/**
 * @brief Search and remove entry from queue by TX ID (for abort).
 * @return true if found and removed, false otherwise.
 */
static bool sTxQueueRemoveByTxId(BspCanTxQueueManager_t* pQueue, uint32_t uTxId)
{
    /* Search all priority queues */
    for (uint8_t byPrio = 0u; byPrio < BSP_CAN_PRIORITY_LEVELS; byPrio++)
    {
        BspCanPriorityQueue_t* pPrioQueue = &pQueue->aQueues[byPrio];

        /* Search this priority queue */
        uint8_t byIdx = pPrioQueue->byHead;
        for (uint8_t i = 0u; i < pPrioQueue->byCount; i++)
        {
            uint8_t byEntryIdx = pPrioQueue->aEntryIndices[byIdx];

            if (pQueue->aEntries[byEntryIdx].uTxId == uTxId)
            {
                /* Found it - remove by shifting remaining entries */
                for (uint8_t j = i; j < (pPrioQueue->byCount - 1u); j++)
                {
                    uint8_t byCurrIdx                    = (pPrioQueue->byHead + j) % CAN_QUEUE_CAPACITY_PER_PRIORITY;
                    uint8_t byNextIdx                    = (pPrioQueue->byHead + j + 1u) % CAN_QUEUE_CAPACITY_PER_PRIORITY;
                    pPrioQueue->aEntryIndices[byCurrIdx] = pPrioQueue->aEntryIndices[byNextIdx];
                }

                pPrioQueue->byTail = (pPrioQueue->byTail == 0u) ? (CAN_QUEUE_CAPACITY_PER_PRIORITY - 1u) : (pPrioQueue->byTail - 1u);
                pPrioQueue->byCount--;

                /* Update bitmap if queue now empty */
                if (pPrioQueue->byCount == 0u)
                {
                    pQueue->byPriorityBitmap &= ~(1u << byPrio);
                }

                /* Free the entry */
                sTxQueueFreeEntry(pQueue, byEntryIdx);

                return true;
            }

            byIdx = (byIdx + 1u) % CAN_QUEUE_CAPACITY_PER_PRIORITY;
        }
    }

    return false;
}

/* ============================================================================
 * Private Helper Functions - RX Buffer Management (lock-free)
 * ========================================================================== */

/**
 * @brief Initialize RX buffer.
 */
static void sRxBufferInit(BspCanRxBuffer_t* pBuffer)
{
    memset(pBuffer, 0, sizeof(BspCanRxBuffer_t));
}

/**
 * @brief Get RX buffer used count.
 */
static uint8_t sRxBufferGetUsed(const BspCanRxBuffer_t* pBuffer)
{
    if (pBuffer->byWriteIndex >= pBuffer->byReadIndex)
    {
        return pBuffer->byWriteIndex - pBuffer->byReadIndex;
    }
    else
    {
        return BSP_CAN_RX_BUFFER_DEPTH - (pBuffer->byReadIndex - pBuffer->byWriteIndex);
    }
}

/* ============================================================================
 * Private Helper Functions - HAL and Hardware Interaction
 * ========================================================================== */

/**
 * @brief Convert mailbox number to index.
 */
static uint8_t sMailboxToIndex(uint32_t uMailbox)
{
    if (uMailbox == CAN_TX_MAILBOX0)
        return 0u;
    if (uMailbox == CAN_TX_MAILBOX1)
        return 1u;
    if (uMailbox == CAN_TX_MAILBOX2)
        return 2u;
    return 0xFFu;
}

/**
 * @brief Find module by HAL handle.
 * @return Module handle or BSP_CAN_INVALID_HANDLE if not found.
 */
static BspCanHandle_t sFindModuleByHalHandle(CAN_HandleTypeDef* pHalHandle)
{
    for (uint8_t i = 0u; i < BSP_CAN_MAX_INSTANCES; i++)
    {
        if (s_aModules[i].bAllocated && s_aModules[i].pHalHandle == pHalHandle)
        {
            return (BspCanHandle_t)i;
        }
    }
    return BSP_CAN_INVALID_HANDLE;
}

/**
 * @brief Submit next queued message to hardware mailbox.
 */
static void sSubmitNextTx(BspCanModule_t* pModule)
{
    /* Check if any mailbox is free */
    uint32_t uFreeLevel = HAL_CAN_GetTxMailboxesFreeLevel(pModule->pHalHandle);
    if (uFreeLevel == 0u)
    {
        return;
    }

    /* Dequeue highest priority message */
    uint8_t byEntryIdx = sTxQueueDequeue(&pModule->tTxQueue);
    if (byEntryIdx == 0xFFu)
    {
        return; /* Queue empty */
    }

    BspCanTxEntry_t* pEntry = &pModule->tTxQueue.aEntries[byEntryIdx];

    /* Prepare HAL TX header */
    CAN_TxHeaderTypeDef tTxHeader = {0};

    if (pEntry->tMessage.eIdType == eBSP_CAN_ID_STANDARD)
    {
        tTxHeader.StdId = pEntry->tMessage.uId;
        tTxHeader.IDE   = CAN_ID_STD;
    }
    else
    {
        tTxHeader.ExtId = pEntry->tMessage.uId;
        tTxHeader.IDE   = CAN_ID_EXT;
    }

    tTxHeader.RTR                = (pEntry->tMessage.eFrameType == eBSP_CAN_FRAME_REMOTE) ? CAN_RTR_REMOTE : CAN_RTR_DATA;
    tTxHeader.DLC                = pEntry->tMessage.byDataLen;
    tTxHeader.TransmitGlobalTime = DISABLE;

    /* Submit to HAL */
    uint32_t          uMailbox  = 0u;
    HAL_StatusTypeDef halStatus = HAL_CAN_AddTxMessage(pModule->pHalHandle, &tTxHeader, pEntry->tMessage.aData, &uMailbox);

    if (halStatus == HAL_OK)
    {
        /* Track mailbox */
        uint8_t byMbxIdx = sMailboxToIndex(uMailbox);
        if (byMbxIdx < CAN_HW_MAILBOX_COUNT)
        {
            pModule->aMailboxes[byMbxIdx].bActive = true;
            pModule->aMailboxes[byMbxIdx].uTxId   = pEntry->uTxId;
        }

        /* Blink TX LED */
        if (pModule->pTxLed != NULL)
        {
            LedBlink(pModule->pTxLed);
        }
    }

    /* Free the queue entry */
    sTxQueueFreeEntry(&pModule->tTxQueue, byEntryIdx);
}

/**
 * @brief Parse HAL RX header into BSP message structure.
 */
static void sParseRxMessage(const CAN_RxHeaderTypeDef* pRxHeader, const uint8_t* pData, BspCanMessage_t* pMessage)
{
    if (pRxHeader->IDE == CAN_ID_STD)
    {
        pMessage->uId     = pRxHeader->StdId;
        pMessage->eIdType = eBSP_CAN_ID_STANDARD;
    }
    else
    {
        pMessage->uId     = pRxHeader->ExtId;
        pMessage->eIdType = eBSP_CAN_ID_EXTENDED;
    }

    pMessage->eFrameType = (pRxHeader->RTR == CAN_RTR_REMOTE) ? eBSP_CAN_FRAME_REMOTE : eBSP_CAN_FRAME_DATA;
    pMessage->byDataLen  = (uint8_t)pRxHeader->DLC;
    pMessage->uTimestamp = HAL_GetTick();

    memcpy(pMessage->aData, pData, pMessage->byDataLen);
}

/* ============================================================================
 * Private Helper Functions - Validation
 * ========================================================================== */

/**
 * @brief Validate handle.
 * @return Pointer to module or NULL if invalid.
 */
static BspCanModule_t* sValidateHandle(BspCanHandle_t handle)
{
    if ((handle < 0) || ((uint8_t)handle >= BSP_CAN_MAX_INSTANCES))
    {
        return NULL;
    }

    if (!s_aModules[handle].bAllocated)
    {
        return NULL;
    }

    return &s_aModules[handle];
}

/* ============================================================================
 * Public API Implementation
 * ========================================================================== */

BspCanHandle_t BspCanAllocate(const BspCanConfig_t* pConfig, LiveLed_t* pTxLed, LiveLed_t* pRxLed)
{
    if (pConfig == NULL)
    {
        return BSP_CAN_INVALID_HANDLE;
    }

    if (pConfig->eInstance >= eBSP_CAN_INSTANCE_COUNT)
    {
        return BSP_CAN_INVALID_HANDLE;
    }

    /* Find free module slot */
    BspCanHandle_t handle = BSP_CAN_INVALID_HANDLE;
    for (uint8_t i = 0u; i < BSP_CAN_MAX_INSTANCES; i++)
    {
        if (!s_aModules[i].bAllocated)
        {
            handle = (BspCanHandle_t)i;
            break;
        }
    }

    if (handle == BSP_CAN_INVALID_HANDLE)
    {
        return BSP_CAN_INVALID_HANDLE;
    }

    BspCanModule_t* pModule = &s_aModules[handle];

    /* Initialize module */
    memset(pModule, 0, sizeof(BspCanModule_t));
    pModule->tConfig = *pConfig;

    /* Lookup HAL handle from instance enum */
    pModule->pHalHandle = s_apHalHandles[pConfig->eInstance];

    pModule->bAllocated = true;
    pModule->bStarted   = false;
    pModule->pTxLed     = pTxLed;
    pModule->pRxLed     = pRxLed;

    /* Initialize queues and buffers */
    sTxQueueInit(&pModule->tTxQueue);
    sRxBufferInit(&pModule->tRxBuffer);

    return handle;
}

BspCanError_e BspCanFree(BspCanHandle_t handle)
{
    BspCanModule_t* pModule = sValidateHandle(handle);
    if (pModule == NULL)
    {
        return eBSP_CAN_ERR_INVALID_HANDLE;
    }

    /* Stop if started */
    if (pModule->bStarted)
    {
        (void)BspCanStop(handle);
    }

    /* Clear module */
    memset(pModule, 0, sizeof(BspCanModule_t));

    return eBSP_CAN_ERR_NONE;
}

BspCanError_e BspCanAddFilter(BspCanHandle_t handle, const BspCanFilter_t* pFilter)
{
    BspCanModule_t* pModule = sValidateHandle(handle);
    if (pModule == NULL)
    {
        return eBSP_CAN_ERR_INVALID_HANDLE;
    }

    if (pFilter == NULL)
    {
        return eBSP_CAN_ERR_INVALID_PARAM;
    }

    if (pModule->bStarted)
    {
        return eBSP_CAN_ERR_ALREADY_STARTED;
    }

    if (pModule->byFilterCount >= BSP_CAN_MAX_FILTERS)
    {
        return eBSP_CAN_ERR_FILTER_FULL;
    }

    /* Store filter configuration (will be activated in BspCanStart) */
    pModule->aFilters[pModule->byFilterCount] = *pFilter;
    pModule->byFilterCount++;

    return eBSP_CAN_ERR_NONE;
}

BspCanError_e BspCanStart(BspCanHandle_t handle)
{
    BspCanModule_t* pModule = sValidateHandle(handle);
    if (pModule == NULL)
    {
        return eBSP_CAN_ERR_INVALID_HANDLE;
    }

    if (pModule->bStarted)
    {
        return eBSP_CAN_ERR_ALREADY_STARTED;
    }

    CAN_HandleTypeDef* pHal = pModule->pHalHandle;

    /* Configure filters atomically */
    for (uint8_t i = 0u; i < pModule->byFilterCount; i++)
    {
        CAN_FilterTypeDef sFilterConfig = {0};

        if (pModule->aFilters[i].eIdType == eBSP_CAN_ID_STANDARD)
        {
            sFilterConfig.FilterIdHigh     = (pModule->aFilters[i].uFilterId << 5u);
            sFilterConfig.FilterMaskIdHigh = (pModule->aFilters[i].uFilterMask << 5u);
            sFilterConfig.FilterIdLow      = 0u;
            sFilterConfig.FilterMaskIdLow  = 0u;
            sFilterConfig.FilterScale      = CAN_FILTERSCALE_32BIT;
        }
        else
        {
            sFilterConfig.FilterIdHigh     = (uint16_t)(pModule->aFilters[i].uFilterId >> 13u);
            sFilterConfig.FilterIdLow      = (uint16_t)((pModule->aFilters[i].uFilterId << 3u) | 0x04u);
            sFilterConfig.FilterMaskIdHigh = (uint16_t)(pModule->aFilters[i].uFilterMask >> 13u);
            sFilterConfig.FilterMaskIdLow  = (uint16_t)((pModule->aFilters[i].uFilterMask << 3u) | 0x04u);
            sFilterConfig.FilterScale      = CAN_FILTERSCALE_32BIT;
        }

        sFilterConfig.FilterMode           = CAN_FILTERMODE_IDMASK;
        sFilterConfig.FilterFIFOAssignment = (pModule->aFilters[i].byFifoAssignment == 0u) ? CAN_FILTER_FIFO0 : CAN_FILTER_FIFO1;
        sFilterConfig.FilterBank           = i;
        sFilterConfig.FilterActivation     = CAN_FILTER_ENABLE;

        if (HAL_CAN_ConfigFilter(pHal, &sFilterConfig) != HAL_OK)
        {
            return eBSP_CAN_ERR_HAL_ERROR;
        }
    }

    /* Start CAN */
    if (HAL_CAN_Start(pHal) != HAL_OK)
    {
        return eBSP_CAN_ERR_HAL_ERROR;
    }

    /* Activate RX interrupts */
    if (HAL_CAN_ActivateNotification(pHal, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING | CAN_IT_TX_MAILBOX_EMPTY |
                                               CAN_IT_ERROR | CAN_IT_BUSOFF | CAN_IT_ERROR_PASSIVE) != HAL_OK)
    {
        HAL_CAN_Stop(pHal);
        return eBSP_CAN_ERR_HAL_ERROR;
    }

    pModule->bStarted = true;

    return eBSP_CAN_ERR_NONE;
}

BspCanError_e BspCanStop(BspCanHandle_t handle)
{
    BspCanModule_t* pModule = sValidateHandle(handle);
    if (pModule == NULL)
    {
        return eBSP_CAN_ERR_INVALID_HANDLE;
    }

    if (!pModule->bStarted)
    {
        return eBSP_CAN_ERR_NOT_STARTED;
    }

    /* Deactivate notifications */
    (void)HAL_CAN_DeactivateNotification(pModule->pHalHandle, CAN_IT_RX_FIFO0_MSG_PENDING);

    /* Stop CAN peripheral */
    (void)HAL_CAN_Stop(pModule->pHalHandle);

    pModule->bStarted = false;

    return eBSP_CAN_ERR_NONE;
}

BspCanError_e BspCanTransmit(BspCanHandle_t handle, const BspCanMessage_t* pMessage, uint8_t byPriority, uint32_t uTxId)
{
    BspCanModule_t* pModule = sValidateHandle(handle);
    if (pModule == NULL)
    {
        return eBSP_CAN_ERR_INVALID_HANDLE;
    }

    if (pMessage == NULL || byPriority >= BSP_CAN_PRIORITY_LEVELS)
    {
        return eBSP_CAN_ERR_INVALID_PARAM;
    }

    if (!pModule->bStarted)
    {
        return eBSP_CAN_ERR_NOT_STARTED;
    }

    /* Allocate entry */
    BspCanTxEntry_t* pEntry = sTxQueueAllocateEntry(&pModule->tTxQueue);
    if (pEntry == NULL)
    {
        return eBSP_CAN_ERR_TX_QUEUE_FULL;
    }

    /* Fill entry */
    pEntry->tMessage            = *pMessage;
    pEntry->uTxId               = uTxId;
    pEntry->byPriority          = byPriority;
    pEntry->tMessage.uTimestamp = HAL_GetTick();

    /* Get entry index */
    uint8_t byEntryIdx = (uint8_t)(pEntry - pModule->tTxQueue.aEntries);

    /* Enqueue with critical section */
    __disable_irq();
    bool bSuccess = sTxQueueEnqueue(&pModule->tTxQueue, byEntryIdx, byPriority);
    __enable_irq();

    if (!bSuccess)
    {
        sTxQueueFreeEntry(&pModule->tTxQueue, byEntryIdx);
        return eBSP_CAN_ERR_TX_QUEUE_FULL;
    }

    /* Try to submit immediately */
    sSubmitNextTx(pModule);

    return eBSP_CAN_ERR_NONE;
}

BspCanError_e BspCanAbortTransmit(BspCanHandle_t handle, uint32_t uTxId)
{
    BspCanModule_t* pModule = sValidateHandle(handle);
    if (pModule == NULL)
    {
        return eBSP_CAN_ERR_INVALID_HANDLE;
    }

    /* Critical section for queue manipulation */
    __disable_irq();
    bool bFound = sTxQueueRemoveByTxId(&pModule->tTxQueue, uTxId);
    __enable_irq();

    return bFound ? eBSP_CAN_ERR_NONE : eBSP_CAN_ERR_INVALID_PARAM;
}

BspCanError_e BspCanGetTxQueueInfo(BspCanHandle_t handle, uint8_t* pUsed, uint8_t* pFree)
{
    BspCanModule_t* pModule = sValidateHandle(handle);
    if (pModule == NULL)
    {
        return eBSP_CAN_ERR_INVALID_HANDLE;
    }

    if (pUsed != NULL)
    {
        *pUsed = pModule->tTxQueue.byTotalUsed;
    }

    if (pFree != NULL)
    {
        *pFree = BSP_CAN_TX_QUEUE_DEPTH - pModule->tTxQueue.byTotalUsed;
    }

    return eBSP_CAN_ERR_NONE;
}

BspCanError_e BspCanRegisterRxCallback(BspCanHandle_t handle, BspCanRxCallback_t pCallback)
{
    BspCanModule_t* pModule = sValidateHandle(handle);
    if (pModule == NULL)
    {
        return eBSP_CAN_ERR_INVALID_HANDLE;
    }

    pModule->pRxCallback = pCallback;

    return eBSP_CAN_ERR_NONE;
}

BspCanError_e BspCanGetRxBufferInfo(BspCanHandle_t handle, uint8_t* pUsed, uint32_t* pOverruns)
{
    BspCanModule_t* pModule = sValidateHandle(handle);
    if (pModule == NULL)
    {
        return eBSP_CAN_ERR_INVALID_HANDLE;
    }

    if (pUsed != NULL)
    {
        *pUsed = sRxBufferGetUsed(&pModule->tRxBuffer);
    }

#if BSP_CAN_ENABLE_STATISTICS
    if (pOverruns != NULL)
    {
        *pOverruns = pModule->tRxBuffer.uOverrunCount;
    }
#else
    if (pOverruns != NULL)
    {
        *pOverruns = 0u;
    }
#endif

    return eBSP_CAN_ERR_NONE;
}

BspCanError_e BspCanRegisterTxCallback(BspCanHandle_t handle, BspCanTxCallback_t pCallback)
{
    BspCanModule_t* pModule = sValidateHandle(handle);
    if (pModule == NULL)
    {
        return eBSP_CAN_ERR_INVALID_HANDLE;
    }

    pModule->pTxCallback = pCallback;

    return eBSP_CAN_ERR_NONE;
}

BspCanError_e BspCanRegisterErrorCallback(BspCanHandle_t handle, BspCanErrorCallback_t pCallback)
{
    BspCanModule_t* pModule = sValidateHandle(handle);
    if (pModule == NULL)
    {
        return eBSP_CAN_ERR_INVALID_HANDLE;
    }

    pModule->pErrorCallback = pCallback;

    return eBSP_CAN_ERR_NONE;
}

BspCanError_e BspCanRegisterBusStateCallback(BspCanHandle_t handle, BspCanBusStateCallback_t pCallback)
{
    BspCanModule_t* pModule = sValidateHandle(handle);
    if (pModule == NULL)
    {
        return eBSP_CAN_ERR_INVALID_HANDLE;
    }

    pModule->pBusStateCallback = pCallback;

    return eBSP_CAN_ERR_NONE;
}

BspCanError_e BspCanGetBusState(BspCanHandle_t handle, BspCanBusState_e* pState)
{
    BspCanModule_t* pModule = sValidateHandle(handle);
    if (pModule == NULL || pState == NULL)
    {
        return eBSP_CAN_ERR_INVALID_HANDLE;
    }

    uint32_t uEsr = pModule->pHalHandle->Instance->ESR;

    if ((uEsr & CAN_ESR_BOFF) != 0u)
    {
        *pState = eBSP_CAN_STATE_BUS_OFF;
    }
    else if ((uEsr & CAN_ESR_EPVF) != 0u)
    {
        *pState = eBSP_CAN_STATE_ERROR_PASSIVE;
    }
    else
    {
        *pState = eBSP_CAN_STATE_ERROR_ACTIVE;
    }

    return eBSP_CAN_ERR_NONE;
}

BspCanError_e BspCanGetErrorCounters(BspCanHandle_t handle, uint8_t* pTxErrors, uint8_t* pRxErrors)
{
    BspCanModule_t* pModule = sValidateHandle(handle);
    if (pModule == NULL)
    {
        return eBSP_CAN_ERR_INVALID_HANDLE;
    }

    uint32_t uEsr = pModule->pHalHandle->Instance->ESR;

    if (pTxErrors != NULL)
    {
        *pTxErrors = (uint8_t)((uEsr & CAN_ESR_TEC) >> 16u);
    }

    if (pRxErrors != NULL)
    {
        *pRxErrors = (uint8_t)((uEsr & CAN_ESR_REC) >> 24u);
    }

    return eBSP_CAN_ERR_NONE;
}

#if BSP_CAN_ENABLE_STATISTICS
BspCanError_e BspCanGetStatistics(BspCanHandle_t handle, BspCanStatistics_t* pStats)
{
    BspCanModule_t* pModule = sValidateHandle(handle);
    if (pModule == NULL || pStats == NULL)
    {
        return eBSP_CAN_ERR_INVALID_HANDLE;
    }

    pStats->uTxCount      = pModule->uTxCount;
    pStats->uRxCount      = pModule->uRxCount;
    pStats->uErrorCount   = pModule->uErrorCount;
    pStats->uOverrunCount = pModule->tRxBuffer.uOverrunCount;

    return eBSP_CAN_ERR_NONE;
}
#endif

/* ============================================================================
 * HAL Callback Implementations (ISR Context)
 * ========================================================================== */

/**
 * @brief RX FIFO 0 message pending callback (overrides HAL weak function).
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan)
{
    BspCanHandle_t handle = sFindModuleByHalHandle(hcan);
    if (handle == BSP_CAN_INVALID_HANDLE)
    {
        return;
    }

    BspCanModule_t* pModule = &s_aModules[handle];

    /* Read message from hardware FIFO */
    CAN_RxHeaderTypeDef tRxHeader  = {0};
    uint8_t             aRxData[8] = {0};

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &tRxHeader, aRxData) != HAL_OK)
    {
        return;
    }

    /* Blink RX LED */
    if (pModule->pRxLed != NULL)
    {
        LedBlink(pModule->pRxLed);
    }

#if BSP_CAN_ENABLE_STATISTICS
    pModule->uRxCount++;
#endif

    /* Invoke callback directly from ISR */
    if (pModule->pRxCallback != NULL)
    {
        BspCanMessage_t tMessage = {0};
        sParseRxMessage(&tRxHeader, aRxData, &tMessage);
        pModule->pRxCallback(handle, &tMessage);
    }
}

/**
 * @brief RX FIFO 1 message pending callback.
 */
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef* hcan)
{
    /* Same implementation as FIFO 0 but reading from FIFO 1 */
    BspCanHandle_t handle = sFindModuleByHalHandle(hcan);
    if (handle == BSP_CAN_INVALID_HANDLE)
    {
        return;
    }

    BspCanModule_t* pModule = &s_aModules[handle];

    CAN_RxHeaderTypeDef tRxHeader  = {0};
    uint8_t             aRxData[8] = {0};

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &tRxHeader, aRxData) != HAL_OK)
    {
        return;
    }

    if (pModule->pRxLed != NULL)
    {
        LedBlink(pModule->pRxLed);
    }

#if BSP_CAN_ENABLE_STATISTICS
    pModule->uRxCount++;
#endif

    if (pModule->pRxCallback != NULL)
    {
        BspCanMessage_t tMessage = {0};
        sParseRxMessage(&tRxHeader, aRxData, &tMessage);
        pModule->pRxCallback(handle, &tMessage);
    }
}

/**
 * @brief TX mailbox 0 complete callback.
 */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef* hcan)
{
    BspCanHandle_t handle = sFindModuleByHalHandle(hcan);
    if (handle == BSP_CAN_INVALID_HANDLE)
    {
        return;
    }

    BspCanModule_t* pModule = &s_aModules[handle];

    /* Mark mailbox as free and invoke callback */
    uint32_t uTxId                 = pModule->aMailboxes[0].uTxId;
    pModule->aMailboxes[0].bActive = false;

#if BSP_CAN_ENABLE_STATISTICS
    pModule->uTxCount++;
#endif

    if (pModule->pTxCallback != NULL)
    {
        pModule->pTxCallback(handle, uTxId);
    }

    /* Submit next queued message */
    sSubmitNextTx(pModule);
}

/**
 * @brief TX mailbox 1 complete callback.
 */
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef* hcan)
{
    BspCanHandle_t handle = sFindModuleByHalHandle(hcan);
    if (handle == BSP_CAN_INVALID_HANDLE)
    {
        return;
    }

    BspCanModule_t* pModule = &s_aModules[handle];

    uint32_t uTxId                 = pModule->aMailboxes[1].uTxId;
    pModule->aMailboxes[1].bActive = false;

#if BSP_CAN_ENABLE_STATISTICS
    pModule->uTxCount++;
#endif

    if (pModule->pTxCallback != NULL)
    {
        pModule->pTxCallback(handle, uTxId);
    }

    sSubmitNextTx(pModule);
}

/**
 * @brief TX mailbox 2 complete callback.
 */
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef* hcan)
{
    BspCanHandle_t handle = sFindModuleByHalHandle(hcan);
    if (handle == BSP_CAN_INVALID_HANDLE)
    {
        return;
    }

    BspCanModule_t* pModule = &s_aModules[handle];

    uint32_t uTxId                 = pModule->aMailboxes[2].uTxId;
    pModule->aMailboxes[2].bActive = false;

#if BSP_CAN_ENABLE_STATISTICS
    pModule->uTxCount++;
#endif

    if (pModule->pTxCallback != NULL)
    {
        pModule->pTxCallback(handle, uTxId);
    }

    sSubmitNextTx(pModule);
}

/**
 * @brief CAN error callback.
 */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef* hcan)
{
    BspCanHandle_t handle = sFindModuleByHalHandle(hcan);
    if (handle == BSP_CAN_INVALID_HANDLE)
    {
        return;
    }

    BspCanModule_t* pModule = &s_aModules[handle];

#if BSP_CAN_ENABLE_STATISTICS
    pModule->uErrorCount++;
#endif

    uint32_t uErrorCode = HAL_CAN_GetError(hcan);

    /* Determine error type */
    BspCanError_e eError = eBSP_CAN_ERR_HAL_ERROR;

    if ((uErrorCode & HAL_CAN_ERROR_BOF) != 0u)
    {
        eError = eBSP_CAN_ERR_BUS_OFF;

        if (pModule->pBusStateCallback != NULL)
        {
            pModule->pBusStateCallback(handle, eBSP_CAN_STATE_BUS_OFF);
        }
    }
    else if ((uErrorCode & HAL_CAN_ERROR_EPV) != 0u)
    {
        eError = eBSP_CAN_ERR_BUS_PASSIVE;

        if (pModule->pBusStateCallback != NULL)
        {
            pModule->pBusStateCallback(handle, eBSP_CAN_STATE_ERROR_PASSIVE);
        }
    }

    /* Invoke error callback */
    if (pModule->pErrorCallback != NULL)
    {
        pModule->pErrorCallback(handle, eError);
    }
}
