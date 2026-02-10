/*
 * hid.c
 *
 *  Created on: Feb 10, 2026
 *      Author: xiongwei
 */


#include "config.h"
#include "acc.h"
#include "ep_hid.h"
#include "hid.h"
#include "timers.h"
#include "debug.h"
#include "fwup_config.h"
#include "wdt.h"

#define KBD_REPORT_ENABLE  				0x02
#define ACC_GETRPT_CMD_RETRY_TIMEOUT_MS	5000

uint8_t hidGetEndpointCount(void)
{
	uint8_t endpointCount 	= 0;
	uint8_t maxCount 		= CNFG_HID_ENDPOINTS_MAX;
	uint16_t *pStoredLen 	= (uint16_t *)DATA_EEPROM_HID_SIZE_0;	// Read internal EEPROM, its context is customized(from ACC)

	while (maxCount--)
	{
		if ((*pStoredLen > 0) && (*pStoredLen < (HID_GET_RPTDESC_MAXLEN + 1))) { endpointCount++; }
		pStoredLen++;
	}
	return endpointCount;
}

uint16_t hidGetIdentifierLength(uint8_t endpoint)
{
	uint16_t *pStoredLen 	= (uint16_t *)DATA_EEPROM_HID_SIZE_0;
	uint16_t len 			= *(pStoredLen + endpoint);
	return len < (HID_GET_RPTDESC_MAXLEN + 1) ? len : 0;                // HID_GET_RPTDESC_MAXLEN + 1 is invalid length
}

uint8_t* hidGetIdentifier(uint8_t endpoint)
{
	uint16_t *pStoredLen = (uint16_t *)DATA_EEPROM_HID_SIZE_0;

	uint8_t *pStoredBlob = (uint8_t *)DATA_EEPROM_HID_START;

	while (endpoint--) {
		pStoredBlob += *pStoredLen++;
	}
	return pStoredBlob;
}

bool dataPendingFlag = false;

bool hidGetReportDataPendingFlag(void)
{
	return dataPendingFlag;
}

void hidSetReportDataPendingFlag(bool pending, uint8_t endpoint)
{
	//DEBUG_PRINT_IAP2_HID("HIDPEND=%d", pending);
	dataPendingFlag = pending;
	if (pending) { epHIDSetActiveEP(endpoint); }
}

bool hidSetReport(uint8_t endpoint, uint8_t reportType, uint8_t reportID, uint8_t *reportData, uint8_t reportLen)
{
	bool rv = false;
	if ((reportType == kIOHIDReportTypeOutput) || (reportType == kIOHIDReportTypeFeature))
	{
		accSetReportByID(endpoint, reportType, reportID, reportData, reportLen);
		return true;
	}
	return rv;
}

uint8_t hidGetReport(uint8_t endpoint, uint8_t reportType, uint8_t reportID, uint8_t *reportData)
{
	volatile uint8_t reportLength 	= 0;
	volatile uint32_t total_time 	= GetTickCount();

	while((GetTickCount() - total_time) < ACC_GETRPT_CMD_RETRY_TIMEOUT_MS) /* Retry if accessory not idle till timeout */
	{
		if (accStateIsIdle()) 												/* Check if accessory is IDLE */
		{
			accSetGetReportCommandPending(true); 							/* Set the get report command pending from iOS to true */
			accGetReportByID(endpoint, reportType, reportID,reportData); 	/* Format the command to be sent to accessory */
			/* Run the accService till we get a response from accessory for the get report command. Response to iOS need to be sent before timeout */
			do
			{
				accService();
				wdtService();
			}while((accStateIsIdle() != true) || accGetSetGetReportCommandPending());

			reportLength = accGetParamLength(ACC_PARAM_ID_HID_GET_REPORT + endpoint);
			memcpy(reportData,accGetParamData(ACC_PARAM_ID_HID_GET_REPORT + endpoint),reportLength);

			break;
		}
		else /* If accessory is not idle run the accessory service and watchdog service */
		{
			wdtService();
			accService();
		}
	}
	return reportLength;
}

uint16_t hidGetPendingReport(uint8_t endpoint, uint8_t *reportData)
{
	uint16_t reportLength 	= accGetParamLength(ACC_PARAM_ID_HID_GET_REPORT + endpoint);
	uint8_t *accData 		= accGetParamData(ACC_PARAM_ID_HID_GET_REPORT + endpoint);

	memcpy(reportData, accData, reportLength);

	hidSetReportDataPendingFlag(false, 0);
	return reportLength;
}


/* define flag to indicate streaming mode */
bool streamingModeFlag = false;
/* define a flag to indicate accessory sending multiple hid reports in single packet */
bool multHidReportFlag = false;

/* Set the stream mode flag */
void hidSetStreamModeFlag(bool enable)
{
	streamingModeFlag = enable;
}

/* Get stream mode flag value */
bool hidGetStreamModeFlag(void)
{
	return streamingModeFlag;
}

/* Set multiple report flag */
void hidSetMultipleReportFlag(bool enable)
{
	multHidReportFlag = enable;
}
/*Get multiple report flag */
bool hidGetMultipleReportFlag(void)
{
	return multHidReportFlag;
}

