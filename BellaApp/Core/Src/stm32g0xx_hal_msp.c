/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32g0xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
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
#include "debug_uart.h"

void HAL_MspInit(void)
{
	__HAL_RCC_SYSCFG_CLK_ENABLE();
	__HAL_RCC_PWR_CLK_ENABLE();

	/* System interrupt init*/

	/** Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral */
	HAL_SYSCFG_StrobeDBattpinsConfig(SYSCFG_CFGR1_UCPD2_STROBE);
}

void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
	if(hadc->Instance==ADC1)
	{
		/* Peripheral clock enable */
		__HAL_RCC_ADC_CLK_ENABLE();
		__HAL_RCC_GPIOA_CLK_ENABLE();
		initGPIO(VOL_SNS, GPIO_MODE_ANALOG, GPIO_NOPULL, 0, 0);	/* PA2/PA3, SNS_VBUS_P0/SNS_VBUS_ORION */
	}
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
{
	if(hadc->Instance==ADC1)
	{
		/* Peripheral clock disable */
		__HAL_RCC_ADC_CLK_DISABLE();
		HAL_GPIO_DeInit(VOL_SNS);
	}
}

void HAL_COMP_MspInit(COMP_HandleTypeDef* hcomp)
{
	if(hcomp->Instance==COMP1)
	{
		__HAL_RCC_GPIOB_CLK_ENABLE();
		initGPIO(COMP_ORION_IN, 	GPIO_MODE_ANALOG, GPIO_NOPULL, 0, 0);				/* PB1/PB2, RX_THRESH/ORION_DATA_GATED */
		initGPIO(COMP_ORION_OUT,	GPIO_MODE_AF_PP,  GPIO_NOPULL, 0, GPIO_AF7_COMP1);	/* PB10, ACC_AID_RX */
	}
}

void HAL_COMP_MspDeInit(COMP_HandleTypeDef* hcomp)
{
	if(hcomp->Instance==COMP1)
	{
		HAL_GPIO_DeInit(COMP_ORION_IN);
		HAL_GPIO_DeInit(COMP_ORION_OUT);
	}
}

void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
	if(hi2c->Instance==I2C1)
	{
		/** Initializes the peripherals clocks */
		PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
		PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_SYSCLK;
		if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) { Error_Handler(); }

		__HAL_RCC_GPIOB_CLK_ENABLE();
		initGPIO(GPIOB, GPIO_PIN_6|GPIO_PIN_7, GPIO_MODE_AF_OD, GPIO_NOPULL, 0, GPIO_AF6_I2C1);	/* PB6/PB7, I2C1_SCL/I2C1_SDA */
		/* Peripheral clock enable */
		__HAL_RCC_I2C1_CLK_ENABLE();
	}
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
	if(hi2c->Instance==I2C1)
	{
		/* Peripheral clock disable */
		__HAL_RCC_I2C1_CLK_DISABLE();
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7);
	}
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
{
	if(htim_base->Instance==HF_TIMER)
	{
		/* Peripheral clock enable */
		__HAL_RCC_TIM6_CLK_ENABLE();
	}
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base)
{
	if(htim_base->Instance==HF_TIMER)
	{
		/* Peripheral clock disable */
		__HAL_RCC_TIM6_CLK_DISABLE();
	}
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
	if(huart->Instance==USART3)
	{
		/** Initializes the peripherals clocks */
		PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART3;
		PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_SYSCLK;
		if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) { Error_Handler(); }

		/* Peripheral clock enable */
		__HAL_RCC_USART3_CLK_ENABLE();
		__HAL_RCC_GPIOB_CLK_ENABLE();
		initGPIO(USART_ACC_AID, GPIO_MODE_AF_PP, GPIO_NOPULL, 0, GPIO_AF4_USART3); /* PB8/PB9,  ACC_AID_TX/ACC_AID_RX */
	}
	else if(huart->Instance==USART4)
	{
		uartDebugMspInit();
	}
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
	if(huart->Instance==USART3)
	{
		/* Peripheral clock disable */
		__HAL_RCC_USART3_CLK_DISABLE();
		HAL_GPIO_DeInit(USART_ACC_AID);
	}
	else if(huart->Instance==USART4)
	{
		uartDebugMspDeInit();
	}
}


