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
#include "aid_pd.h"
#include "idio_bulk_data.h"


#define BTN_DEBOUNCE_TIME 	30

UART_HandleTypeDef idbusUartHandle;

volatile uint32_t mainEvents = 0;

static uint8_t aidActive = false;
//static uint8_t iap2Events = 0;
static bool mainAuthState = false;

static void mainEventService(void);
//static void mainServiceIap2(void);

static bool iapRun = false;
static bool accRun = false;
static uint32_t tAccReady = 0;
static uint32_t tBulkData = 0;
static uint32_t tAuthStart = 0;

static uint32_t tOrionAttach = 0;      // timer from OrionAttach. Set to 0 when not attached

#define ACC_START_DELAY_MS        300  // allow time for accessory to boot before reading params !!!!!!
#define REBOOT_TO_APP_DELAY_MS   1000  // allow traffic to complete before resetting
#define REBOOT_TO_DFU_DELAY_MS   1000  // allow traffic to complete before resetting
#define ACC_ATTACH_FAIL_MS       3500  // soft-detach if acc not ready within this time from attach
#define BATTERY_TRAP_RESTART_MS     0  // restart when in battery trap for discovery by booted device, 0 = unused
#define STOP_ON_INACTIVITY_MS     500  // enter STOP mode after inactivity
#define BULKDATA_ENABLE_IN_TRAP  5000  // if BulkData enable is sent >5s after attach, device is in trap -> allow attach as provider

#define MINIMUM_PROVIDER_MV  4500
#define MINIMUM_PROVIDER_MA   500

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
	orionInit();
	setOrionMode(orionModeAccessory);
	setOrionPowerSource(orionPowerSourceNone);
	bspSetOrionThreshold(bspOrionThreshMed);
	aidpdSetSinkCapability(USB_REQ_USB_NOSUSPEND | USB_REQ_OP_CURRENT(50) | USB_REQ_MINMAX_CURRENT(100));
	aidpdSetSourceCapability(PDO_VSAFE5V_SRC, 0);		// Add 5V PDO

	bspSetDataEnable(true);
//	iap2Init();
//	iap2TransportReady();

	bspSetAccPower(true, true);           // enable power to Acc by default(already done by HW)

	adcInit();
	ucpdInit();
//  MX_USB_Device_Init();
	wdtInit();

	__enable_irq();

	while (1)
	{
		wdtService();
		USBPD_DPM_Run();
		testHarnessService();
		ButtonEvent();

		orionService();
		mainEventService();

//		if (iapRun) { mainServiceIap2(); }
		if (idioBulkDataIsEnabled())
		{
			orionState_t orionState = getOrionState();
			if (tBulkData == 0)
			{
				tBulkData = GetTickCount();
				// soft-detach if we are provider when bulk data is identified
				if ((orionState != orionStateConsumer) && (tBulkData - tOrionAttach < BULKDATA_ENABLE_IN_TRAP))
				{
					DEBUG_PRINT_BOARD("\033[32mProvider -> restart\033[0m");
			        setOrionSoftDelay(0);
			        setOrionState(orionStateWaitDrainDisconnect);
				}
				else
				{
			          DEBUG_PRINT_BOARD("\033[32mBulkData enabled: %s\033[0m", orionStatePrint[orionState]);
			    }
			}
			else if (mainAuthState)
			{
				orionPowerSource_t powerSource = getOrionPowerSource();
				if ((powerSource == orionPowerSourceNone) ||
						((powerSource == orionPowerSourceEnable) && bspGetOrionPowerIsHighPowerIn()))
				{
					accSetDeviceIsPresent(true);       // role swap complete
					iapRun = true;
					DEBUG_PRINT_IAP("IAP Run");
				}
			}
		}

		// service the accessory
		if (accRun) { accService(); }
		else
		{
			if (tAccReady == 0) { tAccReady = GetTickCount(); }
			if (GetTickCount() - tAccReady > ACC_START_DELAY_MS)
			{
				accSetReadyState(true);
				accRun = true;
				DEBUG_PRINT_BOARD("Acc Run");
			}
		}

	    // reset Orion periodically if battery trap so booted device detects us properly
	    if ((tBulkData == 0)                                            &&
			tOrionAttach                                               	&&
	        (GetTickCount() - tOrionAttach > BATTERY_TRAP_RESTART_MS)   &&
	        BATTERY_TRAP_RESTART_MS)
		{
			DEBUG_CH('^');
			tOrionAttach = 0;
			setOrionState(orionStateSoftReset);
		}

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

static void mainEventService(void)
{
	// handle Orion Disconnect
	if (mainCheckEvents(MAIN_EVENT_AID_DISCONNECTED))
	{
		DEBUG_PRINT_ORION("Orion Disconnect");
		idioEnable(false);
		accSetDeviceIsPresent(false);
		mainAuthState = false;

		idioBulkDataInit();   // also resets ep device control, blocks iAP until auth completes

		aidActive 		= false;
		iapRun 			= false;
//		iap2Events 		= 0;
		tAuthStart 		= 0;
		tBulkData 		= 0;
		tOrionAttach 	= 0;
		mainClearEvents(MAIN_EVENT_AID_DISCONNECTED);
	}

	// handle Orion up and running
	if (mainCheckEvents(MAIN_EVENT_AID_RUNNING))
	{
		DEBUG_PRINT_BOARD("Orion AID Up");
		setOrionAIDState(true);   // AID is running
		mainClearEvents(MAIN_EVENT_AID_RUNNING);
	}

	// handle Orion Connected by polling idio
	if (mainCheckEvents(MAIN_EVENT_AID_CONNECT))
	{
	    DEBUG_PRINT_BOARD("Orion Connecting");
	    idioEnable(true);
	    tBulkData = 0;
	    aidActive = true;
//	    iap2TransportReady();

	    tOrionAttach = GetTickCount();
	    mainClearEvents(MAIN_EVENT_AID_CONNECT);
	}

	// handle Orion pause used in role swap to stop devices from talking during role swap
	if (mainCheckEvents(MAIN_EVENT_AID_PAUSE))
	{
		DEBUG_PRINT_BOARD("Orion Pause");
		idbusCaptureIDIO(false);
		mainClearEvents(MAIN_EVENT_AID_PAUSE);
	}

	// handle Orion resume used in role swap to start devices talking after role swap
	if (mainCheckEvents(MAIN_EVENT_AID_RESUME))
	{
		DEBUG_PRINT_BOARD("Orion Resume");
		idbusCaptureIDIO(true);
		mainClearEvents(MAIN_EVENT_AID_RESUME);
	}

	// handle PD change from accessory
	if (mainCheckEvents(MAIN_EVENT_PD_UPDATE))
	{
		uint16_t *pdData = (uint16_t *)accGetParamData(ACC_PARAM_ID_PD_CHARGE);
		uint16_t mV = htons(*pdData);
		pdData++;
		uint16_t mA = htons(*pdData);

		if ((mV >= MINIMUM_PROVIDER_MV) && (mA >= MINIMUM_PROVIDER_MA))
		{
			setOrionPowerSource(orionPowerSourceEnable);
		}
		else
		{
			setOrionPowerSource(orionPowerSourceNone);
		}
		mainSetEvents(MAIN_EVENT_SERVICE_PD);
		mainClearEvents(MAIN_EVENT_PD_UPDATE);
	}

	if (mainCheckEvents(MAIN_EVENT_SERVICE_PD) && mainAuthState)
	{
		static uint16_t mV = 0;
		static uint16_t mA = 0;
		uint16_t *pdData = (uint16_t *)accGetParamData(ACC_PARAM_ID_PD_CHARGE);
		uint16_t pDmV = htons(*pdData);
		pdData++;
		uint16_t pDmA = htons(*pdData);
		if ((mV != pDmV) || (mA != pDmA))
		{
			mV = pDmV;
			mA = pDmA;
			DEBUG_PRINT_BOARD("SetSourceCap: %dmV, %dmA", mV, mA);
			aidpdSetSourceCapability(USB_PDO_TYPE_FIXED | USB_PDO_DUAL_ROLE | USB_PDO_VOLTAGE(mV) | USB_PDO_CURRENT(mA), 0);
			aidpdResetDevice();
			if (((mV < MINIMUM_PROVIDER_MV) || (mA < MINIMUM_PROVIDER_MA)) && (getOrionState() != orionStateConsumer))
			{
				setOrionSoftDelay(0);
				setOrionState(orionStateSoftReset);
			}
		}
		mainClearEvents(MAIN_EVENT_SERVICE_PD);
	}

	if (mainCheckEvents(MAIN_EVENT_REBOOT_TO_APP))
	{
		static uint32_t tRebootApp = 0;
		if (tRebootApp == 0)
		{
			tRebootApp = GetTickCount();
			DEBUG_PRINT_BOARD("Rebooting to App");
		}
		else if (GetTickCount() - tRebootApp > REBOOT_TO_APP_DELAY_MS)
		{
			mainClearEvents(MAIN_EVENT_REBOOT_TO_APP);
			mainReboot(false);  // reboot normal
		}
	}

	if (mainCheckEvents(MAIN_EVENT_REBOOT_TO_DFU))
	{
		static uint32_t tRebootDfu = 0;
		if (tRebootDfu == 0)
		{
			tRebootDfu = GetTickCount();
			DEBUG_PRINT_BOARD("Rebooting to DFU");
		}
		else if (GetTickCount() - tRebootDfu > REBOOT_TO_DFU_DELAY_MS)
		{
			mainClearEvents(MAIN_EVENT_REBOOT_TO_DFU);
			mainReboot(true);  // reboot into dfu
		}
	}

	if (mainCheckEvents(MAIN_EVENT_HID_ENUM_COMPLETE)  	&&    	// HID endpoint enumeration complete
		accGetInitDone()                               	&&    	// Acc Init complete
		(getOrionPowerSource() == orionPowerSourceNone)) 		// no role swap is pending
	{
		accSetDeviceIsPresent(true);
#ifdef CNFG_DEBUG_BOARD_ENABLED
		static bool cleared = 0;
		if (!cleared)
		{
			cleared = true;
			DEBUG_PRINT_BOARD("CLR EV\n");
		}
#endif
		mainClearEvents(MAIN_EVENT_HID_ENUM_COMPLETE);
	}
#ifdef CNFG_DEBUG_BOARD_ENABLED
	else
	{
		static uint8_t last = 0;
		volatile uint8_t noclear = 	(mainCheckEvents(MAIN_EVENT_HID_ENUM_COMPLETE) ? 0 : 1) 	|
									(accGetInitDone()  ? 0 : 2) 								|
									((getOrionPowerSource() == orionPowerSourceNone) ? 0 : 4);
		if (last != noclear)
		{
			last = noclear;
			DEBUG_PRINT_BOARD("NO CLR EV %d l=%d src=%d\n", noclear, last, getOrionPowerSource());
		}
	}
#endif

	if (mainCheckEvents(MAIN_EVENT_AUTH_START))
	{
		iapRun = false;
//		iap2Events = 0;
		tAuthStart = GetTickCount();
//		iap2Reset();
		accResetComm();
		mainAuthState = false;
		mainClearEvents(MAIN_EVENT_AUTH_START);
	}

	if (mainCheckEvents(MAIN_EVENT_AUTH_COMPLETE) && accGetInitDone())
	{
		mainAuthState = true;
		mainClearEvents(MAIN_EVENT_AUTH_COMPLETE);
	}

	// handle Orion Connected by polling idio
	if (aidActive)
	{
		idioService();
	}
}

void mainReboot(bool forceDfu)
{
	mainSetEvents(MAIN_EVENT_PREVENT_SLEEP);
	UNUSED(forceDfu);
	bspOrionDetach(true);   // does not return
}

static void mainServiceIap2(void)
{
	// TBD
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
