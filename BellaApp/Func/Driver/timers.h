/*
 * timers.h
 *
 *  Created on: Jan 4, 2026
 *      Author: xiongwei
 */

#ifndef DRIVER_TIMERS_H_
#define DRIVER_TIMERS_H_


#include "config.h"

void tmrInit(void);
void tmrDelay_ms( uint32_t timeMs );
void tmrDelay_us(uint16_t timeUs);
uint32_t GetTickCount(void);


#endif /* DRIVER_TIMERS_H_ */
