/*
 * ep_hid.c
 *
 *  Created on: Feb 3, 2026
 *      Author: xiongwei
 */


#include "config.h"
#include "timers.h"
#include "ep_hid.h"
#include "hid.h"
#include "utils.h"
#include "main.h"

static uint8_t activeEP = 0;

uint8_t epHIDGetActiveEP(void)               { return activeEP; }
void    epHIDSetActiveEP(uint8_t endpoint)   { activeEP = endpoint; }


uint8_t epHIDHandler(AID_ENDPOINT_PACKET_Type * command, AID_ENDPOINT_PACKET_Type *response)
{
	uint8_t ret = IDIO_RESPONSE_VALID;

	switch(command->endpointPacketType)
	{
		case EP_HID_CMD_GET_HID_DESCRIPTOR:
		{
			uint8_t *hidIdentifier 	= hidGetIdentifier(command->endpoint);
			uint16_t idenLen 		= hidGetIdentifierLength(command->endpoint);

			response->payloadLen 			= idenLen;
			response->endpointPacketType 	= EP_HID_RESP_GET_HID_DESCRIPTOR;
			memcpy(response->buffer, hidIdentifier, idenLen);

			if (command->endpoint == hidGetEndpointCount()) { mainSetEvents(MAIN_EVENT_HID_ENUM_COMPLETE); }
			break;
		}


		case EP_HID_CMD_SET_HID_REPORT:
		{
			uint8_t reportType 	= command->buffer[EP_HID_CMD_SET_REPORT_TYPE_OFFSET];
			uint8_t reportID 	= command->buffer[EP_HID_CMD_SET_REPORT_ID_OFFSET];
			uint8_t reportLen 	= command->payloadLen - EP_HID_CMD_SET_REPORT_DATA_OFFSET;
			uint8_t *reportData = &command->buffer[EP_HID_CMD_SET_REPORT_DATA_OFFSET];

			response->payloadLen = 0; //ACK
			response->endpointPacketType = EP_HID_RESP_SET_HID_REPORT;

			hidSetReport(command->endpoint, reportType, reportID, reportData, reportLen);  // return (success) ignored
			break;
		}

		case EP_HID_CMD_GET_HID_REPORT:
		{
			uint8_t hidReportType = command->buffer[EP_HID_CMD_GET_REPORT_TYPE_OFFSET];
			uint8_t hidReportID = command->buffer[EP_HID_CMD_GET_REPORT_ID_OFFSET];

			uint8_t * hidReportData = &response->buffer[EP_HID_RESP_GET_REPORT_DATA_OFFSET];
			response->payloadLen = hidGetReport(command->endpoint, hidReportType, hidReportID, hidReportData);
			response->endpointPacketType = EP_HID_RESP_GET_HID_REPORT;
			if (response->payloadLen == 0) {
				ret = IDIO_RESPONSE_NONE;
			}
			break;
		}

		default:
			ret = IDIO_RESPONSE_NONE;
			break;
	}

	return ret;
}

uint8_t epHIDGetAsyncDataPacket(AID_ENDPOINT_PACKET_Type * response)
{
	uint8_t ret = IDIO_RESPONSE_NONE;
	if(hidGetMultipleReportFlag())
	{
		response->endpointPacketType = EP_HID_RESP_RET_HID_REPORT_MULTIPLE;
		hidSetMultipleReportFlag(false);
	}
	else
	{
		response->endpointPacketType = EP_HID_RESP_RET_HID_REPORT;
	}
	if (epHIDGetAsyncDataPendingFlag() == true) {
		ret = IDIO_RESPONSE_VALID;
		response->payloadLen = hidGetPendingReport(activeEP, &response->buffer[0]);
	}

	return ret;
}

bool epHIDGetAsyncDataPendingFlag(void)
{
	return hidGetReportDataPendingFlag();
}





