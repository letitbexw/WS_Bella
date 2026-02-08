/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app/usbpd.c
  * @author  MCD Application Team
  * @brief   This file contains the device define.
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
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbpd.h"
#include "main.h"
#include "config.h"


void ucpdInit(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Peripheral clock enable */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UCPD1);
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
	/**UCPD1 GPIO Configuration
	PB15   ------> UCPD1_CC2
	PA8   ------> UCPD1_CC1
	*/
	GPIO_InitStruct.Pin 	= LL_GPIO_PIN_15;
	GPIO_InitStruct.Mode 	= LL_GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull 	= LL_GPIO_PULL_NO;
	LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 	= LL_GPIO_PIN_8;
	GPIO_InitStruct.Mode 	= LL_GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull 	= LL_GPIO_PULL_NO;
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* UCPD1 DMA Init */
	NVIC_SetPriority(DMA1_Channel1_IRQn, 0);
	NVIC_EnableIRQ(DMA1_Channel1_IRQn);

	/* UCPD1_RX Init */
	LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_1, LL_DMAMUX_REQ_UCPD1_RX);
	LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_1, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
	LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PRIORITY_LOW);
	LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MODE_NORMAL);
	LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PERIPH_NOINCREMENT);
	LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MEMORY_INCREMENT);
	LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PDATAALIGN_BYTE);
	LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MDATAALIGN_BYTE);

	/* UCPD1_TX Init */
	LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_2, LL_DMAMUX_REQ_UCPD1_TX);
	LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_2, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
	LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PRIORITY_LOW);
	LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MODE_NORMAL);
	LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PERIPH_NOINCREMENT);
	LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MEMORY_INCREMENT);
	LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PDATAALIGN_BYTE);
	LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MDATAALIGN_BYTE);

	/* UCPD1 interrupt Init */
	NVIC_SetPriority(USB_UCPD1_2_IRQn, 0);
	NVIC_EnableIRQ(USB_UCPD1_2_IRQn);

	/* Global Init of USBPD HW */
	USBPD_HW_IF_GlobalHwInit();
	/* Initialize the Device Policy Manager */
	if (USBPD_OK != USBPD_DPM_InitCore()) 	{ Error_Handler(); }
	/* Initialise the DPM application */
	if (USBPD_OK != USBPD_DPM_UserInit()) 	{ Error_Handler(); }
	if (USBPD_OK != USBPD_DPM_InitOS()) 	{ Error_Handler(); }
}


