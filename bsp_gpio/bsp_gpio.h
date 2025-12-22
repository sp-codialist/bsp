/*
 * bsp_gpio.h
 *
 *  Created on: Jun 20, 2024
 *      Author: IlicAleksander
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum bsp_gpio_port_e
{
    eBSP_GPIO_PORT_A_0 = 0u,
    eBSP_GPIO_PORT_A_1,
    eBSP_GPIO_PORT_A_2,
    eBSP_GPIO_PORT_A_3,
    eBSP_GPIO_PORT_A_4,
    eBSP_GPIO_PORT_A_5,
    eBSP_GPIO_PORT_A_6,
    eBSP_GPIO_PORT_A_7,
    eBSP_GPIO_PORT_B_0,
    eBSP_GPIO_PORT_B_1,
    eBSP_GPIO_PORT_B_2,
    eBSP_GPIO_PORT_B_3,
    eBSP_GPIO_PORT_B_4,
    eBSP_GPIO_PORT_B_5,
    eBSP_GPIO_PORT_B_6,
    eBSP_GPIO_PORT_B_7
} GpioPort_e;

typedef enum bsp_gpio_irq_ch_e
{
    eBSP_GPIO_IRQ_CH_0 = 0u,
    eBSP_GPIO_IRQ_CH_1,
    eBSP_GPIO_IRQ_CH_2,
    eBSP_GPIO_IRQ_CH_3,
    eBSP_GPIO_IRQ_CH_4,
    eBSP_GPIO_IRQ_CH_9_5,
    eBSP_GPIO_IRQ_CH_15_10
} BspGpioIRQChEnum;

typedef enum bsp_gpio_port_pin_cfg_e
{
    eGPIO_CFG_OD_OUT,
    eGPIO_CFG_PP_OUT,
    eGPIO_CFG_INPUT
} GpioPortPinCfg;

typedef enum pin_state_e
{
    ePIN_STATE_LOW = 0u,
    ePIN_STATE_HIGH
} GpioPinState_t;

typedef void (*GpioIrqCb_t)(void);

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

#ifdef __cplusplus
}
#endif
