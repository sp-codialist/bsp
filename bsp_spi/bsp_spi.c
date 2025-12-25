/*
 * bsp_spi.c
 *
 *  Created on: Jul 4, 2024
 *      Author: IlicAleksander
 */

#include "bsp_spi.h"
#include "stm32f4xx_hal.h"

/* --- Constants --- */

/** Maximum number of SPI instances supported */
#define BSP_SPI_MAX_INSTANCES (6u)

/** Default timeout for blocking mode operations (milliseconds) */
#define BSP_SPI_DEFAULT_TIMEOUT_MS (1000u)

/** Invalid handle value */
#define BSP_SPI_INVALID_HANDLE (-1)

/* --- Private Types --- */

/**
 * SPI module structure.
 * Contains the state and configuration for each allocated SPI instance.
 */
typedef struct
{
    BspSpiInstance_e   eInstance;  /**< SPI peripheral instance */
    SPI_HandleTypeDef* pHalHandle; /**< Pointer to HAL SPI handle */
    BspSpiMode_e       eMode;      /**< Operation mode (blocking or DMA) */
    uint32_t           uTimeoutMs; /**< Timeout for blocking mode operations */
    bool               bAllocated; /**< Allocation status flag */

    /* Callbacks for DMA mode */
    BspSpiTxCpltCb_t   pTxCpltCb;   /**< Transmit completion callback */
    BspSpiRxCpltCb_t   pRxCpltCb;   /**< Receive completion callback */
    BspSpiTxRxCpltCb_t pTxRxCpltCb; /**< Transmit-receive completion callback */
    BspSpiErrorCb_t    pErrorCb;    /**< Error callback */
} BspSpiModule_t;

/* --- Private Variables --- */

/** Array of SPI module instances */
static BspSpiModule_t s_spiModules[BSP_SPI_MAX_INSTANCES] = {0};

/* --- External HAL Handles --- */

extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern SPI_HandleTypeDef hspi3;
extern SPI_HandleTypeDef hspi4;
extern SPI_HandleTypeDef hspi5;
extern SPI_HandleTypeDef hspi6;

/* --- Private Function Prototypes --- */

/**
 * Gets the HAL handle for a given SPI instance.
 *
 * @param eInstance The SPI instance
 * @return Pointer to the HAL handle, or NULL if invalid
 */
static SPI_HandleTypeDef* sBspSpiGetHalHandle(BspSpiInstance_e eInstance);

/**
 * Finds the module index for a given handle.
 *
 * @param handle The SPI handle
 * @return Module index, or BSP_SPI_MAX_INSTANCES if invalid
 */
static uint8_t sBspSpiFindModuleIndex(BspSpiHandle_t handle);

/**
 * Validates a handle and returns the module pointer.
 *
 * @param handle The SPI handle to validate
 * @return Pointer to the module, or NULL if invalid
 */
static BspSpiModule_t* sBspSpiValidateHandle(BspSpiHandle_t handle);

/**
 * Finds a module by HAL handle (used in HAL callbacks).
 *
 * @param pHalHandle Pointer to the HAL SPI handle
 * @return Pointer to the module, or NULL if not found
 */
static BspSpiModule_t* sBspSpiFindModuleByHalHandle(SPI_HandleTypeDef* pHalHandle);

/* --- Private Helper Functions --- */

static SPI_HandleTypeDef* sBspSpiGetHalHandle(BspSpiInstance_e eInstance)
{
    SPI_HandleTypeDef* pHandle = NULL;

    switch (eInstance)
    {
        case eBSP_SPI_INSTANCE_1:
            pHandle = &hspi1;
            break;
        case eBSP_SPI_INSTANCE_2:
            pHandle = &hspi2;
            break;
        case eBSP_SPI_INSTANCE_3:
            pHandle = &hspi3;
            break;
        case eBSP_SPI_INSTANCE_4:
            pHandle = &hspi4;
            break;
        case eBSP_SPI_INSTANCE_5:
            pHandle = &hspi5;
            break;
        case eBSP_SPI_INSTANCE_6:
            pHandle = &hspi6;
            break;
        default:
            break;
    }

    return pHandle;
}

static uint8_t sBspSpiFindModuleIndex(BspSpiHandle_t handle)
{
    if ((handle < 0) || (handle >= BSP_SPI_MAX_INSTANCES))
    {
        return BSP_SPI_MAX_INSTANCES;
    }
    return (uint8_t)handle;
}

static BspSpiModule_t* sBspSpiValidateHandle(BspSpiHandle_t handle)
{
    uint8_t index = sBspSpiFindModuleIndex(handle);

    if (index >= BSP_SPI_MAX_INSTANCES)
    {
        return NULL;
    }

    if (!s_spiModules[index].bAllocated)
    {
        return NULL;
    }

    return &s_spiModules[index];
}

static BspSpiModule_t* sBspSpiFindModuleByHalHandle(SPI_HandleTypeDef* pHalHandle)
{
    if (pHalHandle == NULL)
    {
        return NULL;
    }

    for (uint8_t i = 0u; i < BSP_SPI_MAX_INSTANCES; i++)
    {
        if (s_spiModules[i].bAllocated && (s_spiModules[i].pHalHandle == pHalHandle))
        {
            return &s_spiModules[i];
        }
    }

    return NULL;
}

/* --- Public Functions --- */

BspSpiHandle_t BspSpiAllocate(BspSpiInstance_e eInstance, BspSpiMode_e eMode, uint32_t uTimeoutMs)
{
    /* Validate parameters */
    if (eInstance >= eBSP_SPI_INSTANCE_COUNT)
    {
        return BSP_SPI_INVALID_HANDLE;
    }

    /* Get HAL handle for this instance */
    SPI_HandleTypeDef* pHalHandle = sBspSpiGetHalHandle(eInstance);
    if (pHalHandle == NULL)
    {
        return BSP_SPI_INVALID_HANDLE;
    }

    /* Check if this instance is already allocated */
    for (uint8_t i = 0u; i < BSP_SPI_MAX_INSTANCES; i++)
    {
        if (s_spiModules[i].bAllocated && (s_spiModules[i].eInstance == eInstance))
        {
            return BSP_SPI_INVALID_HANDLE;
        }
    }

    /* Find a free slot */
    for (uint8_t i = 0u; i < BSP_SPI_MAX_INSTANCES; i++)
    {
        if (!s_spiModules[i].bAllocated)
        {
            /* Initialize the module */
            s_spiModules[i].eInstance   = eInstance;
            s_spiModules[i].pHalHandle  = pHalHandle;
            s_spiModules[i].eMode       = eMode;
            s_spiModules[i].uTimeoutMs  = (uTimeoutMs == 0u) ? BSP_SPI_DEFAULT_TIMEOUT_MS : uTimeoutMs;
            s_spiModules[i].bAllocated  = true;
            s_spiModules[i].pTxCpltCb   = NULL;
            s_spiModules[i].pRxCpltCb   = NULL;
            s_spiModules[i].pTxRxCpltCb = NULL;
            s_spiModules[i].pErrorCb    = NULL;

            return (BspSpiHandle_t)i;
        }
    }

    return BSP_SPI_INVALID_HANDLE;
}

BspSpiError_e BspSpiFree(BspSpiHandle_t handle)
{
    BspSpiModule_t* pModule = sBspSpiValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_SPI_ERR_INVALID_HANDLE;
    }

    /* Clear the module */
    pModule->bAllocated  = false;
    pModule->eInstance   = eBSP_SPI_INSTANCE_1;
    pModule->pHalHandle  = NULL;
    pModule->pTxCpltCb   = NULL;
    pModule->pRxCpltCb   = NULL;
    pModule->pTxRxCpltCb = NULL;
    pModule->pErrorCb    = NULL;

    return eBSP_SPI_ERR_NONE;
}

BspSpiError_e BspSpiRegisterTxCallback(BspSpiHandle_t handle, BspSpiTxCpltCb_t pCb)
{
    BspSpiModule_t* pModule = sBspSpiValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_SPI_ERR_INVALID_HANDLE;
    }

    pModule->pTxCpltCb = pCb;
    return eBSP_SPI_ERR_NONE;
}

BspSpiError_e BspSpiRegisterRxCallback(BspSpiHandle_t handle, BspSpiRxCpltCb_t pCb)
{
    BspSpiModule_t* pModule = sBspSpiValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_SPI_ERR_INVALID_HANDLE;
    }

    pModule->pRxCpltCb = pCb;
    return eBSP_SPI_ERR_NONE;
}

BspSpiError_e BspSpiRegisterTxRxCallback(BspSpiHandle_t handle, BspSpiTxRxCpltCb_t pCb)
{
    BspSpiModule_t* pModule = sBspSpiValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_SPI_ERR_INVALID_HANDLE;
    }

    pModule->pTxRxCpltCb = pCb;
    return eBSP_SPI_ERR_NONE;
}

BspSpiError_e BspSpiRegisterErrorCallback(BspSpiHandle_t handle, BspSpiErrorCb_t pCb)
{
    BspSpiModule_t* pModule = sBspSpiValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_SPI_ERR_INVALID_HANDLE;
    }

    pModule->pErrorCb = pCb;
    return eBSP_SPI_ERR_NONE;
}

/* --- Blocking Mode Functions --- */

BspSpiError_e BspSpiTransmit(BspSpiHandle_t handle, const uint8_t* pTxData, uint32_t uLength)
{
    BspSpiModule_t* pModule = sBspSpiValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_SPI_ERR_INVALID_HANDLE;
    }

    if (pTxData == NULL)
    {
        return eBSP_SPI_ERR_INVALID_PARAM;
    }

    if (pModule->eMode != eBSP_SPI_MODE_BLOCKING)
    {
        return eBSP_SPI_ERR_INVALID_PARAM;
    }

    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(pModule->pHalHandle, (uint8_t*)pTxData, (uint16_t)uLength, pModule->uTimeoutMs);

    if (halStatus == HAL_TIMEOUT)
    {
        return eBSP_SPI_ERR_TIMEOUT;
    }
    else if (halStatus != HAL_OK)
    {
        return eBSP_SPI_ERR_TRANSFER;
    }

    return eBSP_SPI_ERR_NONE;
}

BspSpiError_e BspSpiReceive(BspSpiHandle_t handle, uint8_t* pRxData, uint32_t uLength)
{
    BspSpiModule_t* pModule = sBspSpiValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_SPI_ERR_INVALID_HANDLE;
    }

    if (pRxData == NULL)
    {
        return eBSP_SPI_ERR_INVALID_PARAM;
    }

    if (pModule->eMode != eBSP_SPI_MODE_BLOCKING)
    {
        return eBSP_SPI_ERR_INVALID_PARAM;
    }

    HAL_StatusTypeDef halStatus = HAL_SPI_Receive(pModule->pHalHandle, pRxData, (uint16_t)uLength, pModule->uTimeoutMs);

    if (halStatus == HAL_TIMEOUT)
    {
        return eBSP_SPI_ERR_TIMEOUT;
    }
    else if (halStatus != HAL_OK)
    {
        return eBSP_SPI_ERR_TRANSFER;
    }

    return eBSP_SPI_ERR_NONE;
}

BspSpiError_e BspSpiTransmitReceive(BspSpiHandle_t handle, const uint8_t* pTxData, uint8_t* pRxData, uint32_t uLength)
{
    BspSpiModule_t* pModule = sBspSpiValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_SPI_ERR_INVALID_HANDLE;
    }

    if ((pTxData == NULL) || (pRxData == NULL))
    {
        return eBSP_SPI_ERR_INVALID_PARAM;
    }

    if (pModule->eMode != eBSP_SPI_MODE_BLOCKING)
    {
        return eBSP_SPI_ERR_INVALID_PARAM;
    }

    HAL_StatusTypeDef halStatus =
        HAL_SPI_TransmitReceive(pModule->pHalHandle, (uint8_t*)pTxData, pRxData, (uint16_t)uLength, pModule->uTimeoutMs);

    if (halStatus == HAL_TIMEOUT)
    {
        return eBSP_SPI_ERR_TIMEOUT;
    }
    else if (halStatus != HAL_OK)
    {
        return eBSP_SPI_ERR_TRANSFER;
    }

    return eBSP_SPI_ERR_NONE;
}

/* --- DMA Mode Functions --- */

BspSpiError_e BspSpiTransmitDMA(BspSpiHandle_t handle, const uint8_t* pTxData, uint32_t uLength)
{
    BspSpiModule_t* pModule = sBspSpiValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_SPI_ERR_INVALID_HANDLE;
    }

    if (pTxData == NULL)
    {
        return eBSP_SPI_ERR_INVALID_PARAM;
    }

    if (pModule->eMode != eBSP_SPI_MODE_DMA)
    {
        return eBSP_SPI_ERR_INVALID_PARAM;
    }

    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit_DMA(pModule->pHalHandle, (uint8_t*)pTxData, (uint16_t)uLength);

    if (halStatus == HAL_BUSY)
    {
        return eBSP_SPI_ERR_BUSY;
    }
    else if (halStatus != HAL_OK)
    {
        return eBSP_SPI_ERR_TRANSFER;
    }

    return eBSP_SPI_ERR_NONE;
}

BspSpiError_e BspSpiReceiveDMA(BspSpiHandle_t handle, uint8_t* pRxData, uint32_t uLength)
{
    BspSpiModule_t* pModule = sBspSpiValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_SPI_ERR_INVALID_HANDLE;
    }

    if (pRxData == NULL)
    {
        return eBSP_SPI_ERR_INVALID_PARAM;
    }

    if (pModule->eMode != eBSP_SPI_MODE_DMA)
    {
        return eBSP_SPI_ERR_INVALID_PARAM;
    }

    HAL_StatusTypeDef halStatus = HAL_SPI_Receive_DMA(pModule->pHalHandle, pRxData, (uint16_t)uLength);

    if (halStatus == HAL_BUSY)
    {
        return eBSP_SPI_ERR_BUSY;
    }
    else if (halStatus != HAL_OK)
    {
        return eBSP_SPI_ERR_TRANSFER;
    }

    return eBSP_SPI_ERR_NONE;
}

BspSpiError_e BspSpiTransmitReceiveDMA(BspSpiHandle_t handle, const uint8_t* pTxData, uint8_t* pRxData, uint32_t uLength)
{
    BspSpiModule_t* pModule = sBspSpiValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_SPI_ERR_INVALID_HANDLE;
    }

    if ((pTxData == NULL) || (pRxData == NULL))
    {
        return eBSP_SPI_ERR_INVALID_PARAM;
    }

    if (pModule->eMode != eBSP_SPI_MODE_DMA)
    {
        return eBSP_SPI_ERR_INVALID_PARAM;
    }

    HAL_StatusTypeDef halStatus = HAL_SPI_TransmitReceive_DMA(pModule->pHalHandle, (uint8_t*)pTxData, pRxData, (uint16_t)uLength);

    if (halStatus == HAL_BUSY)
    {
        return eBSP_SPI_ERR_BUSY;
    }
    else if (halStatus != HAL_OK)
    {
        return eBSP_SPI_ERR_TRANSFER;
    }

    return eBSP_SPI_ERR_NONE;
}

/* --- HAL Callback Functions --- */

// lint -e818
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi)
{
    BspSpiModule_t* pModule = sBspSpiFindModuleByHalHandle(hspi);

    if ((pModule != NULL) && (pModule->pTxCpltCb != NULL))
    {
        BspSpiHandle_t handle = (BspSpiHandle_t)(pModule - s_spiModules);
        pModule->pTxCpltCb(handle);
    }
}

// lint -e818
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef* hspi)
{
    BspSpiModule_t* pModule = sBspSpiFindModuleByHalHandle(hspi);

    if ((pModule != NULL) && (pModule->pRxCpltCb != NULL))
    {
        BspSpiHandle_t handle = (BspSpiHandle_t)(pModule - s_spiModules);
        pModule->pRxCpltCb(handle);
    }
}

// lint -e818
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi)
{
    BspSpiModule_t* pModule = sBspSpiFindModuleByHalHandle(hspi);

    if ((pModule != NULL) && (pModule->pTxRxCpltCb != NULL))
    {
        BspSpiHandle_t handle = (BspSpiHandle_t)(pModule - s_spiModules);
        pModule->pTxRxCpltCb(handle);
    }
}

// lint -e818
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef* hspi)
{
    BspSpiModule_t* pModule = sBspSpiFindModuleByHalHandle(hspi);

    if ((pModule != NULL) && (pModule->pErrorCb != NULL))
    {
        BspSpiHandle_t handle = (BspSpiHandle_t)(pModule - s_spiModules);
        pModule->pErrorCb(handle, eBSP_SPI_ERR_TRANSFER);
    }
}
