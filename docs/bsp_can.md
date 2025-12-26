# BSP CAN Module

## Overview

The BSP CAN module provides a high-performance, event-driven CAN communication interface for STM32 microcontrollers. It features O(1) priority-based message queuing, lock-free RX buffering, and comprehensive error handling, all without external firmware dependencies.

### Key Features

- **Event-Driven Architecture**: Direct ISR callbacks with minimal latency
- **O(1) Priority Queue**: Bitmap-based priority queue for TX messages (0-7 priority levels)
- **Lock-Free RX Buffer**: Thread-safe circular buffer for ISR-to-user communication
- **Optional LED Feedback**: Visual indication of TX/RX activity via BSP LED module
- **Comprehensive Error Handling**: Bus-off, error passive, and overrun detection
- **Self-Contained**: No dependencies on external sequencer or utilities
- **Configurable Memory**: Tune TX queue and RX buffer size via configuration header
- **Statistics Tracking**: Optional TX/RX/error counters for debugging and monitoring
- **96% test coverage** (111 tests)

### Performance Characteristics

- **TX Queue Latency**: <1 µs (O(1) enqueue/dequeue with bitmap lookup)
- **ISR Processing Time**: <10 µs per event (including callback dispatch)
- **Throughput**: 5000+ messages/second @ 500 kbps CAN bus
- **Memory Footprint**: ~1 KB per CAN instance (configurable)

## Architecture

### TX Priority Queue (O(1) Operations)

The module uses a bitmap-based priority queue with equal distribution across 8 priority levels:

```
Priority Bitmap (8 bits)
┌─┬─┬─┬─┬─┬─┬─┬─┐
│7│6│5│4│3│2│1│0│  <- Bit N set = Priority N has queued messages
└─┴─┴─┴─┴─┴─┴─┴─┘

Priority Level 0 (Highest): [Msg1] [Msg2] [Msg3] [Msg4] (4 slots)
Priority Level 1:           [Msg5] [Msg6] [    ] [    ] (4 slots)
...
Priority Level 7 (Lowest):  [    ] [    ] [    ] [    ] (4 slots)

Total: 32 slots distributed equally across 8 priorities
```

**Dequeue Algorithm** (O(1)):
1. Check bitmap: `if (bitmap == 0) → queue empty`
2. Find highest priority: `priority = __builtin_ctz(bitmap)` (count trailing zeros)
3. Get message from priority queue head
4. Update bitmap if priority queue becomes empty

**Enqueue Algorithm** (O(1)):
1. Allocate entry from pool
2. Add to priority queue tail: `O(1)` array indexing
3. Set bitmap bit: `bitmap |= (1 << priority)`

### Lock-Free RX Buffer

Single-producer (ISR) / single-consumer (user callback) circular buffer:

```
ISR (Producer)              User Callback (Consumer)
     ↓                              ↓
[  ] [  ] [Msg] [Msg] [Msg] [  ] [  ]
        ^write              ^read

write_index: modified only by ISR
read_index:  modified only by user callback
→ No race conditions, no critical sections needed
```

### Message Flow

#### TX Path
```
User API                    Priority Queue              HW Mailboxes
────────                    ──────────────              ────────────
BspCanTransmit()    →   Enqueue by priority    →    Submit to HAL
      ↓                         ↓                          ↓
Set priority 0-7          Bitmap + circular         3 HW mailboxes
      ↓                    buffer per level              ↓
Critical section              ↓                    HAL_CAN_AddTxMessage
 (disable IRQ)           O(1) operations                 ↓
      ↓                         ↓                    TX complete ISR
Return immediately        Wait for free                  ↓
                            mailbox               TxCallback(uTxId)
                                                          ↓
                                                   Submit next from
                                                    priority queue
```

#### RX Path
```
HAL ISR                     RX Buffer                User Callback
───────                     ─────────                ─────────────
RX FIFO pending     →   Push to circular    →   RxCallback(pMsg)
      ↓                       buffer                     ↓
HAL_CAN_GetRxMessage          ↓                   Process message
      ↓                  Lock-free write              (ISR context!)
Parse CAN message             ↓                          ↓
      ↓                  Commit write index        Keep <5µs
LedBlink (optional)           ↓
      ↓                  Invoke callback
Direct callback               immediately
```

## Configuration

### Memory and Feature Configuration

Edit `bsp_can/bsp_can_config.h` to tune memory usage and features:

```c
/* TX queue depth (total entries across all priorities) */
#define BSP_CAN_TX_QUEUE_DEPTH      (32u)   /* 32 × 16 bytes = 512 bytes */

/* RX circular buffer depth */
#define BSP_CAN_RX_BUFFER_DEPTH     (16u)   /* 16 × 16 bytes = 256 bytes */

/* Number of priority levels for TX queue */
#define BSP_CAN_PRIORITY_LEVELS     (8u)    /* Valid: 2, 4, or 8 */

/* Maximum hardware filters per instance */
#define BSP_CAN_MAX_FILTERS         (14u)   /* 14 × 16 bytes = 224 bytes */

/* Enable statistics counters */
#define BSP_CAN_ENABLE_STATISTICS   (1u)    /* 1=enabled, 0=disabled */
```

### Memory Footprint Calculation

Per CAN instance:
- **Base structure**: ~200 bytes
- **TX queue**: `BSP_CAN_TX_QUEUE_DEPTH × 16` bytes (default: 512 bytes)
- **RX buffer**: `BSP_CAN_RX_BUFFER_DEPTH × 16` bytes (default: 256 bytes)
- **Filters**: `BSP_CAN_MAX_FILTERS × 16` bytes (default: 224 bytes)
- **Total**: ~1 KB (default configuration)

## API Reference

### Initialization and Configuration

#### BspCanAllocate
```c
BspCanHandle_t BspCanAllocate(const BspCanConfig_t *pConfig,
                               BspCanHandle_t hTxLed,
                               BspCanHandle_t hRxLed);
```
Allocates and initializes a CAN module instance.

**Parameters:**
- `pConfig`: Configuration structure (instance, HAL handle, mode flags)
- `hTxLed`: LED handle for TX indication (`BSP_CAN_INVALID_HANDLE` for none)
- `hRxLed`: LED handle for RX indication (`BSP_CAN_INVALID_HANDLE` for none)

**Returns:** CAN handle (≥0) on success, `BSP_CAN_INVALID_HANDLE` on error

**Example:**
```c
CAN_HandleTypeDef hcan1;  /* Configured by CubeMX */

BspCanConfig_t config = {
    .eInstance = eBSP_CAN_INSTANCE_1,
    .pHalHandle = &hcan1,
    .bLoopback = false,
    .bSilent = false,
    .bAutoRetransmit = true
};

BspCanHandle_t hCan = BspCanAllocate(&config, hTxLed, hRxLed);
if (hCan == BSP_CAN_INVALID_HANDLE) {
    /* Handle allocation error */
}
```

#### BspCanAddFilter
```c
BspCanError_e BspCanAddFilter(BspCanHandle_t handle, const BspCanFilter_t *pFilter);
```
Adds hardware filter. Must be called **before** `BspCanStart()`. Filters are activated atomically when starting.

**Parameters:**
- `handle`: CAN module handle
- `pFilter`: Filter configuration (ID, mask, type, FIFO assignment)

**Returns:** Error code

**Example:**
```c
/* Accept standard IDs 0x100-0x10F on FIFO0 */
BspCanFilter_t filter = {
    .uFilterId = 0x100,
    .uFilterMask = 0x7F0,      /* Mask bits 4-10 */
    .eIdType = eBSP_CAN_ID_STANDARD,
    .byFifoAssignment = 0       /* FIFO0 */
};

BspCanError_e err = BspCanAddFilter(hCan, &filter);
```

#### BspCanStart
```c
BspCanError_e BspCanStart(BspCanHandle_t handle);
```
Activates all configured filters, starts CAN peripheral, and enables interrupts.

**Example:**
```c
BspCanError_e err = BspCanStart(hCan);
if (err != eBSP_CAN_ERR_NONE) {
    /* Handle start error */
}
```

### Transmit API

#### BspCanTransmit
```c
BspCanError_e BspCanTransmit(BspCanHandle_t handle,
                              const BspCanMessage_t *pMessage,
                              uint8_t byPriority,
                              uint32_t uTxId);
```
Queues message for transmission with specified priority.

**Priority Arbitration:**
1. **Queue priority** (`byPriority`): 0 = highest, used for queueing order
2. **FIFO order**: Within same priority level
3. **CAN bus arbitration**: Lower CAN ID wins (hardware)

**Parameters:**
- `handle`: CAN module handle
- `pMessage`: Pointer to CAN message
- `byPriority`: Priority level (0 to `BSP_CAN_PRIORITY_LEVELS-1`)
- `uTxId`: User-defined TX ID (returned in TX callback)

**Returns:**
- `eBSP_CAN_ERR_NONE`: Queued successfully
- `eBSP_CAN_ERR_TX_QUEUE_FULL`: Queue at capacity (non-blocking)

**Example:**
```c
BspCanMessage_t msg = {
    .uId = 0x123,
    .eIdType = eBSP_CAN_ID_STANDARD,
    .eFrameType = eBSP_CAN_FRAME_DATA,
    .byDataLen = 8,
    .aData = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
};

BspCanError_e err = BspCanTransmit(hCan, &msg, 0, 0x1234);
if (err == eBSP_CAN_ERR_TX_QUEUE_FULL) {
    /* Queue full - implement retry or drop */
}
```

#### BspCanAbortTransmit
```c
BspCanError_e BspCanAbortTransmit(BspCanHandle_t handle, uint32_t uTxId);
```
Removes queued message before transmission. Cannot abort messages already in hardware mailboxes.

**Example:**
```c
/* Abort message with TX ID 0x1234 */
BspCanError_e err = BspCanAbortTransmit(hCan, 0x1234);
```

### Receive API

#### BspCanRegisterRxCallback
```c
BspCanError_e BspCanRegisterRxCallback(BspCanHandle_t handle,
                                        BspCanRxCallback_t pCallback);
```
Registers callback invoked from ISR context for each received message.

**⚠️ ISR Context Warning:** Callback executes in interrupt context. Keep execution time <5µs. No blocking calls, no dynamic allocation.

**Example:**
```c
void MyRxCallback(BspCanHandle_t handle, const BspCanMessage_t *pMsg) {
    /* Fast processing only - runs in ISR! */
    if (pMsg->uId == 0x100) {
        /* Copy to buffer for deferred processing */
        memcpy(&s_rxMsgBuffer, pMsg, sizeof(BspCanMessage_t));
        s_rxPendingFlag = true;
    }
}

BspCanRegisterRxCallback(hCan, MyRxCallback);
```

### Error Handling and Callbacks

#### BspCanRegisterErrorCallback
```c
BspCanError_e BspCanRegisterErrorCallback(BspCanHandle_t handle,
                                           BspCanErrorCallback_t pCallback);
```
Registers callback for all error events (bus-off, passive, overrun, HAL errors).

**Example:**
```c
void MyErrorCallback(BspCanHandle_t handle, BspCanError_e eError) {
    switch (eError) {
        case eBSP_CAN_ERR_BUS_OFF:
            /* CAN bus off - requires restart */
            BspCanStop(handle);
            BspCanStart(handle);  /* Auto-recovery */
            break;
        case eBSP_CAN_ERR_RX_OVERRUN:
            /* RX buffer full - messages dropped */
            s_rxOverrunCount++;
            break;
        default:
            break;
    }
}

BspCanRegisterErrorCallback(hCan, MyErrorCallback);
```

### Statistics and Diagnostics

#### BspCanGetStatistics
```c
BspCanError_e BspCanGetStatistics(BspCanHandle_t handle,
                                   BspCanStatistics_t *pStats);
```
Retrieves counters (TX/RX/error/overrun). Only available if `BSP_CAN_ENABLE_STATISTICS=1`.

**Example:**
```c
#if BSP_CAN_ENABLE_STATISTICS
BspCanStatistics_t stats;
BspCanGetStatistics(hCan, &stats);

printf("TX: %lu, RX: %lu, Errors: %lu, Overruns: %lu\n",
       stats.uTxCount, stats.uRxCount,
       stats.uErrorCount, stats.uOverrunCount);
#endif
```

## Usage Examples

### Example 1: Basic CAN Communication

Complete initialization and bi-directional communication:

```c
#include "bsp_can.h"
#include "bsp_led.h"

/* Global CAN handle */
static BspCanHandle_t s_hCan = BSP_CAN_INVALID_HANDLE;

/* RX callback (ISR context) */
void CanRxCallback(BspCanHandle_t handle, const BspCanMessage_t *pMsg) {
    /* Keep fast - ISR context! */
    if (pMsg->uId == 0x100) {
        ProcessCriticalMessage(pMsg);  /* Must be ISR-safe */
    }
}

/* TX completion callback (ISR context) */
void CanTxCallback(BspCanHandle_t handle, uint32_t uTxId) {
    /* Message with uTxId transmitted successfully */
    if (uTxId == 0x1234) {
        s_txCompleteFlag = true;
    }
}

/* Error callback (ISR context) */
void CanErrorCallback(BspCanHandle_t handle, BspCanError_e eError) {
    if (eError == eBSP_CAN_ERR_BUS_OFF) {
        s_busOffFlag = true;
    }
}

/* Initialization */
void CanModuleInit(void) {
    /* Allocate LED handles (from BSP LED module) */
    LiveLed_t* hTxLed = LedAllocate(LED_CAN_TX);
    LiveLed_t* hRxLed = LedAllocate(LED_CAN_RX);

    /* Configure CAN */
    extern CAN_HandleTypeDef hcan1;  /* From CubeMX */

    BspCanConfig_t config = {
        .eInstance = eBSP_CAN_INSTANCE_1,
        .bLoopback = false,
        .bSilent = false,
        .bAutoRetransmit = true
    };

    /* Allocate CAN module */
    s_hCan = BspCanAllocate(&config, hTxLed, hRxLed);
    if (s_hCan == BSP_CAN_INVALID_HANDLE) {
        ErrorHandler();
    }

    /* Add filters */
    BspCanFilter_t filter1 = {
        .uFilterId = 0x100,
        .uFilterMask = 0x7F0,
        .eIdType = eBSP_CAN_ID_STANDARD,
        .byFifoAssignment = 0
    };
    BspCanAddFilter(s_hCan, &filter1);

    /* Register callbacks */
    BspCanRegisterRxCallback(s_hCan, CanRxCallback);
    BspCanRegisterTxCallback(s_hCan, CanTxCallback);
    BspCanRegisterErrorCallback(s_hCan, CanErrorCallback);

    /* Start CAN */
    if (BspCanStart(s_hCan) != eBSP_CAN_ERR_NONE) {
        ErrorHandler();
    }
}

/* Transmit example */
void SendCanMessage(void) {
    BspCanMessage_t msg = {
        .uId = 0x123,
        .eIdType = eBSP_CAN_ID_STANDARD,
        .eFrameType = eBSP_CAN_FRAME_DATA,
        .byDataLen = 8,
        .aData = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
    };

    /* Priority 0 (highest), TX ID 0x1234 */
    BspCanError_e err = BspCanTransmit(s_hCan, &msg, 0, 0x1234);

    if (err == eBSP_CAN_ERR_TX_QUEUE_FULL) {
        /* Queue full - implement backoff/retry */
        HAL_Delay(1);
        BspCanTransmit(s_hCan, &msg, 0, 0x1234);
    }
}
```

### Example 2: Multi-Priority Transmission

Using priority levels for different message types:

```c
/* High-priority critical message */
void SendSafetyMessage(void) {
    BspCanMessage_t msg = {
        .uId = 0x100,
        .eIdType = eBSP_CAN_ID_STANDARD,
        .eFrameType = eBSP_CAN_FRAME_DATA,
        .byDataLen = 2,
        .aData = {SAFETY_STATUS, ERROR_CODE}
    };

    /* Priority 0 (highest) - safety messages always first */
    BspCanTransmit(s_hCan, &msg, 0, TX_ID_SAFETY);
}

/* Normal priority telemetry */
void SendTelemetryData(uint8_t sensor_id, uint16_t value) {
    BspCanMessage_t msg = {
        .uId = 0x200 + sensor_id,
        .eIdType = eBSP_CAN_ID_STANDARD,
        .eFrameType = eBSP_CAN_FRAME_DATA,
        .byDataLen = 4,
        .aData = {sensor_id, (value >> 8), (value & 0xFF), 0}
    };

    /* Priority 4 (medium) - normal telemetry */
    BspCanTransmit(s_hCan, &msg, 4, TX_ID_TELEMETRY_BASE + sensor_id);
}

/* Low priority diagnostics */
void SendDiagnostics(void) {
    BspCanMessage_t msg = {
        .uId = 0x7FF,
        .eIdType = eBSP_CAN_ID_STANDARD,
        .eFrameType = eBSP_CAN_FRAME_DATA,
        .byDataLen = 8,
        .aData = {/* diagnostic data */}
    };

    /* Priority 7 (lowest) - diagnostics when bus is idle */
    BspCanTransmit(s_hCan, &msg, 7, TX_ID_DIAGNOSTICS);
}

/* Transmission order (queue): Safety → Telemetry → Diagnostics */
/* Final CAN bus order: Determined by CAN ID (hardware arbitration) */
```

### Example 3: Advanced Filtering

Multiple filters for different message groups:

```c
void SetupAdvancedFilters(BspCanHandle_t hCan) {
    /* Filter 1: Accept standard IDs 0x100-0x10F (sensor group) */
    BspCanFilter_t sensorFilter = {
        .uFilterId = 0x100,
        .uFilterMask = 0x7F0,      /* Check bits 4-10 (0x1F0 mask) */
        .eIdType = eBSP_CAN_ID_STANDARD,
        .byFifoAssignment = 0       /* FIFO0 for sensors */
    };
    BspCanAddFilter(hCan, &sensorFilter);

    /* Filter 2: Accept standard IDs 0x200-0x20F (actuator group) */
    BspCanFilter_t actuatorFilter = {
        .uFilterId = 0x200,
        .uFilterMask = 0x7F0,
        .eIdType = eBSP_CAN_ID_STANDARD,
        .byFifoAssignment = 0       /* FIFO0 for actuators */
    };
    BspCanAddFilter(hCan, &actuatorFilter);

    /* Filter 3: Accept extended ID 0x18FF1234 (J1939 message) */
    BspCanFilter_t j1939Filter = {
        .uFilterId = 0x18FF1234,
        .uFilterMask = 0x1FFFFFFF,  /* Exact match for extended ID */
        .eIdType = eBSP_CAN_ID_EXTENDED,
        .byFifoAssignment = 1       /* FIFO1 for J1939 */
    };
    BspCanAddFilter(hCan, &j1939Filter);

    /* Filter 4: Accept all diagnostic messages (0x700-0x7FF) */
    BspCanFilter_t diagFilter = {
        .uFilterId = 0x700,
        .uFilterMask = 0x700,       /* Check only upper bits */
        .eIdType = eBSP_CAN_ID_STANDARD,
        .byFifoAssignment = 1       /* FIFO1 for diagnostics */
    };
    BspCanAddFilter(hCan, &diagFilter);
}
```

### Example 4: Deferred Processing Pattern

ISR-safe RX handling with deferred processing:

```c
#define RX_BUFFER_SIZE 32

/* Ring buffer for deferred processing */
typedef struct {
    BspCanMessage_t messages[RX_BUFFER_SIZE];
    volatile uint8_t writeIdx;
    uint8_t readIdx;
} CanRxRingBuffer_t;

static CanRxRingBuffer_t s_rxBuffer = {0};
static volatile bool s_rxPendingFlag = false;

/* Fast ISR callback - just copy data */
void CanRxCallback(BspCanHandle_t handle, const BspCanMessage_t *pMsg) {
    /* Calculate next write position */
    uint8_t nextIdx = (s_rxBuffer.writeIdx + 1) % RX_BUFFER_SIZE;

    /* Check for overflow */
    if (nextIdx == s_rxBuffer.readIdx) {
        /* Buffer full - drop message or handle overflow */
        return;
    }

    /* Copy message to ring buffer (fast!) */
    memcpy(&s_rxBuffer.messages[s_rxBuffer.writeIdx],
           pMsg,
           sizeof(BspCanMessage_t));

    /* Update write index (atomic for single byte) */
    s_rxBuffer.writeIdx = nextIdx;
    s_rxPendingFlag = true;
}

/* Slow processing in main loop context */
void ProcessCanMessages(void) {
    /* Process all pending messages */
    while (s_rxBuffer.readIdx != s_rxBuffer.writeIdx) {
        BspCanMessage_t *pMsg = &s_rxBuffer.messages[s_rxBuffer.readIdx];

        /* Can take as long as needed here - not in ISR! */
        switch (pMsg->uId) {
            case 0x100:
                ProcessSensorData(pMsg->aData, pMsg->byDataLen);
                break;
            case 0x200:
                ProcessActuatorCommand(pMsg->aData, pMsg->byDataLen);
                break;
            case 0x300:
                LogDiagnosticMessage(pMsg);
                break;
            default:
                HandleUnknownMessage(pMsg);
                break;
        }

        /* Advance read index */
        s_rxBuffer.readIdx = (s_rxBuffer.readIdx + 1) % RX_BUFFER_SIZE;
    }
    s_rxPendingFlag = false;
}

/* Main loop integration */
void MainLoop(void) {
    while (1) {
        if (s_rxPendingFlag) {
            ProcessCanMessages();
        }

        /* Other tasks */
        HAL_Delay(1);
    }
}
```

### Example 5: Extended ID and Remote Frames

Working with 29-bit extended IDs and remote frames:

```c
/* Send extended ID message (J1939) */
void SendJ1939Message(void) {
    BspCanMessage_t msg = {
        .uId = 0x18FEEE00,          /* PGN 65262, source address 0 */
        .eIdType = eBSP_CAN_ID_EXTENDED,
        .eFrameType = eBSP_CAN_FRAME_DATA,
        .byDataLen = 8,
        .aData = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}
    };

    BspCanTransmit(s_hCan, &msg, 2, TX_ID_J1939);
}

/* Send remote frame request */
void RequestSensorData(uint16_t sensor_id) {
    BspCanMessage_t remoteFrame = {
        .uId = 0x100 + sensor_id,
        .eIdType = eBSP_CAN_ID_STANDARD,
        .eFrameType = eBSP_CAN_FRAME_REMOTE,  /* Remote frame */
        .byDataLen = 0,                        /* No data in remote frame */
    };

    BspCanTransmit(s_hCan, &remoteFrame, 3, TX_ID_REMOTE_REQ);
}

/* Handle remote frame in RX callback */
void CanRxCallbackWithRemote(BspCanHandle_t handle, const BspCanMessage_t *pMsg) {
    if (pMsg->eFrameType == eBSP_CAN_FRAME_REMOTE) {
        /* Remote frame received - send response */
        if (pMsg->uId == 0x123) {
            SendRequestedData();
        }
    } else {
        /* Normal data frame */
        ProcessDataFrame(pMsg);
    }
}
```

### Example 6: Robust Error Handling and Recovery

Complete error handling with automatic recovery:

```c
static uint32_t s_busOffCount = 0;
static uint32_t s_errorPassiveCount = 0;
static bool s_autoRecoveryEnabled = true;

/* Comprehensive error callback */
void CanErrorCallbackRobust(BspCanHandle_t handle, BspCanError_e eError) {
    switch (eError) {
        case eBSP_CAN_ERR_BUS_OFF:
            s_busOffCount++;

            if (s_autoRecoveryEnabled) {
                /* Automatic recovery with exponential backoff */
                uint32_t delayMs = 100 * (1 << MIN(s_busOffCount - 1, 5));

                BspCanStop(handle);
                HAL_Delay(delayMs);

                BspCanError_e err = BspCanStart(handle);
                if (err == eBSP_CAN_ERR_NONE) {
                    LogInfo("CAN recovered from bus-off");
                } else {
                    LogError("CAN recovery failed");
                }
            } else {
                LogError("CAN bus-off - manual recovery required");
            }
            break;

        case eBSP_CAN_ERR_BUS_PASSIVE:
            s_errorPassiveCount++;
            LogWarning("CAN error passive - check bus quality");
            break;

        case eBSP_CAN_ERR_RX_OVERRUN:
            LogWarning("CAN RX overrun - messages dropped");
            /* Consider reducing message processing time */
            break;

        case eBSP_CAN_ERR_HAL_ERROR:
            LogError("CAN HAL error - check hardware");
            break;

        default:
            LogError("Unknown CAN error: %d", eError);
            break;
    }
}

/* Bus state monitoring */
void CanBusStateCallback(BspCanHandle_t handle, BspCanBusState_e eState) {
    switch (eState) {
        case eBSP_CAN_STATE_ERROR_ACTIVE:
            LogInfo("CAN bus active - normal operation");
            s_busOffCount = 0;  /* Reset counter on recovery */
            break;

        case eBSP_CAN_STATE_ERROR_PASSIVE:
            LogWarning("CAN error passive - degraded operation");
            break;

        case eBSP_CAN_STATE_BUS_OFF:
            LogError("CAN bus off - recovery needed");
            break;
    }
}

/* Periodic health monitoring */
void MonitorCanHealth(BspCanHandle_t hCan) {
    uint8_t txErrors, rxErrors;
    BspCanGetErrorCounters(hCan, &txErrors, &rxErrors);

    /* Check error counters */
    if (txErrors > 96 || rxErrors > 96) {
        LogWarning("CAN errors high: TX=%u, RX=%u", txErrors, rxErrors);
    }

    /* Check queue status */
    uint8_t used, free;
    BspCanGetTxQueueInfo(hCan, &used, &free);

    if (free < 4) {
        LogWarning("CAN TX queue nearly full: %u/%u",
                   used, used + free);
    }

#if BSP_CAN_ENABLE_STATISTICS
    /* Check statistics */
    BspCanStatistics_t stats;
    BspCanGetStatistics(hCan, &stats);

    if (stats.uOverrunCount > 0) {
        LogWarning("CAN RX overruns: %lu", stats.uOverrunCount);
    }

    LogInfo("CAN Stats - TX:%lu RX:%lu Errors:%lu",
            stats.uTxCount, stats.uRxCount, stats.uErrorCount);
#endif
}
```

### Example 7: Dual CAN Instance (CAN1 + CAN2)

Managing multiple CAN peripherals simultaneously:

```c
static BspCanHandle_t s_hCan1 = BSP_CAN_INVALID_HANDLE;
static BspCanHandle_t s_hCan2 = BSP_CAN_INVALID_HANDLE;

/* Separate callbacks for each instance */
void Can1RxCallback(BspCanHandle_t handle, const BspCanMessage_t *pMsg) {
    LogInfo("CAN1 RX: ID=0x%X", pMsg->uId);
    ProcessCan1Message(pMsg);
}

void Can2RxCallback(BspCanHandle_t handle, const BspCanMessage_t *pMsg) {
    LogInfo("CAN2 RX: ID=0x%X", pMsg->uId);
    ProcessCan2Message(pMsg);
}

/* Initialize both CAN peripherals */
void InitDualCan(void) {
    extern CAN_HandleTypeDef hcan1;
    extern CAN_HandleTypeDef hcan2;

    /* CAN1 Configuration - High-speed bus (500 kbps) */
    BspCanConfig_t config1 = {
        .eInstance = eBSP_CAN_INSTANCE_1,
        .bLoopback = false,
        .bSilent = false,
        .bAutoRetransmit = true
    };

    s_hCan1 = BspCanAllocate(&config1, NULL, NULL);
    if (s_hCan1 == BSP_CAN_INVALID_HANDLE) {
        ErrorHandler();
    }

    /* CAN2 Configuration - Low-speed bus (125 kbps) */
    BspCanConfig_t config2 = {
        .eInstance = eBSP_CAN_INSTANCE_2,
        .bLoopback = false,
        .bSilent = false,
        .bAutoRetransmit = true
    };

    s_hCan2 = BspCanAllocate(&config2, NULL, NULL);
    if (s_hCan2 == BSP_CAN_INVALID_HANDLE) {
        ErrorHandler();
    }

    /* Configure filters for CAN1 */
    BspCanFilter_t can1Filter = {
        .uFilterId = 0x100,
        .uFilterMask = 0x700,
        .eIdType = eBSP_CAN_ID_STANDARD,
        .byFifoAssignment = 0
    };
    BspCanAddFilter(s_hCan1, &can1Filter);

    /* Configure filters for CAN2 */
    BspCanFilter_t can2Filter = {
        .uFilterId = 0x200,
        .uFilterMask = 0x700,
        .eIdType = eBSP_CAN_ID_STANDARD,
        .byFifoAssignment = 0
    };
    BspCanAddFilter(s_hCan2, &can2Filter);

    /* Register callbacks */
    BspCanRegisterRxCallback(s_hCan1, Can1RxCallback);
    BspCanRegisterRxCallback(s_hCan2, Can2RxCallback);

    /* Start both */
    BspCanStart(s_hCan1);
    BspCanStart(s_hCan2);
}

/* CAN bridge - forward messages between buses */
void Can1ToCanBridge(BspCanHandle_t handle, const BspCanMessage_t *pMsg) {
    /* Forward message from CAN1 to CAN2 */
    if (pMsg->uId >= 0x500 && pMsg->uId <= 0x5FF) {
        BspCanTransmit(s_hCan2, pMsg, 4, TX_ID_BRIDGE);
    }
}
```

### Example 8: Loopback Mode for Testing

Using loopback mode for self-test and development:

```c
static uint32_t s_loopbackTxCount = 0;
static uint32_t s_loopbackRxCount = 0;

void LoopbackTxCallback(BspCanHandle_t handle, uint32_t uTxId) {
    s_loopbackTxCount++;
}

void LoopbackRxCallback(BspCanHandle_t handle, const BspCanMessage_t *pMsg) {
    s_loopbackRxCount++;

    /* Verify received message matches what was sent */
    if (pMsg->uId == 0x123 && pMsg->byDataLen == 8) {
        LogInfo("Loopback test successful");
    }
}

/* Initialize CAN in loopback mode for self-test */
BspCanHandle_t InitLoopbackTest(void) {
    extern CAN_HandleTypeDef hcan1;

    BspCanConfig_t config = {
        .eInstance = eBSP_CAN_INSTANCE_1,
        .bLoopback = true,          /* Enable loopback mode */
        .bSilent = false,
        .bAutoRetransmit = true
    };

    BspCanHandle_t hCan = BspCanAllocate(&config, NULL, NULL);

    /* In loopback mode, still need at least one filter */
    BspCanFilter_t filter = {
        .uFilterId = 0x000,
        .uFilterMask = 0x000,       /* Accept all messages */
        .eIdType = eBSP_CAN_ID_STANDARD,
        .byFifoAssignment = 0
    };
    BspCanAddFilter(hCan, &filter);

    /* Register callbacks */
    BspCanRegisterRxCallback(hCan, LoopbackRxCallback);
    BspCanRegisterTxCallback(hCan, LoopbackTxCallback);

    BspCanStart(hCan);

    return hCan;
}

/* Run loopback test */
bool RunLoopbackTest(BspCanHandle_t hCan) {
    s_loopbackTxCount = 0;
    s_loopbackRxCount = 0;

    /* Send test message */
    BspCanMessage_t testMsg = {
        .uId = 0x123,
        .eIdType = eBSP_CAN_ID_STANDARD,
        .eFrameType = eBSP_CAN_FRAME_DATA,
        .byDataLen = 8,
        .aData = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
    };

    BspCanTransmit(hCan, &testMsg, 0, TEST_TX_ID);

    /* Wait for loopback */
    HAL_Delay(10);

    /* Verify TX and RX both occurred */
    return (s_loopbackTxCount == 1) && (s_loopbackRxCount == 1);
}
```

### Example 9: Using Without LED Feedback

Minimal configuration without LED indicators:

```c
/* Initialize without LED feedback */
BspCanHandle_t InitMinimalCan(void) {
    extern CAN_HandleTypeDef hcan1;

    BspCanConfig_t config = {
        .eInstance = eBSP_CAN_INSTANCE_1,
        .bLoopback = false,
        .bSilent = false,
        .bAutoRetransmit = true
    };

    /* Pass NULL for LED handles */
    BspCanHandle_t hCan = BspCanAllocate(&config, NULL, NULL);

    /* Add filter and start */
    BspCanFilter_t filter = {
        .uFilterId = 0x000,
        .uFilterMask = 0x000,
        .eIdType = eBSP_CAN_ID_STANDARD,
        .byFifoAssignment = 0
    };
    BspCanAddFilter(hCan, &filter);
    BspCanStart(hCan);

    return hCan;
}
```

### Example 10: Message Abort and Queue Management

Managing queued messages and handling queue full conditions:

```c
#define MAX_RETRIES 3

/* Send with retry on queue full */
BspCanError_e SendWithRetry(BspCanHandle_t hCan,
                             const BspCanMessage_t *pMsg,
                             uint8_t priority) {
    static uint32_t txIdCounter = 0;
    uint32_t txId = txIdCounter++;

    for (int retry = 0; retry < MAX_RETRIES; retry++) {
        BspCanError_e err = BspCanTransmit(hCan, pMsg, priority, txId);

        if (err == eBSP_CAN_ERR_NONE) {
            return eBSP_CAN_ERR_NONE;
        }

        if (err == eBSP_CAN_ERR_TX_QUEUE_FULL) {
            /* Check queue status */
            uint8_t used, free;
            BspCanGetTxQueueInfo(hCan, &used, &free);
            LogWarning("TX queue full: %u/%u, retry %d",
                       used, used + free, retry);

            /* Wait and retry */
            HAL_Delay(1);
            continue;
        }

        /* Other error - don't retry */
        return err;
    }

    /* Max retries exceeded */
    return eBSP_CAN_ERR_TX_QUEUE_FULL;
}

/* Abort low-priority messages when queue gets full */
void AbortLowPriorityOnFull(BspCanHandle_t hCan) {
    uint8_t used, free;
    BspCanGetTxQueueInfo(hCan, &used, &free);

    /* If queue >80% full, abort low-priority messages */
    if (free < (BSP_CAN_TX_QUEUE_DEPTH / 5)) {
        /* Abort diagnostics messages (TX IDs 0x7000-0x7FFF) */
        for (uint32_t txId = 0x7000; txId <= 0x7FFF; txId++) {
            BspCanAbortTransmit(hCan, txId);
        }
    }
}

/* Track and abort superseded messages */
static uint32_t s_latestTelemetryTxId[256] = {0};

void SendTelemetryWithSupersede(uint8_t sensor_id, uint16_t value) {
    /* Abort previous telemetry from same sensor if still queued */
    if (s_latestTelemetryTxId[sensor_id] != 0) {
        BspCanAbortTransmit(s_hCan, s_latestTelemetryTxId[sensor_id]);
    }

    /* Send new telemetry */
    BspCanMessage_t msg = {
        .uId = 0x200 + sensor_id,
        .eIdType = eBSP_CAN_ID_STANDARD,
        .eFrameType = eBSP_CAN_FRAME_DATA,
        .byDataLen = 4,
        .aData = {sensor_id, (value >> 8), (value & 0xFF), 0}
    };

    uint32_t txId = (0x8000 | sensor_id);
    s_latestTelemetryTxId[sensor_id] = txId;

    BspCanTransmit(s_hCan, &msg, 4, txId);
}

void TelemetryTxCallback(BspCanHandle_t handle, uint32_t uTxId) {
    /* Clear tracking when message actually sent */
    if ((uTxId & 0x8000) != 0) {
        uint8_t sensor_id = uTxId & 0xFF;
        s_latestTelemetryTxId[sensor_id] = 0;
    }
}
```

## ISR Context Requirements

All callbacks execute in **interrupt context**. Follow these guidelines:

### ✅ Allowed in Callbacks
- Read/write volatile variables
- Set flags for main loop processing
- Copy message data to buffers
- Simple state machine updates
- Call other ISR-safe functions

### ❌ **Not Allowed** in Callbacks
- `printf()` or other blocking I/O
- `HAL_Delay()` or any delays
- Mutex locks (`osMutexAcquire`, etc.)
- Dynamic memory allocation (`malloc`, `new`)
- Long computations (>5 µs)

### Recommended Pattern: Deferred Processing

See **Example 4: Deferred Processing Pattern** above for a complete implementation showing how to safely handle messages in ISR context and defer processing to the main loop.

## Error Codes

| Error Code | Description | Typical Cause |
|------------|-------------|---------------|
| `eBSP_CAN_ERR_NONE` | No error | Success |
| `eBSP_CAN_ERR_INVALID_PARAM` | Invalid parameter | NULL pointer, priority out of range |
| `eBSP_CAN_ERR_INVALID_HANDLE` | Invalid handle | Unallocated or freed handle |
| `eBSP_CAN_ERR_NOT_STARTED` | CAN not started | Forgot to call `BspCanStart()` |
| `eBSP_CAN_ERR_ALREADY_STARTED` | CAN already running | Called `BspCanStart()` twice |
| `eBSP_CAN_ERR_TX_QUEUE_FULL` | TX queue full | Transmitting too fast, need backoff |
| `eBSP_CAN_ERR_NO_RESOURCE` | No free module slots | All instances allocated |
| `eBSP_CAN_ERR_FILTER_FULL` | Filter bank full | Too many filters (max 14) |
| `eBSP_CAN_ERR_HAL_ERROR` | STM32 HAL error | Hardware issue, check HAL status |
| `eBSP_CAN_ERR_BUS_OFF` | CAN bus off | Too many errors, requires restart |
| `eBSP_CAN_ERR_BUS_PASSIVE` | Error passive state | Degraded bus, check wiring |
| `eBSP_CAN_ERR_RX_OVERRUN` | RX buffer overrun | Process messages faster |

## Bus-Off Recovery

When CAN enters bus-off state (too many errors):

```c
void CanErrorCallback(BspCanHandle_t handle, BspCanError_e eError) {
    if (eError == eBSP_CAN_ERR_BUS_OFF) {
        /* Stop and restart to recover */
        BspCanStop(handle);
        HAL_Delay(100);  /* Wait for bus to stabilize */
        BspCanStart(handle);
    }
}
```

Or register bus state callback for finer control:

```c
void CanBusStateCallback(BspCanHandle_t handle, BspCanBusState_e eState) {
    switch (eState) {
        case eBSP_CAN_STATE_BUS_OFF:
            /* Automatic recovery */
            s_busOffRecoveryPending = true;
            break;
        case eBSP_CAN_STATE_ERROR_PASSIVE:
            /* Log warning */
            s_errorPassiveCount++;
            break;
        case eBSP_CAN_STATE_ERROR_ACTIVE:
            /* Normal operation resumed */
            s_busOffRecoveryPending = false;
            break;
    }
}
```

## Testing and Coverage

Unit tests are located in `tests/bsp_can/` and use Unity + CMock frameworks.

### Test Coverage

The BSP CAN module has comprehensive test coverage:

- **Line Coverage**: 96% (382 of 396 lines)
- **Function Coverage**: 99.4% (153 of 154 functions)
- **Branch Coverage**: 79% (248 of 314 branches)
- **Total Tests**: 111 test cases

### Test Categories

The test suite covers:
- **Allocation and Initialization**: Module allocation, configuration validation, handle management
- **Filtering**: Hardware filter configuration, ID/mask mode, FIFO assignment
- **Transmission**: Priority queue operations, mailbox management, LED feedback, message queueing
- **Reception**: FIFO message reception, circular buffer management, overflow handling
- **Callbacks**: RX, TX, error, and bus state callbacks in ISR context
- **Error Handling**: Bus-off, error passive, invalid parameters, overrun detection
- **Statistics**: Counter tracking, queue/buffer info queries
- **Multi-Instance**: CAN1 and CAN2 concurrent operation
- **Edge Cases**: Queue full, buffer overrun, invalid handles, priority boundaries

### Running Tests

```bash
cd bsp
cmake --preset bsp-test-host
cmake --build build/bsp-test-host --target test_bsp_can
./build/bsp-test-host/tests/bsp_can/test_bsp_can
```

### Coverage Report

Generate detailed coverage report:

```bash
cd build/bsp-test-host
gcovr --filter='.*bsp_can\.c$' --html-details coverage.html
# Open coverage.html in browser

# Or summary only:
gcovr --filter='.*bsp_can\.c$' --print-summary
```

### Coverage by Feature

| Feature | Coverage | Notes |
|---------|----------|-------|
| TX Priority Queue | 100% | All priority levels and queue operations |
| RX Circular Buffer | 100% | Buffer management and overflow |
| Hardware Filters | 98% | ID/mask mode, FIFO assignment |
| Callback Dispatch | 100% | All callback types (RX/TX/Error/BusState) |
| Error Handling | 95% | Bus-off, passive, overrun, HAL errors |
| Statistics | 100% | All counters when enabled |
| Multi-Instance | 100% | CAN1 and CAN2 |

## Migration from Old Implementation

If migrating from the old sequencer-based CAN driver:

| Old API | New API | Notes |
|---------|---------|-------|
| `CanInit()` | `BspCanAllocate()` | Now returns handle |
| `CanStart()` | `BspCanStart()` | Same name, different signature |
| `CanTx()` | `BspCanTransmit()` | Priority parameter separate |
| `CanRegisterCb()` | `BspCanRegisterRxCallback()` | Per-instance callback |
| `CanAddFilter()` | `BspCanAddFilter()` | Similar, deferred activation |

**Key Differences:**
- **No sequencer dependency**: Callbacks are direct from ISR
- **Handle-based**: Each instance has unique handle
- **Priority parameter**: Explicit priority (0-7) instead of embedded in CAN ID
- **Event-driven**: No polling, callbacks invoked automatically

## Performance Tuning

### For High-Throughput Applications

Increase queue depth in `bsp_can_config.h`:
```c
#define BSP_CAN_TX_QUEUE_DEPTH      (64u)  /* Larger queue */
#define BSP_CAN_RX_BUFFER_DEPTH     (32u)  /* Larger buffer */
```

### For Memory-Constrained Systems

Reduce queue depth:
```c
#define BSP_CAN_TX_QUEUE_DEPTH      (16u)  /* Smaller queue */
#define BSP_CAN_RX_BUFFER_DEPTH     (8u)   /* Smaller buffer */
#define BSP_CAN_ENABLE_STATISTICS   (0u)   /* Disable stats */
```

### For Real-Time Critical Systems

Reduce priority levels for faster lookup:
```c
#define BSP_CAN_PRIORITY_LEVELS     (4u)   /* Fewer levels = more slots per level */
```

## Troubleshooting

### Q: Messages not received
**A:** Check filters are configured correctly before `BspCanStart()`. Verify FIFO assignment matches filter configuration.

### Q: TX queue full errors
**A:** Either transmitting too fast or not enough queue depth. Implement backoff/retry or increase `BSP_CAN_TX_QUEUE_DEPTH`.

### Q: RX overruns
**A:** Processing messages too slowly in callback. Use deferred processing pattern (copy to buffer, process in main loop).

### Q: Bus-off errors
**A:** Check CAN bus termination (120Ω), wiring quality, and bit timing configuration in CubeMX.

### Q: LED not blinking
**A:** Verify LED handles are valid (not `BSP_CAN_INVALID_HANDLE`) and LED module is initialized.

## See Also

- [BSP LED](bsp_led.md) - LED control for TX/RX indicators
- [BSP GPIO](bsp_gpio.md) - Low-level GPIO operations
- [Common Utilities](bsp_common.md) - Compiler attributes and common definitions
- [Building](../README.md#building) - Build system and configuration
- [Testing](testing.md) - Unit testing framework and practices

## License and Support

Part of the BSP library. See main project README for license information.

For issues or questions, refer to the project issue tracker or contact the BSP development team.
