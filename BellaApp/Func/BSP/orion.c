/**
  ******************************************************************************
  * @file    orion.c
  * @brief   orion interface Control Routines.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "orion.h"
#include "bsp.h"
#include "timers.h"
#include "debug.h"
#include "aid.h"    // for AID message definitions
#include "main.h"   // for event definitions
#include "adc.h"
#include "idbus_uart.h"
#include "idio.h"
#include "idio_bulk_data.h"
#include "utils.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
// Timing parameters
#define ORION_AID_PING_PU_OFF_ACC_MS   	120    	//ms
#define ORION_AID_PING_PU_OFF_DEV_MS    80    	//ms
#define ORION_AID_PING_PU_ON_US         80U   	//us
#define ORION_AID_DETECT_MS            	100    	//ms
#define ORION_WAKE_TIMER_MS             2    	//ms
#define ORION_POWER_READY_DELAY_MS     	150    	//ms
#define ORION_POWER_POLL_DELAY_MS       60    	//ms
#define ORION_POWER_SWAP_DELAY_MS      	400    	//ms
#define ORION_PENALTY_BOX_MS           	350    	//ms
#define ORION_MONITOR_POLL_DELAY_MS     10    	//ms
#define ORION_AID_DISCONNECT_MS         5    	//ms
#define ORION_DISCONNECT_DEBOUNCE_US    80U   	//us
#define ORION_SOFT_RESET_MS            	200    	//ms

#define POLL_CONSUMER_DISCONNECT

const char *orionStatePrint[] = {
	"Reset",
	"SoftReset",
	"Suspend",
	"PingLow",
	"PingHigh",
	"ProvProvider",
	"Provider",
	"WaitConsumer",
	"ProvConsumer",
	"Consumer",
	"ConsProvSetup",
	"ConsProvSwap",
	"ProvConsSetup",
	"ProvConsDrain",
	"ProvConsSwap",
	"PenaltyBox",
	"WaitDrainDisconnect",
	"WaitDisconnect",
	"Monitor",
};

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static orionState_t orionState 			= orionStateReset;
static orionMode_t orionMode 			= orionModeSuspend;
static uint8_t orionAIDIsUp 			= false;
static orionPowerSource_t orionPower 	= orionPowerSourceNone;
static uint8_t orionPowerPresent 		= false;
static uint8_t orionSwapInProgress 		= false;
#ifdef CNFG_ATS_SUPPORT
static orionConnectState_t orionConnectState = orionDisconnected;
#endif
static timer_t timer;            		// interval timer
static timer_t wakeTimer;            	// interval timer
static uint16_t orionIdleDataVoltage;
static uint32_t orionVbusVoltage;
static uint32_t orionSoftResetDelay = ORION_SOFT_RESET_MS;

#ifdef CNFG_COLLECT_ORION_STATISTICS
uint32_t orionStatistics[orionErrorCount];
#endif

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  This function sets the pullups
  * @param  None
  * @retval current state
  */
void setOrionPull(orionLineState_t state)
{
	//DEBUG_PRINT_ORION("set pull, s=%d", state);
	bspSetOrionPull(state);
}

/**
  * @brief  This function is called to set the power switches for the interface.
  * @param  source
  *           if source = none, we are a consumer
  * @retval None
  */
static void setOrionPower(orionPowerSource_t source, uint8_t highPower)
{
	//DEBUG_PRINT_ORION("set source, s=%d, hp=%d", source, highPower);
	if ((source == orionPowerSourceEnable) && (idioBulkDataIsEnabled()) && (!mainGetAuthState()))
	{
		DEBUG_PRINT_ORION("\033[33mOverride: No power in\033[0m");
		bspSetOrionPower(orionPowerSourceNone, 0);
	}
	else
	{
		// if target is orionPowerSourceDrain or orionPowerSourceNone, execute directly
		DEBUG_PRINT_ORION("SetOrionPower %s", source == orionPowerSourceEnable ? "enable" : "none");
		bspSetOrionPower(source, highPower);
	}
}


/**
  * @brief  This function checks that the data bit is above the disconnect threshold for the debounce time
  * @param  None
  * @retval current state (true is > 2.3V (nom))
  */
uint8_t getOrionDataAboveRMThreshold(void)
{
	uint8_t state;
	uint16_t start = HF_TIMER->CNT;
	while((state = bspOrionDataAboveRMThreshold()) && ((uint16_t)(HF_TIMER->CNT - start) <= ORION_DISCONNECT_DEBOUNCE_US));
	return state;
}


/**
  * @brief  This function checks that the data bit is above the low threshold for the debounce time
  * @param  None
  * @retval current state (true is above 1.1V (nom))
  */
uint8_t getOrionDataAboveRxThreshold(void)
{
	uint8_t state;
	uint16_t start = HF_TIMER->CNT;
	while((state = bspOrionDataBelowRxThreshold()) && ((uint16_t)(HF_TIMER->CNT - start) <= ORION_DISCONNECT_DEBOUNCE_US));
	return !state;
}


/**
  * @brief  This function checks that the data bit is in range for longer than the debounce time
  * @param  None
  * @retval current state (true is above 1.1V (nom))
  */
uint8_t getOrionDataInRange(void)
{
	uint8_t state;
	uint16_t start = HF_TIMER->CNT;
	// data is between 1.5V and 2.4V
	while((state = (!bspOrionDataBelowRxThreshold() && !bspOrionDataAboveRMThreshold())) && ((uint16_t)(HF_TIMER->CNT - start) <= ORION_DISCONNECT_DEBOUNCE_US));
	return state;
}


/**
  * @brief  This function filters the RM and Rx bit status
  *
  * This filter should ensure that we don't detect data being transmitted as a loss of the data signal.
  * @param  None
  * @retval current state (true is above 1.1V (nom))
  */
uint8_t getOrionFilteredDataValid(void)
{
	static uint8_t filter = 9;

	if (getOrionDataAboveRMThreshold()) 		{ filter = 0; }					// data > 2.4V
	else {
		if (getOrionDataAboveRxThreshold()) 	{ if (filter < 16) filter++; }	// 1.5V < data < 2.4V
		else 									{ if (filter > 0) filter--; }	// data < 1.5V
	}
	DEBUG_PRINT_ORION("Rx f=%d", filter);
	return (filter > 8);
}


/**
  * @brief  This function calculates whether the orion Vbus is acceptable
  * @param  None
  * @retval current state (true is above 3.3V (nom))
  */
static uint8_t getOrionVbusAboveThreshold(void)
{
	uint32_t voltage;
	uint8_t powerOk 				= false;
	uint8_t oldOrionPowerPresent 	= orionPowerPresent;

	voltage = bspGetOrionVbusVoltage();

	if (!orionPowerPresent && (voltage > ORION_POWER_PRESENT_THRESHOLD))
	{
		// previously no, now yes
		orionPowerPresent = true;
		powerOk = true;
	}
	else if (orionPowerPresent && (voltage < ORION_POWER_REMOVAL_THRESHOLD))
	{
		// previous yes, now no
		orionPowerPresent = false;
		powerOk = false;
	}
	else
	{
		// no change
		powerOk = orionPowerPresent;
	}

	if (ABS_DIFF(orionVbusVoltage,voltage) > 500)
	{
		DEBUG_PRINT_ORION("Vbus change to %dmV", (unsigned int)voltage);
	}
	if (oldOrionPowerPresent != orionPowerPresent)
	{
		DEBUG_PRINT_ORION("Vbus %spresent", orionPowerPresent ? "" : "not ");
	}
	orionVbusVoltage = voltage;
	return powerOk;
}


/**
  * @brief  This function calculates whether the orion Vbus is acceptable
  * @param  None
  * @retval current state (true is above 3.3V (nom))
  */
static uint8_t getOrionVbusDischarged(void)
{
	uint32_t voltage;
	uint8_t powerFail = false;

	voltage = bspGetOrionVbusVoltage();

	if (voltage < ORION_POWER_DISCHARGE_THRESHOLD)
	{
		DEBUG_PRINT_ORION("Vbus discharged");
		powerFail = true;
	}

	if (ABS_DIFF(orionVbusVoltage,voltage) > 500)
	{
		DEBUG_PRINT_ORION("Vbus change to %dmV", (unsigned int)voltage);
	}
	return powerFail;
}


/* Public functions ----------------------------------------------------------*/

/**
  * @brief  This function gets the system state
  * @param  None
  * @retval current state
  */
orionState_t getOrionState(void) { return orionState; }


/**
  * @brief  This function decodes the system state to see if we are connected
  * @param  None
  * @retval current state
  */
orionConnectState_t orionIsConnected(orionState_t state)
{
	orionConnectState_t connected;
	switch (state)
	{
		case orionStateProvider:
		case orionStateConsumer:
		case orionStateProvProvider:
		case orionStateProvConsumer:
		{
			connected = orionConnected;
			break;
		}
		case orionStatePingLow:
		case orionStatePingHigh:
		case orionStateConsProvSetup:
		case orionStateConsProvSwap:
		case orionStateProvConsSetup:
		case orionStateProvConsDrain:
		case orionStateProvConsSwap:
		{
			connected = orionConnecting;
			break;
		}
	#ifdef CNFG_ATS_SUPPORT
		case orionStateMonitor:
		{
			connected = orionConnectState;
			break;
		}
	#endif
		default:
		{
			connected = orionDisconnected;
			break;
		}
	}
	return connected;
}


/**
  * @brief  This function decodes the system state to see how we are connected
  * @param  None
  * @retval current state
  */
orionConnectAsState_t getOrionConnectedAs(void)
{
	orionConnectAsState_t connected;
	switch (orionState)
	{
   		case orionStateProvider:
   		{
			connected = orionConnectedProvider;
			break;
   		}
   		case orionStateConsumer:
   		{
   			connected = orionConnectedConsumer;
   			break;
   		}
   		default:
   		{
   			connected = orionNotConnected;
   			break;
   		}
	}
  return connected;
}


/**
  * @brief  This function decodes the system state to see if we are connected
  * @param  None
  * @retval current state
  */
uint8_t getOrionConnected(void) { return (uint8_t)orionIsConnected(orionState); }


void pushOrionConnectionState(void)
{
#ifdef CNFG_ATS_SUPPORT
	uint8_t state = getOrionConnected();
	atfReportAccessoryConnectionState(&state);
#endif
}

#ifdef CNFG_DEBUG_ORION_ENABLED
static const char* stateName[] =
{
	"Reset",
	"Soft Reset",
	"Suspend",
	"Ping Low",
	"Ping High",
	"Prov Provider",
	"Provider High",
	"Wait Consumer",
	"Prov Consumer",
	"Consumer",
	"Setup Cons -> Prov",
	"Swap Cons -> Prov",
	"Setup Prov -> Cons",
	"Drain Prov -> Cons",
	"Swap Prov -> Cons",
	"Penalty Box",
	"Drain before Disconnect",
	"Disconnect Wait",
	"Monitor",
	"unknown",
};
#endif

void setOrionSoftDelay(uint32_t delay) { orionSoftResetDelay = delay; }

/**
  * @brief  This function sets the system state, and executes any transitional actions
  * @param  None
  * @retval true, if system changed state
  */
uint8_t setOrionState(orionState_t newState)
{
	/*
	static uint32_t lastTime = 0;
	if (GetTickCount() - lastTime > 1000)
	{
		lastTime = GetTickCount();
		DEBUG_PRINT_ORION("setOrionState ST=%d", newState);
	}
	*/
	uint8_t changeState = false;
	orionState_t previousState = orionState;
	if (orionState != newState)
	{
		if ((newState 	!= orionStatePingLow) &&
			(orionState != orionStatePingLow) &&
			(newState 	!= orionStatePingHigh) &&
			(orionState != orionStatePingHigh))
		{
			DEBUG_PRINT_ORION("ST: %s (from %s)", stateName[newState], stateName[orionState]);
		}
		changeState = true;
		orionState = newState;
		switch (newState)
		{
			case orionStateReset:
			{
				setOrionPull(orionLineNone);
				setOrionPower(orionPowerSourceNone, false);
				TIMER_CLEAR(timer);
				break;
			}
			case orionStateWaitDrainDisconnect:
			{
				setOrionPower(orionPowerSourceDrain, false);

				if (orionIsConnected(previousState) == orionConnected) { mainSetEvents(MAIN_EVENT_AID_DISCONNECTED); }

				TIMER_SET(timer, ORION_POWER_SWAP_DELAY_MS);
				break;
			}
			case orionStateWaitDisconnect:
			{
				setOrionPull(orionLineNone);
				setOrionPower(orionPowerSourceNone, false);

				if (orionIsConnected(previousState) == orionConnected) { mainSetEvents(MAIN_EVENT_AID_DISCONNECTED); }

				TIMER_SET(timer, ORION_AID_DISCONNECT_MS);
				break;
			}
			case orionStateSoftReset:
			{
				setOrionPower(orionPowerSourceNone, false);
				orionAIDIsUp 			= false;
				orionIdleDataVoltage 	= 0;
				orionPowerPresent 		= false;
				if (orionIsConnected(previousState) == orionConnected) { mainSetEvents(MAIN_EVENT_AID_DISCONNECTED); }
				if (getOrionPowerSource() == orionPowerSourceNone) { setOrionSoftDelay(0); }	// use no delay if not self-powered
				TIMER_SET(timer, orionSoftResetDelay);
				setOrionSoftDelay(ORION_SOFT_RESET_MS);  // reset to default
				pushOrionConnectionState();
				break;
			}
			case orionStateSuspend:
			{
				if (orionMode == orionModeMonitor) 	{ setOrionState(orionStateMonitor); }
				else 								{ setOrionPull(orionLinePullDown); }
				break;
			}
			case orionStatePingLow:
			{
				setOrionPull(orionLinePullDown);
				pushOrionConnectionState();

				if ((orionMode & orionModeAccessory) != 0)
				{
					TIMER_SET(timer, ORION_AID_PING_PU_OFF_ACC_MS);
				}
				else if ((orionMode & orionModeDevice) != 0)
				{
					TIMER_SET(timer, ORION_AID_PING_PU_OFF_DEV_MS);
				}
				break;
			}
			case orionStatePingHigh:
			{
				TIMER_CLEAR(timer);
				break;
			}
			case orionStateProvProvider:
			{
				setOrionPull(orionLinePullUp);
				setOrionPower(orionPower, false);

				mainSetEvents(orionSwapInProgress ? MAIN_EVENT_AID_RESUME : MAIN_EVENT_AID_CONNECT);
				orionSwapInProgress = false;

				if ((orionMode & orionModeAccessory) != 0)
				{
					TIMER_SET(wakeTimer, ORION_WAKE_TIMER_MS * 3);
				}
#ifdef CNFG_SUPPORT_DEVICE_MODE
				else if ((orionMode & orionModeDevice) != 0)
				{
					TIMER_SET(wakeTimer, ORION_WAKE_TIMER_MS);
					mainSetEvents(MAIN_EVENT_AID_CONNECT);  // flag AID as connecting for host mode to enable IDIO
				}
#endif
				else
					TIMER_CLEAR(wakeTimer);

				TIMER_SET(timer, ORION_AID_DETECT_MS);
				break;
			}
			case orionStateProvider:
			{
				setOrionPull(orionLinePullUp);
				setOrionPower(orionPower, true);
				pushOrionConnectionState();

#ifdef CNFG_COLLECT_ORION_STATISTICS
				orionStatistics[orionConnectProvider]++;
#endif
				TIMER_CLEAR(timer);
				break;
			}
			case orionStateWaitConsumer:
			{
				setOrionPull(orionLinePullDown);
				setOrionPower(orionPowerSourceNone, false);

				TIMER_SET(timer, ORION_POWER_READY_DELAY_MS);
				break;
			}
			case orionStateProvConsumer:
			{
				setOrionPull(orionLinePullDown);

				if ((orionMode & orionModeAccessory) != 0)
				{
					TIMER_SET(wakeTimer, ORION_WAKE_TIMER_MS);
				}
#ifdef CNFG_SUPPORT_DEVICE_MODE
				else if ((orionMode & orionModeDevice) != 0)
				{
					TIMER_SET(wakeTimer, ORION_WAKE_TIMER_MS);
				}
#endif
				else
					TIMER_CLEAR(wakeTimer);

				mainSetEvents(orionSwapInProgress ? MAIN_EVENT_AID_RESUME : MAIN_EVENT_AID_CONNECT);
				orionSwapInProgress = false;

				TIMER_SET(timer, ORION_AID_DETECT_MS);
				break;
			}
			case orionStateConsumer:
			{
				setOrionPull(orionLinePullDown);
				pushOrionConnectionState();

#ifdef CNFG_COLLECT_ORION_STATISTICS
				orionStatistics[orionConnectConsumer]++;
#endif
				TIMER_SET(timer, ORION_POWER_POLL_DELAY_MS);
				break;
			}
			case orionStateConsProvSetup:
			{
				mainSetEvents(MAIN_EVENT_AID_PAUSE);
				// @TODO: power sink off
				TIMER_SET(timer, ORION_POWER_SWAP_DELAY_MS);
				break;
			}
			case orionStateConsProvSwap:
			{
				setOrionPull(orionLinePullUp);
				orionAIDIsUp 		= false;
				orionSwapInProgress = true;
				TIMER_SET(timer, ORION_POWER_SWAP_DELAY_MS);
				break;
			}
			case orionStateProvConsSetup:
			{
				mainSetEvents(MAIN_EVENT_AID_PAUSE);
				setOrionPull(orionLinePullUp);
				TIMER_SET(timer, ORION_POWER_SWAP_DELAY_MS);
				break;
			}
			case orionStateProvConsDrain:
			{
				setOrionPower(orionPowerSourceDrain, false);
				TIMER_SET(timer, ORION_POWER_SWAP_DELAY_MS);
				break;
			}
			case orionStateProvConsSwap:
			{
				setOrionPower(orionPowerSourceNone, false);
				setOrionPull(orionLinePullDown);
				orionSwapInProgress = true;
				TIMER_SET(timer, ORION_POWER_SWAP_DELAY_MS);
				break;
			}
			case orionStatePenaltyBox:
			{
				setOrionPull(orionLineNone);
				setOrionPower(orionPowerSourceDrain, false);
				if (orionIsConnected(previousState) == orionConnected) { mainSetEvents(MAIN_EVENT_AID_DISCONNECTED); }
				pushOrionConnectionState();
				TIMER_SET(timer, ORION_PENALTY_BOX_MS);
				break;
			}
			case orionStateMonitor:
			{
				setOrionPull(orionLineNone);
				TIMER_SET(timer, ORION_POWER_POLL_DELAY_MS);
				break;
			}
		}
	}
	return changeState;
}


/**
  * @brief  This function is called periodically to service the state.
  * @param  None
  * @retval None
  */
uint8_t orionService(void)
{
	/*
	static uint32_t lastTime = 0;
	if (GetTickCount() - lastTime > 1000)
	{
		lastTime = GetTickCount();
		DEBUG_PRINT_ORION("orionService ST=%d", orionState);
	}
	*/
	switch (orionState)
	{
    	case orionStateReset:
    	{
    		setOrionState(orionStateSoftReset);
    		break;
    	}
    	case orionStateWaitDisconnect:
    	{
    		if ((TIMER_EXPIRED(timer) == true))
    		{
    			setOrionState(orionStateSoftReset);
    		}
    		break;
    	}
    	case orionStateWaitDrainDisconnect:
    	{
    		if (getOrionVbusDischarged()) { setOrionState(orionStateWaitDisconnect); }
    		else if ((TIMER_EXPIRED(timer) == true))
    		{
    			DEBUG_PRINT_ORION("TO: wait for disc drain, v=%d", (unsigned int)orionVbusVoltage);
    			setOrionState(orionStateWaitDisconnect);
    		}
    		break;
    	}
    	case orionStateSoftReset:
    	{
    		if ((TIMER_EXPIRED(timer) == true)) { setOrionState(orionStateSuspend); }
    		break;
    	}
    	case orionStateSuspend:
    	{
    		if (orionMode != orionModeSuspend)
    		{
    			if (orionPower != orionPowerSourceNone)
    			{
    				// power provider
    				setOrionState(orionStatePingLow);
    			}
    			else if (getOrionDataAboveRxThreshold() && !getOrionDataAboveRMThreshold())
    			{
    				// power consumer, wait data bus in range
    				setOrionState(orionStateWaitConsumer);
    			}
    		}
    		break;
    	}
    	case orionStatePingLow:
    	{
    		if ((TIMER_EXPIRED(timer) == true)) { setOrionState(orionStatePingHigh); }
    		else if ((getOrionVbusAboveThreshold()) ||
    				(getOrionDataAboveRxThreshold() &&
    				!getOrionDataAboveRMThreshold()))
    		{
    			setOrionState(orionStateWaitConsumer);
    		}
    		break;
    	}
    	case orionStatePingHigh:
    	{
    		setOrionPull(orionLinePullUp);
    		// @TODO: This should be different timing for device and accessory
    		tmrDelay_us(ORION_AID_PING_PU_ON_US);
    		if (getOrionDataAboveRxThreshold() && !getOrionDataAboveRMThreshold()) 	setOrionState(orionStateProvProvider);
    		else																	setOrionState(orionStatePingLow);
    		break;
    	}
    	case orionStateProvProvider:
    	{
    		if (getOrionDataAboveRMThreshold())
    		{
    			DEBUG_PRINT_ORION("Lost Data as provider, rm=%d", getOrionDataAboveRMThreshold());
    			setOrionState(orionStateWaitDrainDisconnect);
#ifdef CNFG_COLLECT_ORION_STATISTICS
    			orionStatistics[orionErrorLostData]++;
#endif
    		}
    		else if (orionAIDIsUp)
    		{
    			setOrionState(orionStateProvider);
    		}
    		else if ((TIMER_EXPIRED(timer) == true))
    		{
    			DEBUG_PRINT_ORION("No AID Conn");
    			setOrionState(orionStatePenaltyBox);
#ifdef CNFG_COLLECT_ORION_STATISTICS
    			orionStatistics[orionErrorNoAIDComm]++;
#endif
    		}
    		else if (TIMER_EXPIRED(wakeTimer) == true)
    		{
    			if (((orionMode & orionModeAccessory) != 0) && idbusIsIdle())
    			{
    				idbusSendWake();
    			}
#ifdef CNFG_SUPPORT_DEVICE_MODE
    			else if (((orionMode & orionModeDevice) != 0) && idbusIsIdle())
    			{
    				idioSendHostCommand(&idioIDCommand[0], sizeof(idioIDCommand) - 1);
    			}
#endif
    			TIMER_SET(wakeTimer, ORION_WAKE_TIMER_MS);
    		}
    		break;
    	}
    	case orionStateProvider:
    	{
    		if (getOrionDataAboveRMThreshold())
    		{
    			DEBUG_PRINT_ORION("Lost Data as provider, rm=%d", getOrionDataAboveRMThreshold());
    			setOrionState(orionStateWaitDrainDisconnect);
#ifdef CNFG_COLLECT_ORION_STATISTICS
    			orionStatistics[orionErrorLostData]++;
#endif
    		}
    		break;
    	}
    	case orionStatePenaltyBox:
    	{
    		if (orionPowerPresent && !getOrionVbusAboveThreshold()) { setOrionPower(orionPowerSourceNone, false); }
    		if ((TIMER_EXPIRED(timer) == true))						{ setOrionState(orionStateSoftReset); }
    		break;
    	}
    	case orionStateWaitConsumer:
    	{
    		if (getOrionVbusAboveThreshold()) { setOrionState(orionStateProvConsumer); }
    		if (TIMER_EXPIRED(timer) == true)
    		{
    			DEBUG_PRINT_ORION("TO: wait for Vbus, v=%d", (unsigned int)orionVbusVoltage);
    			setOrionState(orionStateWaitDisconnect);
    		}
    		break;
    	}
    	case orionStateProvConsumer:
    	{
    		if (TIMER_EXPIRED(timer) == true)
    		{
    			DEBUG_PRINT_ORION("No AID Conn");
    			setOrionState(orionStateWaitDisconnect);
#ifdef CNFG_COLLECT_ORION_STATISTICS
    			orionStatistics[orionErrorNoAIDComm]++;
#endif
    		}
    		else if (!getOrionVbusAboveThreshold())
    		{
    			DEBUG_PRINT_ORION("Lost Vbus, v=%dmV", (unsigned int)orionVbusVoltage);
    			setOrionState(orionStateWaitDisconnect);
#ifdef CNFG_COLLECT_ORION_STATISTICS
    			orionStatistics[orionErrorLostVbus]++;
#endif
    		}
    		else if (idbusIsIdle() && !getOrionDataAboveRxThreshold())
    		{
    			DEBUG_PRINT_ORION("Lost data, d=%d", getOrionDataAboveRxThreshold());
    			setOrionState(orionStateWaitDisconnect);
#ifdef CNFG_COLLECT_ORION_STATISTICS
    			orionStatistics[orionErrorLostData]++;
#endif
    		}
    		else if (orionAIDIsUp)
    		{
    			setOrionState(orionStateConsumer);
    		}
    		else if (TIMER_EXPIRED(wakeTimer) == true)
    		{
    			if (idbusIsIdle())
    			{
    				if ((orionMode & orionModeAccessory) != 0)	idbusSendWake();
#ifdef CNFG_SUPPORT_DEVICE_MODE
    				else if ((orionMode & orionModeDevice) != 0)
    					idioSendHostCommand(&idioIDCommand[0], sizeof(idioIDCommand) - 1);
#endif
    			}
    			TIMER_SET(wakeTimer, ORION_WAKE_TIMER_MS);
    		}
    		break;
    	}
    	case orionStateConsumer:
    	{
    		if (TIMER_EXPIRED(timer) == true)
    		{
    			if (!getOrionVbusAboveThreshold())
    			{
    				DEBUG_PRINT_ORION("Lost Vbus, v=%dmV", (unsigned int)orionVbusVoltage);
    				setOrionState(orionStateWaitDisconnect);
#ifdef CNFG_COLLECT_ORION_STATISTICS
    				orionStatistics[orionErrorLostVbus]++;
#endif
    			}
    			else
    			{
    				if (idbusIsIdle())
    				{
#ifdef POLL_CONSUMER_DISCONNECT
    					if (!getOrionDataAboveRxThreshold())
    					{
    						DEBUG_PRINT_ORION("Lost Data, Rx=%d", getOrionDataAboveRxThreshold());
    						setOrionState(orionStateWaitDisconnect);
#ifdef CNFG_COLLECT_ORION_STATISTICS
    						orionStatistics[orionErrorLostData]++;
#endif
    					}
#endif
    				}
    			}
    			TIMER_SET(timer, ORION_POWER_POLL_DELAY_MS);
    		}
    		break;
    	}
    	case orionStateConsProvSetup:
    	{
    		if (!getOrionDataAboveRxThreshold()) 	{ setOrionState(orionStateConsProvSwap); }
    		else if (TIMER_EXPIRED(timer) == true)
    		{
    			DEBUG_PRINT_ORION("TO: wait for Vbus removal, v=%dmV", (unsigned int)orionVbusVoltage);
    			//setOrionState(orionStateWaitDisconnect);
    			setOrionState(orionStateConsProvSwap);
#ifdef CNFG_COLLECT_ORION_STATISTICS
    			orionStatistics[orionErrorSwapTimeout]++;
#endif
    		}
    		break;
    	}
    	case orionStateConsProvSwap:
    	{
    		if (getOrionDataInRange())
    		{
    			setOrionState(orionStateProvProvider);
    			orionAIDIsUp = false;
    		}
    		else if (TIMER_EXPIRED(timer) == true)
    		{
    			DEBUG_PRINT_ORION("TO: wait for Vbus removal, v=%dmV", (unsigned int)orionVbusVoltage);
    			setOrionState(orionStateWaitDisconnect);
#ifdef CNFG_COLLECT_ORION_STATISTICS
    			orionStatistics[orionErrorSwapTimeout]++;
#endif
    		}
    		break;
    	}
    	case orionStateProvConsSetup:
    	{
    		if (getOrionDataAboveRMThreshold()) 		{ setOrionState(orionStateProvConsDrain); }
    		else if ((TIMER_EXPIRED(timer) == true))
    		{
    			DEBUG_PRINT_ORION("TO: wait for disconnect");
    			setOrionState(orionStateWaitDrainDisconnect);
#ifdef CNFG_COLLECT_ORION_STATISTICS
    			orionStatistics[orionErrorSwapTimeout]++;
#endif
    		}
    		break;
    	}
    	case orionStateProvConsDrain:
    	{
    		if (getOrionVbusDischarged()) { setOrionState(orionStateProvConsSwap); }
    		else if (TIMER_EXPIRED(timer) == true)
    		{
    			DEBUG_PRINT_ORION("TO: wait for Vbus drain, v=%dmV", (unsigned int)orionVbusVoltage);
    			setOrionState(orionStateWaitDisconnect);
#ifdef CNFG_COLLECT_ORION_STATISTICS
    			orionStatistics[orionErrorSwapTimeout]++;
#endif
    		}
    		break;
    	}
    	case orionStateProvConsSwap:
    	{
    		if (getOrionDataInRange())
    		{
    			orionAIDIsUp = false;
    			setOrionState(orionStateWaitConsumer);
    		}
    		else if (TIMER_EXPIRED(timer) == true)
    		{
    			DEBUG_PRINT_ORION("TO: wait for data, rx=%d, rm=%d", getOrionDataAboveRxThreshold(), getOrionDataAboveRMThreshold());
    			setOrionState(orionStateWaitDisconnect);
#ifdef CNFG_COLLECT_ORION_STATISTICS
    			orionStatistics[orionErrorSwapTimeout]++;
#endif
    		}
    		break;
    	}
    	case orionStateMonitor:
    	{
#ifdef CNFG_ATS_SUPPORT
    		if ((TIMER_EXPIRED(timer) == true))
    		{
    			if (idbusIsIdle())
    			{
					orionConnectState_t connectState = orionDisconnected;
					uint32_t orionVoltage = 0;
					uint32_t orionPullupVoltage = 0;

					orionVoltage = adcComputeVoltage(ADC_Convert(ALT_ORION_RX3_ADC_CH));
					if (orionVoltage > ORION_DATA_PROVIDER_DISC_THRESHOLD)
					{
						connectState = orionConnecting;
					}
					else if (orionVoltage < ORION_DATA_CONSUMER_DISC_THRESHOLD)
					{
						// line appears low, retest with pullup
						GPIO_InitTypeDef          GPIO_InitStruct;
						GPIO_InitStruct.Pin = GPIO_PIN_BIT(ALT_ORION_RX3);
						GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
						GPIO_InitStruct.Pull = GPIO_PULLUP;
						HAL_GPIO_Init(GPIO_PIN_PORT(ALT_ORION_RX3), &GPIO_InitStruct);
						tmrDelay_us(10);  // set empirically
						GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
						GPIO_InitStruct.Pull = GPIO_NOPULL;
						HAL_GPIO_Init(GPIO_PIN_PORT(ALT_ORION_RX3), &GPIO_InitStruct);
						orionPullupVoltage = adcComputeVoltage(ADC_Convert(ALT_ORION_RX3_ADC_CH));

						if (orionPullupVoltage < ORION_DATA_CONSUMER_DISC_THRESHOLD)
						{
							// pull down is active somewhere
							connectState = orionConnecting;
						}
						else
						{
							connectState = orionDisconnected;
						}
					}
					else
					{
						connectState = orionConnected;
					}
					//DEBUG_PRINT_ORION("Monitor State, s=%d, ocv=%lu, puv=%lu", connectState, orionVoltage, orionPullupVoltage);
					if (orionConnectState != connectState)
					{
						DEBUG_PRINT_ORION("Monitor State Change, s=%d, ocv=%lu, puv=%lu", connectState, orionVoltage, orionPullupVoltage);

						orionConnectState = connectState;
						atfReportAccessoryConnectionState(&connectState);
					}
    			}
    			TIMER_SET(timer, ORION_MONITOR_POLL_DELAY_MS);
    		}
#endif
    		break;
    	}
    	default:
    	{
    		break;
    	}
	}
	return true;
}


/**
  * @brief  This function is called to disconnect the FSM (e.g., async bus removal).
  * @param  None
  * @retval true, if system changed state
  */
uint8_t orionDisconnect(void) { return setOrionState(orionStateWaitDisconnect); }


/**
  * @brief  This function is called to suspend / enable the interface.
  *
  * If the system is suspended, it will start up when the service routine is called. If it is not,
  * the system will suspend on the next disconnect.
  *
  * @param  uint8_t, true to suspend
  * @retval true, if system changed state
  */
uint8_t setOrionMode(orionMode_t mode)
{
	orionMode_t oldMode = orionMode;
	orionMode 			= mode;
	return (oldMode != mode);
}


/**
  * @brief  This function is called to get the suspend / enable state of the interface.
  *
  * @param  None
  * @retval suspend state (true = suspended)
  */
orionMode_t getOrionMode(void) { return orionMode; }


/**
  * @brief  This function is called to start the swap in power roles.
  * @param  None
  * @retval true, if system changed state
  */
uint8_t setOrionPowerSwap(void)
{
	uint8_t switching = false;
	switch (orionState)
	{
		case orionStateProvider:
		{
			setOrionState(orionStateProvConsSetup);
			switching = true;
			break;
		}
		case orionStateConsumer:
		{
			if (orionPower != orionPowerSourceNone)
			{
				setOrionState(orionStateConsProvSetup);
				switching = true;
			}
        else
        	DEBUG_PRINT_ORION("C->P fail, p=%d", orionPower);
        break;
		}
		default:
		{
			DEBUG_PRINT_ORION("pswap fail, s=%d", orionState);
			break;
		}
	}
	return switching;
}


/**
  * @brief  This function is called to move from low to high power mode.
  * @param  None
  * @retval true, if system changed state
  */
uint8_t setOrionHighPower(void)
{
	uint8_t switching = false;
	if (orionState == orionStateProvider)
	{
		setOrionPower(orionPower, true);
		switching = true;
	}
	return switching;
}


/**
  * @brief  This function is called to indicate that AID is running.
  * @param  AID state (true = working)
  * @retval None
  */
void setOrionAIDState(uint8_t up) { orionAIDIsUp = up; }


/**
  * @brief  This function is called to get the AID state.
  * @param  None
  * @retval AID state (true = working)
  */
uint8_t getOrionAIDState(void) { return orionAIDIsUp; }


/**
  * @brief  This function is called to get the power source for the interface.
  * @param  None
  * @retval source
  */
orionPowerSource_t getOrionPowerSource(void) { return orionPower; }


/**
  * @brief  This function is called to set the power source for the interface.
  * @param  source
  *           if source = none, we are a consumer
  * @retval None
  */
uint8_t setOrionPowerSource(orionPowerSource_t source)
{
	orionPowerSource_t oldSource = orionPower;
	orionPower = source;
	return (oldSource != orionPower);
}


/**
  * @brief  This function returns the polled data level for the Orion bus.
  * @param  source
  *           if source = none, we are a consumer
  * @retval None
  */
uint16_t getOrionDataIdleLevel(void) { return orionIdleDataVoltage; }


/**
  * @brief  This function is called to disconnect the FSM (e.g., async bus removal).
  * @param  None
  * @retval true, if system changed state
  */
uint8_t orionInit(void)
{
	orionState 		= orionStateReset;
	orionPower 		= orionPowerSourceNone;
	orionAIDIsUp 	= false;

#ifdef CNFG_COLLECT_ORION_STATISTICS
	{
		uint32_t i;
		for (i = 0; i < orionErrorCount; i++)
			orionStatistics[i] = 0;
	}
#endif
	return true;
}


#ifdef CNFG_AID_POWER_CONTROL
/**
  * @brief  This function is called to enable / disable the power to the device based on AID response
  *
  */
void setOrionInterfacePower(uint8_t mode)
{
}


/**
  * @brief  This function is called to get the power state of the device based on the AID response
  * @param  None
  * @retval true = power enabled
  */
uint8_t getOrionInterfacePower(void)
{
	return false;
}
#endif

#ifdef CNFG_AID_ID_VARIABLE_RESPONSE
uint8_t orionCommandIdResponse[6] = {
	AID_RESPONSE_ACCx_NORMAL | AID_RESPONSE_USB_NONE | AID_RESPONSE_UART_NONE,
	AID_RESPONSE_POWER_UHPM | AID_RESPONSE_HIGHVOLTAGE_5V | AID_RESPONSE_ACCINFO /*| AID_RESPONSE_AUTH*/,
	AID_RESPONSE_BD,
	0,
	0,
	0,
};


/**
  * @brief  This function is called to disable the interrupt handler for the RM threshold detector
  * @param  None
  * @retval None
  */
uint8_t* getOrionId(uint16_t systemId)
{
	if (!orionAIDIsUp) { mainSetEvents(MAIN_EVENT_AID_RUNNING); }
	return &orionCommandIdResponse[0];
}
#endif
