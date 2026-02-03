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

#define CNFG_ACCESSORY_MODEL_NUMBER  	"Bella"
#define CNFG_FW_VERSION_MAJ           	0               // Major version number
#define CNFG_FW_VERSION_MIN           	0               // Minor version number
#define CNFG_FW_REVISION              	0              	// Revision number

#define CNFG_HW_VERSION_MAJ           	1               // Major version number
#define CNFG_HW_VERSION_MIN           	0               // Minor version number
#define CNFG_HW_REVISION              	0               // Revision number


/* System Timers */
#define HF_TIMER               			TIM6
#define HF_TIMER_FREQUENCY         		1000000             /* = 1MHZ -> timer runs in microseconds */

/* USART4 - Debug */
#define DEBUG_UART_RATE             	230400
#define DEBUG_UART_IRQ_PRIORITY     	3

//TEST UART
#define CNFG_TEST_UART_RX_BUF_SIZE     	(128)
#define CNFG_TEST_UART_TX_BUF_SIZE     	(512)


#endif /* BSP_CONFIG_H_ */
