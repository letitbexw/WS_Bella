/*
 * debug.h
 *
 *  Created on: Feb 3, 2026
 *      Author: xiongwei
 */

#ifndef BSP_DEBUG_H_
#define BSP_DEBUG_H_


#include "config.h"
#include "timers.h"
#include "main.h"
#include "bsp.h"

int _write (int fd, char *ptr, int len);
#ifdef DEBUG_ENABLED
#define DEBUG_CH(x) fputc(x, stdout)
#else
#define DEBUG_CH(x)
#endif

extern volatile uint32_t debugPrintFlags;
extern volatile uint32_t defaultDebugPrintFlags;

// enable individual logs here
#ifdef DEBUG_ENABLED
#define CNFG_DEBUG_ERROR_ENABLED
#define CNFG_DEBUG_BOARD_ENABLED

//#define CNFG_DEBUG_IDIO_ENABLED
//#define CNFG_DEBUG_IDIO_EXT_ENABLED
//#define CNFG_DEBUG_IDIO_BULK_DATA_ENABLED
//#define CNFG_DEBUG_IDIO_ENDPOINT_ENABLED
//#define CNFG_DEBUG_IDIO_COMMANDS

//#define CNFG_DEBUG_IAP_ENABLED
//#define CNFG_DEBUG_EA_UPDATE_ENABLED
//#define CNFG_DEBUG_IAP2_HID_ENABLED
//#define CNFG_DEBUG_IDIO_EP_AID_PD_ENABLED
//#define CNFG_DEBUG_ORION_ENABLED
#endif // DEBUG_ENABLED

// Control Bits
#define CNFG_DEBUG_ERROR_PRINT_ENABLED                      (1UL << 0)
#define CNFG_DEBUG_BOARD_PRINT_ENABLED                      (1UL << 1)
#define CNFG_DEBUG_IDIO_PRINT_ENABLED                       (1UL << 2)
#define CNFG_DEBUG_IDIO_EXT_PRINT_ENABLED                   (1UL << 3)
#define CNFG_DEBUG_IDIO_BULK_DATA_PRINT_ENABLED             (1UL << 4)
#define CNFG_DEBUG_IDIO_ENDPOINT_PRINT_ENABLED              (1UL << 5)
#define CNFG_DEBUG_IAP_PRINT_ENABLED                        (1UL << 6)
#define CNFG_DEBUG_EA_UPDATE_PRINT_ENABLED                  (1UL << 7)
#define CNFG_DEBUG_IAP2_HID_PRINT_ENABLED                   (1UL << 8)
#define CNFG_DEBUG_AIDPD_PRINT_ENABLED                      (1UL << 9)
#define CNFG_DEBUG_ORION_PRINT_ENABLED                      (1UL << 10)

#define configDebugErrorPrintEnabled()                      (CNFG_DEBUG_ERROR_PRINT_ENABLED & debugPrintFlags)
#define configDebugBoardPrintEnabled()                      (CNFG_DEBUG_BOARD_PRINT_ENABLED & debugPrintFlags)

#define configDebugIdioPrintEnabled()                       (CNFG_DEBUG_IDIO_PRINT_ENABLED & debugPrintFlags)
#define configDebugIdioExtPrintEnabled()                    (CNFG_DEBUG_IDIO_EXT_PRINT_ENABLED & debugPrintFlags)
#define configDebugIdioBulkDataPrintEnabled()               (CNFG_DEBUG_IDIO_BULK_DATA_PRINT_ENABLED & debugPrintFlags)
#define configDebugIdioEndpointPrintEnabled()               (CNFG_DEBUG_IDIO_ENDPOINT_PRINT_ENABLED & debugPrintFlags)

#define configDebugIapPrintEnabled()                        (CNFG_DEBUG_IAP_PRINT_ENABLED & debugPrintFlags)
#define configDebugEaUpdatePrintEnabled()                   (CNFG_DEBUG_EA_UPDATE_PRINT_ENABLED & debugPrintFlags)
#define configDebugIap2HidPrintEnabled()                    (CNFG_DEBUG_IAP2_HID_PRINT_ENABLED & debugPrintFlags)
#define configDebugAidPDPrintEnabled()                      (CNFG_DEBUG_AIDPD_PRINT_ENABLED & debugPrintFlags)

#define configDebugOrionPrintEnabled()                      (CNFG_DEBUG_ORION_PRINT_ENABLED & debugPrintFlags)

#ifdef DEBUG_ENABLED
#define debugPrint(fmt, ...) { printf("[%08u] %d: " fmt "\n", GetTickCount(), __LINE__, ##__VA_ARGS__); }
#else
#define debugPrint(fmt, ...)
#endif

#ifdef CNFG_DEBUG_ERROR_ENABLED
#define DEBUG_PRINT_ERROR(fmt, ...) if (configDebugErrorPrintEnabled()) { debugPrint(fmt, ##__VA_ARGS__); }
#else
#define DEBUG_PRINT_ERROR(fmt, ...)
#endif

#ifdef CNFG_DEBUG_BOARD_ENABLED
#define DEBUG_PRINT_BOARD(fmt, ...) if (configDebugBoardPrintEnabled()) { debugPrint(fmt, ##__VA_ARGS__); }
#else
#define DEBUG_PRINT_BOARD(fmt, ...)
#endif

#ifdef CNFG_DEBUG_IDIO_ENABLED
#define DEBUG_PRINT_IDIO__(fmt, ...) if (configDebugIdioPrintEnabled()) { debugPrint(fmt, ##__VA_ARGS__); }
#else
#define DEBUG_PRINT_IDIO__(fmt, ...)
#endif

#ifdef CNFG_DEBUG_IDIO_EXT_ENABLED
#define DEBUG_PRINT_IDIO_EXT(fmt, ...) if (configDebugIdioExtPrintEnabled()) { debugPrint(fmt, ##__VA_ARGS__); }
#else
#define DEBUG_PRINT_IDIO_EXT(fmt, ...)
#endif

#ifdef CNFG_DEBUG_IDIO_BULK_DATA_ENABLED
#define DEBUG_PRINT_IDIO_BULK_DATA(fmt, ...) if (configDebugIdioBulkDataPrintEnabled()) { debugPrint(fmt, ##__VA_ARGS__); }
#else
#define DEBUG_PRINT_IDIO_BULK_DATA(fmt, ...)
#endif

#ifdef CNFG_DEBUG_IDIO_ENDPOINT_ENABLED
#define DEBUG_PRINT_IDIO_ENDPOINT(fmt, ...) if (configDebugIdioEndpointPrintEnabled()) { debugPrint(fmt, ##__VA_ARGS__); }
#else
#define DEBUG_PRINT_IDIO_ENDPOINT(fmt, ...)
#endif

#ifdef CNFG_DEBUG_IAP_ENABLED
#define DEBUG_PRINT_IAP(fmt, ...) if (configDebugIapPrintEnabled()) { debugPrint(fmt, ##__VA_ARGS__); }
#else
#define DEBUG_PRINT_IAP(fmt, ...)
#endif

#ifdef CNFG_DEBUG_EA_UPDATE_ENABLED
#define DEBUG_PRINT_EA_UPDATE(fmt, ...) if (configDebugEaUpdatePrintEnabled()) { debugPrint(fmt, ##__VA_ARGS__); }
#else
#define DEBUG_PRINT_EA_UPDATE(fmt, ...)
#endif

#ifdef CNFG_DEBUG_IAP2_HID_ENABLED
#define DEBUG_PRINT_IAP2_HID(fmt, ...) if (configDebugIapPrintEnabled()) { debugPrint(fmt, ##__VA_ARGS__); }
#else
#define DEBUG_PRINT_IAP2_HID(fmt, ...)
#endif

#ifdef CNFG_DEBUG_IDIO_EP_AID_PD_ENABLED
#define DEBUG_PRINT_AIDPD(fmt, ...) if (configDebugAidPDPrintEnabled()) { debugPrint("AIDPD: " fmt, ##__VA_ARGS__); }
#else
#define DEBUG_PRINT_AIDPD(fmt, ...)
#endif

#ifdef CNFG_DEBUG_ORION_ENABLED
#define DEBUG_PRINT_ORION(fmt, ...) if (configDebugOrionPrintEnabled()) { debugPrint("OR: " fmt, ##__VA_ARGS__); }
#else
#define DEBUG_PRINT_ORION(fmt, ...)
#endif


// Print the Hex Table Header
void debugPrintHexTableHeader(void);

// Print a part of memory as a formatted hex table with ascii translation
void debugPrintHexTable(uint16_t length, uint8_t* buffer, bool print_header);

// Print a part of memory as a formatted hex stream
void debugPrintHexStream(uint16_t length, uint8_t* buffer, bool newline);

void uint8Print(uint8_t data);
void uint16Print(uint16_t data);
void uint32Print(uint32_t data);
void uint8PrintDec(uint8_t data);


#endif /* BSP_DEBUG_H_ */
