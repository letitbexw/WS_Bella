/*
 * debug_uart.h
 *
 *  Created on: Jan 10, 2026
 *      Author: xiongwei
 */

#ifndef DRIVER_DEBUG_UART_H_
#define DRIVER_DEBUG_UART_H_


#include "main.h"
#include "fifo8.h"
#include <stdbool.h>

extern fifo8_t uartRxFifo;
extern fifo8_t uartTxFifo;

void uartDebugInit(void);
void uartDebugMspInit(void);
void uartDebugMspDeInit(void);

void uartDebugRxComplete();
void uartDebugRxError();
void uartDebugTxDmaStart();
void uartDebugTxDmaComplete();

bool testUartDebugIsIdle(uint32_t timeMs);
bool uartDebugTxByte(fifo8_t* txFifo, uint8_t c);
bool uartDebugRxByte(fifo8_t* fifo, uint8_t* c);
void uartDebugFlush();


#define uartRxBufferFull()           (fifo8Full(&uartRxFifo))
#define uartRxBufferFreeSpace()      (fifo8SpaceAvailable(&uartRxFifo))
#define uartRxBufferCount()          (fifo8Count(&uartRxFifo))
#define uartRxBufferEmpty()          (fifo8Empty(&uartRxFifo))
#define uartRxReadBuffer()           (fifo8Read(&uartRxFifo))

#define uartTxBufferFull()           (fifo8Full(&uartTxFifo))
#define uartTxBufferFreeSpace()      (fifo8SpaceAvailable(&uartTxFifo))
#define uartTxBufferCount()          (fifo8Count(&uartTxFifo))
#define uartTxBufferEmpty()          (fifo8Empty(&uartTxFifo))
#define uartTxReadBuffer()           (fifo8Read(&uartTxFifo))

#define uartRxBufferReset()          fifo8Reset(&uartRxFifo)
#define uartTxBufferReset()          fifo8Reset(&uartTxFifo)


#endif /* DRIVER_DEBUG_UART_H_ */
