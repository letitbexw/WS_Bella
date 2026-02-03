/*
 * ina219.h
 *
 *  Created on: Jan 25, 2026
 *      Author: xiongwei
 */

#ifndef DRIVER_INA219_H_
#define DRIVER_INA219_H_

#include "config.h"

enum {
	REG_CONFIG = 0,
	REG_SHUNT_VOLT,
	REG_BUS_VOLT,
	REG_POWER,
	REG_CURRENT,
	REG_CALIBRATION
};

void WriteInaReg(uint8_t reg, uint16_t value);
uint16_t ReadInaReg(uint8_t reg);
int ReadInaVoltage(void);
int ReadInaCurrent(void);


#endif /* DRIVER_INA219_H_ */
