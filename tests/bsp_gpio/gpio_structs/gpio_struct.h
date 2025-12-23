/* AUTO-GENERATED FILE - DO NOT EDIT MANUALLY */
/* Generated from: /home/s/gh/patel/bsp/_deps/cpu_precompiled_hal-src/include/cpb/main.h */
/* Generation time: 2025-12-23 21:08:23 */

#ifndef GPIO_STRUCT_H
#define GPIO_STRUCT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32f4xx_hal.h"

/* GPIO pin enumeration */
typedef enum
{
    eM_LED1 = 0,
    eM_LED2,
    eM_LED3,
    eM_LED_LIFE,
    eM_FLASH_NCS,
    eM_FLASH_SCK,
    eM_FLASH_SO,
    eM_FLASH_SI,
    eM_WP,
    eM_AMP_1,
    eM_AMP_2,
    eV24_OK,
    eM_HOLDRESET,
    eM_FAN_PWM,
    eM_WATCHDOG,
    eIBAR_LED_GREEN,
    eIBAR_LED_YELLOW,
    eIBAR_LED_RED,
    eBLOOD_PUMP_BUTTON,
    eRESERVE_BUTTON,
    eRESERVE_BUTTON_O,
    eM_CAN1_RX,
    eM_CAN1_TX,
    eBLOOD_PUMP_LED,
    eGPIO_COUNT
} gpio_e;

/* GPIO pin structure */
typedef struct
{
    GPIO_TypeDef* pPort;
    uint32_t      uPin;
} gpio_t;

/* GPIO pins array */
extern const gpio_t gpio_pins[eGPIO_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* GPIO_STRUCT_H */
