/*
 * acc.c
 *
 *  Created on: Feb 10, 2026
 *      Author: xiongwei
 */

#include "main.h"
#include "acc.h"

static uint8_t ChargeVoltageCurrent[2 * sizeof(uint16_t)] = {0x13, 0x88, 0x01, 0xF4};	// 5000mV, 500mA


uint16_t accGetParamLength(uint8_t paramId) 	{ return 0; }


void SetChargeVoltageCurrent(uint16_t mv, uint16_t ma)
{
	ChargeVoltageCurrent[0] = mv >> 8;
	ChargeVoltageCurrent[1] = mv & 0x00FF;
	ChargeVoltageCurrent[2] = ma >> 8;
	ChargeVoltageCurrent[3] = ma & 0x00FF;
}

void GetChargeVoltageCurrent(uint16_t *pmV, uint16_t *pmA)
{
	*pmV = ChargeVoltageCurrent[0]*256 + ChargeVoltageCurrent[1];
	*pmA = ChargeVoltageCurrent[2]*256 + ChargeVoltageCurrent[3];
}

uint8_t* accGetParamData(uint8_t paramId)
{
	if (paramId == ACC_PARAM_ID_PD_CHARGE)
	{
		return (uint8_t *)ChargeVoltageCurrent;
	}
	else
	{
		return NULL;
	}
}

bool accGetIdChanged(void)						{ return false; }
void accResetComm(void)							{}

uint8_t accGetReport(uint8_t endpoint, uint16_t component, uint8_t reportType, uint8_t reportID, uint8_t * reportDataPtr)
{
	UNUSED(endpoint);
	UNUSED(component);
	UNUSED(reportType);
	UNUSED(reportID);
	UNUSED(reportDataPtr);
	return 0;
}

void accSetReportByID(uint8_t endpoint, uint8_t reportType, uint8_t reportID, uint8_t *data, uint8_t len)
{
	UNUSED(endpoint);
	UNUSED(reportType);
	UNUSED(reportID);
	UNUSED(data);
	UNUSED(len);
}

void accGetReportByID(uint8_t endpoint, uint8_t reportType, uint8_t reportID, uint8_t *data)
{
	UNUSED(endpoint);
	UNUSED(reportType);
	UNUSED(reportID);
	UNUSED(data);
}

bool accIapCtlFromDev(uint8_t *data, uint16_t len)
{
	UNUSED(data);
	UNUSED(len);
	return false;
}

bool accIapEaFromDev(uint8_t *data, uint16_t len)
{
	UNUSED(data);
	UNUSED(len);
	return false;
}

bool accIapFileFromDev(uint8_t *data, uint16_t len)
{
	UNUSED(data);
	UNUSED(len);
	return false;
}

bool accGetInitDone(void) 					{ return true; }
bool accGetDeviceIsPresent(void) 			{ return false; }
void accSetDeviceIsPresent(bool newState) 	{ UNUSED(newState); }
bool accGetDeviceIsWake(void)				{ return false; }
void accSetDeviceIsWake(bool newState)		{ UNUSED(newState); }
bool accGetReadyState(void)					{ return false; }
void accSetReadyState(bool newState)		{ UNUSED(newState); }
void accService(void)						{}
bool accIsIdle(uint32_t timeMs)				{ return true; }

void accSetGetReportCommandPending(bool pending) 	{ UNUSED(pending); }
bool accGetSetGetReportCommandPending()				{ return false; }
bool accStateIsIdle()								{ return true; }

