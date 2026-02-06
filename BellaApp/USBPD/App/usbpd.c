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

/* USBPD init function */
void MX_USBPD_Init(void)
{
	/* Global Init of USBPD HW */
	USBPD_HW_IF_GlobalHwInit();
	/* Initialize the Device Policy Manager */
	if (USBPD_OK != USBPD_DPM_InitCore()) 	{ Error_Handler(); }
	/* Initialise the DPM application */
	if (USBPD_OK != USBPD_DPM_UserInit()) 	{ Error_Handler(); }
	if (USBPD_OK != USBPD_DPM_InitOS()) 	{ Error_Handler(); }

	__enable_irq();
}


