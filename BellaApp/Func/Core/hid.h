/*
 * hid.h
 *
 *  Created on: Feb 3, 2026
 *      Author: xiongwei
 */

#ifndef CORE_HID_H_
#define CORE_HID_H_


enum IOHIDReportType{
	kIOHIDReportTypeInput = 1,
	kIOHIDReportTypeOutput,
	kIOHIDReportTypeFeature,
	kIOHIDReportTypeCount
};

typedef enum {
	kHIDReportSuccess               = 0,
	kHIDReportBusy                  = 1,
	kHIDReportError                 = 2,
	kHIDReportErrorIDMismatch       = 3,
	kHIDReportErrorUnsupported      = 4,
	kHIDReportErrorIncorrectLength  = 5,
	kHIDReportErrorLocked           = 6,
} HIDReportReturn;

uint8_t hidGetEndpointCount(void);
uint8_t* hidGetIdentifier(uint8_t endpoint);
uint16_t hidGetIdentifierLength(uint8_t endpoint);

bool hidSetReport(uint8_t endpoint, uint8_t reportType, uint8_t reportID, uint8_t *reportData, uint8_t reportLen);
uint8_t hidGetReport(uint8_t endpoint, uint8_t reportType, uint8_t reportID, uint8_t *reportData);
uint8_t hidGetPendingReport(uint8_t endpoint, uint8_t *reportData);

bool hidGetReportDataPendingFlag(void);
void hidSetReportDataPendingFlag(bool pending, uint8_t endpoint);
/* Set the stream mode flag */
void hidSetStreamModeFlag(bool enable);
/* Get stream mode flag value */
bool hidGetStreamModeFlag(void);
/* Set multiple report flag */
void hidSetMultipleReportFlag(bool enable);
/*Get multiple report flag */
bool hidGetMultipleReportFlag(void);


#endif /* CORE_HID_H_ */
