/*
 * bsp_gpio.h
 *
 *  Created on: Jun 20, 2024
 *      Author: IlicAleksander
 */

#ifndef BSPCMN_BSP_GPIO_H_
#define BSPCMN_BSP_GPIO_H_

#include <stdbool.h>

#include "cpu_specific.h"
#include "mx_config.h"

#define BSP_GPIO_9_5_CB_CNT   5u
#define BSP_GPIO_15_10_CB_CNT 6u
#define BSP_GPIO_4_0_CB_CNT   5u
#define BSP_GPIO_MASK_15_10   0xFC00u
#define BSP_GPIO_MASK_9_5     0x03F0u

typedef enum
{
    eBSP_GPIO_IRQ_CH_0 = 0u,
    eBSP_GPIO_IRQ_CH_1,
    eBSP_GPIO_IRQ_CH_2,
    eBSP_GPIO_IRQ_CH_3,
    eBSP_GPIO_IRQ_CH_4,
    eBSP_GPIO_IRQ_CH_9_5,
    eBSP_GPIO_IRQ_CH_15_10
} BspGpioIRQChEnum;

typedef enum
{
    eGPIO_CFG_OD_OUT,
    eGPIO_CFG_PP_OUT,
    eGPIO_CFG_INPUT
} GpioPortPinCfg;

typedef GPIO_PinState GpioPinState_t;

/**
 * Get Low level handle for pin. Low level handles
 * are needed to change pin configuration
 * @param
 * ePin enum representing the pin
 * @return low level handle
 */
uint32_t BspGpioGetLLHandle(GpioPort_e ePin);

/**
 * Change pin configuration.
 * @param ePin enum representing the pin
 * @param uLLPinHandle low level
 * handle
 * @param eCfg configuration to be set
 */
void BspGpioCfgPin(GpioPort_e ePin, uint32_t uLLPinHandle, GpioPortPinCfg eCfg);

/**
 * Write digital output.
 * @param ePin enum representing the pin
 * @param bSet true for HIGH, false for
 * LOW
 */
void BspGpioWritePin(GpioPort_e ePin, bool bSet);

/**
 * Toggle digital output value
 * @param ePin enum representing the pin
 */
void BspGpioTogglePin(GpioPort_e ePin);

/**
 * Read digital input value
 * @param ePin enum representing the pin
 * @return true if HIGH, false if
 * LOW
 */
bool BspGpioReadPin(GpioPort_e ePin);

/**
 * Interrupt routine. Must be called from interrupt handler
 * @param eIRQCh enum representing interrupt
 * that occurred
 */
void BspGpioIRQ(BspGpioIRQChEnum eIRQCh);

/**
 * Register callback function to be called when corresponding
 * interrupt is triggered
 * @param ePin
 * enum representing the pin
 * @param pCb callback function
 */
void BspGpioSetIRQHandler(GpioPort_e ePin, GpioIrqCb_t pCb);

/**
 * Enable external interrupt on the given pin
 * @param ePin enum representing the pin
 */
void BspGpioEnableIRQ(GpioPort_e ePin);

#endif /* BSPCMN_BSP_GPIO_H_ */
