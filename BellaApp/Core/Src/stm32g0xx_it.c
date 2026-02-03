/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32g0xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#include "bsp.h"
#include "stm32g0xx_it.h"
#include "debug_uart.h"
#include "usbpd.h"


extern PCD_HandleTypeDef hpcd_USB_DRD_FS;
extern UART_HandleTypeDef huartDbg;
extern DMA_HandleTypeDef hdma_Dbg_tx;

/******************************************************************************/
/*           Cortex-M0+ Processor Interruption and Exception Handlers          */
/******************************************************************************/

void NMI_Handler(void)			{ while (1) {} }
void HardFault_Handler(void)	{ while (1) {} }
void SVC_Handler(void)			{}
void PendSV_Handler(void)		{}

void SysTick_Handler(void)
{
	HAL_IncTick();
	HAL_SYSTICK_IRQHandler();
	USBPD_DPM_TimerCounter();
	if (HAL_GetTick() % 1000 == 0) { HAL_GPIO_TogglePin(LED_BLU); }
}

/******************************************************************************/
/* STM32G0xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32g0xx.s).                    */
/******************************************************************************/

void USART3_4_5_6_LPUART1_IRQHandler(void)
{
	HAL_UART_IRQHandler(&huartDbg);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART4)
	{
		uartDebugTxDmaComplete();
	}
	else if (huart->Instance == USART3)
	{
		//uartIdbusTxComplete();
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART4)
	{
		uartDebugRxComplete();
	}
	else if (huart->Instance == USART3)
	{

	}
}

void USB_UCPD1_2_IRQHandler(void)
{
	HAL_PCD_IRQHandler(&hpcd_USB_DRD_FS);
	USBPD_PORT0_IRQHandler();
}

void DMA1_Channel1_IRQHandler(void)
{

}

void DMA1_Channel2_3_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&hdma_Dbg_tx);
}

