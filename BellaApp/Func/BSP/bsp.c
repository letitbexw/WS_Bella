/*
 * bsp.c
 *
 *  Created on: Jan 3, 2026
 *      Author: xiongwei
 */


#include "bsp.h"
#include "i2c.h"


uint8_t hwVersion;
COMP_HandleTypeDef hcompOrion;
I2C_HandleTypeDef hi2c1;


static void getHwRevision(void)
{
	hwVersion = HW_REV_V1;
}

void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/** Configure the main internal regulator output voltage */
	HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters in the RCC_OscInitTypeDef structure. */
	RCC_OscInitStruct.OscillatorType 		= RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI48;
	RCC_OscInitStruct.HSIState 				= RCC_HSI_ON;
	RCC_OscInitStruct.HSI48State 			= RCC_HSI48_ON;
	RCC_OscInitStruct.HSIDiv 				= RCC_HSI_DIV1;
	RCC_OscInitStruct.HSICalibrationValue 	= RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState 			= RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource 		= RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM 				= RCC_PLLM_DIV1;
	RCC_OscInitStruct.PLL.PLLN 				= 8;
	RCC_OscInitStruct.PLL.PLLP 				= RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ 				= RCC_PLLQ_DIV2;
	RCC_OscInitStruct.PLL.PLLR 				= RCC_PLLR_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

	/** Initializes the CPU, AHB and APB buses clocks */
	RCC_ClkInitStruct.ClockType 		= RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1;
	RCC_ClkInitStruct.SYSCLKSource 		= RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider 	= RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider 	= RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) { Error_Handler(); }
}


void initGPIO(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint32_t Mode, uint32_t Pull, GPIO_PinState PinState, uint32_t Alternate)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, PinState);
	GPIO_InitStruct.Pin 		= GPIO_Pin;
	GPIO_InitStruct.Mode 		= Mode;
	GPIO_InitStruct.Pull 		= Pull;
	GPIO_InitStruct.Speed 	 	= GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate 	= Alternate;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

static void gpioInit(void)
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	getHwRevision();

	initGPIO(BTN_OK, 			GPIO_MODE_INPUT, 	 GPIO_PULLUP, 	0, 0);				/* PB14, BTN_OK */
	initGPIO(LED_RGB, 			GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 	GPIO_PIN_RESET, 0);	/* PA4/PA5/PA6, LED_RED/LED_GRN/LED_BLU */
	initGPIO(USBC_HPEN,			GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 	GPIO_PIN_RESET, 0);	/* PC13, USBC_HPEN */
	initGPIO(USBC_LPEN, 		GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 	GPIO_PIN_RESET, 0);	/* PB5, USBC_LPEN */
	initGPIO(USBC_LPOC, 		GPIO_MODE_INPUT, 	 GPIO_NOPULL, 	0, 0);				/* PB3, USBC_LPOC */
	initGPIO(INA_EN, 			GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN, GPIO_PIN_SET, 0);	/* PB11, INA_EN */
	initGPIO(PSEN_P0, 			GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN, GPIO_PIN_RESET, 0);	/* PC14, PSEN_P0 */
	initGPIO(PSEN_ORION, 		GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN, GPIO_PIN_RESET, 0);	/* PC15, PSEN_ORION */
	initGPIO(AID_PU_EN_L, 		GPIO_MODE_OUTPUT_PP, GPIO_NOPULL,	GPIO_PIN_RESET, 0);	/* PF0, AID_PU_EN_L */
	initGPIO(AID_PD_EN, 		GPIO_MODE_OUTPUT_PP, GPIO_NOPULL,	GPIO_PIN_RESET, 0);	/* PF1, AID_PD_EN */
	initGPIO(REMOVAL_DET_L,		GPIO_MODE_INPUT, 	 GPIO_NOPULL, 	0, 0);				/* PC7, REMOVAL_DET_L */
	initGPIO(ORION_DATA_ENABLE, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 	GPIO_PIN_RESET, 0);	/* PA15, ORION_DATA_ENABLE */
	initGPIO(DISCH_ORION, 		GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 	GPIO_PIN_RESET, 0);	/* PB13, DISCH_ORION */
	initGPIO(MAGIC_PD_DIS, 		GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 	GPIO_PIN_RESET, 0);	/* PC6, MAGIC_PD_DIS */
	initGPIO(DEBUG_OUT,			GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 	GPIO_PIN_RESET, 0); /* PB0, DEBUG_OUT */
}

static void compInit(void)
{
	hcompOrion.Instance 		 	= COMP1;
	hcompOrion.Init.InputPlus 		= COMP_INPUT_PLUS_IO2;
	hcompOrion.Init.InputMinus 		= COMP_INPUT_MINUS_1_2VREFINT;
	hcompOrion.Init.OutputPol 		= COMP_OUTPUTPOL_NONINVERTED;
	hcompOrion.Init.WindowOutput 	= COMP_WINDOWOUTPUT_EACH_COMP;
	hcompOrion.Init.Hysteresis 		= COMP_HYSTERESIS_NONE;
	hcompOrion.Init.BlankingSrce 	= COMP_BLANKINGSRC_NONE;
	hcompOrion.Init.Mode 			= COMP_POWERMODE_HIGHSPEED;
	hcompOrion.Init.WindowMode 		= COMP_WINDOWMODE_DISABLE;
	hcompOrion.Init.TriggerMode 	= COMP_TRIGGERMODE_IT_RISING;
	HAL_COMP_Init(&hcompOrion);
	HAL_COMP_Start(&hcompOrion);
}

void bspInit(void)
{
	SystemClock_Config();
	gpioInit();
	compInit();
	I2C_MasterInit(&hi2c1, I2C1, 0xC12166);		/* 0x10B17DB5 --> 100kHz, 0xC12166 --> 400kHz */
	WriteInaReg(REG_CONFIG, 0x299F);				// Set shunt range to be +-80mV,
	WriteInaReg(REG_CALIBRATION, 25600);
}
