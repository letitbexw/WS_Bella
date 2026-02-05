/*
 * adc.h
 *
 *  Created on: Jan 11, 2026
 *      Author: xiongwei
 */

#ifndef DRIVER_ADC_H_
#define DRIVER_ADC_H_


enum {
	PORT0 = 0,
	ORION = 2
};

void adcInit(void);
uint32_t ReadAdcVBUS(uint8_t port);

#endif /* DRIVER_ADC_H_ */
