/*
 * sn.h
 *
 *  Created on: Jan 11, 2026
 *      Author: xiongwei
 */

#ifndef BSP_SN_H_
#define BSP_SN_H_


#include "main.h"
#include <stdbool.h>

#define PRODUCT_MLB_SER_NUM_SZ               	17    	// Product MLB Serial Number Length
#define PRODUCT_FG_SER_NUM_SZ                   0    	// Product FG Serial Number Length

uint8_t* getProductSN(uint16_t* length);
uint8_t* getModuleSN(uint16_t* length);
bool setModuleSN(uint8_t *sn);


#endif /* BSP_SN_H_ */
