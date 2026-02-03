/*
 * utils.h
 *
 *  Created on: Feb 3, 2026
 *      Author: xiongwei
 */

#ifndef BSP_UTILS_H_
#define BSP_UTILS_H_


#include "string.h"
#include <stdbool.h>

typedef struct
{
	uint32_t u;
} __attribute__((packed)) unaligned;

uint32_t u32_unaligned_load(void *ptr);
uint16_t u16_unaligned_load(void *ptr);

void trgUInt16ToBigEndBuf( uint16_t val, uint8_t* buf );
void trgUInt32ToBigEndBuf( uint32_t val, uint8_t* buf );
void trgUInt64ToBigEndBuf( uint64_t val, uint8_t* buf );
uint16_t trgBigEndBufToUInt16( const uint8_t* buf);
uint32_t trgBigEndBufToUInt32( const uint8_t* buf );
uint64_t trgBigEndBufToUInt64( const uint8_t* buf );

uint8_t atob_8(char* str);
bool utilHex2Byte(const uint8_t * hexStr, uint8_t* byte);
uint8_t utilConvertStringHextoByteBuffer(const uint8_t * hexString, uint8_t * hexBytesPtr, uint8_t bufferSize);

// Convert a character to a byte, and return if we were successful
bool charToByte(char ch, uint8_t* byte);
char *utilByteToHex(char *str, uint8_t val, uint8_t digits);
char *utilWordToHex(char *str, uint16_t val, uint8_t digits);
uint32_t utilsHexToWord(char *str, uint8_t digits);


bool utilCheckEvents(uint32_t events, volatile uint32_t * eventReg);
void utilSetEvents(uint32_t events, volatile uint32_t * eventReg);
void utilClearEvents(uint32_t events, volatile uint32_t * eventReg);


#define LOBYTE(x)  ((uint8_t)(x & 0x00FF))
#define HIBYTE(x)  ((uint8_t)((x & 0xFF00) >>8))

#define ABS_DIFF(a,b)   ((a>b) ? ((a)-(b)) : ((b)-(a)))


#endif /* BSP_UTILS_H_ */
