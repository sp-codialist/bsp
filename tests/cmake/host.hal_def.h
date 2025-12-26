/**
 ******************************************************************************
 * @file    stm32g4xx_hal_def.h
 * @author  MCD Application Team
 * @brief   This file contains HAL common defines, enumeration, macros and
 *          structures definitions.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F4xx_HAL_DEF
#define __STM32F4xx_HAL_DEF

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Stub definitions for host compilation -------------------------------------*/
// GPIO peripheral structure stub
typedef struct
{
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

// GPIO Pin state (only define if not already defined in hal_gpio.h)
#ifndef GPIO_PIN_RESET
typedef enum
{
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET
} GPIO_PinState;
#endif

/* IRQn_Type enumeration for NVIC (subset needed for host testing) */
typedef enum
{
    EXTI0_IRQn     = 6,
    EXTI1_IRQn     = 7,
    EXTI2_IRQn     = 8,
    EXTI3_IRQn     = 9,
    EXTI4_IRQn     = 10,
    EXTI9_5_IRQn   = 23,
    EXTI15_10_IRQn = 40
} IRQn_Type;

/* MPU region initialization structure */
typedef struct
{
    uint32_t dummy; /* Placeholder for host testing */
} MPU_Region_InitTypeDef;

/* ADC peripheral structure stub */
typedef struct
{
    volatile uint32_t dummy; /* Placeholder for host testing */
} ADC_TypeDef;

/* ADC common peripheral structure stub */
typedef struct
{
    volatile uint32_t dummy; /* Placeholder for host testing */
} ADC_Common_TypeDef;

/* DMA peripheral structure stub */
typedef struct
{
    volatile uint32_t dummy; /* Placeholder for host testing */
} DMA_TypeDef;

/* DMA Stream structure stub */
typedef struct
{
    volatile uint32_t dummy; /* Placeholder for host testing */
} DMA_Stream_TypeDef;

/* SPI peripheral structure stub */
typedef struct
{
    volatile uint32_t CR1;     /* Control register 1 */
    volatile uint32_t CR2;     /* Control register 2 */
    volatile uint32_t SR;      /* Status register */
    volatile uint32_t DR;      /* Data register */
    volatile uint32_t CRCPR;   /* CRC polynomial register */
    volatile uint32_t RXCRCR;  /* RX CRC register */
    volatile uint32_t TXCRCR;  /* TX CRC register */
    volatile uint32_t I2SCFGR; /* I2S configuration register */
    volatile uint32_t I2SPR;   /* I2S prescaler register */
} SPI_TypeDef;

/* I2C peripheral structure stub */
typedef struct
{
    volatile uint32_t CR1;   /* Control register 1 */
    volatile uint32_t CR2;   /* Control register 2 */
    volatile uint32_t OAR1;  /* Own address register 1 */
    volatile uint32_t OAR2;  /* Own address register 2 */
    volatile uint32_t DR;    /* Data register */
    volatile uint32_t SR1;   /* Status register 1 */
    volatile uint32_t SR2;   /* Status register 2 */
    volatile uint32_t CCR;   /* Clock control register */
    volatile uint32_t TRISE; /* TRISE register */
    volatile uint32_t FLTR;  /* FLTR register */
} I2C_TypeDef;

/* CAN peripheral structure stub */
typedef struct
{
    volatile uint32_t MCR;                     /* Master control register */
    volatile uint32_t MSR;                     /* Master status register */
    volatile uint32_t TSR;                     /* Transmit status register */
    volatile uint32_t RF0R;                    /* Receive FIFO 0 register */
    volatile uint32_t RF1R;                    /* Receive FIFO 1 register */
    volatile uint32_t IER;                     /* Interrupt enable register */
    volatile uint32_t ESR;                     /* Error status register */
    volatile uint32_t BTR;                     /* Bit timing register */
    uint32_t          RESERVED0[88];           /* Reserved */
    volatile uint32_t sTxMailBox[3 * 4];       /* TX MailBox (simplified) */
    volatile uint32_t sFIFOMailBox[2 * 6];     /* FIFO MailBox (simplified) */
    uint32_t          RESERVED1[12];           /* Reserved */
    volatile uint32_t FMR;                     /* Filter master register */
    volatile uint32_t FM1R;                    /* Filter mode register */
    uint32_t          RESERVED2;               /* Reserved */
    volatile uint32_t FS1R;                    /* Filter scale register */
    uint32_t          RESERVED3;               /* Reserved */
    volatile uint32_t FFA1R;                   /* Filter FIFO assignment register */
    uint32_t          RESERVED4;               /* Reserved */
    volatile uint32_t FA1R;                    /* Filter activation register */
    uint32_t          RESERVED5[8];            /* Reserved */
    volatile uint32_t sFilterRegister[28 * 2]; /* Filter banks (simplified) */
} CAN_TypeDef;

/* TIM peripheral structure stub */
typedef struct
{
    volatile uint32_t CR1;   /* Control register 1 */
    volatile uint32_t CR2;   /* Control register 2 */
    volatile uint32_t SMCR;  /* Slave mode control register */
    volatile uint32_t DIER;  /* DMA/Interrupt enable register */
    volatile uint32_t SR;    /* Status register */
    volatile uint32_t EGR;   /* Event generation register */
    volatile uint32_t CCMR1; /* Capture/compare mode register 1 */
    volatile uint32_t CCMR2; /* Capture/compare mode register 2 */
    volatile uint32_t CCER;  /* Capture/compare enable register */
    volatile uint32_t CNT;   /* Counter */
    volatile uint32_t PSC;   /* Prescaler */
    volatile uint32_t ARR;   /* Auto-reload register */
    volatile uint32_t RCR;   /* Repetition counter register */
    volatile uint32_t CCR1;  /* Capture/compare register 1 */
    volatile uint32_t CCR2;  /* Capture/compare register 2 */
    volatile uint32_t CCR3;  /* Capture/compare register 3 */
    volatile uint32_t CCR4;  /* Capture/compare register 4 */
    volatile uint32_t BDTR;  /* Break and dead-time register */
    volatile uint32_t DCR;   /* DMA control register */
    volatile uint32_t DMAR;  /* DMA address for full transfer */
    volatile uint32_t OR;    /* Option register */
} TIM_TypeDef;

/* TIM channel definitions */
#ifndef TIM_CHANNEL_1
    #define TIM_CHANNEL_1 ((uint32_t)0x00000000)
#endif
#ifndef TIM_CHANNEL_2
    #define TIM_CHANNEL_2 ((uint32_t)0x00000004)
#endif
#ifndef TIM_CHANNEL_3
    #define TIM_CHANNEL_3 ((uint32_t)0x00000008)
#endif
#ifndef TIM_CHANNEL_4
    #define TIM_CHANNEL_4 ((uint32_t)0x0000000C)
#endif
#ifndef TIM_CHANNEL_ALL
    #define TIM_CHANNEL_ALL ((uint32_t)0x0000003C)
#endif

/* RCC clock divider definitions */
#ifndef RCC_HCLK_DIV1
    #define RCC_HCLK_DIV1 ((uint32_t)0x00000000)
#endif
#ifndef RCC_HCLK_DIV2
    #define RCC_HCLK_DIV2 ((uint32_t)0x00001000)
#endif
#ifndef RCC_HCLK_DIV4
    #define RCC_HCLK_DIV4 ((uint32_t)0x00001400)
#endif
#ifndef RCC_HCLK_DIV8
    #define RCC_HCLK_DIV8 ((uint32_t)0x00001800)
#endif
#ifndef RCC_HCLK_DIV16
    #define RCC_HCLK_DIV16 ((uint32_t)0x00001C00)
#endif

/* CAN register bit definitions */
#define CAN_ESR_BOFF ((uint32_t)0x00000004) /* Bus-off flag */
#define CAN_ESR_EPVF ((uint32_t)0x00000002) /* Error passive flag */
#define CAN_ESR_TEC  ((uint32_t)0x00FF0000) /* Transmit error counter */
#define CAN_ESR_REC  ((uint32_t)0xFF000000) /* Receive error counter */

/* CAN mailbox definitions */
#ifndef CAN_TX_MAILBOX0
    #define CAN_TX_MAILBOX0 ((uint32_t)0x00000001)
#endif
#ifndef CAN_TX_MAILBOX1
    #define CAN_TX_MAILBOX1 ((uint32_t)0x00000002)
#endif
#ifndef CAN_TX_MAILBOX2
    #define CAN_TX_MAILBOX2 ((uint32_t)0x00000004)
#endif

/* CAN FIFO definitions */
#ifndef CAN_RX_FIFO0
    #define CAN_RX_FIFO0 ((uint32_t)0x00000000)
#endif
#ifndef CAN_RX_FIFO1
    #define CAN_RX_FIFO1 ((uint32_t)0x00000001)
#endif

/* CAN filter definitions */
#ifndef CAN_FILTERMODE_IDMASK
    #define CAN_FILTERMODE_IDMASK ((uint32_t)0x00000000)
#endif
#ifndef CAN_FILTERMODE_IDLIST
    #define CAN_FILTERMODE_IDLIST ((uint32_t)0x00000001)
#endif

#ifndef CAN_FILTERSCALE_16BIT
    #define CAN_FILTERSCALE_16BIT ((uint32_t)0x00000000)
#endif
#ifndef CAN_FILTERSCALE_32BIT
    #define CAN_FILTERSCALE_32BIT ((uint32_t)0x00000001)
#endif

#ifndef CAN_FILTER_FIFO0
    #define CAN_FILTER_FIFO0 ((uint32_t)0x00000000)
#endif
#ifndef CAN_FILTER_FIFO1
    #define CAN_FILTER_FIFO1 ((uint32_t)0x00000001)
#endif
#ifndef CAN_FILTER_DISABLE
    #define CAN_FILTER_DISABLE ((uint32_t)0x00000000)
#endif
#ifndef CAN_FILTER_ENABLE
    #define CAN_FILTER_ENABLE ((uint32_t)0x00000001)
#endif

/* CAN interrupt definitions */
#ifndef CAN_IT_TX_MAILBOX_EMPTY
    #define CAN_IT_TX_MAILBOX_EMPTY ((uint32_t)0x00000001)
#endif
#ifndef CAN_IT_RX_FIFO0_MSG_PENDING
    #define CAN_IT_RX_FIFO0_MSG_PENDING ((uint32_t)0x00000002)
#endif
#ifndef CAN_IT_RX_FIFO0_FULL
    #define CAN_IT_RX_FIFO0_FULL ((uint32_t)0x00000004)
#endif
#ifndef CAN_IT_RX_FIFO0_OVERRUN
    #define CAN_IT_RX_FIFO0_OVERRUN ((uint32_t)0x00000008)
#endif
#ifndef CAN_IT_RX_FIFO1_MSG_PENDING
    #define CAN_IT_RX_FIFO1_MSG_PENDING ((uint32_t)0x00000010)
#endif
#ifndef CAN_IT_RX_FIFO1_FULL
    #define CAN_IT_RX_FIFO1_FULL ((uint32_t)0x00000020)
#endif
#ifndef CAN_IT_RX_FIFO1_OVERRUN
    #define CAN_IT_RX_FIFO1_OVERRUN ((uint32_t)0x00000040)
#endif
#ifndef CAN_IT_WAKEUP
    #define CAN_IT_WAKEUP ((uint32_t)0x00010000)
#endif
#ifndef CAN_IT_SLEEP_ACK
    #define CAN_IT_SLEEP_ACK ((uint32_t)0x00020000)
#endif
#ifndef CAN_IT_ERROR_WARNING
    #define CAN_IT_ERROR_WARNING ((uint32_t)0x00000100)
#endif
#ifndef CAN_IT_ERROR_PASSIVE
    #define CAN_IT_ERROR_PASSIVE ((uint32_t)0x00000200)
#endif
#ifndef CAN_IT_BUSOFF
    #define CAN_IT_BUSOFF ((uint32_t)0x00000400)
#endif
#ifndef CAN_IT_LAST_ERROR_CODE
    #define CAN_IT_LAST_ERROR_CODE ((uint32_t)0x00000800)
#endif
#ifndef CAN_IT_ERROR
    #define CAN_IT_ERROR ((uint32_t)0x00008000)
#endif

/* CAN ID types */
#ifndef CAN_ID_STD
    #define CAN_ID_STD ((uint32_t)0x00000000)
#endif
#ifndef CAN_ID_EXT
    #define CAN_ID_EXT ((uint32_t)0x00000004)
#endif

/* CAN RTR types */
#ifndef CAN_RTR_DATA
    #define CAN_RTR_DATA ((uint32_t)0x00000000)
#endif
#ifndef CAN_RTR_REMOTE
    #define CAN_RTR_REMOTE ((uint32_t)0x00000002)
#endif

/* CAN error codes */
#ifndef HAL_CAN_ERROR_NONE
    #define HAL_CAN_ERROR_NONE ((uint32_t)0x00000000)
#endif
#ifndef HAL_CAN_ERROR_EWG
    #define HAL_CAN_ERROR_EWG ((uint32_t)0x00000001)
#endif
#ifndef HAL_CAN_ERROR_EPV
    #define HAL_CAN_ERROR_EPV ((uint32_t)0x00000002)
#endif
#ifndef HAL_CAN_ERROR_BOF
    #define HAL_CAN_ERROR_BOF ((uint32_t)0x00000004)
#endif
#ifndef HAL_CAN_ERROR_STF
    #define HAL_CAN_ERROR_STF ((uint32_t)0x00000008)
#endif
#ifndef HAL_CAN_ERROR_FOR
    #define HAL_CAN_ERROR_FOR ((uint32_t)0x00000010)
#endif
#ifndef HAL_CAN_ERROR_ACK
    #define HAL_CAN_ERROR_ACK ((uint32_t)0x00000020)
#endif
#ifndef HAL_CAN_ERROR_BR
    #define HAL_CAN_ERROR_BR ((uint32_t)0x00000040)
#endif
#ifndef HAL_CAN_ERROR_BD
    #define HAL_CAN_ERROR_BD ((uint32_t)0x00000080)
#endif
#ifndef HAL_CAN_ERROR_CRC
    #define HAL_CAN_ERROR_CRC ((uint32_t)0x00000100)
#endif

/* CMSIS intrinsics (for IRQ disable/enable) */
#ifndef __disable_irq
    #define __disable_irq() ((void)0)
#endif
#ifndef __enable_irq
    #define __enable_irq() ((void)0)
#endif

/* __IO qualifier stub */
#ifndef __IO
    #define __IO volatile
#endif

/* CAN peripheral base addresses (for host testing) */
#ifndef CAN1
    #define CAN1 ((CAN_TypeDef*)0x40006400UL)
#endif
#ifndef CAN2
    #define CAN2 ((CAN_TypeDef*)0x40006800UL)
#endif

/* FunctionalState enum */
typedef enum
{
    DISABLE = 0,
    ENABLE  = !DISABLE
} FunctionalState;

#define IS_FUNCTIONAL_STATE(STATE) (((STATE) == DISABLE) || ((STATE) == ENABLE))

/* ADC register bit definitions (for ADC channel macros) */
#define ADC_CR1_AWDCH_0 ((uint32_t)0x00000001)
#define ADC_CR1_AWDCH_1 ((uint32_t)0x00000002)
#define ADC_CR1_AWDCH_2 ((uint32_t)0x00000004)
#define ADC_CR1_AWDCH_3 ((uint32_t)0x00000008)
#define ADC_CR1_AWDCH_4 ((uint32_t)0x00000010)

/* ADC sample time register bit definitions */
#define ADC_SMPR1_SMP10_0 ((uint32_t)0x00000001)
#define ADC_SMPR1_SMP10_1 ((uint32_t)0x00000002)
#define ADC_SMPR1_SMP10_2 ((uint32_t)0x00000004)
#define ADC_SMPR1_SMP10   ((uint32_t)0x00000007)

/* Forward declaration of DMA_HandleTypeDef */
typedef struct __DMA_HandleTypeDef DMA_HandleTypeDef;

/* Exported types ------------------------------------------------------------*/

/**
 * @brief  HAL Status structures definition
 */
typedef enum
{
    HAL_OK      = 0x00U,
    HAL_ERROR   = 0x01U,
    HAL_BUSY    = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

/**
 * @brief  HAL Lock structures definition
 */
typedef enum
{
    HAL_UNLOCKED = 0x00U,
    HAL_LOCKED   = 0x01U
} HAL_LockTypeDef;

/**
 * @brief  HAL TICK frequency
 */
typedef enum
{
    HAL_TICK_FREQ_10HZ    = 100U,
    HAL_TICK_FREQ_100HZ   = 10U,
    HAL_TICK_FREQ_1KHZ    = 1U,
    HAL_TICK_FREQ_DEFAULT = HAL_TICK_FREQ_1KHZ
} HAL_TickFreqTypeDef;

/**
 * @brief  CAN State structures definition (for testing)
 */
#ifndef HAL_CAN_STATE_TYPEDEF
    #define HAL_CAN_STATE_TYPEDEF
typedef enum
{
    HAL_CAN_STATE_RESET         = 0x00U,
    HAL_CAN_STATE_READY         = 0x01U,
    HAL_CAN_STATE_LISTENING     = 0x02U,
    HAL_CAN_STATE_SLEEP_PENDING = 0x03U,
    HAL_CAN_STATE_SLEEP_ACTIVE  = 0x04U,
    HAL_CAN_STATE_ERROR         = 0x05U
} HAL_CAN_StateTypeDef;
#endif

/**
 * @brief  CAN Handle structure (forward declaration for testing)
 */
#ifndef CAN_HANDLETYPEDEF
    #define CAN_HANDLETYPEDEF
typedef struct __CAN_HandleTypeDef CAN_HandleTypeDef;

/**
 * @brief  CAN Handle structure (stub for testing)
 */
struct __CAN_HandleTypeDef
{
    CAN_TypeDef*         Instance;  /* CAN peripheral base address */
    void*                Init;      /* CAN initialization parameters (opaque) */
    volatile uint32_t    ErrorCode; /* CAN error code */
    HAL_CAN_StateTypeDef State;     /* CAN State */
    HAL_LockTypeDef      Lock;      /* CAN locking object */
};
#endif

/**
 * @brief  CAN TX/RX Header structures (stubs for testing)
 */
#ifndef CAN_RXHEADERTYPEDEF
    #define CAN_RXHEADERTYPEDEF
typedef struct
{
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
    uint32_t Timestamp;
    uint32_t FilterMatchIndex;
} CAN_RxHeaderTypeDef;
#endif

#ifndef CAN_TXHEADERTYPEDEF
    #define CAN_TXHEADERTYPEDEF
typedef struct
{
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
    uint32_t TransmitGlobalTime;
} CAN_TxHeaderTypeDef;
#endif

/**
 * @brief  CAN Filter structure (stub for testing)
 */
#ifndef CAN_FILTERTYPEDEF
    #define CAN_FILTERTYPEDEF
typedef struct
{
    uint32_t FilterIdHigh;
    uint32_t FilterIdLow;
    uint32_t FilterMaskIdHigh;
    uint32_t FilterMaskIdLow;
    uint32_t FilterFIFOAssignment;
    uint32_t FilterBank;
    uint32_t FilterMode;
    uint32_t FilterScale;
    uint32_t FilterActivation;
    uint32_t SlaveStartFilterBank;
} CAN_FilterTypeDef;
#endif

/**
 * @brief  CAN Callback ID enumeration
 */
typedef enum
{
    HAL_CAN_TX_MAILBOX0_COMPLETE_CB_ID = 0x00U,
    HAL_CAN_TX_MAILBOX1_COMPLETE_CB_ID = 0x01U,
    HAL_CAN_TX_MAILBOX2_COMPLETE_CB_ID = 0x02U,
    HAL_CAN_TX_MAILBOX0_ABORT_CB_ID    = 0x03U,
    HAL_CAN_TX_MAILBOX1_ABORT_CB_ID    = 0x04U,
    HAL_CAN_TX_MAILBOX2_ABORT_CB_ID    = 0x05U,
    HAL_CAN_RX_FIFO0_MSG_PENDING_CB_ID = 0x06U,
    HAL_CAN_RX_FIFO0_FULL_CB_ID        = 0x07U,
    HAL_CAN_RX_FIFO1_MSG_PENDING_CB_ID = 0x08U,
    HAL_CAN_RX_FIFO1_FULL_CB_ID        = 0x09U,
    HAL_CAN_SLEEP_CB_ID                = 0x0AU,
    HAL_CAN_WAKEUP_FROM_RX_MSG_CB_ID   = 0x0BU,
    HAL_CAN_ERROR_CB_ID                = 0x0CU,
    HAL_CAN_MSPINIT_CB_ID              = 0x0DU,
    HAL_CAN_MSPDEINIT_CB_ID            = 0x0EU
} HAL_CAN_CallbackIDTypeDef;

/**
 * @brief  TIM Handle structure (stub for testing)
 */
#ifndef TIM_HANDLETYPEDEF
    #define TIM_HANDLETYPEDEF
typedef struct
{
    TIM_TypeDef*    Instance; /* TIM peripheral base address */
    void*           Init;     /* TIM initialization parameters (opaque) */
    HAL_LockTypeDef Lock;     /* TIM locking object */
    void*           State;    /* TIM State (opaque) */
} TIM_HandleTypeDef;
#endif

/**
 * @brief  RCC clock configuration structure (stub for testing)
 */
#ifndef RCC_CLKINITTYPEDEF
    #define RCC_CLKINITTYPEDEF
typedef struct
{
    uint32_t SYSCLKSource;
    uint32_t AHBCLKDivider;
    uint32_t APB1CLKDivider;
    uint32_t APB2CLKDivider;
} RCC_ClkInitTypeDef;
#endif

/**
 * @brief  DMA Handle structure (stub for testing)
 */
struct __DMA_HandleTypeDef
{
    void*           Instance; /* DMA peripheral base address */
    void*           Init;     /* DMA initialization parameters (opaque) */
    HAL_LockTypeDef Lock;     /* DMA locking object */
    void*           State;    /* DMA State (opaque) */
    void*           Parent;   /* Parent object (opaque) */
};

/* Exported macros -----------------------------------------------------------*/

#define HAL_MAX_DELAY 0xFFFFFFFFU

#define HAL_IS_BIT_SET(REG, BIT) (((REG) & (BIT)) == (BIT))
#define HAL_IS_BIT_CLR(REG, BIT) (((REG) & (BIT)) == 0U)

#define __HAL_LINKDMA(__HANDLE__, __PPP_DMA_FIELD__, __DMA_HANDLE__)                                                                       \
    do                                                                                                                                     \
    {                                                                                                                                      \
        (__HANDLE__)->__PPP_DMA_FIELD__ = &(__DMA_HANDLE__);                                                                               \
        (__DMA_HANDLE__).Parent         = (__HANDLE__);                                                                                    \
    } while (0)

#if !defined(UNUSED)
    #define UNUSED(X) (void)X /* To avoid gcc/g++ warnings */
#endif                        /* UNUSED */

/** @brief Reset the Handle's State field.
 * @param __HANDLE__: specifies the Peripheral Handle.
 * @note  This macro can be used for the following purpose:
 *          - When the Handle is declared as local variable; before passing it as parameter
 *            to HAL_PPP_Init() for the first time, it is mandatory to use this macro
 *            to set to 0 the Handle's "State" field.
 *            Otherwise, "State" field may have any random value and the first time the function
 *            HAL_PPP_Init() is called, the low level hardware initialization will be missed
 *            (i.e. HAL_PPP_MspInit() will not be executed).
 *          - When there is a need to reconfigure the low level hardware: instead of calling
 *            HAL_PPP_DeInit() then HAL_PPP_Init(), user can make a call to this macro then HAL_PPP_Init().
 *            In this later function, when the Handle's "State" field is set to 0, it will execute the function
 *            HAL_PPP_MspInit() which will reconfigure the low level hardware.
 * @retval None
 */
#define __HAL_RESET_HANDLE_STATE(__HANDLE__) ((__HANDLE__)->State = 0)

#if (USE_RTOS == 1U)
    /* Reserved for future use */
    #error " USE_RTOS should be 0 in the current HAL release "
#else
    #define __HAL_LOCK(__HANDLE__)                                                                                                         \
        do                                                                                                                                 \
        {                                                                                                                                  \
            if ((__HANDLE__)->Lock == HAL_LOCKED)                                                                                          \
            {                                                                                                                              \
                return HAL_BUSY;                                                                                                           \
            }                                                                                                                              \
            else                                                                                                                           \
            {                                                                                                                              \
                (__HANDLE__)->Lock = HAL_LOCKED;                                                                                           \
            }                                                                                                                              \
        } while (0U)

    #define __HAL_UNLOCK(__HANDLE__)                                                                                                       \
        do                                                                                                                                 \
        {                                                                                                                                  \
            (__HANDLE__)->Lock = HAL_UNLOCKED;                                                                                             \
        } while (0U)
#endif /* USE_RTOS */

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050) /* ARM Compiler V6 */
    #ifndef __weak
        #define __weak __attribute__((weak))
    #endif
    #ifndef __packed
        #define __packed __attribute__((packed))
    #endif
#elif defined(__GNUC__) && !defined(__CC_ARM) /* GNU Compiler */
    #ifndef __weak
        #define __weak __attribute__((weak))
    #endif /* __weak */
    #ifndef __packed
        #define __packed __attribute__((__packed__))
    #endif /* __packed */
#endif     /* __GNUC__ */

/* Macro to get variable aligned on 4-bytes, for __ICCARM__ the directive "#pragma data_alignment=4" must be used instead */
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050) /* ARM Compiler V6 */
    #ifndef __ALIGN_BEGIN
        #define __ALIGN_BEGIN
    #endif
    #ifndef __ALIGN_END
        #define __ALIGN_END __attribute__((aligned(4)))
    #endif
#elif defined(__GNUC__) && !defined(__CC_ARM) /* GNU Compiler */
    #ifndef __ALIGN_END
        #define __ALIGN_END __attribute__((aligned(4U)))
    #endif /* __ALIGN_END */
    #ifndef __ALIGN_BEGIN
        #define __ALIGN_BEGIN
    #endif /* __ALIGN_BEGIN */
#else
    #ifndef __ALIGN_END
        #define __ALIGN_END
    #endif /* __ALIGN_END */
    #ifndef __ALIGN_BEGIN
        #if defined(__CC_ARM) /* ARM Compiler V5*/
            #define __ALIGN_BEGIN __align(4U)
        #elif defined(__ICCARM__) /* IAR Compiler */
            #define __ALIGN_BEGIN
        #endif /* __CC_ARM */
    #endif     /* __ALIGN_BEGIN */
#endif         /* __GNUC__ */

/**
 * @brief  __RAM_FUNC definition
 */
#if defined(__CC_ARM) || (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
    /* ARM Compiler V4/V5 and V6
       --------------------------
       RAM functions are defined using the toolchain options.
       Functions that are executed in RAM should reside in a separate source module.
       Using the 'Options for File' dialog you can simply change the 'Code / Const'
       area of a module to a memory space in physical RAM.
       Available memory areas are declared in the 'Target' tab of the 'Options for Target'
       dialog.
    */
    #define __RAM_FUNC

#elif defined(__ICCARM__)
    /* ICCARM Compiler
       ---------------
       RAM functions are defined using a specific toolchain keyword "__ramfunc".
    */
    #define __RAM_FUNC __ramfunc

#elif defined(__GNUC__)
    /* GNU Compiler
       ------------
      RAM functions are defined using a specific toolchain attribute
       "__attribute__((section(".RamFunc")))".
    */

    #define __RAM_FUNC //__attribute__((section(".RamFunc")))

#endif /* __CC_ARM */

/**
 * @brief  __NOINLINE definition
 */
#if defined(__CC_ARM) || (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) || defined(__GNUC__)
    /* ARM V4/V5 and V6 & GNU Compiler
       -------------------------------
    */
    #define __NOINLINE __attribute__((noinline))

#elif defined(__ICCARM__)
    /* ICCARM Compiler
       ---------------
    */
    #define __NOINLINE _Pragma("optimize = no_inline")

#endif /* __CC_ARM || __GNUC__ */

/* CMSIS Core instruction interface stubs for host testing */
#define __DMB() ((void)0) /* Data Memory Barrier - no-op for host testing */
#define __DSB() ((void)0) /* Data Synchronization Barrier - no-op for host testing */
#define __ISB() ((void)0) /* Instruction Synchronization Barrier - no-op for host testing */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_HAL_DEF */
