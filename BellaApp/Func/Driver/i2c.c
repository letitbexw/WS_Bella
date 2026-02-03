/*
 * i2c.c
 *
 *  Created on: Jan 10, 2026
 *      Author: xiongwei
 */


#include "i2c.h"
#include "bsp.h"


void I2C_MasterInit(I2C_HandleTypeDef* hi2c, I2C_TypeDef *ins, uint32_t timing)
{
	hi2c->Instance				= ins;
	hi2c->Init.Timing           = timing;
	hi2c->Init.OwnAddress1      = 0;
	hi2c->Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
	hi2c->Init.DualAddressMode  = I2C_DUALADDRESS_DISABLED;
	hi2c->Init.OwnAddress2      = 0;
	hi2c->Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	hi2c->Init.GeneralCallMode  = I2C_GENERALCALL_DISABLED;
	hi2c->Init.NoStretchMode    = I2C_NOSTRETCH_DISABLED;

	if(HAL_I2C_Init(hi2c) != HAL_OK) { Error_Handler(); }
	if (HAL_I2CEx_ConfigAnalogFilter(hi2c, I2C_ANALOGFILTER_ENABLE) != HAL_OK) { Error_Handler(); }
	if (HAL_I2CEx_ConfigDigitalFilter(hi2c, 0) != HAL_OK) { Error_Handler(); }
}

