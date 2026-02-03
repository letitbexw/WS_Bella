/*
 * aid.h
 *
 *  Created on: Feb 3, 2026
 *      Author: xiongwei
 */

#ifndef CORE_AID_H_
#define CORE_AID_H_


#include "config.h"

/**********************************************************************
*  AID Bus high-level protocol
********************************************************************* */

#define AID_MAX_ENDPOINT_PACKET_LENGTH	511
#define AID_MAX_PACKET_LENGTH           64 + 1
#define AID_MAX_PAYLOAD_LENGTH          63
#define AID_LEN_MASK                    0x3F
#define AID_VARIABLE_LEN_PACKET_FLAG    0x40
#define AID_NO_RESPONSE_PACKET_FLAG     0x80

#define AID_NO_RESPONSE_PACKET          255

#define AID_CMD_BULK_DATA_MAX_PAYLOAD_LENGTH    58
#define AID_RESP_BULK_DATA_MAX_PAYLOAD_LENGTH   59
#define AID_BULK_DATA_VARIABLE_LENGTH_RESPONSE  255

//Limited by Tristar1
#define AID_RESP_BULK_DATA_MAX_VARIABLE_PAYLOAD_LENGTH  27


#define AID_COMMAND_SetState                          0x70
#define AID_RESPONSE_SetState                         0x71
#define AID_COMMAND_GetState                          0x72
#define AID_RESPONSE_GetState                         0x73
#define AID_COMMAND_ID                                0x74
#define AID_RESPONSE_ID                               0x75
#define AID_COMMAND_InterfaceDeviceInfo               0x76
#define AID_RESPONSE_InterfaceDeviceInfo              0x77
#define AID_COMMAND_InterfaceModuleSerialNumber       0x78
#define AID_RESPONSE_InterfaceModuleSerialNumber      0x79
#define AID_COMMAND_AccessorySerialNumber             0x7A
#define AID_RESPONSE_AccessorySerialNumber            0x7B
//Tristar2 service mode commands
#define AID_COMMAND_GetESNResponse                    0x7C
#define AID_RESPONSE_GetESNRequest                    0x7D
#define AID_COMMAND_ServiceModeRequest                0x7E
#define AID_RESPONSE_ServiceModeACK                   0x7F

#define AID_COMMAND_GetAccessoryInfoString            0x80
#define AID_RESPONSE_GetAccessoryInfoString           0x81
#define AID_COMMAND_GetAccessoryInfoVersion           0x82
#define AID_RESPONSE_GetAccessoryInfoVersion          0x83
#define AID_COMMAND_SystemInfoString                  0x84

#define AID_COMMAND_GetAuthenticationInfo             0x90
#define AID_RESPONSE_GetAuthenticationInfo            0x91
#define AID_COMMAND_GetAuthenticationCertificate      0x92
#define AID_RESPONSE_GetAuthenticationCertificate     0x93
#define AID_COMMAND_StartAuthentication               0x94
#define AID_RESPONSE_StartAuthentication              0x95
#define AID_COMMAND_CheckAuthentication               0x96
#define AID_RESPONSE_CheckAuthentication              0x97
#define AID_COMMAND_GetAuthenticationSignature        0x98
#define AID_RESPONSE_GetAuthenticationSignature       0x99
#define AID_COMMAND_GetLastError                      0x9A
#define AID_RESPONSE_GetLastError                     0x9B

#define AID_COMMAND_BulkDataIdentify                  0xB0
#define AID_RESPONSE_BulkDataIdentify                 0xB1
#define AID_COMMAND_BulkDataEndpointInfo              0xB2
#define AID_RESPONSE_BulkDataEndpointInfo             0xB3
#define AID_RESPONSE_BulkDataReset                    0xBA
#define AID_COMMAND_BulkDataRead                      0xBB
#define AID_RESPONSE_BulkDataRead                     0xBC
#define AID_COMMAND_BulkData                          0xBD
#define AID_RESPONSE_BulkData                         0xBE
#define AID_COMMAND_BulkDataContinue                  0xBF


#define AID_COMMAND_ProgramOTP                        0xE0
#define AID_RESPONSE_ProgramOTP                       0xE1
#define AID_COMMAND_ProgramOTPStatus                  0xE2
#define AID_RESPONSE_ProgramOTPStatus                 0xE3
#define AID_COMMAND_ContinueData                      0xFF
#define AID_RESPONSE_ContinueData                     0 //Special case responds with the length and data!!!

//Non POR commands
#define AID_RESPONSE_SystemInfoString                 0x85
#define AID_COMMAND_CheckOperationStatus              0xF6      // dummy non-HiFive command
#define AID_RESPONSE_CheckOperationStatus             0xF7
#define AID_COMMAND_Test                              0xFA


// Define some response payload lengths -- these are redundant with values in utilsNumPayloadBytesForCommand(),
// but at least they allow for static definitions that are a bit less magic-numbery
#define AID_RESPONSE_PAYLOAD_LENGTH_ID        6
#define AID_RESPONSE_PAYLOAD_LENGTH_IMSN     20
#define AID_RESPONSE_PAYLOAD_LENGTH_ASN      20
#define AID_RESPONSE_PAYLOAD_LENGTH_IDI      10

#define AID_COMMAND_LENGTH_PROGRAM_OTP        5
#define AID_COMMAND_LENGTH_OTP_STATUS         3



// Define VID and PID values for SN2025 implementation of HiFive
#define VID_SN2025       0x01
#define PID_SN2025       0x25

// Define bits corresponding to elements of the ID response
#define ACCx_MASK         0xC0
#define ACCx_NORMAL       0x00
#define ACCx_AP_DEBUG     0x40
#define ACCx_JTAG         0x80
#define ACCx_HOST_RESET   0xC0
#define DI    0x10

// Define bits corresponding to elements of the Get State response
#define PH    0x8000              // power handshake
#define PT    0x4000              // ID passthrough

// Define bits corresponding to elements of the Interface Device Info response
#define AV    0x80


// AID Response Defines
#define AID_RESPONSE_LEN                    6                       // Length of ID Response

// AID Response Byte Defines
#define AID_RESPONSE_BYTE0                  0                       // AID Response Byte 0
#define AID_RESPONSE_BYTE1                  1                       // AID Response Byte 1
#define AID_RESPONSE_BYTE2                  2                       // AID Response Byte 2
#define AID_RESPONSE_BYTE3                  3                       // AID Response Byte 3
#define AID_RESPONSE_BYTE4                  4                       // AID Response Byte 4
#define AID_RESPONSE_BYTE5                  5                       // AID Response Byte 5

// Byte 0
#define AID_RESPONSE_UART_MASK              0x03                    // AID UART Response Bit Mask
#define AID_RESPONSE_ACCx_MASK              0xC0                    // AID ACCx Response Bit Mask
#define AID_RESPONSE_USB_MASK               0x18                    // AID USB Response Bit Mask

// Byte 0
#define AID_RESPONSE_UART_NONE              (0x00 << 0)
#define AID_RESPONSE_UART_19200             (0x01 << 0)
#define AID_RESPONSE_UART_57600             (0x02 << 0)
#define AID_RESPONSE_UART_115200            (0x03 << 0)
#define AID_RESPONSE_MIKEYBUS               (0x01 << 2)
#define AID_RESPONSE_USB_NONE               (0x00 << 3)
#define AID_RESPONSE_USB_HOST               (0x01 << 3)
#define AID_RESPONSE_USB_DEVICE             (0x02 << 3)
#define AID_RESPONSE_USB_ALLOCATED          (0x03 << 3)
#define AID_RESPONSE_DEBUGBIT               (0x01 << 5)
#define AID_RESPONSE_ACCx_NORMAL            (0x00 << 6)
#define AID_RESPONSE_ACCx_APDEBUG           (0x01 << 6)
#define AID_RESPONSE_ACCx_JTAG              (0x02 << 6)
#define AID_RESPONSE_ACCx_HOSTRESET         (0x04 << 6)

// Byte 1
#define AID_RESPONSE_HIGHVOLTAGE_MASK       0x0C
#define AID_RESPONSE_POWER_MASK             0xF0

// Byte 1
#define AID_RESPONSE_AUTH                   (0x01 << 0)
#define AID_RESPONSE_ACCINFO                (0x01 << 1)
#define AID_RESPONSE_HIGHVOLTAGE_20V        (0x03 << 2)
#define AID_RESPONSE_HIGHVOLTAGE_16V        (0x02 << 2)
#define AID_RESPONSE_HIGHVOLTAGE_5V_RSVD_1  (0x01 << 2)
#define AID_RESPONSE_HIGHVOLTAGE_5V         (0x00 << 2)
#define AID_RESPONSE_POWER_0V               (0x00 << 4)
#define AID_RESPONSE_POWER_0V_RSVD_1        (0x01 << 4)
#define AID_RESPONSE_POWER_0V_RSVD_2        (0x02 << 4)
#define AID_RESPONSE_POWER_0V_RSVD_3        (0x03 << 4)
#define AID_RESPONSE_POWER_2V5              (0x04 << 4)
#define AID_RESPONSE_POWER_2V6              (0x05 << 4)
#define AID_RESPONSE_POWER_2V7              (0x06 << 4)
#define AID_RESPONSE_POWER_2V8              (0x07 << 4)
#define AID_RESPONSE_POWER_2V9              (0x08 << 4)
#define AID_RESPONSE_POWER_3V0              (0x09 << 4)
#define AID_RESPONSE_POWER_3V1              (0x0A << 4)
#define AID_RESPONSE_POWER_3V2              (0x0B << 4)
#define AID_RESPONSE_POWER_3V3              (0x0C << 4)
#define AID_RESPONSE_POWER_3V3_RSVD_D       (0x0D << 4)
#define AID_RESPONSE_POWER_3V3_RSVD_E       (0x0E << 4)
#define AID_RESPONSE_POWER_UHPM             (0x0F << 4)

// Byte 2 (byte 3 in idbspec)
#define AID_RESPONSE_BD                     (0x01 << 0) // Bulk Data over AID
#define AID_RESPONSE_DI                     (0x01 << 4)
#define AID_RESPONSE_CO                     (0x01 << 5)
#define AID_RESPONSE_BP                     (0x01 << 6)
#define AID_RESPONSE_PU                     (0x01 << 7)

// Byte 3 (byte 4 in idbspec)
#define AID_RESPONSE_HPM_MODE12             (0x00 << 0) // HPM bypass 1 (300ma @ Vbatt iPod) or 2 (500mA @ Vbatt iPad/iPhone)
#define AID_RESPONSE_HPM_MODE3              (0x01 << 0) // HPM bypass 3 (1000mA @ 3.3V)
#define AID_RESPONSE_HPM_SHPM               (0x02 << 0) // SHPM (1000mA @ 5V)
#define AID_RESPONSE_HPM_HEM                (0x03 << 0) // High Efficiency mode
#define AID_RESPONSE_DJ                     (0x01 << 3) // Alternate Debug JTAG mode
#define AID_RESPONSE_HS                     (0x07 << 4) // High Speed Mask
#define AID_RESPONSE_HS_NONE                (0x00 << 4) // High Speed - none
#define AID_RESPONSE_HS_THUNDERBOLT         (0x01 << 4) // High Speed - Thunderbolt
#define AID_RESPONSE_HS_SSUSB               (0x02 << 4) // High Speed - SS USB
#define AID_RESPONSE_PO_TOP                 (0x01 << 7) // Plug Orientation (Top Side)

// Get/SetState values
#define AID_SETSTATE_PH                     (0x01 << 7)  // set to enable full charge current
#define AID_SETSTATE_PT                     (0x01 << 6)  // set to enable id passthrough


#define AID_COMMAND_LEN_OFFSET_BulkData         2
#define AID_RESPONSE_LEN_OFFSET_BulkDataRead    1
#define AID_RESPONSE_LEN_OFFSET_BulkData        1


// Define Accessory Info String index values
#define AID_AccessoryInfoString_ManufacturerIndex     0
#define AID_AccessoryInfoString_ModelNumberIndex      1
#define AID_AccessoryInfoString_NameIndex             2

#define AID_BULK_DATA_VERSION                CNFG_AID_BULK_DATA_VERSION

#define AID_ENDPOINT_FUNCTION_RESERVED        0
#define AID_ENDPOINT_FUNCTION_EA              1
#define AID_ENDPOINT_FUNCTION_HID             2
#define AID_ENDPOINT_FUNCTION_DEVICE_CONTROL  3
#define AID_ENDPOINT_FUNCTION_IAP             4
#define AID_ENDPOINT_FUNCTION_POWER_DELIVERY  5


#define AID_CMD_BULK_DATA_CMD_OFFSET          0
#define AID_CMD_BULK_DATA_SEQ_OFFSET          1
#define AID_CMD_BULK_DATA_CMD_LEN_OFFSET      2
#define AID_CMD_BULK_DATA_RESP_LEN_OFFSET     3
#define AID_CMD_BULK_DATA_PTPKT_OFFSET        4
#define AID_CMD_BULK_DATA_ENDPT_OFFSET        5
#define AID_CMD_BULK_DATA_PAYLOAD_OFFSET      6

#define AID_CMD_BULK_DATA_CMD_LEN_PARTIAL_PAYLOAD_MASK    0x80
#define AID_RESP_BULK_DATA_CMD_READ_PENDING_MASK          0x40

#define AID_CMD_BULK_DATA_HEADER_LENGTH				(6 - 1) //Not counting the IDIO command and CRC
#define AID_RESP_BULK_DATA_HEADER_LENGTH			(5 - 1) //Not counting CRC


#define AID_RESP_BULK_DATA_CMD_OFFSET          0
#define AID_RESP_BULK_DATA_SEQ_OFFSET          1
#define AID_RESP_BULK_DATA_LEN_OFFSET          2
#define AID_RESP_BULK_DATA_PTPKT_OFFSET        3
#define AID_RESP_BULK_DATA_ENDPT_OFFSET        4
#define AID_RESP_BULK_DATA_PAYLOAD_OFFSET      5



#define AID_CMD_BULK_DATA_READ_CMD_OFFSET          0
#define AID_CMD_BULK_DATA_READ_SEQ_OFFSET          1
#define AID_CMD_BULK_DATA_READ_RESP_LEN_OFFSET     2


#define AID_RESP_BULK_DATA_ERROR_RESET             0
#define AID_RESP_BULK_DATA_ERROR_UNKNOWN           1
#define AID_RESP_BULK_DATA_ERROR_UNS_CMD           2
#define AID_RESP_BULK_DATA_ERROR_UNS_EP            3
#define AID_RESP_BULK_DATA_ERROR_INVALID           4
#define AID_RESP_BULK_DATA_ERROR_HID_NOT_READY    64
#define AID_RESP_BULK_DATA_ERROR_HID_INV_REPORT   65
#define AID_RESP_BULK_DATA_ERROR_HID_DEV_ERROR    66

// AID UART Response Enumeration
enum eAidResponseUart
{
	aid_uart_none   = AID_RESPONSE_UART_NONE,
	aid_uart_19200  = AID_RESPONSE_UART_19200,
	aid_uart_57600  = AID_RESPONSE_UART_57600,
	aid_uart_115200 = AID_RESPONSE_UART_115200
};

// AID USB Response Enumeration
enum eAidResponseUsb
{
	aid_usb_none   		= AID_RESPONSE_USB_NONE,
	aid_usb_host   		= AID_RESPONSE_USB_HOST,
	aid_usb_device 		= AID_RESPONSE_USB_DEVICE,
	aid_usb_allocated 	= AID_RESPONSE_USB_ALLOCATED
};

// AID ACCx Response Enumeration
enum eAidResponseAccx
{
	aid_accx_normal 	= AID_RESPONSE_ACCx_NORMAL,
	aid_accx_apdebug 	= AID_RESPONSE_ACCx_APDEBUG,
	aid_accx_jtag 		= AID_RESPONSE_ACCx_JTAG,
	aid_accx_hostreset 	= AID_RESPONSE_ACCx_HOSTRESET
};



typedef union
{
	uint8_t buffer[AID_MAX_PACKET_LENGTH];
	struct
	{
		uint8_t packetType;
		uint8_t payload[AID_MAX_PAYLOAD_LENGTH];
		uint8_t crc;
	}bytes;
} AID_GENERIC_PACKET;


typedef struct {
	uint16_t 	payloadLen;	//payload length
	uint16_t 	respPayloadExpLen;	//Expected response length
	uint8_t 	multiPacketFlag;
	uint8_t 	packetProcessFlag;
	uint8_t 	seqNumber;
	uint8_t 	endpoint;
	uint8_t 	endpointPacketType;
	uint16_t 	byteOffset;
	uint8_t * 	payloadPtr;
	uint8_t 	buffer[AID_MAX_ENDPOINT_PACKET_LENGTH];
} AID_ENDPOINT_PACKET_Type;


typedef struct {
	uint8_t 			length;
	AID_GENERIC_PACKET 	packet;
	uint8_t 			state;
	uint8_t 			expCount;
	uint32_t 			timeStamp;
} AID_CMD_Type;


typedef struct {
	uint8_t 	status;
	uint8_t 	data[AID_RESPONSE_LEN];
	uint32_t 	timeStamp;
} AID_ID_RESP_Type;

typedef struct {
	uint8_t length;
	AID_GENERIC_PACKET packet;
} AID_RESP_Type;


void aidInitCRC(uint8_t *crc);
void aidUpdateCRC(uint8_t *crc, uint8_t data_byte);
uint16_t aidNumPayloadBytesForCommand(uint8_t command);

// Print the AID Response
void aidPrintResponse(uint8_t* id_response);


#endif /* CORE_AID_H_ */
