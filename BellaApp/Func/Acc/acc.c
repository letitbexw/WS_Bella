/*
 * acc.c
 *
 *  Created on: Feb 10, 2026
 *      Author: xiongwei
 */

#include "main.h"
#include "acc.h"


uint16_t accGetParamLength(uint8_t paramId) 	{ return 0; }
uint8_t* accGetParamData(uint8_t paramId)		{ return NULL; }
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

bool accGetInitDone(void) 					{ return false; }
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

