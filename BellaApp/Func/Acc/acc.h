/*
 * acc.h
 *
 *  Created on: Feb 10, 2026
 *      Author: xiongwei
 */

#ifndef ACC_ACC_H_
#define ACC_ACC_H_


// Identifier values
#define ACC_PARAM_X533_PROTOCOL           		0x01
#define ACC_PARAM_X533_REVISION           		0x02
#define ACC_PARAM_ID_IAP_OPT_SESSIONS     		0x03
#define ACC_PARAM_ID_RESET_COMM           		0x04

#define ACC_PARAM_ID_IAP_CTL_FROM_DEV     		0x10
#define ACC_PARAM_ID_IAP_CTL_FROM_ACC     		0x11
#define ACC_PARAM_ID_IAP_FILE_FROM_DEV    		0x12
#define ACC_PARAM_ID_IAP_FILE_FROM_ACC    		0x13
#define ACC_PARAM_ID_IAP_EA_FROM_DEV      		0x14
#define ACC_PARAM_ID_IAP_EA_FROM_ACC      		0x15

#define ACC_PARAM_ID_HID_IDENTIFIER       		0x20  // 0x20..0x27
#define ACC_PARAM_ID_HID_GET_REPORT       		0x30  // 0x30..0x37
#define ACC_PARAM_ID_HID_GET_FEATURE      		0x38
#define ACC_PARAM_ID_HID_SET_REPORT       		0x40  // 0x40..0x47

#define ACC_PARAM_ID_PD_CHARGE            		0x50
#define ACC_PARAM_ID_DEVICE_STATUS        		0x51

#define ACC_PARAM_ID_SERVICE_IRQ          		0x60
#define ACC_PARAM_ID_REQUEST_REBOOT       		0x63

#define ACC_PARAM_ID_ERROR                		0xFF

#define ACC_REBOOT_REQUEST_PATTERN      		0x55AA

#define ACC_PARAM_ID_IAP_OPT_SESSION_EA    		(1 << 0)
#define ACC_PARAM_ID_IAP_OPT_SESSION_FILE  		(1 << 1)


#define HID_GETREPORT_MAXLEN    				254
#define HID_GET_RPTDESC_MAXLEN					508
#define HID_SETREPORT_MAXLEN					64
#define HID_PARTIAL_REPORT_LEN					255
#define SERVICEIRQ_PARAM_SIZE					3

#define ACC_REPORT_TYPE_MASK_EVENT_STREAM		(1 << 0)
#define ACC_REPORT_TYPE_MASK_MULTI_HID_REPORT	(1 << 1)


uint16_t accGetParamLength(uint8_t paramId);


uint8_t* accGetParamData(uint8_t paramId);
bool accGetIdChanged(void);

void accResetComm(void);

uint8_t accGetReport(uint8_t endpoint, uint16_t component, uint8_t reportType, uint8_t reportID, uint8_t * reportDataPtr);
void accSetReportByID(uint8_t endpoint, uint8_t reportType, uint8_t reportID, uint8_t *data, uint8_t len);
void accGetReportByID(uint8_t endpoint, uint8_t reportType, uint8_t reportID, uint8_t *data);

bool accIapCtlFromDev(uint8_t *data, uint16_t len);
bool accIapEaFromDev(uint8_t *data, uint16_t len);
bool accIapFileFromDev(uint8_t *data, uint16_t len);

bool accGetInitDone(void);

bool accGetDeviceIsPresent(void);
void accSetDeviceIsPresent(bool newState);

bool accGetDeviceIsWake(void);
void accSetDeviceIsWake(bool newState);

bool accGetReadyState(void);
void accSetReadyState(bool newState);

void accService(void);
bool accIsIdle(uint32_t timeMs);

void accSetGetReportCommandPending(bool pending);
bool accGetSetGetReportCommandPending();
bool accStateIsIdle();


#endif /* ACC_ACC_H_ */
