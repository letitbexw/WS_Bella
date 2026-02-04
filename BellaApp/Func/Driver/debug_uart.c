/*
 * debug_uart.c
 *
 *  Created on: Jan 10, 2026
 *      Author: xiongwei
 */


#include "config.h"
#include "bsp.h"
#include "timers.h"
#include "debug_uart.h"


#define UART_TIMEOUT_VALUE  ((uint32_t) 22000)
#define UART_DMA_SIZE  		64

fifo8_t uartTxFifo;
fifo8_t uartRxFifo;
uint8_t hostTxBuf[CNFG_TEST_UART_TX_BUF_SIZE];
uint8_t hostRxBuf[CNFG_TEST_UART_RX_BUF_SIZE];
uint8_t hostRxTemp;


UART_HandleTypeDef huartDbg;
DMA_HandleTypeDef hdma_Dbg_tx;

static uint32_t tLastEvent = 0;


void uartDebugInit(void)
{
	__HAL_RCC_DMA1_CLK_ENABLE();
	HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, DEBUG_UART_IRQ_PRIORITY, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

	huartDbg.Instance 						= DEBUG_UART_BASE_PTR;
	huartDbg.Init.BaudRate 					= DEBUG_UART_RATE;
	huartDbg.Init.WordLength 				= UART_WORDLENGTH_8B;
	huartDbg.Init.StopBits 					= UART_STOPBITS_1;
	huartDbg.Init.Parity 					= UART_PARITY_NONE;
	huartDbg.Init.Mode 						= UART_MODE_TX_RX;
	huartDbg.Init.HwFlowCtl 				= UART_HWCONTROL_NONE;
	huartDbg.Init.OverSampling 				= UART_OVERSAMPLING_8;
	huartDbg.Init.OneBitSampling 			= UART_ONE_BIT_SAMPLE_DISABLE;
	huartDbg.Init.ClockPrescaler 		 	= UART_PRESCALER_DIV1;
	huartDbg.AdvancedInit.AdvFeatureInit 	= UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huartDbg) != HAL_OK) { Error_Handler(); }

	/* USART4 interrupt Init */
	HAL_NVIC_SetPriority(DEBUG_UART_IRQn, DEBUG_UART_IRQ_PRIORITY, 0);
	HAL_NVIC_EnableIRQ(DEBUG_UART_IRQn);

	fifo8Init(&uartTxFifo, hostTxBuf, CNFG_TEST_UART_TX_BUF_SIZE);
	fifo8Init(&uartRxFifo, hostRxBuf, CNFG_TEST_UART_RX_BUF_SIZE);

	HAL_UART_Receive_IT(&huartDbg, &hostRxTemp, 1);
}

void uartDebugMspInit(void)
{
	/* Peripheral clock enable */
	__HAL_RCC_USART4_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	initGPIO(USART_DBG, GPIO_MODE_AF_PP, GPIO_NOPULL, 0, GPIO_AF4_USART4);		/* PA0/PA1, DBG_TX/DBG_RX */
	hdma_Dbg_tx.Instance 					= DMA1_Channel3;
	hdma_Dbg_tx.Init.Request 				= DMA_REQUEST_USART4_TX;
	hdma_Dbg_tx.Init.Direction 				= DMA_MEMORY_TO_PERIPH;
	hdma_Dbg_tx.Init.PeriphInc 				= DMA_PINC_DISABLE;
	hdma_Dbg_tx.Init.MemInc 				= DMA_MINC_ENABLE;
	hdma_Dbg_tx.Init.PeriphDataAlignment 	= DMA_PDATAALIGN_BYTE;
	hdma_Dbg_tx.Init.MemDataAlignment 		= DMA_MDATAALIGN_BYTE;
	hdma_Dbg_tx.Init.Mode 					= DMA_NORMAL;
	hdma_Dbg_tx.Init.Priority 				= DMA_PRIORITY_LOW;
	if (HAL_DMA_Init(&hdma_Dbg_tx) != HAL_OK) { Error_Handler(); }

	__HAL_LINKDMA(&huartDbg,hdmatx,hdma_Dbg_tx);
}

void uartDebugMspDeInit(void)
{
	/* Peripheral clock disable */
	__HAL_RCC_USART4_CLK_DISABLE();
	HAL_GPIO_DeInit(USART_DBG);

	/* USART4 DMA DeInit */
	HAL_DMA_DeInit(huartDbg.hdmatx);
}

void uartDebugTxDmaStart()
{
	if (!uartTxFifo.dmaBusy)
	{
		if (!fifo8Empty(&uartTxFifo))
		{
			uint16_t txLength;
			uint8_t *pData = fifo8ReadBufferInPlaceLimited(&uartTxFifo, &txLength, UART_DMA_SIZE);
			if (HAL_UART_Transmit_DMA(&huartDbg, pData, txLength) == HAL_OK)
			{
				uartTxFifo.dmaBusy = true;
			}
			else
			{
				uartTxFifo.dmaBusy = false;
			}
		}
	}
}

void uartDebugTxDmaComplete()
{
	if (hdma_Dbg_tx.State == HAL_DMA_STATE_READY)
	{
		// This moves the rdIdx to the rdMarkIdx
		fifo8ReadBufferInPlaceUpdate(&uartTxFifo);
		uartTxFifo.dmaBusy = false;

		// Start another transfer if possible
		uartDebugTxDmaStart();
	}
	else
		return;
}

void uartDebugRxComplete()
{
	fifo8Write(&uartRxFifo, hostRxTemp);				// Put the char into queue
	HAL_UART_Receive_IT(&huartDbg, &hostRxTemp, 1);		// Ready to receive the next char
}

void uartDebugRxError()
{
	uint32_t error = HAL_UART_GetError(&huartDbg);
	if ((error & HAL_UART_ERROR_FE) != 0)	// Frame Error
	{
		// start new character read (will clear errors and return to Rx mode)
		HAL_UART_Receive_IT(&huartDbg, &hostRxTemp, 1);
	}
	if ((error & HAL_UART_ERROR_ORE) != 0)	// Overrun error
	{
		// start new character read (will clear errors)
		HAL_UART_Receive_IT(&huartDbg, &hostRxTemp, 1);
	}
}


bool testUartDebugIsIdle(uint32_t timeMs)
{
	return (GetTickCount() - tLastEvent) > timeMs;
}

bool uartDebugTxByte(fifo8_t* txFifo, uint8_t c)
{
	if (fifo8Write(txFifo, c) == 0)
	{
		uint8_t state = HAL_UART_GetState(&huartDbg);
		if ((state != HAL_UART_STATE_BUSY_TX) && (state != HAL_UART_STATE_BUSY_TX_RX))
		{
			// Start uart DMA if new line or a min number of bytes are in the fifo
			int cnt = fifo8Count(txFifo);
			if (((c == '\n') && (cnt>1))|| (cnt >= UART_DMA_SIZE))
			{
				uartDebugTxDmaStart();
			}
		}
		tLastEvent = GetTickCount();
		return true;
	}
	else
		return false;
}

bool uartDebugRxByte(fifo8_t* fifo, uint8_t* c)
{
	bool rv = 0;
	rv = fifo8ReadA(fifo, c) == 0;
	if (rv) { tLastEvent = GetTickCount(); }
	return rv;
}

void uartDebugFlush()
{
	while (!fifo8Empty(&uartTxFifo) || (HAL_UART_GetState(&huartDbg) > HAL_UART_STATE_READY)) { tmrDelay_ms(1); }
	tmrDelay_ms(5);
}

