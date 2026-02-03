/*
 * idio.h
 *
 *  Created on: Jan 11, 2026
 *      Author: xiongwei
 */

#ifndef CORE_IDIO_H_
#define CORE_IDIO_H_


#include "config.h"
#include "aid.h"
#include "stm32g0xx_hal_conf.h"


typedef struct _IDIO_IF_PROP
{
	UART_HandleTypeDef* uartHandle;
	void    (*init)              (UART_HandleTypeDef* uartHandle, uint32_t baud);
	int     (*rx)                (uint16_t* x);
	int     (*tx)                (uint16_t x);
	int     (*txBuffer)          (const uint8_t* x, uint8_t length);
	int     (*rxCount)           (void);
	void    (*driveBusLow)       (uint32_t breakType);
#ifdef CNFG_AID_POWER_CONTROL
	void    (*setIfPower)        (uint8_t mode);
	uint8_t (*getIfPower)        (void);
#endif
#ifdef CNFG_AID_ID_VARIABLE_RESPONSE
	uint8_t* (*getId)            (uint16_t systemId);
#endif
} idio_IF_Prop_TypeDef;

extern uint32_t idioPacketCount, idioPacketErrorCount;

#define IDIO_RESPONSE_VALID         	0
#define IDIO_COMMAND_VALID          	1
#define IDIO_RESPONSE_NONE          	2
#define IDIO_RESPONSE_PARAM_ERROR   	3
#define IDIO_RESPONSE_LENGTH_ERROR  	4
#define IDIO_RESPONSE_UNKNOWN_ERROR 	5
#define IDIO_RESPONSE_OTHER_ERROR       128

// IDIO Configuration Defines
#define IDIO_PACKET_LEN_MAX                         64                                      // Maximum length for an IDIO packet
#define IDIO_PREVSEQ_INITIAL_VALUE                  0x00                                    // The initial value for the previous sequence (0x00 will never be sent by the phone as the first packet)

// IDIO Command Header Position Defines
#define IDIO_COMMAND_HEADER_COMMAND_POS             0                                       // Command Header Command Position
#define IDIO_COMMAND_HEADER_SEQNUM_POS              1                                       // Command Header Sequence Number Position

// IDIO Command Defines
#define IDIO_COMMAND_HEADER_SIZE                    2                                       // Command Header Length
#define IDIO_COMMAND_PAYLOAD_START_POS              IDIO_COMMAND_HEADER_SIZE                // Start position for the payload

// IDIO Command Payload Header Position Defines
#define IDIO_COMMAND_PAYLOAD_HEADER_LENGTH_POS      (IDIO_COMMAND_PAYLOAD_START_POS + 0)    // Command Payload Header Length Position
#define IDIO_COMMAND_PAYLOAD_HEADER_COMMAND_POS     (IDIO_COMMAND_PAYLOAD_START_POS + 1)    // Command Payload Header Command Position
#define IDIO_COMMAND_PAYLOAD_HEADER_ENDPOINT_POS    (IDIO_COMMAND_PAYLOAD_START_POS + 2)    // Command Payload Header Endpoint Position

// IDIO Command Payload Defines
#define IDIO_COMMAND_PAYLOAD_HEADER_SIZE            3                                                               // Command Payload Header Length
#define IDIO_COMMAND_PAYLOAD_DATA_START_POS         (IDIO_COMMAND_HEADER_SIZE + IDIO_COMMAND_PAYLOAD_HEADER_SIZE)   // Start position for the payload data

// IDIO Response Header Position Defines
#define IDIO_RESPONSE_HEADER_COMMAND_POS            0                                       // Response Header Command Position

// IDIO Response Defines
#define IDIO_RESPONSE_HEADER_SIZE                   1                                       // Response Header Size
#define IDIO_RESPONSE_PAYLOAD_HEADER_START_POS      IDIO_RESPONSE_HEADER_SIZE               // Response Paylaod Start Position

// IDIO Response Payload Header Position Defines
#define IDIO_RESPONSE_PAYLOAD_HEADER_LENGTH_POS     (IDIO_RESPONSE_PAYLOAD_HEADER_START_POS + 0)  // Response Payload Header Length Position
#define IDIO_RESPONSE_PAYLOAD_HEADER_COMMAND_POS    (IDIO_RESPONSE_PAYLOAD_HEADER_START_POS + 1)  // Response Payload Header Command Position
#define IDIO_RESPONSE_PAYLOAD_HEADER_ENDPOINT_POS   (IDIO_RESPONSE_PAYLOAD_HEADER_START_POS + 2)  // Command Payload Header Endpoint Position

// IDIO Response Defines
#define IDIO_RESPONSE_PAYLOAD_HEADER_SIZE           3                                                                 // Response Payload Header Length
#define IDIO_RESPONSE_PAYLOAD_DATA_START_POS        (IDIO_RESPONSE_HEADER_SIZE + IDIO_RESPONSE_PAYLOAD_HEADER_SIZE)   // Start position for the payload data

// IDIO Response Global Defines
#define IDIO_RESPONSE_PAYLOAD_DATA_ENDPOINT_STATE_POS   (IDIO_RESPONSE_PAYLOAD_DATA_START_POS + 0)  // Endpoint Enable Response Position
#define IDIO_RESPONSE_PAYLOAD_DATA_ENDPOINT_STATE_SIZE  1                                           // Endpoint Enable Response Payload Size

#define ENDPOINT_ENABLED    0x01
#define ENDPOINT_DISABLED   0x00

// IDIO Commands
#define IDIO_ENDPOINT_ENABLED                       ENDPOINT_ENABLED
#define IDIO_ENDPOINT_DISABLED                      ENDPOINT_DISABLED

//#define IDIO_SET_ENDPOINT_ENABLE                    SET_ENDPOINT_ENABLE
//#define IDIO_GET_ENDPOINT_ENABLE                    GET_ENDPOINT_ENABLE
//#define IDIO_RET_ENDPOINT_ENABLE                    RET_ENDPOINT_ENABLE

#define IDIO_SET_COM_CTRL                           SET_COM_CTRL                          // Set Comm Control Command
#define IDIO_RET_COM_FIFO                           RET_COM_FIFO                          // Return Comm Control Command

#define IDIO_SET_GPIO_CONFIG                        SET_GPIO_CONFIG
#define IDIO_GET_GPIO_CONFIG                        GET_GPIO_CONFIG
#define IDIO_RET_GPIO_CONFIG                        RET_GPIO_CONFIG

#define IDIO_SET_GPIO_STATUS                        0x80
#define IDIO_GET_GPIO_STATUS                        0x81
#define IDIO_RET_GPIO_STATUS                        0x82

#define IDIO_GET_INT_MSG                            0x89
#define IDIO_RET_INT_MSG                            RET_INT_MSG

// IDIO Endpoints
#define IDIO_ENDPOINT_I2C                           ENDPOINT_I2C                          // I2C Endpoint
#define IDIO_ENDPOINT_GPIO                          ENDPOINT_GPIO0

bool idioSendWake(void);
void idioInit(void);
uint8_t idioEnable(uint8_t enable);
uint32_t idioIdleTime(void);
bool idioIsIdle(uint32_t timeMs);
void idioService(void);
int idioSendResponse(AID_RESP_Type * idioResponsePacketPtr);


#endif /* CORE_IDIO_H_ */
