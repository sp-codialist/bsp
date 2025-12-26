/**
 * @file stm32f4xx_hal_can.h
 * @brief Minimal CAN HAL header stub for host testing
 */

#ifndef __STM32F4xx_HAL_CAN_H
#define __STM32F4xx_HAL_CAN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32f4xx_hal_def.h"

/* Function prototypes for mocking */
HAL_StatusTypeDef HAL_CAN_Init(CAN_HandleTypeDef* hcan);
HAL_StatusTypeDef HAL_CAN_DeInit(CAN_HandleTypeDef* hcan);
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef* hcan);
HAL_StatusTypeDef HAL_CAN_Stop(CAN_HandleTypeDef* hcan);
HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef* hcan, CAN_FilterTypeDef* sFilterConfig);
HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef* hcan, uint32_t ActiveITs);
HAL_StatusTypeDef HAL_CAN_DeactivateNotification(CAN_HandleTypeDef* hcan, uint32_t InactiveITs);
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef* hcan, CAN_TxHeaderTypeDef* pHeader, uint8_t aData[], uint32_t* pTxMailbox);
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef* hcan, uint32_t RxFifo, CAN_RxHeaderTypeDef* pHeader, uint8_t aData[]);
uint32_t          HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef* hcan);
uint32_t          HAL_CAN_GetError(CAN_HandleTypeDef* hcan);

/* Weak callback prototypes (to be overridden by user) */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef* hcan);
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef* hcan);
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef* hcan);
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan);
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef* hcan);
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef* hcan);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_HAL_CAN_H */
