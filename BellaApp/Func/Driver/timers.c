/*
 * timers.c
 *
 *  Created on: Jan 4, 2026
 *      Author: xiongwei
 */


#include "timers.h"
#include "bsp.h"


extern uint32_t SystemCoreClock;

TIM_HandleTypeDef hfTimer;
volatile uint32_t tmrCounter_ms = 0;


void (*tmrTickExternalCallback)(void) = NULL;

void tmrInit(void)
{
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	uint32_t uwPrescalerValue;

	/* Compute the prescaler value to have counter clock equal to 1MHz */
	/* Initialize peripheral as follow:
		+ Period = 10000 - 1
		+ Prescaler = ((SystemCoreClock/2)/10000) - 1
		+ ClockDivision = 0
		+ Counter direction = Up */
	uwPrescalerValue = (uint32_t) ((SystemCoreClock) / HF_TIMER_FREQUENCY) - 1;

	hfTimer.Instance 				= HF_TIMER;
	hfTimer.Init.Period 			= 65535;
	hfTimer.Init.Prescaler 			= uwPrescalerValue;
	hfTimer.Init.CounterMode 		= TIM_COUNTERMODE_UP;
	hfTimer.Init.AutoReloadPreload 	= TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&hfTimer) != HAL_OK) { Error_Handler(); }

	sMasterConfig.MasterOutputTrigger 	= TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode 		= TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&hfTimer, &sMasterConfig) != HAL_OK) { Error_Handler(); }
	if (HAL_TIM_Base_Start(&hfTimer) != HAL_OK) { Error_Handler(); }
}

void HAL_SYSTICK_Callback(void)
{
	tmrCounter_ms++;
	if (tmrTickExternalCallback != NULL) tmrTickExternalCallback();
}

inline uint32_t GetTickCount(void) { return tmrCounter_ms; }

// Millisecond delay
void tmrDelay_ms(uint32_t delay)
{
	uint32_t start_ms = tmrCounter_ms;
	while( (tmrCounter_ms - start_ms) < delay );
}

// us delay: wait busy loop, microseconds
void tmrDelay_us( uint16_t uSecs )
{
	uint16_t start = HF_TIMER->CNT;
	// use 16 bit count wrap around
	while((uint16_t)(HF_TIMER->CNT - start) <= uSecs);
}

