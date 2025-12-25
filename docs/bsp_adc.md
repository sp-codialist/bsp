# BSP ADC Module

## Overview

The BSP ADC module provides high-level analog-to-digital conversion functionality with DMA-based periodic sampling. It supports multiple independent ADC channels across three ADC peripherals (ADC1, ADC2, ADC3) with individual timers and callbacks for each channel.

**Key Features:**
- Multi-instance support (up to 16 independent channels)
- DMA-based non-blocking conversions
- Periodic sampling with configurable intervals
- Individual software timers per channel
- Callback-based result delivery
- Error handling with error callbacks
- Duplicate channel detection
- 12-bit ADC resolution

## Architecture

### Channel Management

Each ADC channel operates independently with its own:
- **Software timer** (`bsp_swtimer`) for periodic sampling
- **Callback function** to receive conversion results
- **Error callback** for handling DMA/conversion errors
- **Configuration** (ADC instance, channel number, sample time)

### DMA Operation

The module uses DMA for efficient, non-blocking ADC conversions:
1. Timer triggers ADC conversion via `HAL_ADC_Start_DMA()`
2. DMA transfers result to channel-specific buffer
3. Completion interrupt invokes `HAL_ADC_ConvCpltCallback()`
4. User callback receives 12-bit ADC value

### Resource Allocation

The module manages a pool of 16 channel instances. Each instance can be allocated with a unique combination of ADC peripheral and channel number. Duplicate allocations are prevented.

## API Reference

### Types

#### `BspAdcInstance_e`
ADC peripheral instance enumeration:
```c
typedef enum {
    eBSP_ADC_INSTANCE_1 = 0u,    // ADC1
    eBSP_ADC_INSTANCE_2,         // ADC2
    eBSP_ADC_INSTANCE_3,         // ADC3
    eBSP_ADC_INSTANCE_COUNT
} BspAdcInstance_e;
```

#### `BspAdcChannel_e`
Physical ADC channel enumeration (0-15):
```c
typedef enum {
    eBSP_ADC_CHANNEL_0 = 0u,
    eBSP_ADC_CHANNEL_1,
    // ... through eBSP_ADC_CHANNEL_15
} BspAdcChannel_e;
```

#### `BspAdcSampleTime_e`
ADC sampling time configuration:
```c
typedef enum {
    eBSP_ADC_SampleTime_3Cycles = 0u,
    eBSP_ADC_SampleTime_15Cycles,
    eBSP_ADC_SampleTime_28Cycles,
    eBSP_ADC_SampleTime_56Cycles,
    eBSP_ADC_SampleTime_84Cycles,
    eBSP_ADC_SampleTime_112Cycles,
    eBSP_ADC_SampleTime_144Cycles,
    eBSP_ADC_SampleTime_480Cycles
} BspAdcSampleTime_e;
```

**Sample Time Selection Guide:**
- **3-15 cycles**: Fast sampling, lower accuracy (high-impedance sources may not settle)
- **56-84 cycles**: Balanced speed and accuracy for general use
- **144-480 cycles**: High accuracy, slower sampling (low-impedance sources, temperature sensors)

#### `BspAdcError_e`
Error codes:
```c
typedef enum {
    eBSP_ADC_ERR_NONE = 0,           // No error
    eBSP_ADC_ERR_CONFIGURATION,      // Channel configuration mismatch
    eBSP_ADC_ERR_CONVERSION,         // ADC conversion failed
    eBSP_ADC_ERR_INVALID_PARAM       // Invalid parameter
} BspAdcError_e;
```

#### `BspAdcChannelHandle_t`
Channel handle type:
```c
typedef int8_t BspAdcChannelHandle_t;
```
- Valid handles: 0-15
- Invalid handle: -1 (returned on allocation failure)

#### `BspAdcValueCb_t`
Callback function type for receiving ADC results:
```c
typedef void (*BspAdcValueCb_t)(uint16_t wValue);
```
- `wValue`: 12-bit ADC conversion result (0-4095)

#### `BspAdcErrorCb_t`
Callback function type for error handling:
```c
typedef void (*BspAdcErrorCb_t)(BspAdcError_e eError);
```

### Functions

#### `BspAdcAllocateChannel()`
Allocate and initialize an ADC channel instance.

```c
BspAdcChannelHandle_t BspAdcAllocateChannel(
    BspAdcInstance_e eAdcInstance,
    BspAdcChannel_e eChannel,
    BspAdcSampleTime_e eSampleTime,
    BspAdcValueCb_t pValueCallback
);
```

**Parameters:**
- `eAdcInstance`: ADC peripheral (ADC1/ADC2/ADC3)
- `eChannel`: Physical ADC channel (0-15)
- `eSampleTime`: Sample time configuration
- `pValueCallback`: Callback to receive conversion results

**Returns:**
- Channel handle (0-15) on success
- -1 on failure (no free slots, duplicate channel, invalid parameters)

**Behavior:**
- Configures HAL ADC channel with specified sample time
- Initializes dedicated software timer for this channel
- Validates parameters and checks for duplicates
- Marks instance as allocated

**Example:**
```c
BspAdcChannelHandle_t hAdc = BspAdcAllocateChannel(
    eBSP_ADC_INSTANCE_1,
    eBSP_ADC_CHANNEL_3,
    eBSP_ADC_SampleTime_84Cycles,
    myAdcCallback
);

if (hAdc == -1) {
    // Allocation failed
}
```

#### `BspAdcFreeChannel()`
Free an allocated ADC channel instance.

```c
void BspAdcFreeChannel(BspAdcChannelHandle_t handle);
```

**Parameters:**
- `handle`: Channel handle to free (0-15)

**Behavior:**
- Stops the channel's timer if running
- Resets all channel fields
- Releases the instance for reuse
- Validates handle before freeing

**Example:**
```c
BspAdcFreeChannel(hAdc);
```

#### `BspAdcStart()`
Start periodic ADC sampling for a channel.

```c
void BspAdcStart(BspAdcChannelHandle_t handle, uint32_t uPeriodMs);
```

**Parameters:**
- `handle`: Channel handle (0-15)
- `uPeriodMs`: Sampling period in milliseconds

**Behavior:**
- Configures and starts the channel's software timer
- Timer callback triggers DMA conversion at specified interval
- Results delivered via registered value callback

**Example:**
```c
BspAdcStart(hAdc, 100); // Sample every 100ms
```

#### `BspAdcStop()`
Stop ADC sampling for a channel.

```c
void BspAdcStop(BspAdcChannelHandle_t handle);
```

**Parameters:**
- `handle`: Channel handle (0-15)

**Behavior:**
- Stops the channel's software timer
- No further conversions will be triggered
- Channel remains allocated

**Example:**
```c
BspAdcStop(hAdc);
```

#### `BspAdcRegisterErrorCallback()`
Register error callback for a channel.

```c
void BspAdcRegisterErrorCallback(
    BspAdcChannelHandle_t handle,
    BspAdcErrorCb_t pErrCb
);
```

**Parameters:**
- `handle`: Channel handle (0-15)
- `pErrCb`: Callback for error handling

**Behavior:**
- Stores error callback for this channel
- Callback invoked on DMA or conversion errors

**Example:**
```c
void myErrorHandler(BspAdcError_e eError) {
    if (eError == eBSP_ADC_ERR_CONVERSION) {
        // Handle conversion error
    }
}

BspAdcRegisterErrorCallback(hAdc, myErrorHandler);
```

## Usage Examples

### Basic ADC Reading

```c
#include "bsp_adc.h"

static uint16_t s_adcValue = 0;

void adcResultCallback(uint16_t wValue) {
    s_adcValue = wValue;
    // Convert to voltage: voltage = (wValue / 4095.0f) * 3.3f
}

void setup_adc(void) {
    // Allocate ADC1 Channel 3
    BspAdcChannelHandle_t hAdc = BspAdcAllocateChannel(
        eBSP_ADC_INSTANCE_1,
        eBSP_ADC_CHANNEL_3,
        eBSP_ADC_SampleTime_84Cycles,
        adcResultCallback
    );

    if (hAdc != -1) {
        // Start sampling at 50Hz (20ms period)
        BspAdcStart(hAdc, 20);
    }
}
```

### Multiple Independent Channels

```c
#include "bsp_adc.h"

static uint16_t s_temperature = 0;
static uint16_t s_voltage = 0;
static uint16_t s_current = 0;

void tempCallback(uint16_t wValue) {
    s_temperature = wValue;
}

void voltageCallback(uint16_t wValue) {
    s_voltage = wValue;
}

void currentCallback(uint16_t wValue) {
    s_current = wValue;
}

void setup_multi_channel(void) {
    // Temperature sensor on ADC1 Channel 0 - slow sampling
    BspAdcChannelHandle_t hTemp = BspAdcAllocateChannel(
        eBSP_ADC_INSTANCE_1,
        eBSP_ADC_CHANNEL_0,
        eBSP_ADC_SampleTime_480Cycles,  // Long sample time for accuracy
        tempCallback
    );
    BspAdcStart(hTemp, 1000);  // 1Hz sampling

    // Voltage monitor on ADC2 Channel 5 - fast sampling
    BspAdcChannelHandle_t hVolt = BspAdcAllocateChannel(
        eBSP_ADC_INSTANCE_2,
        eBSP_ADC_CHANNEL_5,
        eBSP_ADC_SampleTime_28Cycles,   // Fast sampling
        voltageCallback
    );
    BspAdcStart(hVolt, 10);  // 100Hz sampling

    // Current sensor on ADC3 Channel 12
    BspAdcChannelHandle_t hCurr = BspAdcAllocateChannel(
        eBSP_ADC_INSTANCE_3,
        eBSP_ADC_CHANNEL_12,
        eBSP_ADC_SampleTime_84Cycles,
        currentCallback
    );
    BspAdcStart(hCurr, 50);  // 20Hz sampling
}
```

### With Error Handling

```c
#include "bsp_adc.h"

static BspAdcChannelHandle_t s_hAdc = -1;
static bool s_adcError = false;

void adcCallback(uint16_t wValue) {
    // Process value
    s_adcError = false;
}

void adcErrorHandler(BspAdcError_e eError) {
    s_adcError = true;

    if (eError == eBSP_ADC_ERR_CONVERSION) {
        // Restart ADC
        BspAdcStop(s_hAdc);
        BspAdcStart(s_hAdc, 100);
    }
}

void setup_with_error_handling(void) {
    s_hAdc = BspAdcAllocateChannel(
        eBSP_ADC_INSTANCE_1,
        eBSP_ADC_CHANNEL_8,
        eBSP_ADC_SampleTime_84Cycles,
        adcCallback
    );

    if (s_hAdc != -1) {
        BspAdcRegisterErrorCallback(s_hAdc, adcErrorHandler);
        BspAdcStart(s_hAdc, 100);
    }
}
```

### Dynamic Channel Management

```c
#include "bsp_adc.h"

void dynamic_channel_example(void) {
    BspAdcChannelHandle_t hAdc;

    // Allocate channel
    hAdc = BspAdcAllocateChannel(
        eBSP_ADC_INSTANCE_1,
        eBSP_ADC_CHANNEL_5,
        eBSP_ADC_SampleTime_56Cycles,
        myCallback
    );

    // Use for a while
    BspAdcStart(hAdc, 200);

    // ... do work ...

    // Stop and free when done
    BspAdcStop(hAdc);
    BspAdcFreeChannel(hAdc);

    // Can now reuse the slot
}
```

## Hardware Configuration

### STM32F4 ADC Characteristics
- **Resolution**: 12-bit (0-4095)
- **Reference Voltage**: Typically 3.3V (VDDA)
- **Conversion Time**: 3-480 cycles (depends on sample time)
- **Channels per ADC**: 16 regular channels
- **ADC Instances**: ADC1, ADC2, ADC3

### Channel to Pin Mapping (STM32F429)

| Channel | ADC1 Pin | ADC2 Pin | ADC3 Pin |
|---------|----------|----------|----------|
| 0       | PA0      | PA0      | PA0      |
| 1       | PA1      | PA1      | PA1      |
| 2       | PA2      | PA2      | PA2      |
| 3       | PA3      | PA3      | PA3      |
| 4       | PA4      | PA4      | PF6      |
| 5       | PA5      | PA5      | PF7      |
| 6       | PA6      | PA6      | PF8      |
| 7       | PA7      | PA7      | PF9      |
| 8       | PB0      | PB0      | PF10     |
| 9       | PB1      | PB1      | PF3      |
| 10      | PC0      | PC0      | PC0      |
| 11      | PC1      | PC1      | PC1      |
| 12      | PC2      | PC2      | PC2      |
| 13      | PC3      | PC3      | PC3      |
| 14      | PC4      | PC4      | PF4      |
| 15      | PC5      | PC5      | PF5      |

**Note**: Verify pin assignments for your specific STM32F4 variant. Above mapping is for STM32F429.

## Implementation Details

### Internal Structure

```c
typedef struct {
    ADC_HandleTypeDef* pAdcHandle;        // HAL ADC handle
    BspAdcValueCb_t    pCallback;         // Value callback
    uint32_t           uResultData;       // DMA result buffer
    SWTimerModule      timer;             // Independent timer
    BspAdcErrorCb_t    pErrorCallback;    // Error callback
    BspAdcInstance_e   eAdcInstance;      // ADC instance
    BspAdcChannel_e    eChannel;          // Channel number
    bool               bAllocated;        // Allocation flag
    bool               bTimerInitialized; // Timer state
} BspAdcModule_t;
```

### DMA Conversion Flow

1. **Timer Expiration**: Software timer callback invoked
2. **DMA Start**: `HAL_ADC_Start_DMA()` initiated
3. **Hardware Conversion**: ADC performs conversion
4. **DMA Transfer**: Result transferred to `uResultData`
5. **Interrupt**: DMA completion triggers `HAL_ADC_ConvCpltCallback()`
6. **User Callback**: Module delivers result to registered callback

### Timer Callback Architecture

Each channel instance has a dedicated timer callback function (`BspAdcTimerCallback0` through `BspAdcTimerCallback15`). This design:
- Avoids dynamic function pointer registration
- Provides compile-time static callbacks
- Enables efficient timer-to-channel mapping

### Error Handling

Errors are reported through the error callback in these scenarios:
- `eBSP_ADC_ERR_CONVERSION`: `HAL_ADC_Start_DMA()` fails
- User responsibility: Re-initialization, logging, or recovery

## Dependencies

### Required Modules
- **bsp_swtimer**: Provides periodic timer functionality
- **bsp_common**: Compiler attributes (FORCE_STATIC)
- **STM32 HAL**: ADC and DMA drivers

### External HAL Handles
The module requires external declarations of ADC HAL handles:
```c
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;
```

These must be initialized by your main application or HAL initialization code (e.g., CubeMX generated).

## Testing

### FORCE_STATIC Pattern
The module uses `FORCE_STATIC` for internal functions to enable unit testing:
- `BspAdcStartReadDma()`: DMA conversion starter
- `BspAdcTimerCallback0-15()`: Timer callbacks
- `BspAdcGetStmChannelValue()`: Channel enum conversion
- `BspAdcGetStmSampleTimeValue()`: Sample time conversion

### Test Helper
```c
void BspAdcResetModuleForTest(void);
```
Resets all channel instances - used in test `setUp()`.

### Coverage
Current test coverage:
- **Line coverage**: 96% (245/256 lines)
- **Function coverage**: 100% (26/26 functions)
- **Branch coverage**: 88% (97/110 branches)
- Unit tests cover allocation, start/stop, callbacks, error handling
- All 16 channel instances tested
- Duplicate detection verified
- HAL interaction validated via CMock

## Best Practices

### 1. Handle Validation
Always check allocation results:
```c
BspAdcChannelHandle_t hAdc = BspAdcAllocateChannel(...);
if (hAdc == -1) {
    // Handle error - no free slots or duplicate
}
```

### 2. Sample Time Selection
- **High-impedance sources**: Use longer sample times (≥84 cycles)
- **Low-impedance sources**: Shorter times acceptable (28-56 cycles)
- **Temperature sensors**: Use 480 cycles for accuracy

### 3. Sampling Rate
- Maximum practical rate: ~1kHz (1ms period) per channel
- Consider ADC conversion time + DMA overhead
- Multiple channels on same ADC share conversion time

### 4. Error Recovery
Implement error callbacks for production systems:
```c
void errorHandler(BspAdcError_e eError) {
    if (eError == eBSP_ADC_ERR_CONVERSION) {
        // Log error, attempt restart, or notify user
    }
}
```

### 5. Resource Management
Free unused channels to prevent resource exhaustion:
```c
BspAdcStop(hAdc);
BspAdcFreeChannel(hAdc);
```

### 6. Multi-ADC Considerations
- Same channel number can be used across different ADC instances
- Each ADC instance operates independently
- Distribution of channels across ADCs improves performance

## Troubleshooting

### Allocation Fails (Returns -1)
- **No free slots**: Maximum 16 channels allocated. Free unused channels.
- **Duplicate**: Same ADC instance + channel already allocated.
- **Invalid ADC instance**: Check `eAdcInstance` parameter.
- **HAL handle NULL**: Verify external `hadc1/2/3` are initialized.

### No Callback Invoked
- Verify HAL ADC and DMA are configured (CubeMX or manual)
- Check DMA interrupt is enabled
- Ensure `HAL_ADC_ConvCpltCallback()` is not overridden elsewhere
- Verify timer is started with `BspAdcStart()`

### Incorrect Values
- Check reference voltage (VDDA)
- Verify GPIO pin configuration (analog mode)
- Increase sample time for high-impedance sources
- Check for electrical noise or grounding issues

### High CPU Usage
- Reduce sampling rates (increase period)
- Distribute channels across multiple ADC instances
- Verify DMA is enabled (not using polling mode)

## Configuration Parameters

### Module Constants
```c
#define BSP_ADC_MAX_CHANNELS 16u  // Maximum channel instances
```

To support more than 16 channels, increase `BSP_ADC_MAX_CHANNELS` and add corresponding timer callback functions.

## Performance Characteristics

### Memory Usage
- **Per channel**: ~40 bytes (BspAdcModule_t structure)
- **Total**: ~640 bytes for 16 channels + code

### Timing
- **Allocation**: ~100-500 CPU cycles (HAL_ADC_ConfigChannel)
- **Start**: ~50 CPU cycles (timer start)
- **Callback overhead**: ~100 CPU cycles per conversion

### Conversion Rates
- **Minimum period**: 1ms (1kHz per channel)
- **Practical limit**: Depends on sample time, DMA overhead, CPU load
- **Example**: 84 cycles @ 84MHz ≈ 1μs + DMA overhead

## See Also

- [Software Timer Module](bsp_swtimer.md) - Timer implementation
- [Common Utilities](bsp_common.md) - FORCE_STATIC pattern
- [Testing Guide](testing.md) - Unit testing strategies
- [STM32F4 ADC Reference Manual](https://www.st.com/resource/en/reference_manual/rm0090-stm32f405415-stm32f407417-stm32f427437-and-stm32f429439-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) - Hardware details

---

**Module**: bsp_adc
**Dependencies**: bsp_swtimer, bsp_common, STM32 HAL
**Testability**: FORCE_STATIC pattern enabled
