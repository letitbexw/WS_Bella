/*
 * wdt.c
 *
 *  Created on: Feb 5, 2026
 *      Author: xiongwei
 */


#include "config.h"
#include "main.h"
#include "wdt.h"


#define WWDT_RELOAD_COUNT  0x7F


WWDG_HandleTypeDef WwdgHandle;


void wdtInit(void)
{
#ifdef CNFG_WATCHDOG_ENABLED
	__HAL_RCC_WWDG_CLK_ENABLE();

	WwdgHandle.Instance 	  = WWDG;
	WwdgHandle.Init.Prescaler = WWDG_PRESCALER_8;
	WwdgHandle.Init.Window    = 0x7F;
	WwdgHandle.Init.Counter   = WWDT_RELOAD_COUNT;
//	WwdgHandle.Init.EWIMode   = WWDG_EWI_DISABLE;

	if(HAL_WWDG_Init(&WwdgHandle) != HAL_OK) 	{ Error_Handler(); }
#endif
}

void wdtService(void)
{
#ifdef CNFG_WATCHDOG_ENABLED
	if(HAL_WWDG_Refresh(&WwdgHandle) != HAL_OK) { Error_Handler(); }
#endif
}
