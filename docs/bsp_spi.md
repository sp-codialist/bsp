# BSP SPI Module

High-performance SPI communication driver for STM32F4 microcontrollers.

## Features

- Support for all 6 SPI peripherals (SPI1-SPI6)
- Dual operation modes: Blocking and DMA
- Full-duplex communication
- Configurable timeout for blocking operations
- Callback-based DMA completion notification
- Error handling and reporting
- 98.1% test coverage (75 tests)

## API Reference

### Allocation and Configuration

- `BspSpiAllocate(eInstance, eMode, uTimeoutMs)` - Allocate SPI instance
- `BspSpiFree(handle)` - Free SPI instance

**Instances**: `eBSP_SPI_INSTANCE_1` through `eBSP_SPI_INSTANCE_6`

**Modes**:
- `eBSP_SPI_MODE_BLOCKING` - Timeout-based blocking operations
- `eBSP_SPI_MODE_DMA` - Interrupt-driven DMA operations

### Callback Registration (DMA Mode)

- `BspSpiRegisterTxCallback(handle, callback)` - Transmit complete
- `BspSpiRegisterRxCallback(handle, callback)` - Receive complete
- `BspSpiRegisterTxRxCallback(handle, callback)` - Transmit-receive complete
- `BspSpiRegisterErrorCallback(handle, callback)` - Error notification

### Blocking Operations

- `BspSpiTransmit(handle, pTxData, uLength)` - Send data
- `BspSpiReceive(handle, pRxData, uLength)` - Receive data
- `BspSpiTransmitReceive(handle, pTxData, pRxData, uLength)` - Full-duplex

### DMA Operations

- `BspSpiTransmitDMA(handle, pTxData, uLength)` - Send via DMA
- `BspSpiReceiveDMA(handle, pRxData, uLength)` - Receive via DMA
- `BspSpiTransmitReceiveDMA(handle, pTxData, pRxData, uLength)` - Full-duplex via DMA

## Error Codes

- `eBSP_SPI_ERR_NONE` - No error
- `eBSP_SPI_ERR_INVALID_PARAM` - Invalid parameter
- `eBSP_SPI_ERR_INVALID_HANDLE` - Invalid or freed handle
- `eBSP_SPI_ERR_BUSY` - SPI peripheral busy
- `eBSP_SPI_ERR_TIMEOUT` - Blocking operation timed out
- `eBSP_SPI_ERR_TRANSFER` - Transfer error occurred
- `eBSP_SPI_ERR_NO_RESOURCE` - No available SPI slots

## Usage Examples

### Blocking Mode

```c
#include "bsp_spi.h"

// Allocate SPI2 in blocking mode with 1000ms timeout
BspSpiHandle_t spi = BspSpiAllocate(eBSP_SPI_INSTANCE_2, eBSP_SPI_MODE_BLOCKING, 1000);

if (spi >= 0) {
    uint8_t txData[] = {0x01, 0x02, 0x03};
    uint8_t rxData[3];

    // Control CS manually
    GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);

    // Full-duplex communication
    BspSpiError_e err = BspSpiTransmitReceive(spi, txData, rxData, sizeof(txData));

    GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);

    if (err == eBSP_SPI_ERR_NONE) {
        // Process rxData
    }
}
```

### DMA Mode with Callbacks

```c
#include "bsp_spi.h"

static BspSpiHandle_t spi;

void spiTxRxComplete(BspSpiHandle_t handle) {
    GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
    // Process received data
}

void spiError(BspSpiHandle_t handle, BspSpiError_e error) {
    GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
    // Handle error
}

void setup(void) {
    // Allocate SPI1 in DMA mode
    spi = BspSpiAllocate(eBSP_SPI_INSTANCE_1, eBSP_SPI_MODE_DMA, 0);

    if (spi >= 0) {
        BspSpiRegisterTxRxCallback(spi, spiTxRxComplete);
        BspSpiRegisterErrorCallback(spi, spiError);
    }
}

void sendData(void) {
    static uint8_t txData[] = {0xAA, 0xBB, 0xCC};
    static uint8_t rxData[3];

    GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    BspSpiTransmitReceiveDMA(spi, txData, rxData, sizeof(txData));
    // CS will be set in callback
}
```

### Transmit Only

```c
void sendCommand(uint8_t cmd) {
    BspSpiHandle_t spi = BspSpiAllocate(eBSP_SPI_INSTANCE_3, eBSP_SPI_MODE_BLOCKING, 100);

    GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    BspSpiTransmit(spi, &cmd, 1);
    GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);

    BspSpiFree(spi);
}
```

### Receive Only

```c
void readSensor(uint8_t* buffer, uint32_t length) {
    BspSpiHandle_t spi = BspSpiAllocate(eBSP_SPI_INSTANCE_4, eBSP_SPI_MODE_BLOCKING, 500);

    GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    BspSpiReceive(spi, buffer, length);
    GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);

    BspSpiFree(spi);
}
```

## Important Notes

### Chip Select (CS) Management

The SPI module **does not** automatically control chip select. The caller must:
1. Assert CS (drive low) before SPI operations
2. De-assert CS (drive high) after completion
3. For DMA mode, de-assert CS in the completion callback

### DMA Buffer Lifetime

In DMA mode, TX and RX buffers must remain valid until the completion callback is invoked. Use static or heap-allocated buffers, not stack variables that go out of scope.

### Resource Limits

Each SPI instance can only be allocated once. Attempting to allocate the same instance twice will fail with `eBSP_SPI_ERR_NO_RESOURCE`.

### Timeout Configuration

For blocking mode, timeout of 0 uses a default timeout. The timeout prevents indefinite blocking if the slave device doesn't respond.

## Test Coverage

- **Line Coverage**: 98.1%
- **Function Coverage**: 100%
- **Branch Coverage**: 96.1%
- **Tests**: 75 comprehensive unit tests

Coverage includes:
- All allocation/deallocation scenarios
- Both blocking and DMA modes
- All error conditions
- Callback registration and invocation
- HAL error handling
- Timeout behavior
- Invalid parameter handling

## See Also

- [BSP Common](bsp_common.md) - Common BSP patterns
- [Testing](testing.md) - Unit testing guide
- [BSP GPIO](bsp_gpio.md) - GPIO module for CS control
