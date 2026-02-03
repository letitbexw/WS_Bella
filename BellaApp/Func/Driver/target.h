/*
 * target.h
 *
 *  Created on: Jan 10, 2026
 *      Author: xiongwei
 */

#ifndef DRIVER_TARGET_H_
#define DRIVER_TARGET_H_

// Include Target Standard Libraries
#include <stdio.h>                                          // Include Standard I/O
#include <stdlib.h>                                         // Include Standard Lib
#include <string.h>                                         // Include String
#include <ctype.h>                                          // Include CType


// Target Defines
#define TARGET_CPU_HZ                   __SYSTEM_CLOCK    // Target CPU Speed
#define TARGET_INTERRUPT_VECTOR_LENGTH  512               // Interrupt Vector Length

// Map Target's NOP
#define targetNop()   __NOP()

// Map Target Functions
#define targetDisableInterrupts()   	__disable_irq()
#define targetEnableInterrupts()    	__enable_irq()
#define targetResetWDT()               	wdtService()
#define targetReset()               	targetDisableInterrupts(); HAL_NVIC_SystemReset()
#define targetSleep()                                      // In bootloader mode we won't sleep

//#define trgWake()                 __low_power_mode_off_on_exit()
#define targetWFI()                    	__wfi()
#define targetEnableIRQ(x)             	HAL_NVIC_EnableIRQ(x)
#define targetDisableIRQ(x)            	HAL_NVIC_DisableIRQ(x)
//#define trgGetIRQ(x)                 	NVIC_GetIRQ(x)
#define targetClearPendingIRQ(x)       	HAL_NVIC_ClearPendingIRQ(x)
#define targetGetInterruptState(x)     	x = __get_PRIMASK()
#define targetRestoreInterruptState(x)  __set_PRIMASK(x)  // Restore previous interrupt state


// Define target byte-order routines
#define htons(x) (((((uint16_t)(x) & 0x00FF)) << 8) | (((uint16_t)(x) & 0xFF00) >> 8))
#define htonl(x) ((((uint32_t)(x))<<24) | ((((uint32_t)(x))&0x00FF0000l)>>8) | ((((uint32_t)(x))&0x0000FF00l)<<8) | (((uint32_t)(x))>>24))
#define ntohs(x) htons(x)
#define ntohl(x) htonl(x)

#endif /* DRIVER_TARGET_H_ */
