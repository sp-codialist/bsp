/* Test-specific GPIO structure header */
/* This file defines the GPIO structures needed for unit testing */

#ifndef GPIO_STRUCT_H
#define GPIO_STRUCT_H

#define GPIO_STRUCT_TEST_VERSION 1 /* Marker to verify test version is used */

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/* Include HAL definitions needed for GPIO */
#include "stm32f4xx_hal_def.h"

/* GPIO Pin definitions (subset needed for testing) */
#define GPIO_PIN_0  ((uint16_t)0x0001)
#define GPIO_PIN_1  ((uint16_t)0x0002)
#define GPIO_PIN_5  ((uint16_t)0x0020)
#define GPIO_PIN_7  ((uint16_t)0x0080)
#define GPIO_PIN_13 ((uint16_t)0x2000)

/* GPIO pin count for testing - MUST match gpio_pins array size in test */
#define eGPIO_COUNT 6

/* GPIO structure for pin/port pairs */
typedef struct
{
    GPIO_TypeDef* pPort;  /**< GPIO port pointer */
    uint16_t      u16Pin; /**< GPIO pin number */
} gpio_t;

/* GPIO pin enumeration count - defined in test code */

/* Array of GPIO pin structures - defined in test code */

#ifdef __cplusplus
}
#endif

#endif /* GPIO_STRUCT_H */
