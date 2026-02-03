/*
 * sn.c
 *
 *  Created on: Jan 11, 2026
 *      Author: xiongwei
 */


#include "sn.h"
#include "config.h"
#include "bsp.h"


uint8_t serialNumber[PRODUCT_FG_SER_NUM_SZ + 1] 	= "";
uint8_t mlbSerialNumber[PRODUCT_MLB_SER_NUM_SZ + 1] = "MLB_ABCDEFG234567";


uint8_t* getProductSN(uint16_t* length)
{
	if (length) { *length = PRODUCT_FG_SER_NUM_SZ; }
	return &serialNumber[0];
}

uint8_t* getModuleSN(uint16_t* length)
{
	return &mlbSerialNumber[0];
}


bool setModuleSN(uint8_t *sn)
{
	UNUSED(sn);
	return true;
}



