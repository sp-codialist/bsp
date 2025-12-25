/*
 * bsp_gpio.c
 */
#include <stddef.h>

#include "bsp_compiler_attributes.h"

#include "bsp_gpio.h"
#include "gpio_structs/gpio_struct.h"
#include "stm32f4xx_hal_cortex.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_ll_gpio.h"

/** static bsp pins that relate to each gpio in the ioc hal layer */
FORCE_STATIC GpioIrqCb_t s_aBspGpioPins[eGPIO_COUNT] = {NULL};
extern const gpio_t      gpio_pins[eGPIO_COUNT];

/** static functions */
FORCE_STATIC uint32_t getGpioIndexFromPin(uint16_t GPIO_Pin);

void BspGpioWritePin(uint32_t const ePin, bool const bSet)
{
    do
    {
        if (ePin >= eGPIO_COUNT)
        {
            return;
        }
        if (gpio_pins[ePin].pPort == NULL)
        {
            return;
        }
        if (bSet)
        {
            // lint -e{9034}
            HAL_GPIO_WritePin(gpio_pins[ePin].pPort, (uint16_t)gpio_pins[ePin].uPin, GPIO_PIN_SET);
        }
        else
        {
            // lint -e{9034}
            HAL_GPIO_WritePin(gpio_pins[ePin].pPort, (uint16_t)gpio_pins[ePin].uPin, GPIO_PIN_RESET);
        }
    } while (false);
}

void BspGpioTogglePin(uint32_t const ePin)
{
    do
    {
        if (ePin >= eGPIO_COUNT)
        {
            return;
        }
        if (gpio_pins[ePin].pPort == NULL)
        {
            return;
        }
        // lint -e{9034}
        HAL_GPIO_TogglePin(gpio_pins[ePin].pPort, (uint16_t)gpio_pins[ePin].uPin);
    } while (false);
}
bool BspGpioReadPin(uint32_t const ePin)
{
    bool pinState = false;
    do
    {
        if (ePin >= eGPIO_COUNT)
        {
            return pinState;
        }
        if (gpio_pins[ePin].pPort == NULL)
        {
            return pinState;
        }
        // lint -e{9034}
        pinState = (bool)HAL_GPIO_ReadPin(gpio_pins[ePin].pPort, (uint16_t)gpio_pins[ePin].uPin);
    } while (false);
    return pinState;
}

void BspGpioSetIRQHandler(uint32_t const ePin, GpioIrqCb_t const pCb)
{
    do
    {
        if (ePin >= eGPIO_COUNT)
        {
            return;
        }
        if (gpio_pins[ePin].pPort == NULL)
        {
            return;
        }
        s_aBspGpioPins[ePin] = pCb;
    } while (false);
}

void BspGpioEnableIRQ(uint32_t const ePin)
{
    do
    {
        if (ePin >= eGPIO_COUNT)
        {
            return;
        }
        switch (gpio_pins[ePin].uPin)
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
    } while (false);
}

FORCE_STATIC uint32_t getGpioIndexFromPin(uint16_t GPIO_Pin)
{
    uint32_t retIndex = eGPIO_COUNT;
    for (uint32_t i = 0; i < eGPIO_COUNT; i++)
    {
        if (gpio_pins[i].uPin == GPIO_Pin)
        {
            retIndex = i;
            break;
        }
    }
    return retIndex; // Invalid index
}

/*!
 * @brief HAL GPIO EXTI Callback function
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t gpioIndex = getGpioIndexFromPin(GPIO_Pin);
    if (gpioIndex < eGPIO_COUNT)
    {
        if (s_aBspGpioPins[gpioIndex] != NULL)
        {
            s_aBspGpioPins[gpioIndex]();
        }
    }
}
