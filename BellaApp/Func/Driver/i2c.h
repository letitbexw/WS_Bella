/*
 * i2c.h
 *
 *  Created on: Jan 10, 2026
 *      Author: xiongwei
 */

#ifndef DRIVER_I2C_H_
#define DRIVER_I2C_H_

#include "config.h"
#include "ina219.h"


void I2C_MasterInit(I2C_HandleTypeDef* hi2c, I2C_TypeDef *ins, uint32_t timing);


#endif /* DRIVER_I2C_H_ */
