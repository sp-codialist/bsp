# BSP I2C Module

High-performance I2C communication driver for STM32F4 microcontrollers.

## Features

- Support for all 6 I2C peripherals (I2C1-I2C6)
- Dual operation modes: Blocking and Interrupt
- Master transmit/receive operations
- Memory read/write operations (EEPROM support)
- Configurable timeout for blocking operations
- Callback-based interrupt completion notification
- Error handling and reporting
- 93.3% line coverage, 86.8% branch coverage (79 tests)

## API Reference

### Allocation and Configuration

- `BspI2cAllocate(eInstance, eMode, uTimeoutMs)` - Allocate I2C instance
- `BspI2cFree(handle)` - Free I2C instance

**Instances**: `eBSP_I2C_INSTANCE_1` through `eBSP_I2C_INSTANCE_6`

**Modes**:
- `eBSP_I2C_MODE_BLOCKING` - Timeout-based blocking operations
- `eBSP_I2C_MODE_INTERRUPT` - Interrupt-driven non-blocking operations

### Callback Registration (Interrupt Mode)

- `BspI2cRegisterTxCallback(handle, callback)` - Master transmit complete
- `BspI2cRegisterRxCallback(handle, callback)` - Master receive complete
- `BspI2cRegisterMemTxCallback(handle, callback)` - Memory write complete
- `BspI2cRegisterMemRxCallback(handle, callback)` - Memory read complete
- `BspI2cRegisterErrorCallback(handle, callback)` - Error notification

### Blocking Operations

#### Master Transmit/Receive
- `BspI2cTransmit(handle, pConfig)` - Send data to slave
- `BspI2cReceive(handle, pConfig)` - Receive data from slave

**Configuration Structure**:
```c
typedef struct {
    uint16_t devAddr;    // 7-bit slave address (shifted left)
    uint8_t* pData;      // Data buffer
    uint16_t length;     // Number of bytes
} BspI2cTransferConfig_t;
```

#### Memory Operations
- `BspI2cMemRead(handle, pConfig)` - Read from slave memory
- `BspI2cMemWrite(handle, pConfig)` - Write to slave memory

**Memory Configuration Structure**:
```c
typedef struct {
    uint16_t devAddr;              // 7-bit slave address (shifted left)
    uint16_t memAddr;              // Memory address
    BspI2cMemAddrSize_e memAddrSize; // Address size (8-bit or 16-bit)
    uint8_t* pData;                // Data buffer
    uint16_t length;               // Number of bytes
} BspI2cMemConfig_t;
```

**Memory Address Sizes**:
- `eBSP_I2C_MEM_ADDR_SIZE_8BIT` - Single byte address
- `eBSP_I2C_MEM_ADDR_SIZE_16BIT` - Two byte address

### Interrupt Operations

- `BspI2cTransmitIT(handle, pConfig)` - Send via interrupt
- `BspI2cReceiveIT(handle, pConfig)` - Receive via interrupt
- `BspI2cMemReadIT(handle, pConfig)` - Memory read via interrupt
- `BspI2cMemWriteIT(handle, pConfig)` - Memory write via interrupt

## Error Codes

- `eBSP_I2C_ERR_NONE` - No error
- `eBSP_I2C_ERR_INVALID_PARAM` - Invalid parameter
- `eBSP_I2C_ERR_INVALID_HANDLE` - Invalid or freed handle
- `eBSP_I2C_ERR_BUSY` - I2C peripheral busy
- `eBSP_I2C_ERR_TIMEOUT` - Blocking operation timed out
- `eBSP_I2C_ERR_TRANSFER` - Transfer error occurred
- `eBSP_I2C_ERR_NO_RESOURCE` - No available I2C slots

## Usage Examples

### Blocking Mode - Simple Read/Write

```c
#include "bsp_i2c.h"

// Allocate I2C1 in blocking mode with 1000ms timeout
BspI2cHandle_t i2c = BspI2cAllocate(eBSP_I2C_INSTANCE_1, eBSP_I2C_MODE_BLOCKING, 1000);

if (i2c >= 0) {
    uint8_t txData[] = {0x01, 0x02, 0x03};

    BspI2cTransferConfig_t config = {
        .devAddr = 0xA0,  // 7-bit address 0x50 shifted left
        .pData   = txData,
        .length  = sizeof(txData)
    };

    // Transmit data to slave
    BspI2cError_e err = BspI2cTransmit(i2c, &config);

    if (err == eBSP_I2C_ERR_NONE) {
        // Success
    }
}
```

### EEPROM Memory Operations

```c
#include "bsp_i2c.h"

#define EEPROM_ADDR 0xA0  // AT24C256 EEPROM

// Allocate I2C2 for EEPROM
BspI2cHandle_t i2c = BspI2cAllocate(eBSP_I2C_INSTANCE_2, eBSP_I2C_MODE_BLOCKING, 1000);

// Write data to EEPROM at address 0x1234
uint8_t writeData[] = {0xAA, 0xBB, 0xCC, 0xDD};
BspI2cMemConfig_t writeConfig = {
    .devAddr     = EEPROM_ADDR,
    .memAddr     = 0x1234,
    .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_16BIT,
    .pData       = writeData,
    .length      = sizeof(writeData)
};

BspI2cError_e err = BspI2cMemWrite(i2c, &writeConfig);

// Read back from EEPROM
uint8_t readData[4];
BspI2cMemConfig_t readConfig = {
    .devAddr     = EEPROM_ADDR,
    .memAddr     = 0x1234,
    .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_16BIT,
    .pData       = readData,
    .length      = sizeof(readData)
};

err = BspI2cMemRead(i2c, &readConfig);

if (err == eBSP_I2C_ERR_NONE) {
    // Verify readData matches writeData
}
```

### Interrupt Mode with Callbacks

```c
#include "bsp_i2c.h"

static BspI2cHandle_t i2c;
static volatile bool txComplete = false;
static volatile bool rxComplete = false;

void i2cTxComplete(BspI2cHandle_t handle) {
    txComplete = true;
}

void i2cRxComplete(BspI2cHandle_t handle) {
    rxComplete = true;
}

void i2cError(BspI2cHandle_t handle, BspI2cError_e error) {
    // Handle error
}

void setupI2C(void) {
    // Allocate I2C3 in interrupt mode
    i2c = BspI2cAllocate(eBSP_I2C_INSTANCE_3, eBSP_I2C_MODE_INTERRUPT, 0);

    if (i2c >= 0) {
        // Register callbacks
        BspI2cRegisterTxCallback(i2c, i2cTxComplete);
        BspI2cRegisterRxCallback(i2c, i2cRxComplete);
        BspI2cRegisterErrorCallback(i2c, i2cError);
    }
}

void sendData(void) {
    uint8_t data[] = {0x10, 0x20, 0x30};

    BspI2cTransferConfig_t config = {
        .devAddr = 0x50,
        .pData   = data,
        .length  = sizeof(data)
    };

    txComplete = false;
    BspI2cError_e err = BspI2cTransmitIT(i2c, &config);

    if (err == eBSP_I2C_ERR_NONE) {
        // Wait for completion in main loop
        while (!txComplete) { /* ... */ }
    }
}
```

### Sensor Reading Example

```c
#include "bsp_i2c.h"

#define BME280_ADDR 0xEC  // Bosch BME280 sensor

// Read temperature/pressure/humidity from BME280
void readBME280(void) {
    BspI2cHandle_t i2c = BspI2cAllocate(eBSP_I2C_INSTANCE_1,
                                        eBSP_I2C_MODE_BLOCKING,
                                        1000);

    // Read 8 bytes starting at register 0xF7
    uint8_t sensorData[8];
    BspI2cMemConfig_t config = {
        .devAddr     = BME280_ADDR,
        .memAddr     = 0xF7,
        .memAddrSize = eBSP_I2C_MEM_ADDR_SIZE_8BIT,
        .pData       = sensorData,
        .length      = 8
    };

    BspI2cError_e err = BspI2cMemRead(i2c, &config);

    if (err == eBSP_I2C_ERR_NONE) {
        // Parse temperature, pressure, humidity
        uint32_t pressure = (sensorData[0] << 12) | (sensorData[1] << 4) | (sensorData[2] >> 4);
        uint32_t temperature = (sensorData[3] << 12) | (sensorData[4] << 4) | (sensorData[5] >> 4);
        uint32_t humidity = (sensorData[6] << 8) | sensorData[7];

        // Apply calibration and process...
    }

    BspI2cFree(i2c);
}
```

## Important Notes

### 7-bit Addressing

I2C slave addresses are **7-bit** values that must be **shifted left by 1** when passed to the API:

```c
// If sensor datasheet says address is 0x50:
config.devAddr = 0xA0;  // 0x50 << 1 = 0xA0

// For read/write bit:
// Hardware automatically sets bit 0 for read (1) or write (0)
```

### HAL Interrupt Handlers

For interrupt mode to work, ensure HAL I2C event and error interrupt handlers are called:

```c
void I2C1_EV_IRQHandler(void) {
    HAL_I2C_EV_IRQHandler(&hi2c1);
}

void I2C1_ER_IRQHandler(void) {
    HAL_I2C_ER_IRQHandler(&hi2c1);
}
```

### Memory Operations Timing

When writing to EEPROM devices, allow time for the write cycle to complete:

```c
BspI2cMemWrite(i2c, &config);
HAL_Delay(5);  // Typical EEPROM write cycle time
```

## Testing

The module includes 79 comprehensive unit tests covering:

- **Allocation/Deallocation** (10 tests)
- **Callback Registration** (10 tests)
- **Blocking Mode Operations** (18 tests)
- **Interrupt Mode Operations** (18 tests)
- **HAL Callbacks** (13 tests)
- **Integration Tests** (4 tests)
- **Edge Cases** (6 tests)

Coverage metrics:
- **93.3% line coverage** (237/254 lines)
- **86.8% branch coverage** (145/167 branches taken)
- **100% function coverage**

Run tests:
```bash
cmake --preset bsp-test-host
cd build/bsp-test-host
ninja test_bsp_i2c
./tests/bsp_i2c/test_bsp_i2c
```

## Implementation Details

### Resource Management

- Maximum 6 concurrent I2C handles
- Handles are recycled on `BspI2cFree()`
- One instance can only be allocated once at a time

### Timeout Behavior

- Blocking mode: Uses HAL timeout mechanism
- Default timeout: 1000ms (if 0 passed to `BspI2cAllocate`)
- Interrupt mode: Timeout parameter ignored

### Error Recovery

All functions return error codes. On error:
1. Check return value
2. Handle appropriately (retry, reset, log, etc.)
3. For interrupt mode, error callback is also invoked

### Thread Safety

**Not thread-safe**. Use mutexes or disable interrupts when accessing from multiple contexts.
