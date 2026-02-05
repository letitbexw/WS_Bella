/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "idbus_uart.h"
#include "timers.h"
#include "idio.h"
#ifdef CNFG_AID_HAS_BULK_DATA
#include "idio_bulk_data.h"
#endif
#include "main.h"
#include "bsp.h"
#include "debug.h"
#include "sn.h"
#if defined(CNFG_AID_POWER_CONTROL) || defined(CNFG_AID_ID_VARIABLE_RESPONSE)
#include "orion.h"
#endif

#define TX_BUFFER     // send data using buffered IO

/* Private typedef -----------------------------------------------------------*/
typedef struct _IDIO_IF_DATA
{
	AID_CMD_Type idioRxPacket;
	AID_RESP_Type idioResponsePacket;
} idio_IF_Data_TypeDef;

/* Private macro -------------------------------------------------------------*/
//#define CNFG_DEBUG_IDIO_COMMANDS

// access to device unique id
#define         Device1_Identifier          (0x1FF80050)
#define         Device2_Identifier          (0x1FF80054)
#define         Device3_Identifier          (0x1FF80064)

/* Private variables ---------------------------------------------------------*/
static const uint8_t accessoryInfoStringsManufacturer[] = CNFG_ACCESSORY_MANUFACTURER;
static const uint8_t accessoryInfoStringsModel[] 		= CNFG_ACCESSORY_MODEL_NUMBER;
static const uint8_t accessoryInfoStringsName[] 		= CNFG_ACCESSORY_NAME_STRING;

static const uint8_t accessoryInfoStringLengths[3] = {
	sizeof (accessoryInfoStringsManufacturer),
	sizeof (accessoryInfoStringsModel),
	sizeof (accessoryInfoStringsName)   };

#ifndef CNFG_AID_ID_VARIABLE_RESPONSE
static const uint8_t defaultCommandIdResponse[6] = {
	AID_RESPONSE_ACCx_NORMAL | AID_RESPONSE_USB_NONE,
	AID_RESPONSE_POWER_UHPM | AID_RESPONSE_HIGHVOLTAGE_20V,
	0,
	0,
	0,
	0,
};
#endif

static uint8_t idioEnabled = false;
static volatile AID_ID_RESP_Type idbusCurrentIDResponse;

extern UART_HandleTypeDef idbusUartHandle;
static idio_IF_Data_TypeDef ifData;
static idio_IF_Prop_TypeDef* ifOps;  //All[CNFG_IDIO_NUMBER_INTERFACES];

static idio_IF_Prop_TypeDef idioIfOps =
{
	&idbusUartHandle,
	&idbusUartInit,
	&idbusRx,
	&idbusTx,
	&idbusTxBuffer,
	&idbusRxCount,
	&idbusDriveBusLow,
#ifdef CNFG_AID_POWER_CONTROL
	&setOrionInterfacePower,
	&getOrionInterfacePower,
#endif
#ifdef CNFG_AID_ID_VARIABLE_RESPONSE
	&getOrionId,
#endif
};

//static void (*idioSlaveIDResponseCallback)( void) = NULL;

//States for IDBUS process
enum {
	IDRX_WAITBREAK,   		// Waiting for a BREAK signal to indicate start of a command
	IDRX_WAITCMD,     		// Break received, waiting for a command
	IDRX_WAITDATA,    		// Waiting for the data that comes after the command
	IDRX_WAITCRC,     		// Waiting for the final CRC
	IDRX_WAIT_TRAILBREAK,  	// Wait for BREAK after CRC
};

/**********************************************************************
*  Command/Response protocol handler
********************************************************************* */

//0x80 --> 0x81
static uint32_t idioSendGetAccessoryInfoStringResponse(AID_CMD_Type * idioCommandPacketPtr, AID_RESP_Type * idioResponsePacketPtr)
{
	uint8_t accInfoStringID = idioCommandPacketPtr->packet.bytes.payload[0];	// InfoStringID
	uint32_t stringOffset 	= 0;
	uint32_t retLength 		= 0;
	uint32_t i 				= 0;
	uint32_t j 				= 0;
	const uint8_t * infoStringPtr = NULL;
	uint32_t ret = IDIO_RESPONSE_VALID;

	stringOffset = ((uint16_t) idioCommandPacketPtr->packet.bytes.payload[1] << 8) | ((uint16_t) idioCommandPacketPtr->packet.bytes.payload[2]);
	retLength = ((uint16_t) idioCommandPacketPtr->packet.bytes.payload[3] << 8) | ((uint16_t) idioCommandPacketPtr->packet.bytes.payload[4]);

	if ((accInfoStringID > AID_AccessoryInfoString_NameIndex)||(retLength > AID_MAX_PAYLOAD_LENGTH))
	{
		//Invalid param
		ret = IDIO_RESPONSE_PARAM_ERROR;
	}

	if ((retLength + stringOffset) > 0xFFFF)
	{
		//Invalid param
		ret = IDIO_RESPONSE_PARAM_ERROR;
	}

	if (ret == IDIO_RESPONSE_VALID)
	{
		switch(accInfoStringID)
		{
			case 0:
				infoStringPtr = accessoryInfoStringsManufacturer;
				break;

			case 1:
				infoStringPtr = accessoryInfoStringsModel;
				break;

			case 2:
				infoStringPtr = accessoryInfoStringsName;
				break;

			default:
				infoStringPtr = NULL;
				break;
		}
		//
		idioResponsePacketPtr->length = retLength;
		idioResponsePacketPtr->packet.bytes.packetType = AID_RESPONSE_GetAccessoryInfoString;	//0x81

		// return the info string to the host
		for(i = stringOffset; i < (retLength + stringOffset); i++)
		{
			if (i < accessoryInfoStringLengths[accInfoStringID])
			{
				idioResponsePacketPtr->packet.bytes.payload[j] = infoStringPtr[i];
			}
			else
			{
				idioResponsePacketPtr->packet.bytes.payload[j] = 0;
			}
			j++;
		}
	}

	return ret;
}

//0x82 --> 0x83
static uint32_t idioSendGetAccessoryInfoVersionResponse(AID_CMD_Type * idioCommandPacketPtr, AID_RESP_Type * idioResponsePacketPtr)
{
	idioResponsePacketPtr->length 				   = aidNumPayloadBytesForCommand(AID_RESPONSE_GetAccessoryInfoVersion);
	idioResponsePacketPtr->packet.bytes.packetType = AID_RESPONSE_GetAccessoryInfoVersion;	// 0x83

	idioResponsePacketPtr->packet.bytes.payload[0] = CNFG_FW_VERSION_MAJ;
	idioResponsePacketPtr->packet.bytes.payload[1] = CNFG_FW_VERSION_MIN;
	idioResponsePacketPtr->packet.bytes.payload[2] = CNFG_FW_REVISION;

	idioResponsePacketPtr->packet.bytes.payload[3] = CNFG_HW_VERSION_MAJ;
	idioResponsePacketPtr->packet.bytes.payload[4] = CNFG_HW_VERSION_MIN;
	idioResponsePacketPtr->packet.bytes.payload[5] = CNFG_HW_REVISION;

	return IDIO_RESPONSE_VALID;
}

void idioProcessCommand(AID_CMD_Type * idioRxPacketPtr, AID_RESP_Type * idioResponsePacketPtr, idio_IF_Prop_TypeDef *ifOps)
{
	uint8_t i;
	uint32_t ret = IDIO_RESPONSE_NONE;

#ifdef CNFG_DEBUG_IDIO_ENABLED
	if (configDebugIdioPrintEnabled())
	{
		printf("[%08u] %d: Rx %02X ", GetTickCount(), __LINE__, idioRxPacketPtr->packet.bytes.packetType);
		debugPrintHexStream(idioRxPacketPtr->length, &idioRxPacketPtr->packet.bytes.payload[0], true);
	}
#endif
	switch (idioRxPacketPtr->packet.bytes.packetType)
	{
#ifdef CNFG_AID_ID_RESPONSE
    	case AID_COMMAND_ID:	// 0x74 --> 0x75
    	{
    		uint16_t systemId;
#ifdef CNFG_AID_POWER_CONTROL
    		// set PH bit high per SN2025 spec
    		ifOps->setIfPower(true);
#endif
    		systemId = idioRxPacketPtr->packet.bytes.payload[0] << 8 | idioRxPacketPtr->packet.bytes.payload[1];	// HOST ID
    		idioResponsePacketPtr->length = 6;	// Not include CRC and CMD ID
    		idioResponsePacketPtr->packet.bytes.packetType = AID_RESPONSE_ID;	// 0x75
#ifdef CNFG_AID_ID_VARIABLE_RESPONSE
    		memcpy(&idioResponsePacketPtr->packet.bytes.payload[0], ifOps->getId(systemId), 6);		// Return the same HOST ID back
#else
    		(void)systemId;
    		memcpy(&idioResponsePacketPtr->packet.bytes.payload[0], &defaultCommandIdResponse[0], 6);
#endif
    		ret = IDIO_RESPONSE_VALID;
    	}
    		break;
#endif

    	case AID_RESPONSE_ID:	// 0x75, how cound host send a response_id command?
    		// Copy the ID to the current buffer.
    		idbusCurrentIDResponse.status = true;
    		for (i = 0; i < idioRxPacketPtr->length; i++){
    			idbusCurrentIDResponse.data[i] = idioRxPacketPtr->packet.bytes.payload[i];
    		}
    		break;

    	case AID_COMMAND_SetState:	// 0x70 --> 0x71
			idioResponsePacketPtr->length = 0;
			idioResponsePacketPtr->packet.bytes.packetType = AID_RESPONSE_SetState;	//0x71

#ifdef CNFG_AID_POWER
			ifOps->setIfPower((idioRxPacketPtr->packet.bytes.payload[0] & AID_SETSTATE_PH) != 0);
#endif
			ret = IDIO_RESPONSE_VALID;
			break;

    	case AID_COMMAND_GetState:	// 0x72 --> 0x73
			idioResponsePacketPtr->length = aidNumPayloadBytesForCommand(AID_RESPONSE_GetState);;
			idioResponsePacketPtr->packet.bytes.packetType = AID_RESPONSE_GetState;	// 0x73
#ifdef CNFG_AID_POWER
			idioResponsePacketPtr->packet.bytes.payload[0] = ifOps->getIfPower() ? AID_SETSTATE_PH : 0;
#else
			idioResponsePacketPtr->packet.bytes.payload[0] = 0;
#endif
			idioResponsePacketPtr->packet.bytes.payload[1] = 0;		// rsvd
#ifdef CNFG_AID_POWER
			idioResponsePacketPtr->packet.bytes.payload[2] = AID_SETSTATE_PH;
#else
			idioResponsePacketPtr->packet.bytes.payload[2] = 0;
#endif
			idioResponsePacketPtr->packet.bytes.payload[3] = 0;		// rsvd

			ret = IDIO_RESPONSE_VALID;
			break;

    	case AID_COMMAND_InterfaceDeviceInfo:	// 0x76 --> 0x77
    	{
    		union serial_type {
    			uint8_t bytes[8];
    			uint32_t raw[2];
    		};
    		union serial_type sn;

			idioResponsePacketPtr->length 				   = aidNumPayloadBytesForCommand(AID_RESPONSE_InterfaceDeviceInfo);
			idioResponsePacketPtr->packet.bytes.packetType = AID_RESPONSE_InterfaceDeviceInfo;	// 0x77
			idioResponsePacketPtr->packet.bytes.payload[0] = CNFG_AID_VID; 			// VID
			idioResponsePacketPtr->packet.bytes.payload[1] = CNFG_AID_PID; 			// PID
			idioResponsePacketPtr->packet.bytes.payload[2] = CNFG_AID_IF_REVISION; 	// Rev
			idioResponsePacketPtr->packet.bytes.payload[3] = AV; 					// Accessory Vendor
			sn.raw[0] = *(uint32_t*)Device1_Identifier;
			sn.raw[1] = *(uint32_t*)Device2_Identifier;
			idioResponsePacketPtr->packet.bytes.payload[4] = sn.bytes[5]; // Id Serial [5]
			idioResponsePacketPtr->packet.bytes.payload[5] = sn.bytes[4]; // Id Serial [4]
			idioResponsePacketPtr->packet.bytes.payload[6] = sn.bytes[3]; // Id Serial [3]
			idioResponsePacketPtr->packet.bytes.payload[7] = sn.bytes[2]; // Id Serial [2]
			idioResponsePacketPtr->packet.bytes.payload[8] = sn.bytes[1]; // Id Serial [1]
			idioResponsePacketPtr->packet.bytes.payload[9] = sn.bytes[0]; // Id Serial [0]

			ret = IDIO_RESPONSE_VALID;
    	}
    		break;

    	case AID_COMMAND_InterfaceModuleSerialNumber:	// 0x78 --> 0x79
    	{
			uint16_t length;
			idioResponsePacketPtr->length 					= aidNumPayloadBytesForCommand(AID_RESPONSE_InterfaceModuleSerialNumber);
			idioResponsePacketPtr->packet.bytes.packetType 	= AID_RESPONSE_InterfaceModuleSerialNumber;	// 0x79
			memset(&idioResponsePacketPtr->packet.bytes.payload[0], 0, idioResponsePacketPtr->length);
			memcpy(&idioResponsePacketPtr->packet.bytes.payload[0], getModuleSN(&length), length);
			ret = IDIO_RESPONSE_VALID;
			break;
    	}

    	case AID_COMMAND_AccessorySerialNumber:		// 0x7A --> 0x7B
    	{
			uint16_t length;
			idioResponsePacketPtr->length = aidNumPayloadBytesForCommand(AID_RESPONSE_AccessorySerialNumber);
			idioResponsePacketPtr->packet.bytes.packetType = AID_RESPONSE_AccessorySerialNumber;	// 0x7B
			memset(&idioResponsePacketPtr->packet.bytes.payload[0], 0, idioResponsePacketPtr->length);
			memcpy(&idioResponsePacketPtr->packet.bytes.payload[0], getProductSN(&length), length);
			ret = IDIO_RESPONSE_VALID;
			break;
    	}

    	case AID_COMMAND_GetAccessoryInfoString:	// 0x80 --> 0x81
    		ret = idioSendGetAccessoryInfoStringResponse(idioRxPacketPtr, idioResponsePacketPtr);
    		break;
    	case AID_COMMAND_GetAccessoryInfoVersion:	// 0x82 --> 0x83
    		ret = idioSendGetAccessoryInfoVersionResponse(idioRxPacketPtr,  idioResponsePacketPtr);
    		break;

#ifdef CNFG_AID_HAS_BULK_DATA
		case AID_COMMAND_BulkDataIdentify:
		case AID_COMMAND_BulkDataEndpointInfo:
		case AID_COMMAND_BulkDataRead:
		case AID_COMMAND_BulkData:
		case AID_COMMAND_BulkDataContinue:
			ret = idioBulkDataHandler(idioRxPacketPtr, idioResponsePacketPtr);
			break;
#endif

		default:
			break;
	}

    if (ret == IDIO_RESPONSE_VALID) { idioSendResponse(idioResponsePacketPtr); }

#ifdef CNFG_DEBUG_IDIO_COMMANDS
	printf("\n IDBUS Cmd 0x%02X, ", idioRxPacketPtr->packet.bytes.packetType);
	for (i = 0; i < idioResponsePacketPtr->length; i++) printf (" 0x%02X", idioResponsePacketPtr->packet.bytes.payload[i]);
#endif
}


uint32_t idioProcessRxSymbol(uint16_t sym, AID_CMD_Type * idioRxPacketPtr, AID_RESP_Type * idioResponsePacketPtr, idio_IF_Prop_TypeDef *ifOps)
{
	uint32_t retVal = 0;
	//printf("[%02X,%01X]",sym,idioRxPacketPtr->state);

	//Reset the state machine, if we see a wake or break (other than trailing break)
	if (sym == IDBUS_WAKE) { idioRxPacketPtr->state = IDRX_WAITBREAK; }		// 0xFFCC

	if ((idioRxPacketPtr->state != IDRX_WAIT_TRAILBREAK) && (sym == IDBUS_BREAK)) {
		idioRxPacketPtr->state = IDRX_WAITBREAK;
	}

	switch (idioRxPacketPtr->state)
	{
		default:
		case IDRX_WAITBREAK:
			if ((sym == IDBUS_BREAK)||(sym == IDBUS_WAKE)) {
				idioRxPacketPtr->state = IDRX_WAITCMD;
#ifdef CNFG_AID_HAS_BULK_DATA
				//If we have a bulk data read command pending, clear it.
				idioBulkDataClearReadPendingFlag();
#endif
			}
			break;

		case IDRX_WAITCMD:
			//Process the symbol of if its a valid data byte.
			if (sym <= 0xFF)
			{
				aidInitCRC(&idioRxPacketPtr->packet.bytes.crc);

				idioRxPacketPtr->packet.bytes.packetType = (uint8_t) sym;
				idioRxPacketPtr->length = 0;

				aidUpdateCRC(&idioRxPacketPtr->packet.bytes.crc,idioRxPacketPtr->packet.bytes.packetType);

				idioRxPacketPtr->expCount = aidNumPayloadBytesForCommand(sym);	// Expected count

				if (idioRxPacketPtr->expCount == 0) {
					// ID command/response has no payload bytes
					idioRxPacketPtr->state = IDRX_WAITCRC;
				}
				else if (idioRxPacketPtr->expCount <= AID_MAX_PAYLOAD_LENGTH) {
					idioRxPacketPtr->state = IDRX_WAITDATA;
				}
				else if (idioRxPacketPtr->expCount & AID_VARIABLE_LEN_PACKET_FLAG){
					//Variable length data
					idioRxPacketPtr->state = IDRX_WAITDATA;
				}
				else {
					// unknown command
					idioRxPacketPtr->state = IDRX_WAITBREAK;
#ifdef CNFG_DEBUG_IDIO_ENABLED
					if (configDebugIdioPrintEnabled())
					{
						printf("[%08u] %d: Invalid Command %02X ", GetTickCount(), __LINE__, idioRxPacketPtr->packet.bytes.packetType);
						debugPrintHexStream(idioRxPacketPtr->length, &idioRxPacketPtr->packet.bytes.payload[0], true);
					}
#endif
				}
			}
			break;

		case IDRX_WAITDATA:
			if (idioRxPacketPtr->length < AID_MAX_PACKET_LENGTH)
			{
				aidUpdateCRC(&idioRxPacketPtr->packet.bytes.crc,(uint8_t) sym);
				idioRxPacketPtr->packet.bytes.payload[idioRxPacketPtr->length] = sym;
				idioRxPacketPtr->length++;

				/* if we have all payload bytes, wait for the CRC and we're done */
				if (idioRxPacketPtr->expCount == idioRxPacketPtr->length) {
					idioRxPacketPtr->state = IDRX_WAITCRC;
				}
				else if (idioRxPacketPtr->expCount & AID_VARIABLE_LEN_PACKET_FLAG)
				{	// Variable length
					if (idioRxPacketPtr->length == (idioRxPacketPtr->expCount & AID_LEN_MASK))
					{
						switch(idioRxPacketPtr->packet.bytes.packetType) {
							case AID_RESPONSE_BulkDataRead:
							case AID_RESPONSE_BulkData:
								idioRxPacketPtr->expCount = (sym & AID_LEN_MASK) + 4;	// Update the expected count and clear AID_VARIABLE_LEN_PACKET_FLAG
								break;

							case AID_COMMAND_BulkData:
								idioRxPacketPtr->expCount = (sym & AID_LEN_MASK) + 5;
								break;

							default:
								// malformed command
								idioRxPacketPtr->state = IDRX_WAITBREAK;
#ifdef CNFG_DEBUG_IDIO_ENABLED
								if (configDebugIdioPrintEnabled())
								{
									printf("[%08u] %d: Malformed Command %02X ", GetTickCount(), __LINE__, idioRxPacketPtr->packet.bytes.packetType);
									debugPrintHexStream(idioRxPacketPtr->length, &idioRxPacketPtr->packet.bytes.payload[0], true);
								}
#endif
								break;
						}

						if (idioRxPacketPtr->expCount > AID_MAX_PAYLOAD_LENGTH) {
							// malformed command
							idioRxPacketPtr->state = IDRX_WAITBREAK;
#ifdef CNFG_DEBUG_IDIO_ENABLED
							if (configDebugIdioPrintEnabled())
							{
								printf("[%08u] %d: Cmd too long %02X ", GetTickCount(), __LINE__, idioRxPacketPtr->packet.bytes.packetType);
								debugPrintHexStream(idioRxPacketPtr->length, &idioRxPacketPtr->packet.bytes.payload[0], true);
							}
#endif
						//TODO: Is this optimal - recheck.
						//idioRxPacketPtr->expCount = AID_MAX_PAYLOAD_LENGTH;
						}
					}
				}
			}
			else {
				/* malformed command */
				idioRxPacketPtr->state = IDRX_WAITBREAK;
			}
			break;

		case IDRX_WAITCRC:
			if (sym == idioRxPacketPtr->packet.bytes.crc) {
				idioRxPacketPtr->state = IDRX_WAIT_TRAILBREAK;
				idioRxPacketPtr->timeStamp = GetTickCount();
			}
			else {
				// Bad CRC detected.
				idioRxPacketPtr->state = 0xFF;   //CRC Error state
#ifdef CNFG_DEBUG_IDIO_ENABLED
				if (configDebugIdioPrintEnabled())
				{
					printf("[%08u] %d: Bad CRC %02X ", GetTickCount(), __LINE__, idioRxPacketPtr->packet.bytes.packetType);
					debugPrintHexStream(idioRxPacketPtr->length, &idioRxPacketPtr->packet.bytes.payload[0], true);
				}
#endif
				idioRxPacketPtr->state = IDRX_WAITBREAK;
			}
			break;

		case IDRX_WAIT_TRAILBREAK:
			if (sym == IDBUS_BREAK) {
				idioProcessCommand(idioRxPacketPtr, idioResponsePacketPtr, ifOps);	// Received the whole command
				retVal = idioRxPacketPtr->packet.bytes.packetType;
				idioRxPacketPtr->state = IDRX_WAITBREAK;
			}
			else if ((GetTickCount() - idioRxPacketPtr->timeStamp) > 2) {     // 1ms max to get trailing break
				idioRxPacketPtr->state = IDRX_WAITBREAK;
			}
			break;
	}
	return retVal;
}

#if 0
static volatile uint16_t idioTimeExpired = false;

static uint16_t idioTimeoutEventHandler(void)
{
	idioTimeExpired = true;
	return(0);
}

static void idioSetTimeout(uint16_t num_ms)
{
	uint32_t primask;
	static tmrFuncType tmrIdioTimeout = {NULL, 0, idioTimeoutEventHandler};
	targetGetInterruptState(primask);
	targetDisableInterrupts();

	if (tmrIdioTimeout.mS == 0){
		tmrFuncAdd(&tmrIdioTimeout);
	}
	tmrIdioTimeout.mS = num_ms;
	idioTimeExpired = false;

	targetRestoreInterruptState(primask);
}
#endif

//idbusSendSlaveResponse is blocking, modify to allow for interrupt driven TX
int idioSendResponse(AID_RESP_Type * idioResponsePacketPtr)
{
	uint8_t respcrc = 0;
	int ret = 0;
	int i;

	// If we are not in TX / Emulation mode return
	//if (ifOps->getMode() != IDBUS_MODE_SLAVE_EMULATION) return;

	tmrDelay_us(20);  //Need to tweak this!!! CAC_FIX

	/* Compute the CRC while sending */
	aidInitCRC(&respcrc);
#ifdef TX_BUFFER
	aidUpdateCRC(&respcrc,idioResponsePacketPtr->packet.bytes.packetType);
	for (i = 0; i < idioResponsePacketPtr->length; i++) {
		aidUpdateCRC(&respcrc,idioResponsePacketPtr->packet.bytes.payload[i]);
	}
	idioResponsePacketPtr->packet.bytes.payload[i] = respcrc;

	ret = ifOps->txBuffer(&idioResponsePacketPtr->packet.buffer[0], idioResponsePacketPtr->length + 2);
#else
	aidUpdateCRC(&respcrc,idioResponsePacketPtr->packet.bytes.packetType);
	ifOps->tx(idioResponsePacketPtr->packet.bytes.packetType);

	for (i = 0; i < idioResponsePacketPtr->length; i++) {
		aidUpdateCRC(&respcrc,idioResponsePacketPtr->packet.bytes.payload[i]);
		ifOps->tx(idioResponsePacketPtr->packet.bytes.payload[i]);
	}

	ifOps->tx(respcrc);
#endif
#ifdef CNFG_DEBUG_IDIO_ENABLED
	if (configDebugIdioPrintEnabled())
	{
		printf("[%08u] %d: Tx %02X ", GetTickCount(), __LINE__, idioResponsePacketPtr->packet.bytes.packetType);
		debugPrintHexStream(idioResponsePacketPtr->length, &idioResponsePacketPtr->packet.bytes.payload[0], true);
	}
#endif
	return ret;
}

//Send a wake only if TX is IDLE
bool idioSendWake(void)
{
	idbusSendWake();
	return true;
}

uint32_t idioIdleTime(void)
{
	return((uint32_t) GetTickCount() - idbusTimeOfLastActivityOnTheBus);
}

bool idioIsIdle(uint32_t timeMs)
{
	return (GetTickCount() - idbusTimeOfLastActivityOnTheBus > timeMs);
}

void idioService(void)
{
	uint16_t sym;

	if (idioEnabled) {
		if (ifOps->rxCount() > 0) {
			while (ifOps->rx(&sym)) {
#ifdef CNFG_DEBUG_IDIO_COMMANDS
				if (sym == IDBUS_BREAK) { printf("\nBR "); }
				else { printf("%02X ",sym); }
#endif
				idioProcessRxSymbol(sym, (AID_CMD_Type *) &ifData.idioRxPacket, (AID_RESP_Type *) &ifData.idioResponsePacket, ifOps);
			}
		}
	}
#ifdef CNFG_AID_HAS_BULK_DATA
	idioBulkDataService();
#endif
}

void idioInit(void)
{
	idioEnabled = false;
	ifOps 		= &idioIfOps;
	ifOps->init(ifOps->uartHandle);
#ifdef CNFG_AID_HAS_BULK_DATA
	idioBulkDataInit();
#endif
}

uint8_t idioEnable(uint8_t enable)
{
	uint8_t changed = false;
	if (enable != idioEnabled)
	{
		idbusCaptureIDIO(enable);

#ifdef CNFG_AID_HAS_BULK_DATA
		idioBulkDataServiceEnable(enable);
#endif
		idioEnabled = enable;
		changed = true;
	}
	return changed;
}
