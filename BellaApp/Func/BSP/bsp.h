/*
 * bsp.h
 *
 *  Created on: Jan 3, 2026
 *      Author: xiongwei
 */

#ifndef BSP_BSP_H_
#define BSP_BSP_H_

#include "main.h"
#include "orion.h"
#include <stdbool.h>


#define HW_REV_V1    1

#define BTN_OK				GPIOB, GPIO_PIN_14
#define LED_RGB				GPIOA, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
#define LED_RED				GPIOA, GPIO_PIN_4
#define LED_GRN				GPIOA, GPIO_PIN_5
#define LED_BLU				GPIOA, GPIO_PIN_6
#define USBC_HPEN			GPIOC, GPIO_PIN_13
#define USBC_LPEN			GPIOB, GPIO_PIN_5
#define USBC_LPOC			GPIOB, GPIO_PIN_3
#define INA_EN				GPIOB, GPIO_PIN_11
#define PSEN_P0				GPIOC, GPIO_PIN_14
#define PSEN_ORION			GPIOC, GPIO_PIN_15
#define AID_PU_EN_L			GPIOF, GPIO_PIN_0
#define AID_PD_EN			GPIOF, GPIO_PIN_1
#define REMOVAL_DET_L		GPIOC, GPIO_PIN_7
#define ORION_DATA_ENABLE	GPIOA, GPIO_PIN_15
#define DISCH_ORION			GPIOB, GPIO_PIN_13
#define MAGIC_PD_DIS		GPIOC, GPIO_PIN_6

#define USART_DBG			GPIOA, GPIO_PIN_0|GPIO_PIN_1
#define USART_ACC_AID		GPIOB, GPIO_PIN_8|GPIO_PIN_9
#define DEBUG_OUT			GPIOB, GPIO_PIN_0
#define ACC_AID_TX			GPIOB, GPIO_PIN_8
#define ACC_AID_RX			GPIOB, GPIO_PIN_9
#define VOL_SNS				GPIOA, GPIO_PIN_2|GPIO_PIN_3
#define COMP_ORION_IN		GPIOB, GPIO_PIN_1|GPIO_PIN_2
#define COMP_ORION_OUT		GPIOB, GPIO_PIN_10


// USART4 - Debug
#define DEBUG_UART_BASE_PTR           	USART4
#define DEBUG_UART_IRQn                 USART3_4_5_6_LPUART1_IRQn
#define DEBUG_UART_RATE             	230400
#define DEBUG_UART_IRQ_PRIORITY     	3

// USART3 - Orion
#define ORION_UART_BASE_PTR           	USART3
#define ORION_UART_IRQn                 USART3_4_5_6_LPUART1_IRQn
#define ORION_UART_RATE					1000000
#define ORION_UART_IRQ_PRIORITY			1


extern uint8_t hwVersion;


void initGPIO(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint32_t Mode, uint32_t Pull, GPIO_PinState PinState, uint32_t Alternate);
void bspInit(void);

static __INLINE bool bspReadoutProtectionIsSet(void)
{
	//return ((uint8_t)(FLASH->OBR) != (uint8_t)OB_RDP_Level_0);
	return false;
}

#define bspOrionDataAboveRMThreshold()    (HAL_GPIO_ReadPin(REMOVAL_DET_L) == GPIO_PIN_RESET)
#define bspOrionDataBelowRxThreshold()    (HAL_GPIO_ReadPin(ACC_AID_RX) == GPIO_PIN_RESET)

// board power source
typedef enum {
	bspOrionPowerSourceNone = 0,
	bspOrionPowerSourceBattery = 2,
	bspOrionPowerSourceUSB,
} bspOrionPowerSource_t;

typedef enum {
	bspOrionThreshHigh = 0,
	bspOrionThreshMed,
	bspOrionThreshLow,
} bspOrionThresh_t;

void bspSetDataEnable(bool enable);
void bspSetAccPower(bool enable, bool highPower);
bool bspGetOrionPowerIsHighPowerIn(void);
void bspMozartPower(bool enable);
void bspConfigureWakeEvents(void);
void bspSystemStop(void);
void bspOrionDetach(bool reset);

void bspSetOrionPull(orionLineState_t state);
void bspSetOrionPower(orionPowerSource_t source, uint8_t highPower);
uint8_t bspPowerAvailable(orionPowerSource_t power);
uint32_t bspGetOrionVbusVoltage(void);
void bspSetOrionThreshold(bspOrionThresh_t threshold);
uint8_t bspSinkEnable(uint8_t enable);
void bspEnterSTOPMode(uint32_t Regulator, uint8_t STOPEntry);


#endif /* BSP_BSP_H_ */
