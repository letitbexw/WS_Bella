/*
 * idio.c
 *
 *  Created on: Jan 11, 2026
 *      Author: xiongwei
 */


#include "idio.h"

static uint8_t idioEnabled = false;
static volatile AID_ID_RESP_Type idbusCurrentIDResponse;
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

void idioInit(void)
{
	idioEnabled = false;
	ifOps = &idioIfOps;
	ifOps->init(ifOps->uartHandle, ORION_UART_RATE);
#ifdef CNFG_AID_HAS_BULK_DATA
	idioBulkDataInit();
#endif
}
