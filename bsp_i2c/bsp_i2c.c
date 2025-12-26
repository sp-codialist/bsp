/*
 * bsp_i2c.c
 */

#include "bsp_i2c.h"
#include "bsp_compiler_attributes.h"
#include "stm32f4xx_hal.h"

/* --- Constants --- */

/** Maximum number of I2C instances supported */
#define BSP_I2C_MAX_INSTANCES (6u)

/** Default timeout for blocking mode operations (milliseconds) */
#define BSP_I2C_DEFAULT_TIMEOUT_MS (1000u)

/** Invalid handle value */
#define BSP_I2C_INVALID_HANDLE (-1)

/* --- Private Types --- */

/**
 * I2C module structure.
 * Contains the state and configuration for each allocated I2C instance.
 */
typedef struct
{
    BspI2cInstance_e   eInstance;  /**< I2C peripheral instance */
    I2C_HandleTypeDef* pHalHandle; /**< Pointer to HAL I2C handle */
    BspI2cMode_e       eMode;      /**< Operation mode (blocking or interrupt) */
    uint32_t           uTimeoutMs; /**< Timeout for blocking mode operations */
    bool               bAllocated; /**< Allocation status flag */

    /* Callbacks for interrupt mode */
    BspI2cTxCpltCb_t    pTxCpltCb;    /**< Transmit completion callback */
    BspI2cRxCpltCb_t    pRxCpltCb;    /**< Receive completion callback */
    BspI2cMemTxCpltCb_t pMemTxCpltCb; /**< Memory transmit completion callback */
    BspI2cMemRxCpltCb_t pMemRxCpltCb; /**< Memory receive completion callback */
    BspI2cErrorCb_t     pErrorCb;     /**< Error callback */
} BspI2cModule_t;

/* --- Private Variables --- */

/** Array of I2C module instances */
FORCE_STATIC BspI2cModule_t s_i2cModules[BSP_I2C_MAX_INSTANCES] = {0};

/* --- External HAL Handles --- */

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c3;
extern I2C_HandleTypeDef hi2c4;
extern I2C_HandleTypeDef hi2c5;
extern I2C_HandleTypeDef hi2c6;

/* --- Private Function Prototypes --- */

/**
 * Gets the HAL handle for a given I2C instance.
 *
 * @param eInstance The I2C instance
 * @return Pointer to the HAL handle, or NULL if invalid
 */
FORCE_STATIC I2C_HandleTypeDef* sBspI2cGetHalHandle(BspI2cInstance_e eInstance);

/**
 * Finds the module index for a given handle.
 *
 * @param handle The I2C handle
 * @return Module index, or BSP_I2C_MAX_INSTANCES if invalid
 */
FORCE_STATIC uint8_t sBspI2cFindModuleIndex(BspI2cHandle_t handle);

/**
 * Validates a handle and returns the module pointer.
 *
 * @param handle The I2C handle to validate
 * @return Pointer to the module, or NULL if invalid
 */
FORCE_STATIC BspI2cModule_t* sBspI2cValidateHandle(BspI2cHandle_t handle);

/**
 * Finds a module by HAL handle (used in HAL callbacks).
 *
 * @param pHalHandle Pointer to the HAL I2C handle
 * @return Pointer to the module, or NULL if not found
 */
FORCE_STATIC BspI2cModule_t* sBspI2cFindModuleByHalHandle(I2C_HandleTypeDef* pHalHandle);

/* --- Private Helper Functions --- */

FORCE_STATIC I2C_HandleTypeDef* sBspI2cGetHalHandle(BspI2cInstance_e eInstance)
{
    I2C_HandleTypeDef* pHandle = NULL;

    switch (eInstance)
    {
        case eBSP_I2C_INSTANCE_1:
            pHandle = &hi2c1;
            break;
        case eBSP_I2C_INSTANCE_2:
            pHandle = &hi2c2;
            break;
        case eBSP_I2C_INSTANCE_3:
            pHandle = &hi2c3;
            break;
        case eBSP_I2C_INSTANCE_4:
            pHandle = &hi2c4;
            break;
        case eBSP_I2C_INSTANCE_5:
            pHandle = &hi2c5;
            break;
        case eBSP_I2C_INSTANCE_6:
            pHandle = &hi2c6;
            break;
        default:
            break;
    }

    return pHandle;
}

FORCE_STATIC uint8_t sBspI2cFindModuleIndex(BspI2cHandle_t handle)
{
    if ((handle < 0) || (handle >= (int8_t)BSP_I2C_MAX_INSTANCES))
    {
        return BSP_I2C_MAX_INSTANCES;
    }
    return (uint8_t)handle;
}

FORCE_STATIC BspI2cModule_t* sBspI2cValidateHandle(BspI2cHandle_t handle)
{
    uint8_t index = sBspI2cFindModuleIndex(handle);

    if (index >= BSP_I2C_MAX_INSTANCES)
    {
        return NULL;
    }

    if (!s_i2cModules[index].bAllocated)
    {
        return NULL;
    }

    return &s_i2cModules[index];
}

FORCE_STATIC BspI2cModule_t* sBspI2cFindModuleByHalHandle(I2C_HandleTypeDef* pHalHandle)
{
    if (pHalHandle == NULL)
    {
        return NULL;
    }

    for (uint8_t i = 0u; i < BSP_I2C_MAX_INSTANCES; i++)
    {
        if (s_i2cModules[i].bAllocated && (s_i2cModules[i].pHalHandle == pHalHandle))
        {
            return &s_i2cModules[i];
        }
    }

    return NULL;
}

/* --- Public Functions --- */

BspI2cHandle_t BspI2cAllocate(BspI2cInstance_e eInstance, BspI2cMode_e eMode, uint32_t uTimeoutMs)
{
    /* Validate parameters */
    if (eInstance >= eBSP_I2C_INSTANCE_COUNT)
    {
        return BSP_I2C_INVALID_HANDLE;
    }

    /* Get HAL handle for this instance */
    I2C_HandleTypeDef* pHalHandle = sBspI2cGetHalHandle(eInstance);
    if (pHalHandle == NULL)
    {
        return BSP_I2C_INVALID_HANDLE;
    }

    /* Check if this instance is already allocated */
    for (uint8_t i = 0u; i < BSP_I2C_MAX_INSTANCES; i++)
    {
        if (s_i2cModules[i].bAllocated && (s_i2cModules[i].eInstance == eInstance))
        {
            return BSP_I2C_INVALID_HANDLE;
        }
    }

    /* Find a free slot */
    for (uint8_t i = 0u; i < BSP_I2C_MAX_INSTANCES; i++)
    {
        if (!s_i2cModules[i].bAllocated)
        {
            /* Initialize the module */
            s_i2cModules[i].eInstance    = eInstance;
            s_i2cModules[i].pHalHandle   = pHalHandle;
            s_i2cModules[i].eMode        = eMode;
            s_i2cModules[i].uTimeoutMs   = (uTimeoutMs == 0u) ? BSP_I2C_DEFAULT_TIMEOUT_MS : uTimeoutMs;
            s_i2cModules[i].bAllocated   = true;
            s_i2cModules[i].pTxCpltCb    = NULL;
            s_i2cModules[i].pRxCpltCb    = NULL;
            s_i2cModules[i].pMemTxCpltCb = NULL;
            s_i2cModules[i].pMemRxCpltCb = NULL;
            s_i2cModules[i].pErrorCb     = NULL;

            return (BspI2cHandle_t)i;
        }
    }

    return BSP_I2C_INVALID_HANDLE;
}

BspI2cError_e BspI2cFree(BspI2cHandle_t handle)
{
    BspI2cModule_t* pModule = sBspI2cValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_I2C_ERR_INVALID_HANDLE;
    }

    /* Clear the module */
    pModule->bAllocated   = false;
    pModule->eInstance    = eBSP_I2C_INSTANCE_1;
    pModule->pHalHandle   = NULL;
    pModule->pTxCpltCb    = NULL;
    pModule->pRxCpltCb    = NULL;
    pModule->pMemTxCpltCb = NULL;
    pModule->pMemRxCpltCb = NULL;
    pModule->pErrorCb     = NULL;

    return eBSP_I2C_ERR_NONE;
}

BspI2cError_e BspI2cRegisterTxCallback(BspI2cHandle_t handle, BspI2cTxCpltCb_t pCb)
{
    BspI2cModule_t* pModule = sBspI2cValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_I2C_ERR_INVALID_HANDLE;
    }

    pModule->pTxCpltCb = pCb;
    return eBSP_I2C_ERR_NONE;
}

BspI2cError_e BspI2cRegisterRxCallback(BspI2cHandle_t handle, BspI2cRxCpltCb_t pCb)
{
    BspI2cModule_t* pModule = sBspI2cValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_I2C_ERR_INVALID_HANDLE;
    }

    pModule->pRxCpltCb = pCb;
    return eBSP_I2C_ERR_NONE;
}

BspI2cError_e BspI2cRegisterMemTxCallback(BspI2cHandle_t handle, BspI2cMemTxCpltCb_t pCb)
{
    BspI2cModule_t* pModule = sBspI2cValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_I2C_ERR_INVALID_HANDLE;
    }

    pModule->pMemTxCpltCb = pCb;
    return eBSP_I2C_ERR_NONE;
}

BspI2cError_e BspI2cRegisterMemRxCallback(BspI2cHandle_t handle, BspI2cMemRxCpltCb_t pCb)
{
    BspI2cModule_t* pModule = sBspI2cValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_I2C_ERR_INVALID_HANDLE;
    }

    pModule->pMemRxCpltCb = pCb;
    return eBSP_I2C_ERR_NONE;
}

BspI2cError_e BspI2cRegisterErrorCallback(BspI2cHandle_t handle, BspI2cErrorCb_t pCb)
{
    BspI2cModule_t* pModule = sBspI2cValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_I2C_ERR_INVALID_HANDLE;
    }

    pModule->pErrorCb = pCb;
    return eBSP_I2C_ERR_NONE;
}

/* --- Blocking Mode Functions --- */

BspI2cError_e BspI2cTransmit(BspI2cHandle_t handle, const BspI2cTransferConfig_t* pConfig)
{
    BspI2cModule_t* pModule = sBspI2cValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_I2C_ERR_INVALID_HANDLE;
    }

    if ((pConfig == NULL) || (pConfig->pData == NULL))
    {
        return eBSP_I2C_ERR_INVALID_PARAM;
    }

    if (pModule->eMode != eBSP_I2C_MODE_BLOCKING)
    {
        return eBSP_I2C_ERR_INVALID_PARAM;
    }

    HAL_StatusTypeDef halStatus =
        HAL_I2C_Master_Transmit(pModule->pHalHandle, pConfig->devAddr, pConfig->pData, pConfig->length, pModule->uTimeoutMs);

    if (halStatus == HAL_TIMEOUT)
    {
        return eBSP_I2C_ERR_TIMEOUT;
    }
    else if (halStatus != HAL_OK)
    {
        return eBSP_I2C_ERR_TRANSFER;
    }

    return eBSP_I2C_ERR_NONE;
}

BspI2cError_e BspI2cReceive(BspI2cHandle_t handle, const BspI2cTransferConfig_t* pConfig)
{
    BspI2cModule_t* pModule = sBspI2cValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_I2C_ERR_INVALID_HANDLE;
    }

    if ((pConfig == NULL) || (pConfig->pData == NULL))
    {
        return eBSP_I2C_ERR_INVALID_PARAM;
    }

    if (pModule->eMode != eBSP_I2C_MODE_BLOCKING)
    {
        return eBSP_I2C_ERR_INVALID_PARAM;
    }

    HAL_StatusTypeDef halStatus =
        HAL_I2C_Master_Receive(pModule->pHalHandle, pConfig->devAddr, pConfig->pData, pConfig->length, pModule->uTimeoutMs);

    if (halStatus == HAL_TIMEOUT)
    {
        return eBSP_I2C_ERR_TIMEOUT;
    }
    else if (halStatus != HAL_OK)
    {
        return eBSP_I2C_ERR_TRANSFER;
    }

    return eBSP_I2C_ERR_NONE;
}

BspI2cError_e BspI2cMemRead(BspI2cHandle_t handle, const BspI2cMemConfig_t* pConfig)
{
    BspI2cModule_t* pModule = sBspI2cValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_I2C_ERR_INVALID_HANDLE;
    }

    if ((pConfig == NULL) || (pConfig->pData == NULL))
    {
        return eBSP_I2C_ERR_INVALID_PARAM;
    }

    if (pModule->eMode != eBSP_I2C_MODE_BLOCKING)
    {
        return eBSP_I2C_ERR_INVALID_PARAM;
    }

    HAL_StatusTypeDef halStatus = HAL_I2C_Mem_Read(pModule->pHalHandle, pConfig->devAddr, pConfig->memAddr, (uint16_t)pConfig->memAddrSize,
                                                   pConfig->pData, pConfig->length, pModule->uTimeoutMs);

    if (halStatus == HAL_TIMEOUT)
    {
        return eBSP_I2C_ERR_TIMEOUT;
    }
    else if (halStatus != HAL_OK)
    {
        return eBSP_I2C_ERR_TRANSFER;
    }

    return eBSP_I2C_ERR_NONE;
}

BspI2cError_e BspI2cMemWrite(BspI2cHandle_t handle, const BspI2cMemConfig_t* pConfig)
{
    BspI2cModule_t* pModule = sBspI2cValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_I2C_ERR_INVALID_HANDLE;
    }

    if ((pConfig == NULL) || (pConfig->pData == NULL))
    {
        return eBSP_I2C_ERR_INVALID_PARAM;
    }

    if (pModule->eMode != eBSP_I2C_MODE_BLOCKING)
    {
        return eBSP_I2C_ERR_INVALID_PARAM;
    }

    HAL_StatusTypeDef halStatus = HAL_I2C_Mem_Write(pModule->pHalHandle, pConfig->devAddr, pConfig->memAddr, (uint16_t)pConfig->memAddrSize,
                                                    pConfig->pData, pConfig->length, pModule->uTimeoutMs);

    if (halStatus == HAL_TIMEOUT)
    {
        return eBSP_I2C_ERR_TIMEOUT;
    }
    else if (halStatus != HAL_OK)
    {
        return eBSP_I2C_ERR_TRANSFER;
    }

    return eBSP_I2C_ERR_NONE;
}

/* --- Interrupt Mode Functions --- */

BspI2cError_e BspI2cTransmitIT(BspI2cHandle_t handle, const BspI2cTransferConfig_t* pConfig)
{
    BspI2cModule_t* pModule = sBspI2cValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_I2C_ERR_INVALID_HANDLE;
    }

    if ((pConfig == NULL) || (pConfig->pData == NULL))
    {
        return eBSP_I2C_ERR_INVALID_PARAM;
    }

    if (pModule->eMode != eBSP_I2C_MODE_INTERRUPT)
    {
        return eBSP_I2C_ERR_INVALID_PARAM;
    }

    HAL_StatusTypeDef halStatus = HAL_I2C_Master_Transmit_IT(pModule->pHalHandle, pConfig->devAddr, pConfig->pData, pConfig->length);

    if (halStatus == HAL_BUSY)
    {
        return eBSP_I2C_ERR_BUSY;
    }
    else if (halStatus != HAL_OK)
    {
        return eBSP_I2C_ERR_TRANSFER;
    }

    return eBSP_I2C_ERR_NONE;
}

BspI2cError_e BspI2cReceiveIT(BspI2cHandle_t handle, const BspI2cTransferConfig_t* pConfig)
{
    BspI2cModule_t* pModule = sBspI2cValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_I2C_ERR_INVALID_HANDLE;
    }

    if ((pConfig == NULL) || (pConfig->pData == NULL))
    {
        return eBSP_I2C_ERR_INVALID_PARAM;
    }

    if (pModule->eMode != eBSP_I2C_MODE_INTERRUPT)
    {
        return eBSP_I2C_ERR_INVALID_PARAM;
    }

    HAL_StatusTypeDef halStatus = HAL_I2C_Master_Receive_IT(pModule->pHalHandle, pConfig->devAddr, pConfig->pData, pConfig->length);

    if (halStatus == HAL_BUSY)
    {
        return eBSP_I2C_ERR_BUSY;
    }
    else if (halStatus != HAL_OK)
    {
        return eBSP_I2C_ERR_TRANSFER;
    }

    return eBSP_I2C_ERR_NONE;
}

BspI2cError_e BspI2cMemReadIT(BspI2cHandle_t handle, const BspI2cMemConfig_t* pConfig)
{
    BspI2cModule_t* pModule = sBspI2cValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_I2C_ERR_INVALID_HANDLE;
    }

    if ((pConfig == NULL) || (pConfig->pData == NULL))
    {
        return eBSP_I2C_ERR_INVALID_PARAM;
    }

    if (pModule->eMode != eBSP_I2C_MODE_INTERRUPT)
    {
        return eBSP_I2C_ERR_INVALID_PARAM;
    }

    HAL_StatusTypeDef halStatus = HAL_I2C_Mem_Read_IT(pModule->pHalHandle, pConfig->devAddr, pConfig->memAddr,
                                                      (uint16_t)pConfig->memAddrSize, pConfig->pData, pConfig->length);

    if (halStatus == HAL_BUSY)
    {
        return eBSP_I2C_ERR_BUSY;
    }
    else if (halStatus != HAL_OK)
    {
        return eBSP_I2C_ERR_TRANSFER;
    }

    return eBSP_I2C_ERR_NONE;
}

BspI2cError_e BspI2cMemWriteIT(BspI2cHandle_t handle, const BspI2cMemConfig_t* pConfig)
{
    BspI2cModule_t* pModule = sBspI2cValidateHandle(handle);

    if (pModule == NULL)
    {
        return eBSP_I2C_ERR_INVALID_HANDLE;
    }

    if ((pConfig == NULL) || (pConfig->pData == NULL))
    {
        return eBSP_I2C_ERR_INVALID_PARAM;
    }

    if (pModule->eMode != eBSP_I2C_MODE_INTERRUPT)
    {
        return eBSP_I2C_ERR_INVALID_PARAM;
    }

    HAL_StatusTypeDef halStatus = HAL_I2C_Mem_Write_IT(pModule->pHalHandle, pConfig->devAddr, pConfig->memAddr,
                                                       (uint16_t)pConfig->memAddrSize, pConfig->pData, pConfig->length);

    if (halStatus == HAL_BUSY)
    {
        return eBSP_I2C_ERR_BUSY;
    }
    else if (halStatus != HAL_OK)
    {
        return eBSP_I2C_ERR_TRANSFER;
    }

    return eBSP_I2C_ERR_NONE;
}

/* --- HAL Callback Functions --- */

// lint -e818
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* hi2c)
{
    BspI2cModule_t* pModule = sBspI2cFindModuleByHalHandle(hi2c);

    if ((pModule != NULL) && (pModule->pTxCpltCb != NULL))
    {
        BspI2cHandle_t handle = (BspI2cHandle_t)(pModule - s_i2cModules);
        pModule->pTxCpltCb(handle);
    }
}

// lint -e818
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef* hi2c)
{
    BspI2cModule_t* pModule = sBspI2cFindModuleByHalHandle(hi2c);

    if ((pModule != NULL) && (pModule->pRxCpltCb != NULL))
    {
        BspI2cHandle_t handle = (BspI2cHandle_t)(pModule - s_i2cModules);
        pModule->pRxCpltCb(handle);
    }
}

// lint -e818
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef* hi2c)
{
    BspI2cModule_t* pModule = sBspI2cFindModuleByHalHandle(hi2c);

    if ((pModule != NULL) && (pModule->pMemTxCpltCb != NULL))
    {
        BspI2cHandle_t handle = (BspI2cHandle_t)(pModule - s_i2cModules);
        pModule->pMemTxCpltCb(handle);
    }
}

// lint -e818
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef* hi2c)
{
    BspI2cModule_t* pModule = sBspI2cFindModuleByHalHandle(hi2c);

    if ((pModule != NULL) && (pModule->pMemRxCpltCb != NULL))
    {
        BspI2cHandle_t handle = (BspI2cHandle_t)(pModule - s_i2cModules);
        pModule->pMemRxCpltCb(handle);
    }
}

// lint -e818
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c)
{
    BspI2cModule_t* pModule = sBspI2cFindModuleByHalHandle(hi2c);

    if ((pModule != NULL) && (pModule->pErrorCb != NULL))
    {
        BspI2cHandle_t handle = (BspI2cHandle_t)(pModule - s_i2cModules);
        pModule->pErrorCb(handle, eBSP_I2C_ERR_TRANSFER);
    }
}
