/*
 * idbus_uart.h
 *
 *  Created on: Feb 3, 2026
 *      Author: xiongwei
 */

#ifndef DRIVER_IDBUS_UART_H_
#define DRIVER_IDBUS_UART_H_


#include "fifo16.h"
#include "stm32g0xx_hal.h"


extern fifo16_t idbusRxFifo;
extern fifo16_t idbusTxFifo;

extern volatile uint32_t idbusTimeOfLastActivityOnTheBus;

#define IDBUS_WAKE      0xFFCC    // magic token in fifo representing 'wake' character
#define IDBUS_BREAK     0xFFFF    // magic token in fifo representing 'break' character

#define idbusSendBreak()        idbusDriveBusLow(12)
#define idbusSendWake()         idbusDriveBusLow(22)
#define idbusSendDebugWake()    idbusDriveBusLow(150)


//Function prototypes

void idbusUartInit(UART_HandleTypeDef *uartHandle, uint32_t baud);
void idbusMspInit(UART_HandleTypeDef *huart);
void idbusMspDeInit(UART_HandleTypeDef *huart);

void idbusCaptureIDIO(uint8_t enable);

uint8_t idbusWaitIdle(void);
uint8_t idbusIsIdle(void);
int idbusRx(uint16_t *x);
int idbusTx(uint16_t x);
int idbusTxBuffer(const uint8_t* x, uint8_t length);
int idbusRxCount(void);

void idbusDriveBusLow(uint32_t low_us);
void idioSetSlaveIDResponseCallback(void * fp);

uint32_t idbusUartIrqHandler(UART_HandleTypeDef *huart);


#endif /* DRIVER_IDBUS_UART_H_ */
