/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "bsp.h"
#include "timers.h"
#include "debug_uart.h"
#include "idio.h"
#include "test.h"
#include "adc.h"
#include "wdt.h"
#include "usbpd.h"
#include "usbpd_dpm_user.h"
#include "usb_device.h"
#include "target.h"
#include "debug.h"


#define BTN_DEBOUNCE_TIME 	30

UART_HandleTypeDef idbusUartHandle;

volatile uint32_t mainEvents = 0;
static bool mainAuthState = false;

/* Private function prototypes -----------------------------------------------*/

static void ButtonEvent(void)
{
	static uint32_t tBtnPressed = 0;

	if (HAL_GPIO_ReadPin(BTN_OK) == GPIO_PIN_RESET && !mainCheckEvents(MAIN_EVENT_BTNPRESSED))
	{
		mainSetEvents(MAIN_EVENT_BTNPRESSED);
		tBtnPressed = HAL_GetTick();
	}
	if (HAL_GPIO_ReadPin(BTN_OK) == GPIO_PIN_SET && mainCheckEvents(MAIN_EVENT_BTNPRESSED))
	{
		// Button released
		if (HAL_GetTick() - tBtnPressed > BTN_DEBOUNCE_TIME)
		{
			HAL_GPIO_TogglePin(LED_GRN);
			USBPD_SetRequestedVoltage(9000);
			USBPD_DPM_RequestGetSourceCapability(USBPD_PORT_0);
			mainClearEvents(MAIN_EVENT_BTNPRESSED);
		}
	}
}


int main(void)
{
	HAL_Init();
	bspInit();
	tmrInit();
	uartDebugInit();
	testInit();
	debugInit();

	idbusUartHandle.Instance = ORION_UART_BASE_PTR;
	idioInit();
	adcInit();
	ucpdInit();
//  MX_USB_Device_Init();
	wdtInit();

	__enable_irq();

	while (1)
	{
		USBPD_DPM_Run();
		testHarnessService();
		ButtonEvent();
		wdtService();
		HAL_Delay(1);
	}
}


bool mainGetAuthState(void) { return mainAuthState; }

void mainSetEvents(uint32_t event)
{
	uint32_t primask;
	targetGetInterruptState(primask);
	targetDisableInterrupts();

	mainEvents |= event;
	targetRestoreInterruptState(primask);
}

void mainClearEvents(uint32_t event)
{
	uint32_t primask;
	targetGetInterruptState(primask);
	targetDisableInterrupts();

	mainEvents &= ~event;
	targetRestoreInterruptState(primask);
}

void mainReboot(bool forceDfu)
{
	UNUSED(forceDfu);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
