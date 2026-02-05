/*
 * adc.c
 *
 *  Created on: Jan 11, 2026
 *      Author: xiongwei
 */

#include "config.h"
#include "bsp.h"
#include "adc.h"

#define ADC_CONVERSION_TIMEOUT	100
#define ADC_AMAX				3000		//mV


ADC_HandleTypeDef hadc1;


void adcInit(void)
{
	hadc1.Instance 						= ADC1;
	hadc1.Init.ClockPrescaler 			= ADC_CLOCK_SYNC_PCLK_DIV2;
	hadc1.Init.Resolution 				= ADC_RESOLUTION_12B;
	hadc1.Init.DataAlign 				= ADC_DATAALIGN_RIGHT;
	hadc1.Init.ScanConvMode 			= ADC_SCAN_DISABLE;
	hadc1.Init.EOCSelection 			= ADC_EOC_SINGLE_CONV;
	hadc1.Init.LowPowerAutoWait 		= DISABLE;
	hadc1.Init.LowPowerAutoPowerOff 	= DISABLE;
	hadc1.Init.ContinuousConvMode 		= DISABLE;
	hadc1.Init.NbrOfConversion 			= 1;
	hadc1.Init.DiscontinuousConvMode 	= DISABLE;
	hadc1.Init.ExternalTrigConv 		= ADC_SOFTWARE_START;
	hadc1.Init.ExternalTrigConvEdge 	= ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.DMAContinuousRequests 	= DISABLE;
	hadc1.Init.Overrun 					= ADC_OVR_DATA_PRESERVED;
	hadc1.Init.SamplingTimeCommon1 		= ADC_SAMPLETIME_1CYCLE_5;
	hadc1.Init.SamplingTimeCommon2 		= ADC_SAMPLETIME_1CYCLE_5;
	hadc1.Init.OversamplingMode 		= DISABLE;
	hadc1.Init.TriggerFrequencyMode 	= ADC_TRIGGER_FREQ_HIGH;

	if (HAL_ADC_Init(&hadc1) != HAL_OK) { Error_Handler(); }
}

uint32_t ReadAdcVBUS(uint8_t port)
{
	ADC_ChannelConfTypeDef sConfig = {0};
	uint32_t mV;

	sConfig.Channel 	 = port==PORT0? ADC_CHANNEL_2 : ADC_CHANNEL_3;
	sConfig.Rank 		 = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) { Error_Handler(); }
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, ADC_CONVERSION_TIMEOUT);
	mV = HAL_ADC_GetValue(&hadc1) * ADC_AMAX * (330 + 50) / (4096 * 50);
	HAL_ADC_Stop(&hadc1);
	return mV;
}
