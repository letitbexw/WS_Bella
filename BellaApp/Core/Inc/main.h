/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif


#define MAIN_EVENT_ACC_I2C_RDY             	(1UL << 0)
#define MAIN_EVENT_IAP_CTL_FROM_ACC        	(1UL << 1)
#define MAIN_EVENT_IAP_FILE_FROM_ACC       	(1UL << 2)
#define MAIN_EVENT_IAP_EA_FROM_ACC         	(1UL << 3)
#define MAIN_EVENT_REBOOT_TO_APP           	(1UL << 4)
#define MAIN_EVENT_REBOOT_TO_DFU           	(1UL << 5)
#define MAIN_EVENT_PD_UPDATE               	(1UL << 6)
#define MAIN_EVENT_SERVICE_PD              	(1UL << 7)

#define MAIN_EVENT_DEVICE_IS_SLEEP         	(1UL << 8)
#define MAIN_EVENT_DEVICE_IS_AWAKE         	(1UL << 9)
#define MAIN_EVENT_WAKE_DEVICE             	(1UL << 10)

#define MAIN_EVENT_CP_SLEEP                	(1UL << 11)

#define MAIN_EVENT_PREVENT_SLEEP           	(1UL << 12)

#define MAIN_EVENT_AID_CONNECT             	(1UL << 13)
#define MAIN_EVENT_AID_RUNNING             	(1UL << 14)
#define MAIN_EVENT_AID_DISCONNECTED        	(1UL << 15)
#define MAIN_EVENT_AID_PAUSE               	(1UL << 16)
#define MAIN_EVENT_AID_RESUME              	(1UL << 17)

#define MAIN_EVENT_AUTH_START              	(1UL << 18)
#define MAIN_EVENT_AUTH_COMPLETE           	(1UL << 19)
#define MAIN_EVENT_HID_ENUM_COMPLETE       	(1UL << 20)
#define MAIN_EVENT_BTNPRESSED       		(1UL << 21)

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"
#include "stm32g0xx_ll_ucpd.h"
#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_cortex.h"
#include "stm32g0xx_ll_rcc.h"
#include "stm32g0xx_ll_system.h"
#include "stm32g0xx_ll_utils.h"
#include "stm32g0xx_ll_pwr.h"
#include "stm32g0xx_ll_gpio.h"
#include "stm32g0xx_ll_dma.h"
#include "stm32g0xx_ll_exti.h"

#include <stdbool.h>


void Error_Handler(void);


extern volatile uint32_t mainEvents;

void mainSetEvents(uint32_t event);
void mainClearEvents(uint32_t event);

bool mainGetAuthState(void);

#define mainCheckEvents(event)  ((mainEvents & (event)))

void mainReboot(bool forceDfu);


#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
