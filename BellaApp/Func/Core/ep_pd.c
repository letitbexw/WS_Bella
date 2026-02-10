/*
 * ep_pd.c
 *
 *  Created on: Feb 9, 2026
 *      Author: xiongwei
 */


#include "config.h"
#include "timers.h"
#include "ep_pd.h"
#include "aid_pd.h"
#include "debug.h"

#define EPPD_BUFFER_COUNT    4

typedef struct {
	uint8_t length;
	uint8_t command;
	uint8_t payload[8*4];
} epPDMessage_t;

typedef struct fifoA_s {
	uint32_t sizeMask;
	volatile uint32_t wrIdx;
	volatile uint32_t rdIdx;
} fifoA_t;


epPDMessage_t txBuffer[EPPD_BUFFER_COUNT];
static fifoA_t txFifo;

static void fifoAInit(fifoA_t * fifo, uint8_t size)
{
    fifo->sizeMask = size - 1;
    fifo->rdIdx = fifo->wrIdx = 0;
}

#ifdef CNFG_DEBUG_IDIO_EP_AID_PD_ENABLED
static int fifoACount(fifoA_t *fifo) {
	int rval;
	int32_t r = fifo->rdIdx;
	int32_t w = fifo->wrIdx;

	rval = (w - r) & fifo->sizeMask;

	return rval;
}
#endif

static int fifoAFull(fifoA_t *fifo) {
	return ((fifo->wrIdx + 1) & fifo->sizeMask) == fifo->rdIdx;
}

static int fifoAEmpty(fifoA_t *fifo) {
	return fifo->rdIdx == fifo->wrIdx;
}

static void fifoAReset(fifoA_t *fifo) {
	fifo->rdIdx = fifo->wrIdx = 0;
}


uint8_t epPDHandler(AID_ENDPOINT_PACKET_Type * command, AID_ENDPOINT_PACKET_Type *response)
{
	uint8_t ret = IDIO_RESPONSE_NONE;

#ifdef CNFG_DEBUG_IDIO_EP_AID_PD_ENABLED
	DEBUG_PRINT_AIDPD("Rx %02X ", command->endpointPacketType);
	if (configDebugAidPDPrintEnabled())
	{
		debugPrintHexStream(command->payloadLen, &command->buffer[0], true);
	}
#endif

	// prep response
	response->payloadLen = 0; //ACK
	response->endpointPacketType = command->endpointPacketType;

	if (command->payloadLen == 0)
	{
		switch(command->endpointPacketType)
		{
			case EP_PD_CMD_GOTO_MIN:
			case EP_PD_CMD_ACCEPT:
			case EP_PD_CMD_REJECT:
			case EP_PD_CMD_PS_READY:
			case EP_PD_CMD_POWER_SWAP:
			case EP_PD_CMD_WAIT:
			case EP_PD_CMD_RESET:
			case EP_PD_CMD_GET_SOURCE_CAPABILITY:
			case EP_PD_CMD_GET_SINK_CAPABILITY:
			{
				if (aidPDReceivedCommand(command->endpointPacketType))
				{
					ret = IDIO_RESPONSE_VALID;
				}
				else
				{
					DEBUG_PRINT_AIDPD("failed to process command 0x%02x", command->endpointPacketType);
				}
				break;
			}

			case 0x00:
			case 0x01:
			case 0x0b:
			case 0x0e:
			case 0x0f:
				// unused messages that we must ACK
				DEBUG_PRINT_AIDPD("ignored unimplemented command 0x%02x", command->endpointPacketType);
				ret = IDIO_RESPONSE_VALID;
				break;

			default:
				break;
		}
	}
	else  // message has payload
	{
		switch(command->endpointPacketType)
		{
			case EP_PD_RESP_SOURCE_CAPABILITY:
			case EP_PD_RESP_REQUEST:
			case EP_PD_RESP_SINK_CAPABILITY:
			{
				if (aidPDReceivedData(command->endpointPacketType, &command->buffer[0], command->payloadLen))
				{
					ret = IDIO_RESPONSE_VALID;
				}
				else
				{
					DEBUG_PRINT_AIDPD("failed to process command 0x%02x", command->endpointPacketType);
				}
				break;
			}

			case 0x00:
			case 0x03:
			case 0x05:
			case 0x06:
			case 0x07:
			case 0x08:
			case 0x09:
			case 0x0a:
			case 0x0b:
			case 0x0c:
			case 0x0e:
			case 0x0f:
				// unused messages that we must ACK
				ret = IDIO_RESPONSE_VALID;
				break;

			default:
				break;
		}
	}
	return ret;
}

uint8_t epPDGetAsyncDataPacket(AID_ENDPOINT_PACKET_Type * response)
{
	uint8_t ret = IDIO_RESPONSE_NONE;

	if (epPDGetAsyncDataPendingFlag() == true)
	{
		ret 							= IDIO_RESPONSE_VALID;
		response->endpointPacketType 	= txBuffer[txFifo.rdIdx].command;
		response->payloadLen 			= txBuffer[txFifo.rdIdx].length;
		memcpy(&response->buffer[0], &txBuffer[txFifo.rdIdx].payload[0], txBuffer[txFifo.rdIdx].length);
		txFifo.rdIdx 					= (txFifo.rdIdx + 1) & txFifo.sizeMask;

#ifdef CNFG_DEBUG_IDIO_EP_AID_PD_ENABLED
		DEBUG_PRINT_AIDPD("TxQ %02X", response->endpointPacketType);
		if (configDebugAidPDPrintEnabled() && (response->payloadLen > 0))
		{
			debugPrintHexStream(response->payloadLen, &response->buffer[0], true);
		}
#endif
	}

	return ret;
}

bool epPDGetAsyncDataPendingFlag(void)
{
	//DEBUG_PRINT_AIDPD("pending? %d", !epPacketBufferEmpty);
	return !fifoAEmpty(&txFifo);
}

uint8_t epPDQueueData(uint8_t command, uint8_t* data, uint8_t length)
{
	uint8_t ret = false;
	DEBUG_PRINT_AIDPD("Qc+d 0x%02x, l=%d, qd=%d", command, length, fifoACount(&txFifo));
	if (!fifoAFull(&txFifo))
	{
		txBuffer[txFifo.wrIdx].command 	= command;
		txBuffer[txFifo.wrIdx].length 	= length;
		memcpy(&txBuffer[txFifo.wrIdx].payload[0], data, length);
		txFifo.wrIdx 					= (txFifo.wrIdx + 1) & txFifo.sizeMask;
		ret = true;
	}
	else
	{
		DEBUG_PRINT_AIDPD("failed to queue command + data 0x%02x, resetting", command);
		fifoAReset(&txFifo);
		aidpdResetDevice();
	}

	return ret;
}

uint8_t epPDQueueCommand(uint8_t command)
{
	uint8_t ret = false;
	DEBUG_PRINT_AIDPD("Qc 0x%02x, qd=%d", command, fifoACount(&txFifo));
	if (!fifoAFull(&txFifo))
	{
		txBuffer[txFifo.wrIdx].command 	= command;
		txBuffer[txFifo.wrIdx].length 	= 0;
		txFifo.wrIdx = (txFifo.wrIdx + 1) & txFifo.sizeMask;
		ret 							= true;
	}
	else
	{
		DEBUG_PRINT_AIDPD("failed to queue command 0x%02x, resetting", command);
		fifoAReset(&txFifo);
		aidpdResetDevice();
	}
	return ret;
}

uint8_t epPDTxQueueEmpty(void)
{
	return fifoAEmpty(&txFifo);
}

void epPDInit(void)
{
	fifoAInit(&txFifo, EPPD_BUFFER_COUNT);
}

void epPDService(void)
{
	aidpdService();
}


