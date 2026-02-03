/*
 * retarget.c
 *
 *  Created on: Jan 11, 2026
 *      Author: xiongwei
 */


#include "config.h"
#include "bsp.h"
#include "debug_uart.h"
#include "main.h"
#include "timers.h"

//#define fputc __io_putchar

struct __FILE { int handle; };
FILE __stdin, __stdout;
FILE *dummyFile = (FILE *)0;

int fputc(int c, FILE *f)
{
	static bool overflow = false;
	// If in overflow condition, just drop characters until the buffer has space
	if (overflow)
	{
		// Drop an indicator when recovering from overflow
		if (fifo8SpaceAvailable(&uartTxFifo) > 5)
		{
			// ZZ ...
			uartDebugTxByte(&uartTxFifo, '\n');
			uartDebugTxByte(&uartTxFifo, 'Z');
			uartDebugTxByte(&uartTxFifo, 'Z');
			uartDebugTxByte(&uartTxFifo, '\n');
			overflow = false;
		}
	}
	if (!overflow)
	{
		if (!uartDebugTxByte(&uartTxFifo, c))
		{
			overflow = true;
		}
	}
	return c;
}

int _write(int fd, char *ptr, int len)
{
	/* Write "len" of char from "ptr" to file id "fd"
	* Return number of char written.
	* Need implementing with UART here. */
	(void) fd;
	int ret = 0;
	while (len-- && *ptr)
	{
		fputc(*ptr, dummyFile);
		ptr++;
		ret++;
	}
	return ret;
}

void _ttywrch(int c)
{
	/* Write one char "c" to the default console */
	fputc('\n', dummyFile);
	fputc(c, dummyFile);
	fputc('\n', dummyFile);
}


