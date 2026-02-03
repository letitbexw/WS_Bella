/*
 * fifo16.c
 *
 *  Created on: Feb 3, 2026
 *      Author: xiongwei
 */


#include "target.h"
#include "fifo16.h"

// Initialize a FIFO
void fifo16Init(fifo16_t * fifo, uint16_t * buf, uint16_t size)
{
    fifo->data = buf;
    fifo->sizeMask = size - 1;
    fifo->wrIdx = fifo->rdIdx = 0;
}

int fifo16Count(fifo16_t *fifo)
{
	int rval;
	int32_t r = fifo->rdIdx;
	int32_t w = fifo->wrIdx;

	rval = (w - r) & fifo->sizeMask;

	return rval;
}

int fifo16SpaceAvailable(fifo16_t *fifo)
{
	int rval;
	int32_t r = fifo->rdIdx;
	int32_t w = fifo->wrIdx;

	rval = (w - r) & fifo->sizeMask;
	rval = fifo->sizeMask - rval;
	return rval;
}

int fifo16Full(fifo16_t *fifo)
{
	return ((fifo->wrIdx + 1) & fifo->sizeMask) == fifo->rdIdx;
}

int fifo16Empty(fifo16_t *fifo)
{
	return fifo->rdIdx == fifo->wrIdx;
}

void fifo16Reset(fifo16_t *fifo)
{
	fifo->rdIdx = fifo->wrIdx = 0;
}

int fifo16Write(fifo16_t *fifo, uint16_t data) {
	int ret = 0;
	uint32_t primask = 0;
	targetGetInterruptState(primask);
	targetDisableInterrupts();

	// Check if the buffer has space, dont write data if its going to overrun.
	if (((fifo->wrIdx + 1) & fifo->sizeMask) != fifo->rdIdx) {
		fifo->data[fifo->wrIdx] = data;
		fifo->wrIdx = (fifo->wrIdx + 1) & fifo->sizeMask;
	}
	else {
		ret = -1;
	}

	targetRestoreInterruptState(primask);
	return ret;
}

int fifo16WriteBuffer(fifo16_t *fifo, const uint16_t *data, uint16_t length) {
	int ret = 0;
	uint32_t i;
	uint32_t primask = 0;
	targetGetInterruptState(primask);

	// Check if the buffer has space, dont write data if its going to overrun.
	if (fifo16SpaceAvailable(fifo) >= length)
	{
		targetDisableInterrupts();
		for (i = 0; i < length; i++)
		{
		  fifo->data[fifo->wrIdx] = *data++;
		  fifo->wrIdx = (fifo->wrIdx + 1) & fifo->sizeMask;
		}
		targetRestoreInterruptState(primask);
	}
	else {
		ret = -1;
	}

	return ret;
}


int fifo16WriteBuffer8(fifo16_t *fifo, const uint8_t *data, uint8_t length) {
	int ret = 0;
	uint32_t i;
	uint32_t primask = 0;
	targetGetInterruptState(primask);

	// Check if the buffer has space, dont write data if its going to overrun.
	if (fifo16SpaceAvailable(fifo) >= length) {
		targetDisableInterrupts();
		for (i = 0; i < length; i++)
		{
			fifo->data[fifo->wrIdx] = *data++;
			fifo->wrIdx = (fifo->wrIdx + 1) & fifo->sizeMask;
		}
		targetRestoreInterruptState(primask);
	}
	else {
		ret = -1;
	}

	return ret;
}


uint16_t fifo16Read(fifo16_t *fifo) {
	uint16_t data = 0;
	uint32_t primask = 0;
	targetGetInterruptState(primask);
	targetDisableInterrupts();

	//Check if the fifo has bytes to read
	if (fifo->rdIdx != fifo->wrIdx) {
		data = fifo->data[fifo->rdIdx];
		fifo->rdIdx = (fifo->rdIdx + 1) & fifo->sizeMask;
	}

	targetRestoreInterruptState(primask);
	return data;
}


int fifo16ReadA(fifo16_t *fifo, uint16_t *data) {
	int rval = -1;
	uint32_t primask = 0;
	targetGetInterruptState(primask);
	targetDisableInterrupts();

	if( fifo->rdIdx != fifo->wrIdx ) {
		*data = fifo->data[fifo->rdIdx];
		fifo->rdIdx = (fifo->rdIdx + 1) & fifo->sizeMask;
		rval = 0;
	}

	targetRestoreInterruptState(primask);
	return rval;
}


int fifo16ReadBuffer(fifo16_t *fifo, uint16_t *data, uint16_t length) {
	int rval = 0;
	int readCount;
	uint32_t primask = 0;

	targetGetInterruptState(primask);
	targetDisableInterrupts();

	// Check if there are enough bytes in the buffer, read the minimum of requested vs available bytes.
	readCount = (((fifo->wrIdx - fifo->rdIdx) & fifo->sizeMask) <= length)? ((fifo->wrIdx - fifo->rdIdx) & fifo->sizeMask) : length;

	while (readCount-- > 0)
	{
		*data++ = fifo->data[fifo->rdIdx];
		fifo->rdIdx = (fifo->rdIdx + 1) & fifo->sizeMask;
		rval++;
	}

	targetRestoreInterruptState(primask);
	return rval;
}



