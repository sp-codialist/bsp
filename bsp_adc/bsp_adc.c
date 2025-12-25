/*
 * bsp_adc.c
 */

#include "bsp_adc.h"
#include "bsp_compiler_attributes.h"
#include "bsp_swtimer.h"
#include "stm32f4xx_hal.h"

#include <stddef.h>

// --- External HAL Handle Declarations ---

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;

// --- Local Types ---

/**
 * @brief ADC channel instance structure to hold single channel data
 * @note Each instance manages one physical ADC channel independently
 */
typedef struct
{
    ADC_HandleTypeDef* pAdcHandle;        /**< Pointer to HAL ADC handle (ADC1/ADC2/ADC3) */
    BspAdcValueCb_t    pCallback;         /**< Callback function for conversion result */
    uint32_t           uResultData;       /**< ADC conversion result (single value) */
    SWTimerModule      timer;             /**< Independent software timer for periodic sampling */
    BspAdcErrorCb_t    pErrorCallback;    /**< Error callback for DMA errors */
    BspAdcInstance_e   eAdcInstance;      /**< ADC peripheral instance (1/2/3) */
    BspAdcChannel_e    eChannel;          /**< Physical ADC channel number (0-15) */
    bool               bAllocated;        /**< Allocation flag - true if instance is in use */
    bool               bTimerInitialized; /**< Timer initialization flag */
} BspAdcModule_t;

// --- Local Variables ---

FORCE_STATIC BspAdcModule_t s_adcModules[BSP_ADC_MAX_CHANNELS] = {0};

// --- Local Function Prototypes ---

/**
 * @brief Converts ADC channel enum to STM32 HAL ADC channel value.
 * @param eChannel BSP ADC channel enum
 * @param pStmChannel Pointer to store STM32 HAL channel value
 * @return true if conversion successful, false otherwise
 */
FORCE_STATIC bool BspAdcGetStmChannelValue(BspAdcChannel_e eChannel, uint32_t* pStmChannel);

/**
 * @brief Converts ADC sample time enum to STM32 HAL sample time value.
 * @param eSampleTime BSP ADC sample time enum
 * @param pStmSampleTime Pointer to store STM32 HAL sample time value
 * @return true if conversion successful, false otherwise
 */
FORCE_STATIC bool BspAdcGetStmSampleTimeValue(BspAdcSampleTime_e eSampleTime, uint32_t* pStmSampleTime);

/**
 * @brief Starts ADC DMA conversion for a specific channel instance.
 * @param handle Channel handle (0-15)
 */
FORCE_STATIC void BspAdcStartReadDma(BspAdcChannelHandle_t handle);

/**
 * @brief Timer callback wrappers - one per channel instance (0-15)
 * These static functions enable independent timer callbacks without modifying SWTimerModule
 */
FORCE_STATIC void BspAdcTimerCallback0(void);
FORCE_STATIC void BspAdcTimerCallback1(void);
FORCE_STATIC void BspAdcTimerCallback2(void);
FORCE_STATIC void BspAdcTimerCallback3(void);
FORCE_STATIC void BspAdcTimerCallback4(void);
FORCE_STATIC void BspAdcTimerCallback5(void);
FORCE_STATIC void BspAdcTimerCallback6(void);
FORCE_STATIC void BspAdcTimerCallback7(void);
FORCE_STATIC void BspAdcTimerCallback8(void);
FORCE_STATIC void BspAdcTimerCallback9(void);
FORCE_STATIC void BspAdcTimerCallback10(void);
FORCE_STATIC void BspAdcTimerCallback11(void);
FORCE_STATIC void BspAdcTimerCallback12(void);
FORCE_STATIC void BspAdcTimerCallback13(void);
FORCE_STATIC void BspAdcTimerCallback14(void);
FORCE_STATIC void BspAdcTimerCallback15(void);

// --- Local Constants ---

/**
 * @brief Lookup table mapping channel handle to timer callback function
 */
FORCE_STATIC SWTimerCallbackFunction const s_timerCallbacks[BSP_ADC_MAX_CHANNELS] = {
    BspAdcTimerCallback0,  BspAdcTimerCallback1,  BspAdcTimerCallback2,  BspAdcTimerCallback3, BspAdcTimerCallback4,  BspAdcTimerCallback5,
    BspAdcTimerCallback6,  BspAdcTimerCallback7,  BspAdcTimerCallback8,  BspAdcTimerCallback9, BspAdcTimerCallback10, BspAdcTimerCallback11,
    BspAdcTimerCallback12, BspAdcTimerCallback13, BspAdcTimerCallback14, BspAdcTimerCallback15};

// --- Public API Implementation ---

BspAdcChannelHandle_t BspAdcAllocateChannel(BspAdcInstance_e eAdcInstance, BspAdcChannel_e eChannel, BspAdcSampleTime_e eSampleTime,
                                            BspAdcValueCb_t pValueCallback)
{
    BspAdcChannelHandle_t handle          = -1;
    bool                  bDuplicateFound = false;

    do
    {
        // Validate parameters
        if (eAdcInstance >= eBSP_ADC_INSTANCE_COUNT)
        {
            break;
        }

        // Check for duplicate allocation (same ADC instance + same channel)
        for (uint8_t i = 0; i < BSP_ADC_MAX_CHANNELS; i++)
        {
            if (s_adcModules[i].bAllocated && s_adcModules[i].eAdcInstance == eAdcInstance && s_adcModules[i].eChannel == eChannel)
            {
                // Duplicate detected - return error
                bDuplicateFound = true;
                break;
            }
        }

        if (bDuplicateFound) // Duplicate detected, exit
        {
            break;
        }

        // Find first free slot
        for (uint8_t i = 0; i < BSP_ADC_MAX_CHANNELS; i++)
        {
            if (!s_adcModules[i].bAllocated)
            {
                handle = (BspAdcChannelHandle_t)i;
                break;
            }
        }

        if (handle == -1)
        {
            // No free slots
            break;
        }

        // Map ADC instance enum to HAL handle
        FORCE_STATIC ADC_HandleTypeDef* const aAdcHandles[eBSP_ADC_INSTANCE_COUNT] = {
            [eBSP_ADC_INSTANCE_1] = &hadc1, [eBSP_ADC_INSTANCE_2] = &hadc2, [eBSP_ADC_INSTANCE_3] = &hadc3};

        BspAdcModule_t* pModule = &s_adcModules[handle];

        pModule->pAdcHandle = aAdcHandles[eAdcInstance];
        if (pModule->pAdcHandle == NULL)
        {
            handle = -1;
            break;
        }

        // Configure HAL channel (Rank=1 for single-channel DMA)
        uint32_t uStmChannel    = 0u;
        uint32_t uStmSampleTime = 0u;
        bool     result         = BspAdcGetStmChannelValue(eChannel, &uStmChannel);
        result                  = BspAdcGetStmSampleTimeValue(eSampleTime, &uStmSampleTime) && result;

        if (!result)
        {
            handle = -1;
            break;
        }

        ADC_ChannelConfTypeDef sConfig = {0};
        sConfig.Channel                = uStmChannel;
        sConfig.Rank                   = 1u; // Always rank 1 for single-channel mode
        sConfig.SamplingTime           = uStmSampleTime;
        sConfig.Offset                 = 0u;

        HAL_StatusTypeDef status = HAL_ADC_ConfigChannel(pModule->pAdcHandle, &sConfig);
        if (status != HAL_OK)
        {
            handle = -1;
            break;
        }

        // Initialize timer with instance-specific callback
        pModule->timer.interval          = 100u; // Default, will be set by BspAdcStart()
        pModule->timer.pCallbackFunction = s_timerCallbacks[handle];
        pModule->timer.periodic          = true;
        pModule->timer.active            = false;

        pModule->bTimerInitialized = SWTimerInit(&pModule->timer);
        if (!pModule->bTimerInitialized)
        {
            handle = -1;
            break;
        }

        // Store configuration and mark as allocated
        pModule->eAdcInstance = eAdcInstance;
        pModule->eChannel     = eChannel;
        pModule->pCallback    = pValueCallback;
        pModule->bAllocated   = true;

    } while (false);

    return handle;
}

void BspAdcFreeChannel(BspAdcChannelHandle_t handle)
{
    do
    {
        // Validate handle
        if (handle < 0 || (uint8_t)handle >= BSP_ADC_MAX_CHANNELS)
        {
            break;
        }

        BspAdcModule_t* pModule = &s_adcModules[handle];

        if (!pModule->bAllocated)
        {
            break;
        }

        // Stop timer if running
        if (pModule->bTimerInitialized)
        {
            SWTimerStop(&pModule->timer);
        }

        // Reset all fields
        pModule->pAdcHandle        = NULL;
        pModule->pCallback         = NULL;
        pModule->uResultData       = 0;
        pModule->pErrorCallback    = NULL;
        pModule->eAdcInstance      = 0;
        pModule->eChannel          = 0;
        pModule->bAllocated        = false;
        pModule->bTimerInitialized = false;

    } while (false);
}

void BspAdcStart(BspAdcChannelHandle_t handle, uint32_t uPeriodMs)
{
    do
    {
        // Validate handle
        if (handle < 0 || (uint8_t)handle >= BSP_ADC_MAX_CHANNELS)
        {
            break;
        }

        BspAdcModule_t* pModule = &s_adcModules[handle];

        if (!pModule->bAllocated || !pModule->bTimerInitialized)
        {
            break;
        }

        pModule->timer.interval = uPeriodMs;
        SWTimerStart(&pModule->timer);

    } while (false);
}

void BspAdcStop(BspAdcChannelHandle_t handle)
{
    do
    {
        // Validate handle
        if (handle < 0 || (uint8_t)handle >= BSP_ADC_MAX_CHANNELS)
        {
            break;
        }

        BspAdcModule_t* pModule = &s_adcModules[handle];

        if (!pModule->bAllocated)
        {
            break;
        }

        SWTimerStop(&pModule->timer);

    } while (false);
}

void BspAdcRegisterErrorCallback(BspAdcChannelHandle_t handle, BspAdcErrorCb_t pErrCb)
{
    do
    {
        // Validate handle
        if (handle < 0 || (uint8_t)handle >= BSP_ADC_MAX_CHANNELS)
        {
            break;
        }

        BspAdcModule_t* pModule = &s_adcModules[handle];

        if (!pModule->bAllocated)
        {
            break;
        }

        pModule->pErrorCallback = pErrCb;

    } while (false);
}

// --- Test Support Functions ---

/**
 * @brief Reset module state for unit testing
 * @note This function should only be used in unit tests
 */
void BspAdcResetModuleForTest(void)
{
    for (uint8_t i = 0; i < BSP_ADC_MAX_CHANNELS; i++)
    {
        s_adcModules[i].pAdcHandle        = NULL;
        s_adcModules[i].pCallback         = NULL;
        s_adcModules[i].uResultData       = 0;
        s_adcModules[i].pErrorCallback    = NULL;
        s_adcModules[i].eAdcInstance      = 0;
        s_adcModules[i].eChannel          = 0;
        s_adcModules[i].bAllocated        = false;
        s_adcModules[i].bTimerInitialized = false;
    }
}

// --- Local Function Implementation ---

FORCE_STATIC void BspAdcStartReadDma(BspAdcChannelHandle_t handle)
{
    do
    {
        // Validate handle
        if (handle < 0 || (uint8_t)handle >= BSP_ADC_MAX_CHANNELS)
        {
            break;
        }

        BspAdcModule_t* pModule = &s_adcModules[handle];

        if (!pModule->bAllocated || pModule->pAdcHandle == NULL)
        {
            break;
        }

        // Start single-channel DMA conversion
        HAL_StatusTypeDef status = HAL_ADC_Start_DMA(pModule->pAdcHandle, &pModule->uResultData, 1u);

        if (status != HAL_OK)
        {
            if (pModule->pErrorCallback != NULL)
            {
                pModule->pErrorCallback(eBSP_ADC_ERR_CONVERSION);
            }
        }

    } while (false);
}

// Timer callback wrappers - one per instance
FORCE_STATIC void BspAdcTimerCallback0(void)
{
    BspAdcStartReadDma(0);
}
FORCE_STATIC void BspAdcTimerCallback1(void)
{
    BspAdcStartReadDma(1);
}
FORCE_STATIC void BspAdcTimerCallback2(void)
{
    BspAdcStartReadDma(2);
}
FORCE_STATIC void BspAdcTimerCallback3(void)
{
    BspAdcStartReadDma(3);
}
FORCE_STATIC void BspAdcTimerCallback4(void)
{
    BspAdcStartReadDma(4);
}
FORCE_STATIC void BspAdcTimerCallback5(void)
{
    BspAdcStartReadDma(5);
}
FORCE_STATIC void BspAdcTimerCallback6(void)
{
    BspAdcStartReadDma(6);
}
FORCE_STATIC void BspAdcTimerCallback7(void)
{
    BspAdcStartReadDma(7);
}
FORCE_STATIC void BspAdcTimerCallback8(void)
{
    BspAdcStartReadDma(8);
}
FORCE_STATIC void BspAdcTimerCallback9(void)
{
    BspAdcStartReadDma(9);
}
FORCE_STATIC void BspAdcTimerCallback10(void)
{
    BspAdcStartReadDma(10);
}
FORCE_STATIC void BspAdcTimerCallback11(void)
{
    BspAdcStartReadDma(11);
}
FORCE_STATIC void BspAdcTimerCallback12(void)
{
    BspAdcStartReadDma(12);
}
FORCE_STATIC void BspAdcTimerCallback13(void)
{
    BspAdcStartReadDma(13);
}
FORCE_STATIC void BspAdcTimerCallback14(void)
{
    BspAdcStartReadDma(14);
}
FORCE_STATIC void BspAdcTimerCallback15(void)
{
    BspAdcStartReadDma(15);
}

FORCE_STATIC bool BspAdcGetStmChannelValue(BspAdcChannel_e eChannel, uint32_t* pStmChannel)
{
    bool result = true;

    do
    {
        if (pStmChannel == NULL)
        {
            result = false;
            break;
        }

        switch (eChannel)
        {
            case eBSP_ADC_CHANNEL_0:
                *pStmChannel = ADC_CHANNEL_0;
                break;
            case eBSP_ADC_CHANNEL_1:
                *pStmChannel = ADC_CHANNEL_1;
                break;
            case eBSP_ADC_CHANNEL_2:
                *pStmChannel = ADC_CHANNEL_2;
                break;
            case eBSP_ADC_CHANNEL_3:
                *pStmChannel = ADC_CHANNEL_3;
                break;
            case eBSP_ADC_CHANNEL_4:
                *pStmChannel = ADC_CHANNEL_4;
                break;
            case eBSP_ADC_CHANNEL_5:
                *pStmChannel = ADC_CHANNEL_5;
                break;
            case eBSP_ADC_CHANNEL_6:
                *pStmChannel = ADC_CHANNEL_6;
                break;
            case eBSP_ADC_CHANNEL_7:
                *pStmChannel = ADC_CHANNEL_7;
                break;
            case eBSP_ADC_CHANNEL_8:
                *pStmChannel = ADC_CHANNEL_8;
                break;
            case eBSP_ADC_CHANNEL_9:
                *pStmChannel = ADC_CHANNEL_9;
                break;
            case eBSP_ADC_CHANNEL_10:
                *pStmChannel = ADC_CHANNEL_10;
                break;
            case eBSP_ADC_CHANNEL_11:
                *pStmChannel = ADC_CHANNEL_11;
                break;
            case eBSP_ADC_CHANNEL_12:
                *pStmChannel = ADC_CHANNEL_12;
                break;
            case eBSP_ADC_CHANNEL_13:
                *pStmChannel = ADC_CHANNEL_13;
                break;
            case eBSP_ADC_CHANNEL_14:
                *pStmChannel = ADC_CHANNEL_14;
                break;
            case eBSP_ADC_CHANNEL_15:
                *pStmChannel = ADC_CHANNEL_15;
                break;
            default:
                result = false;
                break;
        }

    } while (false);

    return result;
}

FORCE_STATIC bool BspAdcGetStmSampleTimeValue(BspAdcSampleTime_e eSampleTime, uint32_t* pStmSampleTime)
{
    bool result = true;

    do
    {
        if (pStmSampleTime == NULL)
        {
            result = false;
            break;
        }

        switch (eSampleTime)
        {
            case eBSP_ADC_SampleTime_3Cycles:
                *pStmSampleTime = ADC_SAMPLETIME_3CYCLES;
                break;
            case eBSP_ADC_SampleTime_15Cycles:
                *pStmSampleTime = ADC_SAMPLETIME_15CYCLES;
                break;
            case eBSP_ADC_SampleTime_28Cycles:
                *pStmSampleTime = ADC_SAMPLETIME_28CYCLES;
                break;
            case eBSP_ADC_SampleTime_56Cycles:
                *pStmSampleTime = ADC_SAMPLETIME_56CYCLES;
                break;
            case eBSP_ADC_SampleTime_84Cycles:
                *pStmSampleTime = ADC_SAMPLETIME_84CYCLES;
                break;
            case eBSP_ADC_SampleTime_112Cycles:
                *pStmSampleTime = ADC_SAMPLETIME_112CYCLES;
                break;
            case eBSP_ADC_SampleTime_144Cycles:
                *pStmSampleTime = ADC_SAMPLETIME_144CYCLES;
                break;
            case eBSP_ADC_SampleTime_480Cycles:
                *pStmSampleTime = ADC_SAMPLETIME_480CYCLES;
                break;
            default:
                result = false;
                break;
        }

    } while (false);

    return result;
}

// --- HAL Callback Implementation ---

/**
 * @brief HAL ADC conversion complete callback.
 * Called from ADC IRQ when DMA transfer completes.
 * Iterates all allocated instances and invokes callbacks for matching ADC handles.
 * @param hadc ADC handle
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    // Iterate all instances to find matching ADC handle(s)
    for (uint8_t i = 0u; i < BSP_ADC_MAX_CHANNELS; i++)
    {
        BspAdcModule_t* pModule = &s_adcModules[i];

        if (pModule->bAllocated && pModule->pAdcHandle == hadc)
        {
            // Deliver result to user callback
            if (pModule->pCallback != NULL)
            {
                pModule->pCallback((uint16_t)pModule->uResultData);
            }
        }
    }
}
