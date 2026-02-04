/*
 * idbus_uart.c
 *
 *  Created on: Feb 3, 2026
 *      Author: xiongwei
 */

#include "config.h"
#include "bsp.h"
#include "timers.h"
#include "idbus_uart.h"
#include "idio_bulk_data.h"
//#include "debug.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

//#define USE_HAL

#define UART_TIMEOUT_VALUE      ((uint32_t) 22000)
#define UART_CR1_FIELDS  		((uint32_t)(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS | USART_CR1_TE | USART_CR1_RE | USART_CR1_OVER8))

#define AID_ONEZEROTHRESH   	4    	/* # of bits zero to distinguish 1 from 0 */
#define AID_ONEZEROCHAR     	0xF0 	/* char value to distinguish 1 from 0 */
#define AID_ZEROCHAR        	0xC0  	/* tx for zero (7us) */
#define AID_ONECHAR         	0xFE  	/* tx for one */

#define HAL_UART_STATE_BIT_TX  	0x10
#define HAL_UART_STATE_BIT_RX  	0x20

#define IDBUS_IDLE_TIMEOUT     	10  // ms

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint16_t idbusUartRxBuffer[CNFG_IDBUS_UART_RX_BUF_SIZE];
uint16_t idbusUartTxBuffer[CNFG_IDBUS_UART_TX_BUF_SIZE];

fifo16_t idbusRxFifo;
fifo16_t idbusTxFifo;

UART_HandleTypeDef* idioUartHandle;

static uint8_t idbusTmpRxBuff[8];
static uint8_t idbusTmpTxBuff[8];
static volatile uint8_t txIdle = true;    		// sending data or break / wake
static volatile uint8_t delayTxIdle = true;    	// sending data or break / wake
                                            	// this is delayed by 1 Rx character to account for the
                                              	//  Tx holding register that skews Rx from Tx by about a byte
static volatile uint8_t rxSniff = false;  		// put Rx data into ATS queue
static volatile uint8_t rxProcess = true; 		// put Rx data into IDIO queue
static volatile uint8_t rxReProcess = false; 	// put Rx data into IDIO queue, if transmitting

/* Private functions ---------------------------------------------------------*/

volatile uint32_t idbusTimeOfLastActivityOnTheBus;
static volatile uint16_t idbusIdle = true;

#define INTERCHAR_DELAY_CHAR_COUNT 	8
#define INTERCHAR_DLEAY_US         	10
static uint8_t idbusInterCharCount = INTERCHAR_DELAY_CHAR_COUNT;

/* Flag for collision detection */
volatile uint8_t collision = 0;

#if 0
static uint16_t idbusTimeoutEventHandler(void)
{
	idbusIdle = true;
	return(0);
}

static void idbusSetTimeout(uint16_t num_ms)
{
	//uint32_t primask;
	static tmrFuncType tmrIdbusTimeout = {NULL, 0, idbusTimeoutEventHandler};
	//targetGetInterruptState(primask);
	//targetDisableInterrupts();

	if (tmrIdbusTimeout.mS == 0){
		tmrFuncAdd(&tmrIdbusTimeout);
	}
	tmrIdbusTimeout.mS = num_ms;
	idbusIdle = false;

	//targetRestoreInterruptState(primask);
}
#endif

__STATIC_INLINE void idbusSetTimeOfLastActivityOnTheBus(void) { idbusTimeOfLastActivityOnTheBus = GetTickCount(); }

/**
  * @brief  Sends an amount of data in non blocking mode.
  * @param  huart: UART handle
  * @param  pData: Pointer to data buffer
  * @param  Size: Amount of data to be sent
  * @retval HAL status
  */
HAL_StatusTypeDef idbusTransmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
	if((pData == NULL ) || (Size == 0)) 		{ return HAL_ERROR; }
	if (huart->gState != HAL_UART_STATE_READY) 	{ return HAL_BUSY; }

	huart->gState 		= HAL_UART_STATE_BUSY_TX;
	huart->pTxBuffPtr 	= pData;
	huart->TxXferSize 	= Size;
	huart->TxXferCount 	= Size;


	/* Disable the UART Transmit Complete Interrupt */
	__HAL_UART_DISABLE_IT(huart, UART_IT_TC);
	huart->Instance->ICR |= USART_ICR_TCCF;
	/* Enable the UART Transmit Empty Interrupt. This kicks off an idle Tx */
	__HAL_UART_ENABLE_IT(huart, UART_IT_TXE);

	return HAL_OK;
}

/**
  * @brief  Receives an amount of data in non blocking mode
  * @param  huart: UART handle
  * @param  pData: Pointer to data buffer
  * @param  Size: Amount of data to be received
  * @retval HAL status
  */
HAL_StatusTypeDef idbusReceive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
	if((pData == NULL ) || (Size == 0)) 		{ return HAL_ERROR; }
	if (huart->RxState != HAL_UART_STATE_READY) { return HAL_BUSY; }

	huart->RxState 		= HAL_UART_STATE_BUSY_RX;
	huart->pRxBuffPtr 	= pData;
	huart->RxXferSize 	= Size;
	huart->RxXferCount 	= Size;

	/* Enable the UART Parity Error Interrupt */
	__HAL_UART_ENABLE_IT(huart, UART_IT_PE);
	/* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
	__HAL_UART_ENABLE_IT(huart, UART_IT_ERR);
	/* Enable the UART Data Register not empty Interrupt */
	__HAL_UART_ENABLE_IT(huart, UART_IT_RXNE);
	return HAL_OK;
}


void idbusCaptureIDIO(uint8_t enable) { rxProcess = enable; }


// wait for Tx idle
uint8_t idbusWaitIdle(void)
{
	uint32_t safe = -1;
	while (!txIdle && (safe-- != 0))
	{
		//__WFI();
		__NOP();
	}
	return (safe != 0);
}

uint8_t idbusIsIdle(void) { return txIdle && idbusIdle; }

int idbusRxCount(void) { return (fifo16Count(&idbusRxFifo)); }


int idbusRx(uint16_t *x)
{
	if (!fifo16Empty(&idbusRxFifo))
	{
		*x = fifo16Read(&idbusRxFifo);
		return 1;
	}
	return 0;
}


// tx buffer completed
//  terminate if FIFO empty and switch to read mode
//  else queue next byte as 8 characters
// this function called when last bit has cleared Tx
void idbusUARTTxStart(UART_HandleTypeDef* huart)
{
	uint8_t bit;
	uint16_t txByte;

	txIdle = false;

	txByte = fifo16Read(&idbusTxFifo);

	for (bit = 0; bit < 8; bit++)
	{
		idbusTmpTxBuff[bit] = (txByte & 1) ? AID_ONECHAR : AID_ZEROCHAR;
		txByte >>= 1;
	}
	// start transmitting block of data
	idbusTransmit(huart, &idbusTmpTxBuff[0], 8);
}

// terminate tx (usually called from Rx break)
//  terminate transmission
//  flush tx buffer
static void idbusUARTTxStop(UART_HandleTypeDef* huart)
{
	// stop transmitter
	huart->pTxBuffPtr = NULL;
	huart->TxXferSize = 0;
	huart->TxXferCount = 0;

	/* Disable the UART Transmit Complete and Tx Empty Interrupt */
	__HAL_UART_DISABLE_IT(huart, UART_IT_TC);
	__HAL_UART_DISABLE_IT(huart, UART_IT_TXE);
	if (huart->gState == HAL_UART_STATE_BUSY_TX) { huart->gState = HAL_UART_STATE_READY; }

	txIdle = true;

	// flush FIFO
	fifo16Reset(&idbusTxFifo);
	idbusSetTimeOfLastActivityOnTheBus();
}

// tx buffer completed
//  terminate if FIFO empty and switch to read mode
//  else queue next byte as 8 characters
// this function called when last bit has cleared Tx
static __inline void idbusUARTTxComplete(UART_HandleTypeDef* huart)
{
	if (fifo16Empty(&idbusTxFifo))
	{  	/* no bytes left, transmitter idle */
		txIdle = true;
	}
	else
	{
		//tIBT Inter-Byte Time should be atleast 20uS, this includes the last cycle time.
		//  since the last bit has cleared the FIFO, the processing time here makes up the
		//  time. Measured at about 40us from start of last bit to start of first bit.

		if (--idbusInterCharCount == 0)		// A byte was transmitted
		{
			tmrDelay_us(10);
			idbusInterCharCount = INTERCHAR_DELAY_CHAR_COUNT;
		}
		idbusUARTTxStart(huart);
	}
}


int idbusTx(uint16_t x)
{
	int ret = 0;
	// @note: assumes that a higher level process controls keeping the device in half
	//  duplex mode
	if (fifo16Write(&idbusTxFifo, x) == 0)
	{
		//DEBUG_PRINT_IDIO__("TxQ %x", x);

		// kick the transmitter if not running
		if (txIdle) { idbusUARTTxStart(idioUartHandle); }
	}
	else { ret = -1; }
	return ret;
}



int idbusTxBuffer(const uint8_t* x, uint8_t length)
{
	int ret = 0;
	// @note: assumes that a higher level process controls keeping the device in half
	//  duplex mode
	if (fifo16WriteBuffer8(&idbusTxFifo, x, length) == 0)
	{
		//DEBUG_PRINT_IDIO__("TxQB %x", x[0]);

		// kick the transmitter if not running
		if (txIdle)
		{
			idbusInterCharCount = INTERCHAR_DELAY_CHAR_COUNT;
			idbusUARTTxStart(idioUartHandle);
		}
	}
	else { ret = -1; }
	return ret;
}



// rx buffer complete
//  received 8 transition bits, parse into a character and queue
static __inline void idbusUARTRxComplete(UART_HandleTypeDef* huart)
{
	uint8_t bit;
	uint16_t rxByte;

	// process buffer
	rxByte = 0;
	for (bit = 0; bit < 8; bit++)
	{
		rxByte = (rxByte >> 1) | (((idbusTmpRxBuff[bit] & AID_ONEZEROCHAR) == AID_ONEZEROCHAR) ? 0x80 : 0);
	}
	if (rxProcess && (rxReProcess || delayTxIdle))
	{
		fifo16Write(&idbusRxFifo, rxByte);
		rxReProcess = false;
	}
	delayTxIdle = txIdle;

	// start next read into buffer
	idbusReceive(huart, &idbusTmpRxBuff[0], 8);
}


/**
  * @brief  Sends an amount of data in non blocking mode.
  * @param  huart: UART handle
  * @retval HAL status
  */
__inline static HAL_StatusTypeDef idbusTxNext(UART_HandleTypeDef *huart)
{
	if(huart->TxXferCount > 0)
	{
		huart->Instance->TDR = *huart->pTxBuffPtr++;
		huart->TxXferCount--;
		if(huart->TxXferCount == 0)
		{
			/* Disable the UART Transmit Empty Interrupt */
			//huart->Instance->RQR |= USART_RQR_TXFRQ;
			__HAL_UART_DISABLE_IT(huart, UART_IT_TXE);
			/* Enable the UART Transmit Complete Interrupt */
			__HAL_UART_ENABLE_IT(huart, UART_IT_TC);
		}
	}
	return HAL_OK;
}


/**
  * @brief  Receives an amount of data in non blocking mode
  * @param  huart: UART handle
  * @retval HAL status
  */
__inline static HAL_StatusTypeDef idbusRxHandler(UART_HandleTypeDef *huart)
{
	*huart->pRxBuffPtr++ = (uint8_t)(huart->Instance->RDR & (uint8_t)0x00FF);
	if(--huart->RxXferCount == 0)
	{
		uint8_t retry = 3;
		while(retry-- && huart->Instance->ISR & UART_FLAG_RXNE);
		__HAL_UART_DISABLE_IT(huart, UART_IT_RXNE);
		/* Disable the UART Parity Error Interrupt */
		__HAL_UART_DISABLE_IT(huart, UART_IT_PE);
		/* Disable the UART Error Interrupt: (Frame error, noise error, overrun error) */
		//__HAL_UART_DISABLE_IT(huart, UART_IT_ERR);
		CLEAR_BIT(huart->Instance->CR3, USART_CR3_EIE);
		huart->RxState = HAL_UART_STATE_READY;

		idbusUARTRxComplete(huart);
	}
	return HAL_OK;
}

/**
  * @brief  This function handles UART interrupt request.
  * @param  huart: UART handle
  * @retval None
  *
  * assumes all interrupts are enabled
  */
uint32_t idbusUartIrqHandler(UART_HandleTypeDef *huart)
{
	uint32_t flags = 0;

	// handle Tx Irqs only if we are sending (register bits are not gated by TxIE)
	if (!txIdle)
	{
		// refetch flags to speed up Tx processing
		flags = READ_REG(huart->Instance->ISR);
		/* UART in mode Transmitter (character sent)--------------------------------*/
		if ((flags & USART_ISR_TC) != 0)
		{
			WRITE_REG(huart->Instance->ICR, USART_ICR_TCCF);
			/* Disable the UART Transmit Complete Interrupt */
			__HAL_UART_DISABLE_IT(huart, UART_IT_TC);
			__HAL_UART_DISABLE_IT(huart, UART_IT_TXE);
			/* Call the Tx callback API to give the possibility to
			   start again the Transmission under the Tx callback API */
			idbusUARTTxComplete(huart);
		}
		/* UART in mode Transmitter (Tx Empty)     ---------------------------------*/
		else if ((flags & UART_FLAG_TXE) != 0)
		{
			idbusTxNext(huart);
		}
		idbusSetTimeOfLastActivityOnTheBus();
	}

	// refetch flags to speed up Rx processing
	flags = READ_REG(huart->Instance->ISR);
	/* UART noise error interrupt occurred -------------------------------------*/
	/* UART frame error interrupt occurred -------------------------------------*/
	/* UART overrun error interrupt occurred -----------------------------------*/
	if ((flags & (USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE | USART_ISR_PE)) != 0)
	{
		if ((flags & (USART_ISR_FE)) != 0)
		{
			// clear the framing error
			WRITE_REG(huart->Instance->ICR, USART_ICR_FECF);
			// clear Rx pending bit to drop charagter
			WRITE_REG(huart->Instance->RQR, USART_RQR_RXFRQ);
			// @TODO: receiving break should stop transmission, if in progress
			// @TODO: differentiate break and wake

			// receiving break should stop transmission, if in progress
			if (!txIdle)
			{
				idbusUARTTxStop(huart);
				collision 	= 1;  	/* Set collision flag to 1 here */
				rxReProcess = true; /* Set to True to put the Rx Data into IDIO Queue as X533c  is transmitting data and collision has occurred . See <rdar://problem/55049140> */
			}
			idioBulkDataClearReadPendingFlag();

			if (rxProcess && txIdle)
				fifo16Write(&idbusRxFifo, IDBUS_BREAK);
		}
		else if ((flags & (UART_FLAG_ORE)) != 0)
		{
			// clear the framing error
			WRITE_REG(huart->Instance->ICR, USART_ICR_ORECF);
			// clear Rx pending bit to drop charagter
			WRITE_REG(huart->Instance->RQR, USART_RQR_RXFRQ);
		}
		else if ((flags & USART_ISR_PE) != 0U)
        {
            WRITE_REG(huart->Instance->ICR, USART_ICR_PECF);
        }

		// start new character read (will clear errors and return to Rx mode)
		idbusReceive(huart, &idbusTmpRxBuff[0], 8);
		idbusSetTimeOfLastActivityOnTheBus();
	}
	/* UART in mode Receiver ---------------------------------------------------*/
	// receiving break / error and data are mutually exclusive
	else if ((flags & UART_FLAG_RXNE) != 0)
	{
		idbusRxHandler(huart);
		idbusSetTimeOfLastActivityOnTheBus();
		// @TODO: collision detect
	}
	return flags;
}


// @TODO: review if this should be done via UART
//  delay is very short, and there are no other tasks
void idbusDriveBusLow(uint32_t low_us)
{
	//DEBUG_PRINT_IDIO__("Wake %lu", low_us);
	// Synchronously wait for whatever's in the TX fifo to drain out
	idbusWaitIdle();
	txIdle = false;

	HAL_NVIC_DisableIRQ(ORION_UART_IRQn);
	//convert to GPIO
	initGPIO(ACC_AID_TX, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_PIN_SET, GPIO_AF4_USART3);
	tmrDelay_us(5);
	HAL_GPIO_WritePin(ACC_AID_TX, GPIO_PIN_RESET);
	tmrDelay_us(low_us);
	initGPIO(ACC_AID_TX, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_PIN_SET, GPIO_AF4_USART3);
	HAL_NVIC_EnableIRQ(ORION_UART_IRQn);

	txIdle = true;
}


void idbusUartInit(UART_HandleTypeDef* huart)
{
	idioUartHandle = huart;
	txIdle = true;

	delayTxIdle = true;
	rxSniff = false;
	rxProcess = true;

	huart->Init.BaudRate 				= ORION_UART_RATE;
	huart->Init.WordLength 				= UART_WORDLENGTH_8B;
	huart->Init.StopBits 				= UART_STOPBITS_1;
	huart->Init.Parity 					= UART_PARITY_NONE;
	huart->Init.Mode 					= UART_MODE_TX_RX;
	huart->Init.HwFlowCtl 				= UART_HWCONTROL_NONE;
	huart->Init.OverSampling 			= UART_OVERSAMPLING_8;
	huart->Init.OneBitSampling 			= UART_ONE_BIT_SAMPLE_DISABLE;
	huart->Init.ClockPrescaler 			= UART_PRESCALER_DIV1;
	huart->AdvancedInit.AdvFeatureInit 	= UART_ADVFEATURE_NO_INIT;
	HAL_UART_Init(huart);

	// enable ONEBIT to disable noise flag interrupt
	__HAL_UART_DISABLE(huart);
	SET_BIT(huart->Instance->CR3, USART_CR3_ONEBIT);
	__HAL_UART_ENABLE(huart);

	fifo16Init(&idbusTxFifo, &idbusUartTxBuffer[0], CNFG_IDBUS_UART_TX_BUF_SIZE);
	fifo16Init(&idbusRxFifo, &idbusUartRxBuffer[0], CNFG_IDBUS_UART_RX_BUF_SIZE);

	// start new character read (will clear errors)
	idbusReceive(idioUartHandle, &idbusTmpRxBuff[0], 8);
}


void idbusMspInit(void)
{
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
	/** Initializes the peripherals clocks */
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART3;
	PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_SYSCLK;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) { Error_Handler(); }

	/* Peripheral clock enable */
	__HAL_RCC_USART3_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	initGPIO(ACC_AID_TX, GPIO_MODE_AF_PP, GPIO_NOPULL, 0, GPIO_AF4_USART3); /* PB8,  ACC_AID_TX */
	initGPIO(ACC_AID_RX, GPIO_PULLUP, GPIO_NOPULL, 0, GPIO_AF4_USART3); 	/* PB9,  ACC_AID_RX */
	HAL_NVIC_SetPriority(ORION_UART_IRQn, ORION_UART_IRQ_PRIORITY, 0);
	HAL_NVIC_EnableIRQ(ORION_UART_IRQn);
}

/**
  * @brief UART MSP De-Initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO, and NVIC configuration to their default state
  * @param huart: UART handle pointer
  * @retval None
  */
void idbusMspDeInit()
{
	__HAL_RCC_USART3_CLK_DISABLE();
	HAL_GPIO_WritePin(ACC_AID_TX, GPIO_PIN_SET);
	HAL_GPIO_DeInit(USART_ACC_AID);
	HAL_NVIC_DisableIRQ(ORION_UART_IRQn);
}




