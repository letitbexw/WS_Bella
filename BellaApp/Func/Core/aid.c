/*
 * aid.c
 *
 *  Created on: Feb 3, 2026
 *      Author: xiongwei
 */


#include "aid.h"          // Include file header


// Define parameters needed by the CRC algorithm
#define AID_CRC_SEED          0xFF
#define AID_CRC_POLYNOMIAL    0x8C

void aidInitCRC(uint8_t *crc) { *crc = AID_CRC_SEED; }

void aidUpdateCRC(uint8_t *crc, uint8_t data_byte)
{
	unsigned int i;
	unsigned char temp_data = data_byte;
	uint8_t newcrc = *crc;

	for (i = 0; i < 8; i++) {
		data_byte ^= newcrc;
		newcrc >>= 1;
		temp_data >>= 1;

		if (data_byte & 0x01)
			newcrc ^= AID_CRC_POLYNOMIAL;

		data_byte = temp_data;
	}
	*crc = newcrc;
}

uint16_t aidNumPayloadBytesForCommand(uint8_t command)
{
 	uint16_t ret = 0;

 	switch (command) {
		case AID_COMMAND_GetState:
		case AID_COMMAND_InterfaceDeviceInfo:
		case AID_COMMAND_InterfaceModuleSerialNumber:
		case AID_COMMAND_AccessorySerialNumber:
		case AID_COMMAND_CheckOperationStatus:
		case AID_COMMAND_GetAccessoryInfoVersion:
		case AID_COMMAND_GetAuthenticationInfo:
		case AID_COMMAND_BulkDataIdentify:
		case AID_COMMAND_CheckAuthentication:
		case AID_COMMAND_GetLastError:
		case AID_COMMAND_ContinueData:
		case AID_RESPONSE_SetState:
		case AID_RESPONSE_ProgramOTP:
		case AID_RESPONSE_ServiceModeACK:
			ret = 0;                // ID command/response has no payload bytes
			break;

		case AID_COMMAND_Test:
		case AID_COMMAND_ServiceModeRequest:
		case AID_COMMAND_SystemInfoString:
		case AID_COMMAND_BulkDataEndpointInfo:
		case AID_RESPONSE_CheckOperationStatus:
		case AID_RESPONSE_CheckAuthentication:
		case AID_RESPONSE_GetLastError:
			ret  = 1;                // ID command/response has 1 payload bytes
			break;

		case AID_COMMAND_ID:
		case AID_COMMAND_SetState:
		case AID_COMMAND_BulkDataContinue:
		case AID_COMMAND_BulkDataRead:
		case AID_RESPONSE_BulkDataIdentify:
		case AID_COMMAND_ProgramOTPStatus:
		case AID_RESPONSE_ProgramOTPStatus:
		case AID_RESPONSE_StartAuthentication:
			ret = 2;                // ID command/response has 2 payload bytes
			break;

		case AID_RESPONSE_BulkDataEndpointInfo:
			ret = 3;                // ID command/response has 3 payload bytes
			break;

		case AID_COMMAND_GetAuthenticationCertificate:
		case AID_COMMAND_GetAuthenticationSignature:
		case AID_COMMAND_ProgramOTP:
		case AID_RESPONSE_GetState:
			ret = 4;                // ID command/response has 4 payload bytes
			break;

		case AID_COMMAND_GetAccessoryInfoString:
		case AID_RESPONSE_GetAuthenticationInfo:
			ret = 5;
			break;

		case AID_RESPONSE_ID:
		case AID_RESPONSE_GetAccessoryInfoVersion:
			ret = 6;                // ID command/response has 6 payload bytes
			break;

		case AID_RESPONSE_GetESNRequest:
		  ret = 9;
		  break;

		case AID_RESPONSE_InterfaceDeviceInfo:
			ret = 10;            // ID command/response has 10 payload bytes
			break;

		case AID_COMMAND_GetESNResponse:
			ret = 16;
			break;

		case AID_COMMAND_StartAuthentication:
		case AID_RESPONSE_InterfaceModuleSerialNumber:
		case AID_RESPONSE_AccessorySerialNumber:
			ret = 20;            // ID command/response has 20 payload bytes
		  	break;

		default:
		case AID_RESPONSE_SystemInfoString:
			ret = AID_NO_RESPONSE_PACKET_FLAG;
			break;

		//// Variable length response, response length provided in the command
		case AID_RESPONSE_GetAccessoryInfoString:
		case AID_RESPONSE_GetAuthenticationCertificate:
		case AID_RESPONSE_GetAuthenticationSignature:
			ret = AID_VARIABLE_LEN_PACKET_FLAG;
			break;

		case AID_RESPONSE_BulkDataRead:
			ret = AID_RESPONSE_LEN_OFFSET_BulkDataRead | AID_VARIABLE_LEN_PACKET_FLAG;
			break;

		case AID_RESPONSE_BulkData:
			ret = AID_RESPONSE_LEN_OFFSET_BulkData | AID_VARIABLE_LEN_PACKET_FLAG;
			break;

		case AID_COMMAND_BulkData:
			ret = AID_COMMAND_LEN_OFFSET_BulkData | AID_VARIABLE_LEN_PACKET_FLAG;
			break;
 	}

 	return ret;
}
