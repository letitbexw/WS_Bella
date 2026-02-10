/*
 * aid_pd.h
 *
 *  Created on: Feb 9, 2026
 *      Author: xiongwei
 */

#ifndef CORE_AID_PD_H_
#define CORE_AID_PD_H_


#include "usb_pd.h"

typedef enum {
	aidpdStateReset = 0,
	aidpdStateIdle,
	aidpdStateConfigPower,
	aidpdStateWaitAccept,
	aidpdStateWaitReady,
	aidpdStateWaitSwapAccept,
	aidpdStateSwap,
	aidpdStateSendHardReset,
} aidpdState_t;

typedef enum {
	aidpdPwrStateNone = 0,
	aidpdPwrStateCons,
	aidpdPwrStateProv,
} aidpdPwrState_t;

bool aidPDReceivedCommand(uint8_t command);
bool aidPDReceivedData(uint8_t command, uint8_t* dataPtr, uint8_t length);

bool aidpdSetSourceCapability(uint32_t capability, int8_t position);
bool aidpdSetSinkCapability(uint32_t capability);

void aidpdInit(void);
void aidpdResetDevice(void);
uint8_t aidpdService(void);


#endif /* CORE_AID_PD_H_ */
