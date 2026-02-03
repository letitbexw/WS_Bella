/*
 * fifo8.h
 *
 *  Created on: Jan 10, 2026
 *      Author: xiongwei
 */

#ifndef CORE_FIFO8_H_
#define CORE_FIFO8_H_


#include "config.h"

typedef struct fifo8_s {
	uint32_t 			sizeMask;
	volatile uint32_t 	wrIdx;
	volatile uint32_t 	rdIdx;
	volatile uint32_t 	rdMarkIdx;
	uint8_t 			*data;
	bool 				dmaBusy;
} fifo8_t;

void fifo8Init(fifo8_t * fifo, uint8_t * buf, uint16_t size);
int fifo8Count(fifo8_t *fifo);
int fifo8SpaceAvailable(fifo8_t *fifo);
int fifo8Full(fifo8_t *fifo);
int fifo8Empty(fifo8_t *fifo);
void fifo8Reset(fifo8_t *fifo);

int fifo8Write(fifo8_t *fifo, uint8_t data);
int fifo8WriteBuffer(fifo8_t *fifo, const uint8_t *data, uint16_t length);
uint8_t fifo8Read(fifo8_t *fifo);

// Read a byte from the FIFO atomically.
// Returns 0 on success
int fifo8ReadA(fifo8_t *fifo, uint8_t *data);
int fifo8ReadBuffer(fifo8_t *fifo, uint8_t *data, uint16_t length);
uint8_t* fifo8ReadBufferInPlace(fifo8_t *fifo, uint16_t *length);
uint8_t* fifo8ReadBufferInPlaceLimited(fifo8_t *fifo, uint16_t *length, uint16_t maxLength);
void fifo8ReadBufferInPlaceUpdate(fifo8_t *fifo);


#endif /* CORE_FIFO8_H_ */
