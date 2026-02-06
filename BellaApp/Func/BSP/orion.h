/*
 * orion.h
 *
 *  Created on: Feb 4, 2026
 *      Author: xiongwei
 */

#ifndef BSP_ORION_H_
#define BSP_ORION_H_


/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum {
	orionStateReset = 0,
	orionStateSoftReset,
	orionStateSuspend,
	orionStatePingLow,
	orionStatePingHigh,
	orionStateProvProvider,
	orionStateProvider,
	orionStateWaitConsumer,
	orionStateProvConsumer,
	orionStateConsumer,
	orionStateConsProvSetup,
	orionStateConsProvSwap,
	orionStateProvConsSetup,
	orionStateProvConsDrain,
	orionStateProvConsSwap,
	orionStatePenaltyBox,
	orionStateWaitDrainDisconnect,
	orionStateWaitDisconnect,
	orionStateMonitor
} orionState_t;

typedef enum {
	orionModeMonitor = 0,
	orionModeAccessory = 1,
	orionModeDevice = 2,
	orionModeDeviceAndAccessory = 3,
	orionModeSuspend = 4,
} orionMode_t;

typedef enum {
	orionDisconnected = 0,
	orionConnected,
	orionDisconnecting,
	orionConnecting
} orionConnectState_t;

typedef enum {
	orionNotConnected = 0,
	orionConnectedConsumer,
	orionConnectedProvider,
} orionConnectAsState_t;

typedef enum {
	orionPowerSourceNone = 0,
	orionPowerSourceDrain,
	orionPowerSourceEnable,
} orionPowerSource_t;

typedef enum {
	orionLineNone = 0,
	orionLinePullUp,
	orionLinePullDown,
	orionLineMagicPullDown
} orionLineState_t;

typedef enum {
	orionErrorNoAIDComm = 0,
	orionErrorLostData,
	orionErrorLostVbus,
	orionErrorSwapTimeout,
	orionConnectConsumer,
	orionConnectProvider,
	orionErrorCount    // placeholder for max value (array size)
} orionStatistics_t;

extern const char *orionStatePrint[];

/* Exported constants --------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
#ifdef CNFG_AID_ID_VARIABLE_RESPONSE
extern uint8_t orionCommandIdResponse[6];
#endif
#ifdef CNFG_COLLECT_ORION_STATISTICS
extern uint32_t orionStatistics[orionErrorCount];
#endif

/* Exported macro ------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
orionState_t getOrionState(void);
uint8_t getOrionConnected(void);

uint8_t setOrionState(orionState_t new);
orionConnectAsState_t getOrionConnectedAs(void);
uint8_t orionDisconnect(void);
uint8_t setOrionMode(orionMode_t mode);
orionMode_t getOrionMode(void);
uint8_t setOrionPowerSwap(void);
uint8_t setOrionHighPower(void);
void setOrionAIDState(uint8_t up);
uint8_t getOrionAIDState(void);
orionPowerSource_t getOrionPowerSource(void);
uint8_t setOrionPowerSource(orionPowerSource_t source);
uint16_t getOrionDataIdleLevel(void);
uint8_t getOrionDataAboveRMThreshold(void);
uint8_t getOrionDataAboveRxThreshold(void);

void orionRxWake(void);

void setOrionSoftDelay(uint32_t delay);

uint8_t orionInit(void);
uint8_t orionService(void);

#ifdef CNFG_AID_POWER_CONTROL
void    setOrionInterfacePower(uint8_t mode);
uint8_t getOrionInterfacePower(void);
#endif

#ifdef CNFG_AID_ID_VARIABLE_RESPONSE
uint8_t* getOrionId(uint16_t systemId);
#endif



#endif /* BSP_ORION_H_ */
