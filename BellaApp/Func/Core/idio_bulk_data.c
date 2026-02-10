/*
 * idio_bulk_data.c
 *
 *  Created on: Feb 5, 2026
 *      Author: xiongwei
 */


#include "config.h"
#include "debug.h"
#include "timers.h"
#ifdef CNFG_AID_HAS_BULK_DATA_DEVICE_CTL
#include "ep_device_control.h"
#endif
#ifdef CNFG_AID_HAS_BULK_DATA_IAP2
#include "ep_iap.h"
#endif
#ifdef CNFG_AID_HAS_BULK_DATA_HID
#include "ep_hid.h"
#include "hid.h"
#endif
#ifdef CNFG_AID_HAS_BULK_DATA_EA
#include "ep_ea.h"
#endif
#ifdef CNFG_AID_HAS_BULK_DATA_PD
#include "ep_pd.h"
#include "aid_pd.h"
#endif
#include "idio.h"
#include "idio_bulk_data.h"
#include "main.h"


#define IDIO_BULK_READ_RESPONSE_TO_COMMAND_TIMEOUT_MS   			200
#define IDIO_BULK_READ_RESPONSE_TO_COMMAND_TIMEOUT_INITIAL_MS     	100
#define IDIO_BULK_READ_RESPONSE_TO_COMMAND_TIMEOUT_SUBSEQUENT_MS  	1000

#define IDIO_ENDPOINTS_MAX         									(4 + CNFG_HID_ENDPOINTS_MAX)  // DevCtl, iAP, EA, PD

// Endpoint Async Data State Enumeration
typedef enum
{
	EAD_State_HID,
	EAD_State_DeviceControl,
	EAD_State_IAP,
	EAD_State_EA,
	EAD_State_PD,
	EAD_State_idle,
} enEADState;

// Local variables and buffers
static AID_RESP_Type 	idioLastBulkDataResponsePacket;
static uint8_t 			idioLastBulkDataSeqNum;

static AID_RESP_Type 	idioBulkDataReadResponsePacket;
static uint8_t 			idioBulkDataReadCommandSeqNum;

static AID_RESP_Type 	idioLastBulkDataReadResponsePacket;
static uint8_t 			idioLastBulkDataReadSeqNum;

static bool 			idioBulkDataReadCommandPending 					= false;
static uint8_t 			idioBulkDataReadCommandResponsePayloadExpLen 	= 0;

static AID_ENDPOINT_PACKET_Type idioBulkDataEndpointCommandPacket;
static AID_ENDPOINT_PACKET_Type idioBulkDataEndpointResponsePacket;
static AID_ENDPOINT_PACKET_Type idioBulkDataReadEndpointResponsePacket;

static bool idioBulkDataEnabled 		= false;
static uint8_t idioBulkDataRetryCount 	= 0;

static uint8_t idioEndpointCount 						= 0;
static uint16_t idioEndpointConfig[IDIO_ENDPOINTS_MAX] 	= {};

extern volatile uint8_t collision;		// idbus uart collision


bool idioBulkDataIsEnabled(void) { return idioBulkDataEnabled; }

static void idioEnumerateEndpoints(void)
{
	idioEndpointCount = 0;
#ifdef CNFG_AID_HAS_BULK_DATA_HID
	uint8_t hidEndpoints = hidGetEndpointCount();
	if (hidEndpoints > CNFG_HID_ENDPOINTS_MAX) 	{ hidEndpoints = CNFG_HID_ENDPOINTS_MAX; }
	if (hidEndpoints == 0) 						{ mainSetEvents(MAIN_EVENT_HID_ENUM_COMPLETE); }
	while (hidEndpoints--) 						{ idioEndpointConfig[idioEndpointCount++] = AID_ENDPOINT_FUNCTION_HID; }
#endif
#ifdef CNFG_AID_HAS_BULK_DATA_DEVICE_CTL
	idioEndpointConfig[idioEndpointCount++] = AID_ENDPOINT_FUNCTION_DEVICE_CONTROL;
#endif
#ifdef CNFG_AID_HAS_BULK_DATA_IAP2
	idioEndpointConfig[idioEndpointCount++] = AID_ENDPOINT_FUNCTION_IAP;
#endif
#ifdef CNFG_AID_HAS_BULK_DATA_EA
	idioEndpointConfig[idioEndpointCount++] = AID_ENDPOINT_FUNCTION_EA;
#endif
#ifdef CNFG_AID_HAS_BULK_DATA_PD
	idioEndpointConfig[idioEndpointCount++] = AID_ENDPOINT_FUNCTION_POWER_DELIVERY;
#endif
}

static uint16_t idioGetEndpointByType(uint8_t endpointType)
{
	uint8_t endpoint;
	for (endpoint = 0 ; endpoint < idioEndpointCount ; endpoint++)
	{
		if (idioEndpointConfig[endpoint] == endpointType) { break; }
	}
	return endpoint;
}

__STATIC_INLINE void idioBulkDataEnable(void)
{
	idioLastBulkDataSeqNum 		= 0x00;
	idioLastBulkDataReadSeqNum 	= 0x00;
	idioBulkDataEnabled 		= true;
	idioBulkDataRetryCount    	= 0;
	DEBUG_PRINT_IDIO_BULK_DATA("Bulk data enabled");
}

static uint8_t idioSendBulkDataIdentifyResponse(AID_CMD_Type * idioCommandPacketPtr, AID_RESP_Type * idioResponsePacketPtr)
{
	uint8_t ret = IDIO_RESPONSE_VALID;

	idioResponsePacketPtr->length 					= aidNumPayloadBytesForCommand(AID_RESPONSE_BulkDataIdentify);
	idioResponsePacketPtr->packet.bytes.packetType 	= AID_RESPONSE_BulkDataIdentify;

	idioResponsePacketPtr->packet.bytes.payload[0] 	= AID_BULK_DATA_VERSION;
	idioResponsePacketPtr->packet.bytes.payload[1] 	= idioEndpointCount;

	idioBulkDataEnable();

	return ret;
}

static uint8_t idioBulkDataCheckEndpointsForAsyncData(void)
{
	bool dataFlag = false;

#ifdef CNFG_AID_HAS_BULK_DATA_DEVICE_CTL
	if (!dataFlag && epDeviceControlGetAsyncDataPendingFlag()) 	{ dataFlag = true; }
#endif

#ifdef CNFG_AID_HAS_BULK_DATA_HID
	if (!dataFlag && epHIDGetAsyncDataPendingFlag()) 			{ dataFlag = true; }
#endif

#ifdef CNFG_AID_HAS_IAP2
	if (!dataFlag && epIAPGetAsyncDataPendingFlag()) 			{ dataFlag = true; }
#endif  // CNFG_AID_HAS_IAP2

#ifdef CNFG_AID_HAS_BULK_DATA_PD
	if (!dataFlag && epPDGetAsyncDataPendingFlag()) 			{ dataFlag = true; }
#endif  // CNFG_AID_HAS_BULK_DATA_PD
	return dataFlag;
}

static uint8_t idioBulkDataResponseGetPacket(AID_ENDPOINT_PACKET_Type * endpointResponse, AID_RESP_Type * idioResponsePacketPtr)
{
	uint8_t ret = IDIO_RESPONSE_NONE;
	uint16_t i = 0;

	if (endpointResponse->packetProcessFlag == true)
	{
		if (endpointResponse->respPayloadExpLen == AID_BULK_DATA_VARIABLE_LENGTH_RESPONSE)
		{
			//Variable-length response
			if (endpointResponse->payloadLen > AID_RESP_BULK_DATA_MAX_VARIABLE_PAYLOAD_LENGTH)
			{
				//If we have more bytes than Tristar 1 can accept, then truncate it to a 27 byte response.
				endpointResponse->respPayloadExpLen = AID_RESP_BULK_DATA_MAX_VARIABLE_PAYLOAD_LENGTH;
			}
			else
			{
				//Otherwise send as many bytes as required by the endpoint response.
				endpointResponse->respPayloadExpLen = endpointResponse->payloadLen;
			}
		}

		if (endpointResponse->respPayloadExpLen <= AID_RESP_BULK_DATA_MAX_PAYLOAD_LENGTH)
		{
			idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_CMD_OFFSET] 	= AID_RESPONSE_BulkData;
			idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_SEQ_OFFSET] 	= endpointResponse->seqNumber;
			idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_PTPKT_OFFSET] 	= endpointResponse->endpointPacketType;
			idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_ENDPT_OFFSET] 	= endpointResponse->endpoint;

			//Check if the payload will fit in the resp_len provided by the host.
			if (endpointResponse->payloadLen > endpointResponse->respPayloadExpLen)
			{
				if (endpointResponse->multiPacketFlag == false)
				{
					endpointResponse->multiPacketFlag 	= true;
					endpointResponse->payloadPtr 		= &endpointResponse->buffer[0];
					endpointResponse->byteOffset 		= 0;
				}
				idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_LEN_OFFSET] = endpointResponse->respPayloadExpLen;
				idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_LEN_OFFSET] |= AID_CMD_BULK_DATA_CMD_LEN_PARTIAL_PAYLOAD_MASK;
			}
			else
			{
				if (endpointResponse->multiPacketFlag == false)
				{
					endpointResponse->payloadPtr = &endpointResponse->buffer[0];
					endpointResponse->byteOffset = 0;
				}
				idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_LEN_OFFSET] = endpointResponse->payloadLen;
				endpointResponse->multiPacketFlag 									= false;
				endpointResponse->packetProcessFlag 								= false;
			}

			for(i = 0; i < endpointResponse->respPayloadExpLen; i++)
			{
				//Clearout the payload data.Optionally pad response with 0xFF to minimize AID low time
				idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_PAYLOAD_OFFSET + i] = CNFG_AID_BULK_DATA_PAD_DATA;	// Fill with 0xFF

				if (((i + endpointResponse->byteOffset) < AID_MAX_ENDPOINT_PACKET_LENGTH) && (i < endpointResponse->payloadLen))
				{
					idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_PAYLOAD_OFFSET + i] = endpointResponse->buffer[i + endpointResponse->byteOffset];
				}
			}

			idioResponsePacketPtr->length = endpointResponse->respPayloadExpLen + AID_RESP_BULK_DATA_HEADER_LENGTH;
			if (idioBulkDataCheckEndpointsForAsyncData() || collision)
			{
				idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_LEN_OFFSET] |= AID_RESP_BULK_DATA_CMD_READ_PENDING_MASK;
				collision = 0;
			}

			endpointResponse->byteOffset += endpointResponse->respPayloadExpLen;
			endpointResponse->payloadLen -= endpointResponse->respPayloadExpLen;

			ret = IDIO_RESPONSE_VALID;
		}
	}
	return ret;
}

static uint8_t idioSendBulkDataEndpointInfoResponse(AID_CMD_Type * idioCommandPacketPtr, AID_RESP_Type * idioResponsePacketPtr)
{
	uint8_t ret 		= IDIO_RESPONSE_NONE;
	uint8_t endpoint 	= idioCommandPacketPtr->packet.bytes.payload[0];

	if (endpoint < idioEndpointCount)
	{
		ret = IDIO_RESPONSE_VALID;

		idioResponsePacketPtr->length 					= aidNumPayloadBytesForCommand(AID_RESPONSE_BulkDataEndpointInfo);
		idioResponsePacketPtr->packet.bytes.packetType 	= AID_RESPONSE_BulkDataEndpointInfo;
		idioResponsePacketPtr->packet.bytes.payload[0] 	= endpoint;
		idioResponsePacketPtr->packet.bytes.payload[1] 	= 0;
		idioResponsePacketPtr->packet.bytes.payload[2] 	= idioEndpointConfig[endpoint];
	}
	return ret;
}

static uint8_t idioSendBulkDataResetResponse(AID_CMD_Type * idioCommandPacketPtr, AID_RESP_Type * idioResponsePacketPtr)
{
	uint8_t ret = IDIO_RESPONSE_VALID;

	//Get the response length
	uint8_t respPayloadExpLen = 0;

	//Check if its the bulk read command and cont, its response length offset is different
	if (idioCommandPacketPtr->packet.bytes.packetType == AID_COMMAND_BulkData)
	{
		respPayloadExpLen = idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_RESP_LEN_OFFSET];
	}
	else
	{
		respPayloadExpLen = idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_READ_RESP_LEN_OFFSET];
	}

	//Set the packet type to -> reset
	idioResponsePacketPtr->packet.bytes.packetType 							= AID_RESPONSE_BulkDataReset;
	//Response is zero!
	//idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_CMD_OFFSET] 	= AID_RESPONSE_BulkDataReset; //Same as above
	idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_SEQ_OFFSET] 	= 0;
	idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_LEN_OFFSET] 	= 0; //0 payload bytes
	idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_PTPKT_OFFSET] 	= 0;
	idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_ENDPT_OFFSET] 	= 0;
	//Set the response payload length
	idioResponsePacketPtr->length 											= 0;

	//If the host is expecting a variable length response, then send a zero payload response.
	if (respPayloadExpLen == AID_BULK_DATA_VARIABLE_LENGTH_RESPONSE)
	{
		idioResponsePacketPtr->length = AID_RESP_BULK_DATA_HEADER_LENGTH;
	}
	else if (respPayloadExpLen <= AID_RESP_BULK_DATA_MAX_PAYLOAD_LENGTH)
	{
		uint8_t i = 0;
		idioResponsePacketPtr->length = respPayloadExpLen + AID_RESP_BULK_DATA_HEADER_LENGTH;
		for(i = 0; i < respPayloadExpLen; i++)
		{
			idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_PAYLOAD_OFFSET + i] = 0;
		}
	}
	else
	{
		//Error dont send a response.
		ret = IDIO_RESPONSE_NONE;
	}
	return ret;
}

static uint8_t idioBulkDataGetEndpointsForAsyncData(void)
{
	uint8_t ret 					= IDIO_RESPONSE_NONE;
	bool wakeFlag 					= false;
	static enEADState syncDataState = EAD_State_idle;

#ifdef CNFG_AID_HAS_BULK_DATA_DEVICE_CTL
	if (epDeviceControlGetAsyncDataPendingFlag())
	{
		if (idioBulkDataReadCommandPending)
		{
			if ((syncDataState == EAD_State_idle) || (syncDataState == EAD_State_DeviceControl))
			{
				if (idioBulkDataReadEndpointResponsePacket.packetProcessFlag == false)
				{
					//Check the Device endpoint for data
					ret = epDeviceControlGetAsyncDataPacket(&idioBulkDataReadEndpointResponsePacket);

					//We have an endpoint response ... now move it to the idio response packet
					if (ret == IDIO_RESPONSE_VALID)
					{
						syncDataState = EAD_State_DeviceControl;
						idioBulkDataReadEndpointResponsePacket.endpoint = idioGetEndpointByType(AID_ENDPOINT_FUNCTION_DEVICE_CONTROL);
					}
				}
			}
		}
		else
		{
			wakeFlag = true;
		}
	}
#endif

#ifdef CNFG_AID_HAS_BULK_DATA_HID
	if (epHIDGetAsyncDataPendingFlag())
	{
		if (idioBulkDataReadCommandPending)
		{
			if ((syncDataState == EAD_State_idle) || (syncDataState == EAD_State_HID))
			{
				if (idioBulkDataReadEndpointResponsePacket.packetProcessFlag == false)
				{
					//Check the Device endpoint for data
					ret = epHIDGetAsyncDataPacket(&idioBulkDataReadEndpointResponsePacket);

					//We have an endpoint response ... now move it to the idio response packet
					if (ret == IDIO_RESPONSE_VALID)
					{
						syncDataState = EAD_State_HID;
						idioBulkDataReadEndpointResponsePacket.endpoint = idioGetEndpointByType(AID_ENDPOINT_FUNCTION_HID) + epHIDGetActiveEP();
					}
				}
			}
		}
		else
		{
			wakeFlag = true;
		}
	}
#endif

#ifdef CNFG_AID_HAS_BULK_DATA_IAP2
	if (epIAPGetAsyncDataPendingFlag())
	{
		if (idioBulkDataReadCommandPending)
		{
			if ((syncDataState == EAD_State_idle) || (syncDataState == EAD_State_IAP))
			{
				if (idioBulkDataReadEndpointResponsePacket.packetProcessFlag == false)
				{
					//Check the Device endpoint for data
					ret = epIAPGetAsyncDataPacket(&idioBulkDataReadEndpointResponsePacket);

					//We have an endpoint response ... now move it to the idio response packet
					if (ret == IDIO_RESPONSE_VALID)
					{
						syncDataState = EAD_State_IAP;
						idioBulkDataReadEndpointResponsePacket.endpoint = idioGetEndpointByType(AID_ENDPOINT_FUNCTION_IAP);
					}
				}
			}
		}
		else
		{
			wakeFlag = true;
		}
	}
#endif  // CNFG_AID_HAS_BULK_DATA_IAP2

#ifdef CNFG_AID_HAS_BULK_DATA_EA
	if (epEAGetAsyncDataPendingFlag())
	{
		if (idioBulkDataReadCommandPending)
		{
			if ((syncDataState == EAD_State_idle) || (syncDataState == EAD_State_EA))
			{
				if (idioBulkDataReadEndpointResponsePacket.packetProcessFlag == false)
				{
					//Check the Device endpoint for data
					ret = epEAGetAsyncDataPacket(&idioBulkDataReadEndpointResponsePacket);

					/We have an endpoint response ... now move it to the idio response packet
					if (ret == IDIO_RESPONSE_VALID)
					{
						syncDataState = EAD_State_EA;
						idioBulkDataReadEndpointResponsePacket.endpoint = idioGetEndpointByType(AID_ENDPOINT_FUNCTION_EA);
					}
				}
			}
		}
		else
		{
			wakeFlag = true;
		}
	}
#endif  // CNFG_AID_HAS_BULK_DATA_EA

#ifdef CNFG_AID_HAS_BULK_DATA_PD
	if (epPDGetAsyncDataPendingFlag())
	{
		if (idioBulkDataReadCommandPending)
		{
			if ((syncDataState == EAD_State_idle) || (syncDataState == EAD_State_PD))
			{
				if (idioBulkDataReadEndpointResponsePacket.packetProcessFlag == false)
				{
					//Check the Device endpoint for data
					ret = epPDGetAsyncDataPacket(&idioBulkDataReadEndpointResponsePacket);

					//We have an endpoint response ... now move it to the idio response packet
					if (ret == IDIO_RESPONSE_VALID)
					{
						syncDataState = EAD_State_PD;
						idioBulkDataReadEndpointResponsePacket.endpoint = idioGetEndpointByType(AID_ENDPOINT_FUNCTION_POWER_DELIVERY);
					}
				}
			}
		}
		else
		{
			wakeFlag = true;
		}
	}
#endif  // CNFG_AID_HAS_BULK_DATA_PD

	if (ret == IDIO_RESPONSE_VALID)
	{
		idioBulkDataReadEndpointResponsePacket.respPayloadExpLen 	= idioBulkDataReadCommandResponsePayloadExpLen;
		idioBulkDataReadEndpointResponsePacket.seqNumber 			= idioBulkDataReadCommandSeqNum;

		DEBUG_PRINT_IDIO_BULK_DATA("BDR pkt");
		idioBulkDataReadEndpointResponsePacket.packetProcessFlag 	= true;
		idioBulkDataReadEndpointResponsePacket.byteOffset 			= 0;
		//Response is valid, check if it will fit.
		ret = idioBulkDataResponseGetPacket(&idioBulkDataReadEndpointResponsePacket, &idioBulkDataReadResponsePacket);

		if (ret == IDIO_RESPONSE_VALID)
		{
			idioBulkDataReadResponsePacket.packet.buffer[AID_RESP_BULK_DATA_CMD_OFFSET] = AID_RESPONSE_BulkDataRead;
			memcpy(&idioLastBulkDataReadResponsePacket, &idioBulkDataReadResponsePacket, sizeof(AID_RESP_Type));
#ifdef CNFG_AID_HAS_BULK_DATA_HID
			/* If the event is a streaming event,set the read request bit to 1 to prevent collision from iOS */
			if(hidGetStreamModeFlag())
			{
				idioBulkDataReadResponsePacket.packet.buffer[AID_RESP_BULK_DATA_LEN_OFFSET] |= AID_RESP_BULK_DATA_CMD_READ_PENDING_MASK;
				hidSetStreamModeFlag(false);
			}
#endif
			//Do we need this redundancy
			idioLastBulkDataReadSeqNum 		= idioBulkDataReadResponsePacket.packet.buffer[AID_RESP_BULK_DATA_SEQ_OFFSET];
			idioBulkDataRetryCount         	= 0;
			idioBulkDataReadCommandPending 	= false;
			idioSendResponse(&idioBulkDataReadResponsePacket);
		}
	}

	if (1)
	{ //idioBulkDataReadEndpointResponsePacket.packetProcessFlag == false) {
		syncDataState = EAD_State_idle;      // this should only be set to idle if packetProcessFlag is false...independent of wakeFlag logic
		if (wakeFlag)
		{
			uint32_t timeout = (idioBulkDataRetryCount == 0) ? IDIO_BULK_READ_RESPONSE_TO_COMMAND_TIMEOUT_INITIAL_MS : IDIO_BULK_READ_RESPONSE_TO_COMMAND_TIMEOUT_SUBSEQUENT_MS; // See <rdar://problem/55115232>
			if (idioIdleTime() > timeout)
			{
				idioBulkDataRetryCount++;
				if (idioSendWake())
				{
					DEBUG_PRINT_IDIO_BULK_DATA("Read timeout -> sending wake");
					wakeFlag = false;
				}
			}
		}
	}
	return ret;
}

void idioBulkDataInit(void)
{
	memset(&idioLastBulkDataResponsePacket, 	0, sizeof(AID_RESP_Type));
	memset(&idioLastBulkDataReadResponsePacket, 0, sizeof(AID_RESP_Type));

	memset(&idioBulkDataEndpointCommandPacket, 		0, sizeof(AID_ENDPOINT_PACKET_Type));
	memset(&idioBulkDataEndpointResponsePacket, 	0, sizeof(AID_ENDPOINT_PACKET_Type));
	memset(&idioBulkDataReadEndpointResponsePacket, 0, sizeof(AID_ENDPOINT_PACKET_Type));

	idioLastBulkDataSeqNum 		= 0x00;
	idioLastBulkDataReadSeqNum 	= 0x00;

	idioBulkDataEnabled 		= false;
	idioEnumerateEndpoints();

#ifdef CNFG_AID_HAS_IAP2
	epIapReset();
#endif
#ifdef CNFG_AID_HAS_BULK_DATA_PD
	epPDInit();
	aidpdInit();
#endif
#ifdef CNFG_AID_HAS_BULK_DATA_DEVICE_CTL
	epDeviceControlInit();
#endif
}

void idioBulkDataServiceEnable(uint8_t enable)
{
	if (idioBulkDataEnabled != enable)
	{
		if (enable) { idioBulkDataInit(); }
		idioBulkDataEnabled = false;
	}
}

void idioBulkDataService(void)
{
	if (idioBulkDataEnabled)
	{
		idioBulkDataGetEndpointsForAsyncData();

#ifdef CNFG_AID_HAS_BULK_DATA_PD
		epPDService();
#endif
	}
}

void idioBulkDataClearReadPendingFlag(void)
{
	idioBulkDataReadCommandPending = false;
}

uint8_t idioBulkDataHandler(AID_CMD_Type * idioCommandPacketPtr, AID_RESP_Type * idioResponsePacketPtr)
{
	uint8_t ret = IDIO_RESPONSE_NONE;
	//DEBUG_PRINT_IDIO_BULK_DATA("(BDL %02X %d, %d)", idioCommandPacketPtr->packet.bytes.packetType, idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_CMD_LEN_OFFSET],  idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_SEQ_OFFSET]);

	if (idioBulkDataEnabled)
	{
		switch (idioCommandPacketPtr->packet.bytes.packetType)
		{
		case AID_COMMAND_BulkDataIdentify:
			ret = idioSendBulkDataIdentifyResponse(idioCommandPacketPtr, idioResponsePacketPtr);
			break;

		case AID_COMMAND_BulkDataEndpointInfo:
			ret = idioSendBulkDataEndpointInfoResponse(idioCommandPacketPtr, idioResponsePacketPtr);
			break;

		case AID_COMMAND_BulkData:
			if (idioLastBulkDataSeqNum == idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_SEQ_OFFSET])
			{
				//Same seq number received, just resend the last packet.
				if ((idioLastBulkDataResponsePacket.length > 0) && (idioLastBulkDataResponsePacket.packet.buffer[AID_RESP_BULK_DATA_SEQ_OFFSET] == idioLastBulkDataSeqNum))
				{
					memcpy(idioResponsePacketPtr, &idioLastBulkDataResponsePacket, sizeof(AID_RESP_Type));
					ret = IDIO_RESPONSE_VALID;
				}
			}
			else
			{
				//New sequence number, need to either store or process the packet.
				//get the payload length
				uint8_t payloadLength = idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_CMD_LEN_OFFSET] & AID_LEN_MASK;
				//Store the sequence number.
				idioLastBulkDataSeqNum = idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_SEQ_OFFSET];

				//Check the length,
				if (idioCommandPacketPtr->length == payloadLength + AID_CMD_BULK_DATA_HEADER_LENGTH)
				{
					//Check if this is a partial command
					if (idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_CMD_LEN_OFFSET] & AID_CMD_BULK_DATA_CMD_LEN_PARTIAL_PAYLOAD_MASK)
					{
						//Partial packet, we need to store it, no processing.
						if (idioBulkDataEndpointCommandPacket.multiPacketFlag == false)
						{
							//First packet. so lets clear the endpoint packet and start fresh.
							idioBulkDataEndpointCommandPacket.payloadLen 			= payloadLength;
							idioBulkDataEndpointCommandPacket.payloadPtr 			= &idioBulkDataEndpointCommandPacket.buffer[0];
							idioBulkDataEndpointCommandPacket.endpoint 				= idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_ENDPT_OFFSET];
							idioBulkDataEndpointCommandPacket.endpointPacketType 	= idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_PTPKT_OFFSET];

							memcpy(idioBulkDataEndpointCommandPacket.payloadPtr, &idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_PAYLOAD_OFFSET], payloadLength);

							idioBulkDataEndpointCommandPacket.payloadPtr 			+= payloadLength;
							idioBulkDataEndpointCommandPacket.multiPacketFlag 		= true;
							ret = IDIO_COMMAND_VALID;
						}
						else
						{
							//subsequent packets:
							//Check if the end point matches
							if (idioBulkDataEndpointCommandPacket.endpoint == idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_ENDPT_OFFSET])
							{
								//Check if the end point command matches
								if (idioBulkDataEndpointCommandPacket.endpointPacketType == idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_PTPKT_OFFSET])
								{
									//Check if the payload will fit in our buffer
									if (idioBulkDataEndpointCommandPacket.payloadLen +  payloadLength < AID_MAX_ENDPOINT_PACKET_LENGTH)
									{
										idioBulkDataEndpointCommandPacket.payloadLen += payloadLength;
										memcpy(idioBulkDataEndpointCommandPacket.payloadPtr, &idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_PAYLOAD_OFFSET], payloadLength);
										idioBulkDataEndpointCommandPacket.payloadPtr += payloadLength;

										ret = IDIO_COMMAND_VALID;
									}
									else
									{
										ret = IDIO_RESPONSE_LENGTH_ERROR;
									}
								}
							}
						}

						//Do we need to respond? Yes - Send an ACK
						if (ret == IDIO_COMMAND_VALID)
						{
							idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_CMD_OFFSET] 	= AID_RESPONSE_BulkData;
							idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_SEQ_OFFSET] 	= idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_SEQ_OFFSET];
							idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_LEN_OFFSET] 	= 0; //0 payload bytes
							idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_PTPKT_OFFSET] 	= idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_PTPKT_OFFSET];
							idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_ENDPT_OFFSET] 	= idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_ENDPT_OFFSET];
							idioResponsePacketPtr->length = AID_RESP_BULK_DATA_HEADER_LENGTH;
							// set the BulkDataReadPending bit if needed
							if (idioBulkDataCheckEndpointsForAsyncData() || collision)
							{
								idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_LEN_OFFSET] |= AID_RESP_BULK_DATA_CMD_READ_PENDING_MASK;
								collision = 0;
							}

							//Store the packet for retrys
							memcpy(&idioLastBulkDataResponsePacket, idioResponsePacketPtr, sizeof(AID_RESP_Type));
							//Do we need this redundancy
							idioLastBulkDataSeqNum = idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_SEQ_OFFSET];

							ret = IDIO_RESPONSE_VALID;

							//Should we look at the response length and pad with zeros?
							if (idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_RESP_LEN_OFFSET] > 0)
							{
								DEBUG_PRINT_IDIO_BULK_DATA("Err: need to send an ack, but the response length > 0");
							}
						}
					}
					else
					{
						//Not a partial packet, we might be at the end of a large packet too.
						if (idioBulkDataEndpointCommandPacket.multiPacketFlag == true)
						{
							uint8_t payloadLength = idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_CMD_LEN_OFFSET] & AID_LEN_MASK;
							//last partial packet, we can process it if its good.
							//Check if the end point matches
							if (idioBulkDataEndpointCommandPacket.endpoint == idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_ENDPT_OFFSET])
							{
								//Check if the end point command matches
								if (idioBulkDataEndpointCommandPacket.endpointPacketType == idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_PTPKT_OFFSET])
								{
									//Check if the payload will fit in our buffer
									if (idioBulkDataEndpointCommandPacket.payloadLen +  payloadLength < AID_MAX_ENDPOINT_PACKET_LENGTH)
									{
										idioBulkDataEndpointCommandPacket.payloadLen += payloadLength;
										memcpy(idioBulkDataEndpointCommandPacket.payloadPtr, &idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_PAYLOAD_OFFSET], payloadLength);
										idioBulkDataEndpointCommandPacket.payloadPtr = &idioBulkDataEndpointCommandPacket.buffer[0];

										ret = IDIO_COMMAND_VALID;
									}
									else
									{
										ret = IDIO_RESPONSE_LENGTH_ERROR;
									}
								}
							}
							//DEBUG_PRINT_IDIO_BULK_DATA("(MPCC %d)", idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_CMD_LEN_OFFSET]);
							idioBulkDataEndpointCommandPacket.multiPacketFlag = false;
						}
						else
						{
							//Lets store the data in our local command buffer and send it for processing.
							idioBulkDataEndpointCommandPacket.payloadLen 			= payloadLength;
							idioBulkDataEndpointCommandPacket.payloadPtr 			= &idioBulkDataEndpointCommandPacket.buffer[0];
							idioBulkDataEndpointCommandPacket.endpoint 				= idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_ENDPT_OFFSET];
							idioBulkDataEndpointCommandPacket.endpointPacketType 	= idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_PTPKT_OFFSET];

							memcpy(idioBulkDataEndpointCommandPacket.payloadPtr, &idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_PAYLOAD_OFFSET], payloadLength);

							ret = IDIO_COMMAND_VALID;
						}

						//We have the whole command lets process it.
						if (ret == IDIO_COMMAND_VALID)
						{
							//DEBUG_PRINT_IDIO_BULK_DATA("EPP %d", idioBulkDataEndpointCommandPacket.payloadLen);
							//Pick up the response payload length from the command.
							idioBulkDataEndpointCommandPacket.respPayloadExpLen		= idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_RESP_LEN_OFFSET];
							idioBulkDataEndpointCommandPacket.endpoint 				= idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_ENDPT_OFFSET];
							idioBulkDataEndpointCommandPacket.endpointPacketType 	= idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_PTPKT_OFFSET];

							idioBulkDataEndpointResponsePacket.respPayloadExpLen 	= idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_RESP_LEN_OFFSET];
							idioBulkDataEndpointResponsePacket.payloadLen 			= 0;
							idioBulkDataEndpointResponsePacket.seqNumber 			= idioLastBulkDataSeqNum;
							idioBulkDataEndpointResponsePacket.endpoint 			= idioBulkDataEndpointCommandPacket.endpoint;

							//Check the response length if its correct then process the command.
							//Response length is checked by idioBulkDataResponseGetPacket
							if (idioBulkDataEndpointCommandPacket.respPayloadExpLen <= AID_RESP_BULK_DATA_MAX_PAYLOAD_LENGTH)
							{
								switch(idioEndpointConfig[idioBulkDataEndpointCommandPacket.endpoint])
								{
#ifdef CNFG_AID_HAS_BULK_DATA_DEVICE_CTL
									case AID_ENDPOINT_FUNCTION_DEVICE_CONTROL:
										ret = epDeviceControlHandler(&idioBulkDataEndpointCommandPacket, &idioBulkDataEndpointResponsePacket);
										break;
#endif

#ifdef CNFG_AID_HAS_BULK_DATA_HID
									case AID_ENDPOINT_FUNCTION_HID:
										ret = epHIDHandler(&idioBulkDataEndpointCommandPacket, &idioBulkDataEndpointResponsePacket);
										break;
#endif

#ifdef CNFG_AID_HAS_BULK_DATA_IAP2
									case AID_ENDPOINT_FUNCTION_IAP:
										ret = epIAPHandler(&idioBulkDataEndpointCommandPacket, &idioBulkDataEndpointResponsePacket);
										break;
#endif

#ifdef CNFG_AID_HAS_BULK_DATA_EA
									case AID_ENDPOINT_FUNCTION_EA:
										ret = epEAHandler(&idioBulkDataEndpointCommandPacket, &idioBulkDataEndpointResponsePacket);
										break;
#endif

#ifdef CNFG_AID_HAS_BULK_DATA_PD
									case AID_ENDPOINT_FUNCTION_POWER_DELIVERY:
										ret = epPDHandler(&idioBulkDataEndpointCommandPacket, &idioBulkDataEndpointResponsePacket);
										break;
#endif

									default:
										break;
								}
								//We have an endpoint response ... now move it to the idio response packet
								if (ret == IDIO_RESPONSE_VALID)
								{
									idioBulkDataEndpointResponsePacket.packetProcessFlag 	= true;
									idioBulkDataEndpointResponsePacket.byteOffset 			= 0;
									//Response is valid, check if it will fit.
									ret = idioBulkDataResponseGetPacket(&idioBulkDataEndpointResponsePacket, idioResponsePacketPtr);

									if (ret == IDIO_RESPONSE_VALID)
									{
										memcpy(&idioLastBulkDataResponsePacket, idioResponsePacketPtr, sizeof(AID_RESP_Type));
										//Do we need this redundancy
										idioLastBulkDataSeqNum = idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_SEQ_OFFSET];
									}
								}
							}
							else
							{
								ret = IDIO_RESPONSE_LENGTH_ERROR;
							}
						}
					}
				}
			}
			break;

		case AID_COMMAND_BulkDataRead:
			if (idioLastBulkDataReadSeqNum == idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_SEQ_OFFSET])
			{
				//Same seq number received, just resend the last packet.
				if ((idioLastBulkDataReadResponsePacket.length > 0) && (idioLastBulkDataReadResponsePacket.packet.buffer[AID_RESP_BULK_DATA_SEQ_OFFSET] == idioLastBulkDataReadSeqNum))
				{
					memcpy(idioResponsePacketPtr, &idioLastBulkDataReadResponsePacket, sizeof(AID_RESP_Type));
					ret = IDIO_RESPONSE_VALID;
				}
				else
				{
					//TODO: temp workaround for <rdar://problem/17580175>
					idioLastBulkDataReadSeqNum = 0xFF;
				}
			}
			else
			{
				idioBulkDataReadCommandSeqNum 					= idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_READ_SEQ_OFFSET];
				idioBulkDataReadCommandResponsePayloadExpLen 	= idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_READ_RESP_LEN_OFFSET];
				idioBulkDataReadCommandPending 					= true;

				if (idioBulkDataReadEndpointResponsePacket.packetProcessFlag == true)
				{
					idioBulkDataReadEndpointResponsePacket.respPayloadExpLen 	= idioBulkDataReadCommandResponsePayloadExpLen;
					idioBulkDataReadEndpointResponsePacket.seqNumber 			= idioBulkDataReadCommandSeqNum;

					//Response is valid, check if it will fit.
					ret = idioBulkDataResponseGetPacket(&idioBulkDataReadEndpointResponsePacket, idioResponsePacketPtr);

					if (ret == IDIO_RESPONSE_VALID)
					{
						idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_CMD_OFFSET] = AID_RESPONSE_BulkDataRead;

						memcpy(&idioLastBulkDataReadResponsePacket, idioResponsePacketPtr, sizeof(AID_RESP_Type));
						//Do we need this redundancy
						idioLastBulkDataReadSeqNum 		= idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_SEQ_OFFSET];
						idioBulkDataReadCommandPending 	= false;
					}
				}
			}
			break;

		case AID_COMMAND_BulkDataContinue:
			if (idioLastBulkDataSeqNum == idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_SEQ_OFFSET])
			{
				//Same seq number received, just resend the last packet.
				if ((idioLastBulkDataResponsePacket.length > 0) && (idioLastBulkDataResponsePacket.packet.buffer[AID_RESP_BULK_DATA_SEQ_OFFSET] == idioLastBulkDataSeqNum))
				{
					memcpy(idioResponsePacketPtr, &idioLastBulkDataResponsePacket, sizeof(AID_RESP_Type));
					ret = IDIO_RESPONSE_VALID;
				}
			}
			else
			{
				//If we have multipacket response in progress, check and return the response.
				//Get the response length from the command packet and update the endpoint packet
				idioBulkDataEndpointResponsePacket.respPayloadExpLen = idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_READ_RESP_LEN_OFFSET];

				ret = idioBulkDataResponseGetPacket(&idioBulkDataEndpointResponsePacket, idioResponsePacketPtr);

				idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_SEQ_OFFSET] = idioCommandPacketPtr->packet.buffer[AID_CMD_BULK_DATA_SEQ_OFFSET];
				if (ret == IDIO_RESPONSE_VALID)
				{
					memcpy(&idioLastBulkDataResponsePacket, idioResponsePacketPtr, sizeof(AID_RESP_Type));
					//Do we need this redundancy
					idioLastBulkDataSeqNum = idioResponsePacketPtr->packet.buffer[AID_RESP_BULK_DATA_SEQ_OFFSET];
				}
			}
			break;
		}
	}
	else  // bulk data has not been initalized, send a reset response to bulk data, read and continue commands.
	{
		switch (idioCommandPacketPtr->packet.bytes.packetType)
		{
			case AID_COMMAND_BulkDataIdentify:
				ret = idioSendBulkDataIdentifyResponse(idioCommandPacketPtr, idioResponsePacketPtr);
				break;

			case AID_COMMAND_BulkDataEndpointInfo:
				ret = idioSendBulkDataEndpointInfoResponse(idioCommandPacketPtr, idioResponsePacketPtr);
				break;

			case AID_COMMAND_BulkData:
			case AID_COMMAND_BulkDataRead:
			case AID_COMMAND_BulkDataContinue:
				ret = idioSendBulkDataResetResponse(idioCommandPacketPtr, idioResponsePacketPtr);
				break;
		}
	}
	return ret;
}
