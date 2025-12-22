/*
 * bsp_gpio.c
 *
 *  Created on: Jun 20, 2024
 *      Author: IlicAleksander
 */
#include "bsp_gpio.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_hal_conf.h"

const uint32_t BSP_GPIO_MASK_15_10 =  0xFC00U;
const uint32_t BSP_GPIO_MASK_9_5 =    0x03F0u;

/** private structure */
typedef struct port_pin_s
{
    GPIO_TypeDef *pPort;
    uint32_t uPin;
    GpioIrqCb_t pCb;
} PortPin_t;


static void sBspGpioCallIrq(const PortPin_t *pPinHandle);
static void sHandleIRQs(uint32_t uMask, uint32_t uRegisteredIRQCbsCnt, PortPin_t *const aRegisteredIRQCbs[]);
static void sRegisterIRQCb(PortPin_t *pPinHandle);
static void sEnableIRQ(const PortPin_t *pPinHandle);
static inline uint32_t sBspGpioGetLLPinHandle(uint32_t uInPin);
static PortPin_t *PortGetHandle(GpioPort_e ePin);

uint32_t BspGpioGetLLHandle(GpioPort_e ePin)
{
    uint32_t uRet = 0u;
    const PortPin_t *pPinHandle = PortGetHandle(ePin);
    if(pPinHandle != NULL)
    {
        uRet = sBspGpioGetLLPinHandle(pPinHandle->uPin);
    }
    return uRet;
}

void BspGpioCfgPin(GpioPort_e ePin, uint32_t uLLPinHandle, GpioPortPinCfg eCfg)
{
    if(uLLPinHandle != 0u)
    {
        const PortPin_t *pPinHandle = PortGetHandle(ePin);
        if(pPinHandle != NULL)
        {
            if(eCfg == eGPIO_CFG_OD_OUT)
            {
                LL_GPIO_SetPinMode(pPinHandle->pPort, uLLPinHandle, LL_GPIO_MODE_OUTPUT);
                LL_GPIO_SetPinOutputType(pPinHandle->pPort, uLLPinHandle, LL_GPIO_OUTPUT_OPENDRAIN);
            }
            else if(eCfg == eGPIO_CFG_PP_OUT)
            {
                LL_GPIO_SetPinMode(pPinHandle->pPort, uLLPinHandle, LL_GPIO_MODE_OUTPUT);
                LL_GPIO_SetPinOutputType(pPinHandle->pPort, uLLPinHandle, LL_GPIO_OUTPUT_PUSHPULL);
            }
            else if(eCfg == eGPIO_CFG_INPUT)
            {
                LL_GPIO_SetPinMode(pPinHandle->pPort, uLLPinHandle, LL_GPIO_MODE_INPUT);
            }
            else
            {
                // No action
            }
        }
    }
}

void BspGpioWritePin(GpioPort_e ePin, bool bSet)
{
    const PortPin_t *pPinHandle = PortGetHandle(ePin);
    if(pPinHandle != NULL)
    {
        if(bSet)
        {
            // lint -e{9034}
            HAL_GPIO_WritePin(pPinHandle->pPort, (uint16_t)pPinHandle->uPin, GPIO_PIN_SET);
        }
        else
        {
            // lint -e{9034}
            HAL_GPIO_WritePin(pPinHandle->pPort, (uint16_t)pPinHandle->uPin, GPIO_PIN_RESET);
        }
    }
}

void BspGpioTogglePin(GpioPort_e ePin)
{
    const PortPin_t *pPinHandle = PortGetHandle(ePin);
    if(pPinHandle != NULL)
    {
        HAL_GPIO_TogglePin(pPinHandle->pPort, (uint16_t)pPinHandle->uPin);
    }
}
bool BspGpioReadPin(GpioPort_e ePin)
{
    bool bSet = false;
    const PortPin_t *pPinHandle = PortGetHandle(ePin);
    if(pPinHandle != NULL)
    {
        GPIO_PinState tState;
        tState = HAL_GPIO_ReadPin(pPinHandle->pPort, (uint16_t)pPinHandle->uPin);
        bSet = ((uint8_t)tState == (uint8_t)GPIO_PIN_SET);
    }
    return bSet;
}

void BspGpioSetIRQHandler(GpioPort_e ePin, GpioIrqCb_t pCb)
{

    PortPin_t *pPinHandle = PortGetHandle(ePin);
    if(pPinHandle != NULL)
    {
        pPinHandle->pCb = pCb;
        sRegisterIRQCb(pPinHandle);
    }
}

void BspGpioEnableIRQ(GpioPort_e ePin)
{
    const PortPin_t *pPinHandle = PortGetHandle(ePin);
    if(pPinHandle != NULL)
    {
        sEnableIRQ(pPinHandle);
    }
}
void BspGpioIRQ(BspGpioIRQChEnum eIRQCh)
{

}

static void sBspGpioCallIrq(const PortPin_t *pPinHandle)
{
    if((pPinHandle != NULL) && (pPinHandle->pCb != NULL))
    {
        pPinHandle->pCb();
    }
}

static void sRegisterIRQCb(PortPin_t *pPinHandle)
{
}

static void sEnableIRQ(const PortPin_t *pPinHandle)
{

}

static void sHandleIRQs(uint32_t uMask, uint32_t uRegisteredIRQCbsCnt, PortPin_t *const aRegisteredIRQCbs[])
{
    for(uint32_t i = 0; i < uRegisteredIRQCbsCnt; i++)
    {
        uint32_t uPin = aRegisteredIRQCbs[i]->uPin;
        if((uMask & uPin) != 0u)
        {
            const PortPin_t *pPinHandle = aRegisteredIRQCbs[i];
            sBspGpioCallIrq(pPinHandle);
        }
    }
}

static inline uint32_t sBspGpioGetLLPinHandle(uint32_t uInPin)
{
    uint32_t uRet = 0u;
    switch(uInPin)
    {
    case GPIO_PIN_0:
        uRet = LL_GPIO_PIN_0;
        break;
    case GPIO_PIN_1:
        uRet = LL_GPIO_PIN_1;
        break;
    case GPIO_PIN_2:
        uRet = LL_GPIO_PIN_2;
        break;
    case GPIO_PIN_3:
        uRet = LL_GPIO_PIN_3;
        break;
    case GPIO_PIN_4:
        uRet = LL_GPIO_PIN_4;
        break;
    case GPIO_PIN_5:
        uRet = LL_GPIO_PIN_5;
        break;
    case GPIO_PIN_6:
        uRet = LL_GPIO_PIN_6;
        break;
    case GPIO_PIN_7:
        uRet = LL_GPIO_PIN_7;
        break;
    case GPIO_PIN_8:
        uRet = LL_GPIO_PIN_8;
        break;
    case GPIO_PIN_9:
        uRet = LL_GPIO_PIN_9;
        break;
    case GPIO_PIN_10:
        uRet = LL_GPIO_PIN_10;
        break;
    case GPIO_PIN_11:
        uRet = LL_GPIO_PIN_11;
        break;
    case GPIO_PIN_12:
        uRet = LL_GPIO_PIN_12;
        break;
    case GPIO_PIN_13:
        uRet = LL_GPIO_PIN_13;
        break;
    case GPIO_PIN_14:
        uRet = LL_GPIO_PIN_14;
        break;
    case GPIO_PIN_15:
        uRet = LL_GPIO_PIN_15;
        break;
    default:
        // uRet stays 0u
        break;
    }
    return uRet;
}

PortPin_t *PortGetHandle(GpioPort_e ePin)
{
    PortPin_t *pRet = NULL;
    //if(ePin < eGPIO_CNT)

    return pRet;
}
