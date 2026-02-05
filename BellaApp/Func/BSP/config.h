/*
 * config.h
 *
 *  Created on: Jan 4, 2026
 *      Author: xiongwei
 */

#ifndef BSP_CONFIG_H_
#define BSP_CONFIG_H_

#include "main.h"
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>


#define CNFG_BUILD_INFO  	"Build Date " __DATE__ " Time " __TIME__

#define CNFG_ACCESSORY_MANUFACTURER       "XYZInc."
#define CNFG_ACCESSORY_MODEL_NUMBER       "X33pr"
#define CNFG_ACCESSORY_NAME_STRING        "XYZ X33 pro"

#define CNFG_FW_VERSION_MAJ           	0               // Major version number
#define CNFG_FW_VERSION_MIN           	0               // Minor version number
#define CNFG_FW_REVISION              	0              	// Revision number

#define CNFG_HW_VERSION_MAJ           	1               // Major version number
#define CNFG_HW_VERSION_MIN           	0               // Minor version number
#define CNFG_HW_REVISION              	0               // Revision number



#define CNFG_WATCHDOG_ENABLED

/* System Timers */
#define HF_TIMER               			TIM6
#define HF_TIMER_FREQUENCY         		1000000             /* = 1MHZ -> timer runs in microseconds */

// AID BUS / ID_IO UART
#define CNFG_IDBUS_UART_RX_BUF_SIZE     (64)
#define CNFG_IDBUS_UART_TX_BUF_SIZE     (128)

//TEST UART
#define CNFG_TEST_UART_RX_BUF_SIZE     	(128)
#define CNFG_TEST_UART_TX_BUF_SIZE     	(512)



// AID bus defines
//#define CFNG_AID_CONTROLS_TRISTAR
#define CNFG_AID_ID_RESPONSE                                     	// IDIO responds to COMMAND_ID (i.e., there is no HiFive)
#define CNFG_AID_ID_VARIABLE_RESPONSE                            	// IDIO response to COMMAND_ID is set a runtime
//#define CNFG_AID_POWER_CONTROL                                   	// IDIO controls LoFive equivalent Hardware
#define CNFG_AID_VID                    	0x00                    // IDIO Interface Information VID
#define CNFG_AID_PID                    	0x58                    // IDIO Interface Information PID
#define CNFG_AID_IF_REVISION            	0x01                    // IDIO Interface Information Revision
#define CNFG_AID_HAS_BULK_DATA                                  	// IDIO responds to BULK DATA commands
#ifdef CNFG_AID_HAS_BULK_DATA
#define CNFG_AID_BULK_DATA_PAD_DATA   		0xFF                 	// pad BD ReadResponse unused byte with this value
#define CNFG_AID_HAS_BULK_DATA_DEVICE_CTL                        	// IDIO responds to BULK DATA DEVICE CONTROL commands
#define CNFG_AID_BULK_DATA_AUTH                                  	// IDIO exposes Mozart for iOS raw control
#define CNFG_AID_HAS_BULK_DATA_IAP2                              	// IDIO responds to BULK DATA IAP commands
#define CNFG_AID_HAS_BULK_DATA_HID                               	// IDIO responds to BULK DATA HID commands
#define CNFG_AID_HAS_BULK_DATA_EA                                	// IDIO responds to BULK DATA EA commands
#define CNFG_AID_HAS_BULK_DATA_PD                                	// IDIO responds to BULK DATA PD commands
#endif



#endif /* BSP_CONFIG_H_ */
