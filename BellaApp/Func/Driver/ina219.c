/*
 * ina219.c
 *
 *  Created on: Jan 25, 2026
 *      Author: xiongwei
 */


#include "ina219.h"
#include "bsp.h"


#define I2C_TIMEOUT	1000
#define INA219_SA	0x80


extern I2C_HandleTypeDef hi2c1;

volatile uint32_t tIna = 0;
static uint8_t buf[3];

uint16_t ReadInaReg(uint8_t reg)
{
	buf[0] = reg;
	while (HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)INA219_SA, buf, 1, I2C_TIMEOUT) != HAL_OK)
	{
		if (HAL_I2C_GetError(&hi2c1) != HAL_I2C_ERROR_AF) {  Error_Handler(); }
	}

	while (HAL_I2C_Master_Receive(&hi2c1, (uint16_t)INA219_SA, buf, 2, I2C_TIMEOUT) != HAL_OK)
	{
		if (HAL_I2C_GetError(&hi2c1) != HAL_I2C_ERROR_AF) {  Error_Handler(); }
	}
	uint16_t value = (buf[0] << 8) | buf[1];

	return value;
}

void WriteInaReg(uint8_t reg, uint16_t value)
{
	buf[0] = reg;
	buf[1] = (uint8_t)(value >> 8);
	buf[2] = (uint8_t)value;

	while (HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)INA219_SA, buf, 3, I2C_TIMEOUT) != HAL_OK)
	{
		if (HAL_I2C_GetError(&hi2c1) != HAL_I2C_ERROR_AF) {  Error_Handler(); }
	}
}

int ReadInaVoltage(void)
{
	static uint16_t value;

	if (HAL_GetTick() - tIna > 100 || tIna == 0 )
	{
		// debugPrint("ReadInaVoltage");
		value = ReadInaReg(REG_BUS_VOLT);
		tIna = HAL_GetTick();
	}

	//if (!(value & 0x02)) { warnPrint("INA219 ADC Conv not ready! [0x%4X]", value); }
	return (int)(value>>3)*4;
}

int ReadInaCurrent(void)
{
	static int i;

	//debugPrint("ReadInaCurrent");
	i = (int16_t)ReadInaReg(REG_SHUNT_VOLT);
	tIna = HAL_GetTick();
//	uint16_t Vshunt = ReadInaReg(REG_SHUNT_VOLT) * 10;	//uV
//	uint16_t RegCal = ReadInaReg(REG_CALIBRATION);
//	int i = Vshunt * RegCal / 4096;

	return (int)i*1000/1090;	// Add Cal
}
