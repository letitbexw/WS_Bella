/*
 * ep_hid.h
 *
 *  Created on: Feb 10, 2026
 *      Author: xiongwei
 */

#ifndef CORE_EP_HID_H_
#define CORE_EP_HID_H_


#include "config.h"
#include "idio.h"


#define EP_HID_CMD_GET_HID_DESCRIPTOR             	0x0C
#define EP_HID_RESP_GET_HID_DESCRIPTOR            	0x0D

#define EP_HID_CMD_SET_HID_REPORT                 	0x10
#define EP_HID_RESP_SET_HID_REPORT                	0x10  //ACK

#define EP_HID_CMD_GET_HID_REPORT                 	0x01
#define EP_HID_RESP_GET_HID_REPORT                	0x02

#define EP_HID_RESP_RET_HID_REPORT                	0x11

#define EP_HID_RESP_RET_HID_REPORT_MULTIPLE	      	0x22

#define EP_HID_RESP_DESCRIPTOR_VID_OFFSET           0
#define EP_HID_RESP_DESCRIPTOR_PID_OFFSET           2
#define EP_HID_RESP_DESCRIPTOR_COUNTRY_CODE_OFFSET  4
#define EP_HID_RESP_DESCRIPTOR_REPORT_OFFSET        5

#define EP_HID_RESP_DESCRIPTOR_HEADER_LENGTH        5


#define EP_HID_CMD_SET_REPORT_TYPE_OFFSET           0
#define EP_HID_CMD_SET_REPORT_ID_OFFSET             1
#define EP_HID_CMD_SET_REPORT_DATA_OFFSET           2

#define EP_HID_CMD_GET_REPORT_TYPE_OFFSET           0
#define EP_HID_CMD_GET_REPORT_ID_OFFSET             1

#define EP_HID_RESP_GET_REPORT_DATA_OFFSET          0


uint8_t epHIDGetActiveEP(void);
void    epHIDSetActiveEP(uint8_t endpoint);
uint8_t epHIDHandler(AID_ENDPOINT_PACKET_Type * command, AID_ENDPOINT_PACKET_Type *response);
uint8_t epHIDGetAsyncDataPacket(AID_ENDPOINT_PACKET_Type * response);
bool epHIDGetAsyncDataPendingFlag(void);


#endif /* CORE_EP_HID_H_ */
