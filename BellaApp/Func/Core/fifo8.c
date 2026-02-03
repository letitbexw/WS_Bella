/*
 * fifo8.c
 *
 *  Created on: Jan 10, 2026
 *      Author: xiongwei
 */


#include "target.h"
#include "fifo8.h"

// Initialize a FIFO
void fifo8Init(fifo8_t * fifo, uint8_t * buf, uint16_t size)
{
    fifo->data 		= buf;
    fifo->sizeMask 	= size - 1;
    fifo->rdIdx 	= fifo->wrIdx = fifo->rdMarkIdx = 0;
    fifo->dmaBusy 	= false;
}

int fifo8Count(fifo8_t *fifo)
{
	int rval;
	int32_t r = fifo->rdIdx;
	int32_t w = fifo->wrIdx;

	rval 	 = (w - r) & fifo->sizeMask;

	return rval;
}

int fifo8SpaceAvailable(fifo8_t *fifo)
{
	int rval;
	int32_t r = fifo->rdIdx;
	int32_t w = fifo->wrIdx;

	rval 	  = (w - r) & fifo->sizeMask;
	rval 	  = fifo->sizeMask - rval;
	return rval;
}

int fifo8Full(fifo8_t *fifo)  	{ return ((fifo->wrIdx + 1) & fifo->sizeMask) == fifo->rdIdx; }
int fifo8Empty(fifo8_t *fifo) 	{ return fifo->rdIdx == fifo->wrIdx; }
void fifo8Reset(fifo8_t *fifo) 	{ fifo->rdIdx = fifo->wrIdx = 0; }

int fifo8Write(fifo8_t *fifo, uint8_t data)
{
	// Write one byte to FIFO
	int ret = 0;

	// Check if the buffer has space, dont write data if its going to overrun.
	if (((fifo->wrIdx + 1) & fifo->sizeMask) != fifo->rdIdx)
	{
		fifo->data[fifo->wrIdx] = data;
		fifo->wrIdx = (fifo->wrIdx + 1) & fifo->sizeMask;
	}
	else { ret = -1; }

	return ret;
}

int fifo8WriteBuffer(fifo8_t *fifo, const uint8_t *data, uint16_t length)
{
	int ret = 0;
	uint32_t i;
	uint32_t primask = 0;

	// Check if the buffer has space, dont write data if its going to overrun.
	if (fifo8SpaceAvailable(fifo) >= length)
	{
		targetGetInterruptState(primask);
		targetDisableInterrupts();
		for (i = 0; i < length; i++)
		{
			fifo->data[fifo->wrIdx] = *data++;
			fifo->wrIdx = (fifo->wrIdx + 1) & fifo->sizeMask;
		}
		targetRestoreInterruptState(primask);
	}
	else { ret = -1; }

	return ret;
}

uint8_t fifo8Read(fifo8_t *fifo)
{
	// Read one byte from FIFO and return this byte
	uint8_t data = 0;
	uint32_t primask = 0;

	targetGetInterruptState(primask);
	targetDisableInterrupts();

	//Check if the fifo has bytes to read
	if (fifo->rdIdx != fifo->wrIdx)
	{
		data = fifo->data[fifo->rdIdx];
		fifo->rdIdx = (fifo->rdIdx + 1) & fifo->sizeMask;
		fifo->rdMarkIdx = fifo->rdIdx;
	}
	targetRestoreInterruptState(primask);
	return data;
}


int fifo8ReadA(fifo8_t *fifo, uint8_t *data)
{
	// Read one byte from FIFO to a buffer
	int rval = -1;
	uint32_t primask = 0;

	targetGetInterruptState(primask);
	targetDisableInterrupts();

	if( fifo->rdIdx != fifo->wrIdx )
	{
		*data = fifo->data[fifo->rdIdx];
		fifo->rdIdx = (fifo->rdIdx + 1) & fifo->sizeMask;
		fifo->rdMarkIdx = fifo->rdIdx;
		rval = 0;
	}
	targetRestoreInterruptState(primask);
	return rval;
}


int fifo8ReadBuffer(fifo8_t *fifo, uint8_t *data, uint16_t length)
{
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
	fifo->rdMarkIdx = fifo->rdIdx;

	targetRestoreInterruptState(primask);
	return rval;
}


uint8_t* fifo8ReadBufferInPlace(fifo8_t *fifo, uint16_t *length)
{
	// Read data, but will not update rdIdx
	uint8_t* rval = 0;
	uint32_t primask = 0;

	targetGetInterruptState(primask);
	targetDisableInterrupts();

	fifo->rdIdx = fifo->rdMarkIdx;
	rval = &fifo->data[fifo->rdIdx];
	if ((fifo->rdIdx + fifo8Count(fifo)) > fifo->sizeMask)
	{
		// fifo will wrap, length is to end of buffer
		*length = fifo->sizeMask - fifo->rdIdx + 1;
		fifo->rdMarkIdx = 0;
	}
	else
	{
		// fifo won't wrap, length is to end of data
		*length = fifo8Count(fifo);
		fifo->rdMarkIdx = fifo->rdIdx + *length;
	}
	targetRestoreInterruptState(primask);
	return rval;
}

uint8_t* fifo8ReadBufferInPlaceLimited(fifo8_t *fifo, uint16_t *length, uint16_t maxLength)
{
	// Read data, but will not update rdIdx
	uint8_t* rval = 0;
	uint16_t readCount = 0;
	uint32_t primask = 0;

	targetGetInterruptState(primask);
	targetDisableInterrupts();

	readCount = fifo8Count(fifo) > maxLength ? maxLength : fifo8Count(fifo);
	rval = &fifo->data[fifo->rdIdx];
	if ((fifo->rdIdx + readCount) > fifo->sizeMask)
	{
		// fifo will wrap, length is to end of buffer
		*length = fifo->sizeMask - fifo->rdIdx + 1;
		fifo->rdMarkIdx = 0;
	}
	else
	{
		// fifo won't wrap, length is to end of data
		*length = readCount;
		fifo->rdMarkIdx = fifo->rdIdx + *length;
	}
	targetRestoreInterruptState(primask);
	return rval;
}


void fifo8ReadBufferInPlaceUpdate(fifo8_t *fifo) { fifo->rdIdx = fifo->rdMarkIdx; }
