/*
 * ep_pd.h
 *
 *  Created on: Feb 9, 2026
 *      Author: xiongwei
 */

#ifndef CORE_EP_PD_H_
#define CORE_EP_PD_H_


#include "config.h"
#include "idio.h"


// Control Messages
//  messages with Ids from 0x00 to 0x0F should always be ACK'd
#define EP_PD_CMD_GOTO_MIN                        0x02
#define EP_PD_CMD_ACCEPT                          0x03
#define EP_PD_CMD_REJECT                          0x04
#define EP_PD_CMD_PS_READY                        0x06
#define EP_PD_CMD_GET_SOURCE_CAPABILITY           0x07
#define EP_PD_CMD_GET_SINK_CAPABILITY             0x08
#define EP_PD_CMD_POWER_SWAP                      0x0A
#define EP_PD_CMD_WAIT                            0x0C
#define EP_PD_CMD_RESET                           0x80

// Data Messages
#define EP_PD_RESP_SOURCE_CAPABILITY              0x01
#define EP_PD_RESP_REQUEST                        0x02
#define EP_PD_RESP_SINK_CAPABILITY                0x04


uint8_t epPDHandler(AID_ENDPOINT_PACKET_Type * command, AID_ENDPOINT_PACKET_Type *response);
bool epPDGetAsyncDataPendingFlag(void);
uint8_t epPDGetAsyncDataPacket(AID_ENDPOINT_PACKET_Type * response);

uint8_t epPDQueueData(uint8_t command, uint8_t* data, uint8_t length);
uint8_t epPDQueueCommand(uint8_t command);
uint8_t epPDTxQueueEmpty(void);

void epPDInit(void);
void epPDService(void);


#endif /* CORE_EP_PD_H_ */
