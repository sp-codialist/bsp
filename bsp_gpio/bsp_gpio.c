/*
 * bsp_gpio.c
 *
 *  Created on: Jun 20, 2024
 *      Author: IlicAleksander
 */
#include "bsp_gpio.h"
#include "stm32f4xx_ll_gpio.h"
#include "cpu_specific.h"

static PortPin_t *s_aRegisteredIRQCbs_15_10[BSP_GPIO_15_10_CB_CNT];
static uint32_t s_uRegisteredIRQCbs_15_10_Cnt = 0;

static PortPin_t *s_aRegisteredIRQCbs_9_5[BSP_GPIO_9_5_CB_CNT];
static uint32_t s_uRegisteredIRQCbs_9_5_Cnt = 0;

static PortPin_t *s_aRegisteredIRQCbs_4_0[BSP_GPIO_4_0_CB_CNT];

static void sBspGpioCallIrq(const PortPin_t *pPinHandle);
static void sHandleIRQs(uint32_t uMask, uint32_t uRegisteredIRQCbsCnt, PortPin_t *const aRegisteredIRQCbs[]);
static void sRegisterIRQCb(PortPin_t *pPinHandle);
static void sEnableIRQ(const PortPin_t *pPinHandle);
static inline uint32_t sBspGpioGetLLPinHandle(uint32_t uInPin);

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
    //lint --e{923} SOUP
    //lint --e{9078} SOUP
    uint32_t uMask = EXTI->PR;
    uint32_t uMask_15_10 = 0;
    uint32_t uMask_9_5 = 0;
    const PortPin_t *pPinHandle = NULL;
    switch(eIRQCh)
    {
    case eBSP_GPIO_IRQ_CH_15_10:
        uMask_15_10 = uMask & BSP_GPIO_MASK_15_10;
        EXTI->PR = uMask_15_10;
        sHandleIRQs(uMask_15_10, s_uRegisteredIRQCbs_15_10_Cnt, s_aRegisteredIRQCbs_15_10);
        break;
    case eBSP_GPIO_IRQ_CH_9_5:
        uMask_9_5 = uMask & BSP_GPIO_MASK_9_5;
        EXTI->PR = uMask_9_5;
        sHandleIRQs(uMask_9_5, s_uRegisteredIRQCbs_9_5_Cnt, s_aRegisteredIRQCbs_9_5);
        break;
    case eBSP_GPIO_IRQ_CH_0:
        EXTI->PR = GPIO_PIN_0;
        pPinHandle = s_aRegisteredIRQCbs_4_0[eBSP_GPIO_IRQ_CH_0];
        sBspGpioCallIrq(pPinHandle);
        break;
    case eBSP_GPIO_IRQ_CH_1:
        EXTI->PR = GPIO_PIN_1;
        pPinHandle = s_aRegisteredIRQCbs_4_0[eBSP_GPIO_IRQ_CH_1];
        sBspGpioCallIrq(pPinHandle);
        break;
    case eBSP_GPIO_IRQ_CH_2:
        EXTI->PR = GPIO_PIN_2;
        pPinHandle = s_aRegisteredIRQCbs_4_0[eBSP_GPIO_IRQ_CH_2];
        sBspGpioCallIrq(pPinHandle);
        break;
    case eBSP_GPIO_IRQ_CH_3:
        EXTI->PR = GPIO_PIN_3;
        pPinHandle = s_aRegisteredIRQCbs_4_0[eBSP_GPIO_IRQ_CH_3];
        sBspGpioCallIrq(pPinHandle);
        break;
    case eBSP_GPIO_IRQ_CH_4:
        EXTI->PR = GPIO_PIN_4;
        pPinHandle = s_aRegisteredIRQCbs_4_0[eBSP_GPIO_IRQ_CH_4];
        sBspGpioCallIrq(pPinHandle);
        break;
    default:
        // invalid channel parameter
        break;
    }
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
    switch(pPinHandle->uPin)
    {
    case GPIO_PIN_0:
        s_aRegisteredIRQCbs_4_0[eBSP_GPIO_IRQ_CH_0] = pPinHandle;
        break;
    case GPIO_PIN_1:
        s_aRegisteredIRQCbs_4_0[eBSP_GPIO_IRQ_CH_1] = pPinHandle;
        break;
    case GPIO_PIN_2:
        s_aRegisteredIRQCbs_4_0[eBSP_GPIO_IRQ_CH_2] = pPinHandle;
        break;
    case GPIO_PIN_3:
        s_aRegisteredIRQCbs_4_0[eBSP_GPIO_IRQ_CH_3] = pPinHandle;
        break;
    case GPIO_PIN_4:
        s_aRegisteredIRQCbs_4_0[eBSP_GPIO_IRQ_CH_4] = pPinHandle;
        break;
    case GPIO_PIN_5:
    case GPIO_PIN_6:
    case GPIO_PIN_7:
    case GPIO_PIN_8:
    case GPIO_PIN_9:
        if(s_uRegisteredIRQCbs_9_5_Cnt < BSP_GPIO_9_5_CB_CNT)
        {
            s_aRegisteredIRQCbs_9_5[s_uRegisteredIRQCbs_9_5_Cnt] = pPinHandle;
            s_uRegisteredIRQCbs_9_5_Cnt++;
        }
        break;
    case GPIO_PIN_10:
    case GPIO_PIN_11:
    case GPIO_PIN_12:
    case GPIO_PIN_13:
    case GPIO_PIN_14:
    case GPIO_PIN_15:
        if(s_uRegisteredIRQCbs_15_10_Cnt < BSP_GPIO_15_10_CB_CNT)
        {
            s_aRegisteredIRQCbs_15_10[s_uRegisteredIRQCbs_15_10_Cnt] = pPinHandle;
            s_uRegisteredIRQCbs_15_10_Cnt++;
        }
        break;
    default:
        // default case unreachable
        break;
    }
}

static void sEnableIRQ(const PortPin_t *pPinHandle)
{
    switch(pPinHandle->uPin)
    {
    case GPIO_PIN_0:
        HAL_NVIC_EnableIRQ(EXTI0_IRQn);
        break;
    case GPIO_PIN_1:
        HAL_NVIC_EnableIRQ(EXTI1_IRQn);
        break;
    case GPIO_PIN_2:
        HAL_NVIC_EnableIRQ(EXTI2_IRQn);
        break;
    case GPIO_PIN_3:
        HAL_NVIC_EnableIRQ(EXTI3_IRQn);
        break;
    case GPIO_PIN_4:
        HAL_NVIC_EnableIRQ(EXTI4_IRQn);
        break;
    case GPIO_PIN_5:
    case GPIO_PIN_6:
    case GPIO_PIN_7:
    case GPIO_PIN_8:
    case GPIO_PIN_9:
        HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
        break;
    case GPIO_PIN_10:
    case GPIO_PIN_11:
    case GPIO_PIN_12:
    case GPIO_PIN_13:
    case GPIO_PIN_14:
    case GPIO_PIN_15:
        HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
        break;
    default:
        // default case unreachable
        break;
    }
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
