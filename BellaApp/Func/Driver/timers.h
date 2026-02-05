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

typedef uint32_t timer_t;

#define TIMER_RUNNING_MASK (1 << 31)
#define TIMER_RUNNING(t) ((t) & TIMER_RUNNING_MASK)
#define TIMER_CLEAR(t) t = 0
#define TIMER_SET(t,v) t = (GetTickCount() + (timer_t)(v)) | TIMER_RUNNING_MASK
#define TIMER_EXPIRED(t) ( ((t) & TIMER_RUNNING_MASK) && (((int32_t) (GetTickCount() - (t & ~TIMER_RUNNING_MASK))) >= 0) )

#endif /* DRIVER_TIMERS_H_ */
