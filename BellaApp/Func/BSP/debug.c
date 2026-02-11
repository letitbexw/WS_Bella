/*
 * debug.c
 *
 *  Created on: Feb 3, 2026
 *      Author: xiongwei
 */


#include "config.h"         // Include configuration
#include "debug.h"          // Include file header

// enable default printing of individual logs here
volatile uint32_t defaultDebugPrintFlags = (
	CNFG_DEBUG_ERROR_PRINT_ENABLED 			|
	CNFG_DEBUG_BOARD_PRINT_ENABLED 			|
//	CNFG_DEBUG_IDIO_PRINT_ENABLED 			|
//	CNFG_DEBUG_IDIO_EXT_PRINT_ENABLED 		|
//	CNFG_DEBUG_IDIO_BULK_DATA_PRINT_ENABLED |
//	CNFG_DEBUG_IDIO_ENDPOINT_PRINT_ENABLED 	|
//
//	CNFG_DEBUG_IAP_PRINT_ENABLED 			|
//	CNFG_DEBUG_EA_UPDATE_PRINT_ENABLED 		|
//	CNFG_DEBUG_IAP2_HID_PRINT_ENABLED 		|
	CNFG_DEBUG_AIDPD_PRINT_ENABLED 			|
	CNFG_DEBUG_ORION_PRINT_ENABLED 			|
//	CNFG_DEBUG_USBPD_PRINT_ENABLED			|
	0
);

volatile uint32_t debugPrintFlags = 0;


void debugInit(void)
{
	debugPrintFlags = defaultDebugPrintFlags;
}

// Print the Hex Table Header
void debugPrintHexTableHeader(void)
{
	// print the low order address indicies and ASCII header
	printf("     00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F");  printf("  0123456789ABCDEF\n");
	printf("     -----------------------------------------------");  printf("  ---- ASCII -----\n");
}

// Print a part of memory as a formatted hex table with ascii translation
void debugPrintHexTable(uint16_t length, uint8_t* buffer, bool print_header)
{
	uint8_t j;      // scratch variable

	// print the low order address indicies and ASCII header
	if (print_header) { debugPrintHexTableHeader(); }

	// print the data
	for(j=0; j<((length+15)>>4); j++)
	{
		uint8_t i;    // scratch variable

		if (print_header)
		{
			// print the high order address index for this line
			printf("%04x ", (j<<4));
		}

		// print the hex data
		for(i=0; i<0x10; i++)
		{
			// be nice and print only up to the exact end of the data
			if( ((j<<4)+i) < length)
			{
				// print hex byte
				printf("%02x ", buffer[(j<<4)+i]);
			}
			else
			{
				// we're past the end of the data's length
				// print spaces
				printf("   ");
			}
		}

		if (print_header)
		{
			// leave some space
			printf(" ");

			// print the ascii data
			for(i=0; i<0x10; i++)
			{
				// be nice and print only up to the exact end of the data
				if( ((j<<4)+i) < length)
				{
					// get the character
					uint8_t s = buffer[(j<<4)+i];
					// make sure character is printable
					if(s >= 0x20)
						printf("%c", s);
					else
						printf("%c", '.');
				}
				else
				{
					// we're past the end of the data's length
					// print a space
					printf(" ");
				}
			}
		}
		printf("\n");
	}
}


// Print a part of memory as a formatted hex stream
void debugPrintHexStream(uint16_t length, uint8_t* buffer, bool newline)
{
	uint8_t j;      // scratch variable

	// print the data
	for(j=0; j<length; j++)
	{
		// print hex byte
		printf("%02X ", buffer[j]);
	}

	if (newline) { printf("\n"); }
}
/*
static uint8_t nibble2ascii(uint8_t c)
{
	switch (c) {
		case  0: return '0';
		case  1: return '1';
		case  2: return '2';
		case  3: return '3';
		case  4: return '4';
		case  5: return '5';
		case  6: return '6';
		case  7: return '7';
		case  8: return '8';
		case  9: return '9';
		case 10: return 'A';
		case 11: return 'B';
		case 12: return 'C';
		case 13: return 'D';
		case 14: return 'E';
		case 15: return 'F';
		default: return 'F';
	}
}

void uint8Print(uint8_t data)
{
	_ttywrch(nibble2ascii((data >> 4) & 0xF));
	_ttywrch(nibble2ascii((data >> 0) & 0xF));
	_ttywrch('\n');
}

void uint16Print(uint16_t data)
{
	_ttywrch(nibble2ascii((data >> 12) & 0xF));
	_ttywrch(nibble2ascii((data >> 8) & 0xF));
	_ttywrch(nibble2ascii((data >> 4) & 0xF));
	_ttywrch(nibble2ascii((data >> 0) & 0xF));
	_ttywrch('\n');
}

void uint32Print(uint32_t data)
{
	_ttywrch(nibble2ascii((data >> 28) & 0xF));
	_ttywrch(nibble2ascii((data >> 24) & 0xF));
	_ttywrch(nibble2ascii((data >> 20) & 0xF));
	_ttywrch(nibble2ascii((data >> 16) & 0xF));
	_ttywrch(nibble2ascii((data >> 12) & 0xF));
	_ttywrch(nibble2ascii((data >> 8) & 0xF));
	_ttywrch(nibble2ascii((data >> 4) & 0xF));
	_ttywrch(nibble2ascii((data >> 0) & 0xF));
	_ttywrch('\n');
}

void uint8PrintDec(uint8_t data)
{
	bool pr = false;
	uint8_t c = 0;

  	while (data > 99) {
		pr = true;
		c += 1;
		data -= 100;
	}
  	if (pr) { _ttywrch(c + '0'); }

	c = 0;
	while (data > 9) {
		pr = true;
		c += 1;
		data -= 10;
	}
	if (pr) { _ttywrch(c + '0'); }

	c = data;
	_ttywrch(c + '0');
}
*/
