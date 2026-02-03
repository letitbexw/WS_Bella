/*
 * fifo16.h
 *
 *  Created on: Feb 3, 2026
 *      Author: xiongwei
 */

#ifndef CORE_FIFO16_H_
#define CORE_FIFO16_H_


#include "config.h"
#include <stdint.h>

typedef struct fifo16_s {
	uint32_t sizeMask;
	volatile uint32_t wrIdx;
	volatile uint32_t rdIdx;
	uint16_t *data;
} fifo16_t;

void fifo16Init(fifo16_t * fifo, uint16_t * buf, uint16_t size);
int fifo16Count(fifo16_t *fifo);
int fifo16SpaceAvailable(fifo16_t *fifo);
int fifo16Full(fifo16_t *fifo);
int fifo16Empty(fifo16_t *fifo);
void fifo16Reset(fifo16_t *fifo);

int fifo16Write(fifo16_t *fifo, uint16_t data);
int fifo16WriteBuffer(fifo16_t *fifo, const uint16_t *data, uint16_t length);
int fifo16WriteBuffer8(fifo16_t *fifo, const uint8_t *data, uint8_t length);
uint16_t fifo16Read(fifo16_t *fifo);

// Read a byte from the FIFO atomically.
// Returns 0 on success
int fifo16ReadA(fifo16_t *fifo, uint16_t *data);
int fifo16ReadBuffer(fifo16_t *fifo, uint16_t *data, uint16_t length);


#endif /* CORE_FIFO16_H_ */
