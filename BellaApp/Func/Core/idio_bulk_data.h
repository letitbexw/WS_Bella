/*
 * idio_bulk_data.h
 *
 *  Created on: Feb 3, 2026
 *      Author: xiongwei
 */

#ifndef CORE_IDIO_BULK_DATA_H_
#define CORE_IDIO_BULK_DATA_H_


#include "config.h"
#include "idio.h"

void idioBulkDataInit(void);
void idioBulkDataService(void);
void idioBulkDataServiceEnable(uint8_t enable);

uint8_t idioBulkDataHandler(AID_CMD_Type * idioCommandPacketPtr, AID_RESP_Type * idioResponsePacketPtr);

void idioBulkDataClearReadPendingFlag(void);
bool idioBulkDataIsEnabled(void);


#endif /* CORE_IDIO_BULK_DATA_H_ */
