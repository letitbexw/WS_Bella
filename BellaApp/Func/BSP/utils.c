/*
 * utils.c
 *
 *  Created on: Feb 3, 2026
 *      Author: xiongwei
 */


#include "config.h"
#include "utils.h"
#include "target.h"


const unsigned char dec2hex[] = "0123456789ABCDEF";

uint32_t u32_unaligned_load(void *ptr)
{
     unaligned *uptr = (unaligned *)ptr;
     return uptr->u;
}

uint16_t u16_unaligned_load(void *ptr)
{
     unaligned *uptr = (unaligned *)ptr;
     return uptr->u;
}


void trgUInt16ToBigEndBuf( uint16_t val, uint8_t* buf )
{
	buf[0] = (uint8_t)(val >> 8);
	buf[1] = (uint8_t)val;
}

void trgUInt32ToBigEndBuf( uint32_t val, uint8_t* buf )
{
	buf[0] = (uint8_t)(val >> 24);
	buf[1] = (uint8_t)(val >> 16);
	buf[2] = (uint8_t)(val >> 8);
	buf[3] = (uint8_t)val;
}

void trgUInt64ToBigEndBuf( uint64_t val, uint8_t* buf )
{
	trgUInt32ToBigEndBuf((uint32_t) (val >> 32), &buf[0]);
	trgUInt32ToBigEndBuf((uint32_t) (val >> 0), &buf[4]);
}


uint16_t trgBigEndBufToUInt16( const uint8_t* buf)
{
	return( (buf[0]<<8) + buf[1]);
}

uint32_t trgBigEndBufToUInt32( const uint8_t* buf )
{
	uint32_t a = 0;
	uint8_t i;

	for( i = 0; i < 4; i++ ) {
		a <<= 8;
		a += buf[i];
	}

	return a;
}

uint64_t trgBigEndBufToUInt64( const uint8_t* buf )
{
	uint64_t a = 0;
	uint8_t i;

	for( i = 0; i < 8; i++ ) {
		a <<= 8;
		a += buf[i];
	}

	return a;
}


// Convert a one or two character string to a byte
// e.g. "A" = 0x0a, "a1" = 0xa1, "xx" = 0x00
uint8_t atob_8(char* str)
{
	uint8_t ret = 0;

	// Try to convert the first byte
	if (charToByte(str[0], &ret) == true)
	{
		// Try to convert the second byte
		uint8_t byte;
		if (charToByte(str[1], &byte) == true)
		{
			ret = ret * 16 + byte;
		}
	}

	// Return our conversion
	return ret;
}

uint8_t utilConvertStringHextoByteBuffer(const uint8_t * hexString, uint8_t * hexBytesPtr, uint8_t bufferSize)
{
	uint8_t nibble = 0;
	uint8_t retVal = 0;
	uint8_t hexByte = 0;

	if ((hexString != NULL)&&(hexBytesPtr != NULL)) {
		while((*hexString != '\n')||(*hexString != 0)) {
			if (*hexString == ' ') { hexString++; }
			else {
				nibble++;
				hexByte &= ~0x0F;

				if ((*hexString >= '0')&&(*hexString <= '9')) {
					hexByte |= *hexString - '0';
				}
				else if ((*hexString >= 'A')&&(*hexString <= 'F')) {
					hexByte |= *hexString - 'A' + 10;
				}
				hexString++;

				if (nibble >= 2) {
					*hexBytesPtr++ = hexByte;
					retVal++;

					hexByte = 0;
					nibble = 0;

					if (retVal == bufferSize) break;
				}
				else {
					hexByte <<= 4;
				}
			}
		}
	}

	return retVal;
}


bool utilHex2Byte(const uint8_t * hexString, uint8_t* byte)
{
	bool retval = true;
	uint8_t dataByte = 0;

	if ((*hexString >= '0')&&(*hexString <= '9')) {
		dataByte |= *hexString - '0';
	}
	else if ((*hexString >= 'A')&&(*hexString <= 'F')) {
		dataByte |= *hexString - 'A' + 10;
	}
	else {
		retval = false;
	}

	hexString++;
	dataByte <<= 4;

	if (retval == true) {
		if ((*hexString >= '0')&&(*hexString <= '9')) {
			dataByte |= *hexString - '0';
		}
		else if ((*hexString >= 'A')&&(*hexString <= 'F')) {
			dataByte |= *hexString - 'A' + 10;
		}
		else {
			retval = false;
		}
	}

	if (retval == true) {
		*byte = dataByte;
	}
	else {
		*byte = 0;
	}

	return retval;
}


// Convert a character to a byte, and return if we were successful
bool charToByte(char ch, uint8_t* byte)
{
	bool ret = true;

	// Try to convert the character
	if ((ch >= '0') && (ch <= '9'))
	{
		*byte = ch - '0';
	}
	else if ((ch >= 'a') && (ch <='f'))
	{
		*byte = ch - 'a' + 10;
	}
	else if ((ch >= 'A') && (ch <='F'))
	{
		*byte = ch - 'A' + 10;
	}
	else
	{
		*byte = 0;
		ret = false;
	}

	// Return if we were successful
	return ret;
}


char *utilByteToHex(char *str, uint8_t val, uint8_t digits)
{
	if (digits != 1) *str++ = dec2hex[val >> 4];
	*str++ = dec2hex[val & 0x0F];

	return str;
}


char *utilWordToHex(char *str, uint16_t val, uint8_t digits)
{
	if (digits > 2)
	{
		str = utilByteToHex(str, (uint8_t)(val >> 8), digits - 2);
		digits -= 2;
	}
	str = utilByteToHex(str, (uint8_t)(val & 0xFF), digits);

	return str;
}


uint32_t utilsHexToWord(char *str, uint8_t digits)
{
	uint32_t retVal = 0;

	while(digits--)
	{
		char c = *str++;

		if (isxdigit((int)c))
		{
			retVal <<= 4;
			retVal += ((isdigit((int)c)) ? (c - '0') : (toupper((int)c) - ('A' - 10)));
		}
		else
		{
			return 0;
		}
	}

	return retVal;
}

bool utilCheckEvents(uint32_t events, volatile uint32_t * eventReg)
{
	return ((*eventReg & (events)) != 0);
}

void utilSetEvents(uint32_t events, volatile uint32_t * eventReg)
{
	uint32_t primask = 0;
	targetGetInterruptState(primask);
	targetDisableInterrupts();

	// Set the event
	*eventReg |= events;
	targetRestoreInterruptState(primask);
}



void utilClearEvents(uint32_t events, volatile uint32_t * eventReg)
{
	uint32_t primask = 0;
	targetGetInterruptState(primask);
	targetDisableInterrupts();

	*eventReg &= ~events;
	targetRestoreInterruptState(primask);
}

