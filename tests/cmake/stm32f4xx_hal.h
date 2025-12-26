/**
 * @file stm32f4xx_hal.h
 * @brief Stub HAL header for host testing
 * @note This file provides minimal definitions for compiling production code in test environment
 */

#ifndef __STM32F4xx_HAL_H
#define __STM32F4xx_HAL_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Include the host-compatible HAL definitions */
#include "stm32f4xx_hal_def.h"

/* Include ADC header for ADC types and functions */
#include "stm32f4xx_hal_adc.h"

/* Include SPI header for SPI types and functions */
#include "stm32f4xx_hal_spi.h"

/* Include I2C header for I2C types and functions */
#include "stm32f4xx_hal_i2c.h"

/* Include CAN header for CAN types and functions */
#include "stm32f4xx_hal_can.h"

/* Include TIM header for TIM types and functions */
#include "stm32f4xx_hal_tim.h"

/* Include RCC header for RCC types and functions */
#include "stm32f4xx_hal_rcc.h"

/* Include RTC header for RTC types and functions */
#include "stm32f4xx_hal_rtc.h"

/* Tick function declarations (will be mocked) */
uint32_t HAL_GetTick(void);

/* GPIO pin defines */
#define GPIO_PIN_0  ((uint16_t)0x0001)
#define GPIO_PIN_1  ((uint16_t)0x0002)
#define GPIO_PIN_2  ((uint16_t)0x0004)
#define GPIO_PIN_3  ((uint16_t)0x0008)
#define GPIO_PIN_4  ((uint16_t)0x0010)
#define GPIO_PIN_5  ((uint16_t)0x0020)
#define GPIO_PIN_6  ((uint16_t)0x0040)
#define GPIO_PIN_7  ((uint16_t)0x0080)
#define GPIO_PIN_8  ((uint16_t)0x0100)
#define GPIO_PIN_9  ((uint16_t)0x0200)
#define GPIO_PIN_10 ((uint16_t)0x0400)
#define GPIO_PIN_11 ((uint16_t)0x0800)
#define GPIO_PIN_12 ((uint16_t)0x1000)
#define GPIO_PIN_13 ((uint16_t)0x2000)
#define GPIO_PIN_14 ((uint16_t)0x4000)
#define GPIO_PIN_15 ((uint16_t)0x8000)

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_HAL_H */
