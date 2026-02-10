/*
 * aid_pd.c
 *
 *  Created on: Feb 9, 2026
 *      Author: xiongwei
 */


/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "timers.h"
#include "aid_pd.h"
#include "ep_pd.h"
#include "debug.h"
#include "orion.h"
#include "bsp.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
//#define POLL_SOURCE_CAP  // required for iOS < 13C45

// Timing parameters
#define AIDPD_SOURCE_CAP_MS           	1000    //ms
#define AIDPD_SINK_WAIT_CAP_MS         	250    	//ms
#define AIDPD_IPS_TRANSITION_MS        	550    	//ms
#define AIDPD_WAIT_SOURCE_CAP_MS      	1000    //ms
#define AIDPD_WAIT_RESPONSE_MS        	1000    //ms
#define AIDPD_WAIT_COMMAND_MS         	5000    //ms
#define AIDPD_SOURCE_POWER_ENABLE_MS    10    	//ms
#define AIDPD_RETRY_SEND_MS             6    	//ms
#define AIDPD_POWER_POLL_MS             50    	//ms
#define AIDPD_TX_FLUSH_MS               1    	//ms
#define AIDPD_REQ_INTERVAL_MS          	750    	//ms

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static aidpdState_t aidpdState 			= aidpdStateReset;
static aidpdPwrState_t aidpdPwrState 	= aidpdPwrStateNone;

// PDO storage
static uint32_t deviceSourceCapability[7];	// device means iPad?
static uint32_t accSourceCapability[7];
static uint32_t accSinkNeed;
static uint32_t deviceSinkNeed;
static uint8_t numValidDeviceSourceCapability 	= 0;
static uint8_t numValidAccSourceCapability 		= 0;

static timer_t timer;
#ifdef POLL_SOURCE_CAP
static timer_t reqTimer;
#endif

#ifdef CNFG_DEBUG_IDIO_EP_AID_PD_ENABLED
static const char* stateName[] =
{
	"Reset",
	"Idle",
	"ConfigPower",
	"WaitAccept",
	"WaitReady",
	"WaitSwapAccept",
	"Swap",
	"SendHardReset",
};

static const char* commandName[] =
{
	"RESET",    //                        0x80
	"",         //                        0x01
	"GOTO_MIN", //                        0x02
	"ACCEPT",   //                        0x03
	"REJECT",   //                        0x04
	"",         //                        0x05
	"PS_READY", //                        0x06
	"GET_SOURCE_CAPABILITY",  //          0x07
	"GET_SINK_CAPABILITY", //             0x08
	"",         //                        0x09
	"POWER_SWAP", //                      0x0A
	"",         //                        0x05
	"WAIT", //                            0x0C
};

static const char* respName[] =
{
	"SOURCE_CAPABILITY", //               0x01
	"REQUEST", //                         0x02
	"",         //                        0x03
	"SINK_CAPABILITY", //                 0x04
};
#endif


/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  This determines if the actual capacity exceeds the needs
  * @param  None
  * @retval updated need PDO, 0 if no match
  */
static uint32_t capabilityMet(uint32_t* have, uint8_t numHave, uint32_t need)
{
	uint32_t PDO 		= (need >> USB_REQ_OBJECT_POSITION_SHIFT) & USB_REQ_OBJECT_POSITION_MASK;
	uint32_t updatedPDO = 0;
	uint32_t reqCurrent = (need >> USB_REQ_MINMAX_CURRENT_SHIFT) & USB_REQ_MINMAX_CURRENT_MASK;
	uint32_t PDOCurrent;
	//DEBUG_PRINT_AIDPD("PDO=%u, req=%u", PDO, reqCurrent);
	if (PDO == 0)
	{
		// scan available PDSs for a match
		while ((PDO < numHave) && (updatedPDO == 0))
		{
			PDOCurrent = (have[PDO] >> USB_PDO_CURRENT_SHIFT) & USB_PDO_CURRENT_MASK;
			//DEBUG_PRINT_AIDPD("PDO=%u, current=%u", PDO, PDOCurrent);
			if (PDOCurrent >= reqCurrent)
				updatedPDO = need | USB_REQ_OBJECT_POSITION(PDO);
			PDO++;
		}
	}
	else
	{
		PDOCurrent = (have[PDO-1] >> USB_PDO_CURRENT_SHIFT) & USB_PDO_CURRENT_MASK;
		if (PDOCurrent >= reqCurrent)
			updatedPDO = need;
	}

	DEBUG_PRINT_AIDPD("new req=0x%08x, PDO=%u", (uint32_t)updatedPDO, PDO);
	return updatedPDO;
}

/**
  * @brief  dump PDO if debugging
  * @param  PDO - pointer to array of PDOs
  * @param  num - number of valid PDOs
  */
static void dumpPDO(uint32_t* PDO, uint8_t num)
{
#ifdef CNFG_DEBUG_IDIO_EP_AID_PD_ENABLED
	static const char* PDOType[] 		= {"fixed", "battery", "variable"};
	static const char* PDOP1[] 			= {"flags", "maxV", "maxV"};
	static const char* PDOP2[] 			= {"V", "minV", "minV"};
	static const char* PDOP3[] 			= {"I(mA)", "P(mW)", "I(mA)"};
	static const uint32_t PDOP1Scale[] 	= {1, 50, 50};
	static const uint32_t PDOP2Scale[] 	= {50, 50, 50};
	static const uint32_t PDOP3Scale[] 	= {10, 250, 10};
#ifdef DEBUG_ENABLED
	if (configDebugAidPDPrintEnabled())
	{
		uint8_t i = 0;
		debugPrint("AIDPD: %d PDOs", num);
		while (i < num)
		{
			uint8_t pdoType = (*PDO >> USB_PDO_TYPE_SHIFT) & USB_PDO_TYPE_MASK;
			printf("  %d is %s %s %s %s=%u, %s=%u, %s=%u\n",
					i + 1, PDOType[pdoType],
					(*PDO & USB_PDO_DUAL_ROLE) ? "DFP" : "",
					(*PDO & USB_PDO_EXTERNAL_POWER) ? "EP" : "",
					PDOP1[pdoType], (unsigned int)(((*PDO >> USB_PDO_MAX_VOLTAGE_SHIFT) & USB_PDO_MAX_VOLTAGE_MASK) * PDOP1Scale[pdoType]),
					PDOP2[pdoType], (unsigned int)(((*PDO >> USB_PDO_MIN_VOLTAGE_SHIFT) & USB_PDO_MIN_VOLTAGE_MASK) * PDOP2Scale[pdoType]),
					PDOP3[pdoType], (unsigned int)(((*PDO >> USB_PDO_CURRENT_SHIFT) & USB_PDO_CURRENT_MASK) * PDOP3Scale[pdoType]) );
			i++;
			PDO++;
		}
	}
#endif
#endif
}


/**
  * @brief  dump Req if debugging
  * @param  req - req value
  */
static void dumpReq(uint32_t req)
{
#ifdef CNFG_DEBUG_IDIO_EP_AID_PD_ENABLED
	if (configDebugAidPDPrintEnabled())
	{
		printf("  op=%u, gb=%u, oc=%u, max oc=%u\n",
				(unsigned int)((req >> USB_REQ_OBJECT_POSITION_SHIFT) & USB_REQ_OBJECT_POSITION_MASK),
				(unsigned int)((req & USB_REQ_GIVE_BACK)),
				(unsigned int)(((req >> USB_REQ_OP_CURRENT_SHIFT) & USB_REQ_OP_CURRENT_MASK) * 10),
				(unsigned int)(((req >> USB_REQ_MINMAX_CURRENT_SHIFT) & USB_REQ_MINMAX_CURRENT_MASK) * 10 ));
	}
#endif
}


/**
  * @brief  send SourceCapabilities
  * @return true if sent
  */
uint8_t sendSourceCap(bool auth)
{
	uint8_t ret = false;

	if (auth)
	{
		uint32_t tmpCapability[7];
		uint8_t i;

		orionPowerSource_t power = getOrionPowerSource();
		uint8_t showPower = (power != orionPowerSourceNone) ? bspPowerAvailable(power) : false;
		for (i=0; i<numValidAccSourceCapability; i++)
		{
			tmpCapability[i] = accSourceCapability[i] | (showPower ? (USB_PDO_DUAL_ROLE | USB_PDO_EXTERNAL_POWER) : 0);
		}
		if (epPDQueueData(EP_PD_RESP_SOURCE_CAPABILITY, (uint8_t*) &tmpCapability[0], numValidAccSourceCapability * sizeof(uint32_t)))
		{
			dumpPDO(&tmpCapability[0], numValidAccSourceCapability);
			ret = true;
		}
	}
	else
	{
		uint32_t tmpCapability = PDO_VSAFE0V;
		if (epPDQueueData(EP_PD_RESP_SOURCE_CAPABILITY, (uint8_t*) &tmpCapability, sizeof(uint32_t)))
		{
			dumpPDO(&tmpCapability, 1);
			ret = true;
		}
	}
	return ret;
}


/**
  * @brief  This function sets the system state, and executes any transitional actions
  * @param  None
  * @retval true, if system changed state
  */
static uint8_t setAidPDState(aidpdState_t newState)
{
	uint8_t changeState = false;
	if (aidpdState != newState)
	{
		if ((newState != aidpdStateWaitAccept) && (aidpdState!=aidpdStateWaitAccept) &&
			(newState != aidpdStateWaitReady) && (aidpdState!=aidpdStateWaitReady))
		{
			DEBUG_PRINT_AIDPD("ST: %s (from %s)", stateName[newState], stateName[aidpdState]);
		}
		changeState = true;
		aidpdState = newState;

		switch (newState)
		{
			case aidpdStateReset:
			{
				epPDInit();
				bspSinkEnable(false);
				aidpdPwrState = aidpdPwrStateNone;
				setAidPDState(aidpdStateIdle);
				TIMER_CLEAR(timer);
#ifdef POLL_SOURCE_CAP
				TIMER_SET(reqTimer, AIDPD_REQ_INTERVAL_MS);
#endif
				break;
			}
			case aidpdStateIdle:
			{
				TIMER_SET(timer, AIDPD_POWER_POLL_MS);
				break;
			}
			case aidpdStateWaitAccept:
			case aidpdStateWaitSwapAccept:
			{
				TIMER_SET(timer, AIDPD_WAIT_RESPONSE_MS);
#ifdef POLL_SOURCE_CAP
				TIMER_CLEAR(reqTimer);
#endif
				break;
			}
			case aidpdStateWaitReady:
			{
				TIMER_SET(timer, AIDPD_IPS_TRANSITION_MS);
				break;
			}
			case aidpdStateConfigPower:
			{
				TIMER_CLEAR(timer);
				break;
			}
			case aidpdStateSwap:
			{
				TIMER_CLEAR(timer);
#ifdef POLL_SOURCE_CAP
				TIMER_SET(reqTimer, AIDPD_REQ_INTERVAL_MS);
#endif
				break;
			}
			case aidpdStateSendHardReset:
			{
				if (epPDQueueCommand(EP_PD_CMD_RESET))
				{
					bspSinkEnable(false);
					aidpdPwrState = aidpdPwrStateNone;
					setAidPDState(aidpdStateIdle);
				}
				break;
			}
			default:
			{
				break;
			}
		}
	}
	return changeState;
}


/**
  * @brief  This function is called when a command is received.
  *
  * It may change state depending on the current state and message.
  *
  * @param  None
  * @retval None
  */
bool aidPDReceivedCommand(uint8_t command)
{
	bool actedOn = false;
	//DEBUG_PRINT_AIDPD("Rx command 0x%02x", command);
	switch(command)
	{
    	case EP_PD_CMD_RESET:
    	{
    		epPDInit();   // flush Tx FIFO
    		aidpdPwrState = aidpdPwrStateNone;
    		setAidPDState(aidpdStateIdle);
    		actedOn = true;
    		break;
    	}
    	case EP_PD_CMD_ACCEPT:
    	{
    		if (aidpdState == aidpdStateWaitAccept)
    		{
    			setAidPDState(aidpdStateWaitReady);
    		}
    		else if (aidpdState == aidpdStateWaitSwapAccept)
    		{
    			setAidPDState(aidpdStateSwap);
    		}
    		else
    			DEBUG_PRINT_AIDPD("ignored command %s in state %s", commandName[command], stateName[aidpdState]);
    		actedOn = true;
    		break;
    	}
    	case EP_PD_CMD_REJECT:
    	{
    		if ((aidpdState == aidpdStateWaitAccept) ||
    			(aidpdState == aidpdStateWaitSwapAccept))
    		{
    			DEBUG_PRINT_AIDPD("Rx Reject from state %s", stateName[aidpdState]);
    			setAidPDState(aidpdStateSendHardReset);
    		}
    		else
    			DEBUG_PRINT_AIDPD("ignored command %s in state %s", commandName[command], stateName[aidpdState]);
    		actedOn = true;
    		break;
    	}
    	case EP_PD_CMD_PS_READY:
    	{
    		if (aidpdState == aidpdStateWaitReady)
    		{
    			aidpdPwrState = aidpdPwrStateCons;
    			DEBUG_PRINT_AIDPD("Sink Ready");
    			bspSinkEnable(true);
    			setAidPDState(aidpdStateIdle);
    		}
    		else
    			DEBUG_PRINT_AIDPD("ignored command %s in state %s", commandName[command], stateName[aidpdState]);
    		actedOn = true;
    		break;
    	}
    	case EP_PD_CMD_GET_SOURCE_CAPABILITY:
    	{
    		// iOS randomly sends GetSourceCap requests
    		// @TODO: should we always respond regardless of state?
    		if ((aidpdState == aidpdStateIdle) || (aidpdState == aidpdStateWaitAccept))
    		{
    			if (mainGetAuthState())
    			{
    				sendSourceCap(true);
    				DEBUG_PRINT_AIDPD("GetSourceCap");
    			}
    			else
    			{
    				sendSourceCap(false);
    				mainSetEvents(MAIN_EVENT_SERVICE_PD);
    				DEBUG_PRINT_AIDPD("DEFER GetSourceCap");
    			}
    		}
    		else
    			DEBUG_PRINT_AIDPD("ignored command %s in state %s", commandName[command], stateName[aidpdState]);
    		actedOn = true;
    		break;
    	}
    	case EP_PD_CMD_POWER_SWAP:
    	{
    		if (aidpdState == aidpdStateIdle)
    		{
    			if (((aidpdPwrState == aidpdPwrStateNone) || (aidpdPwrState == aidpdPwrStateCons))
    					&& (numValidAccSourceCapability > 0))
    			{
    				if (epPDQueueCommand(EP_PD_CMD_ACCEPT))
    				{
    					setAidPDState(aidpdStateSwap);
    				}
    			}
    			else if (aidpdPwrState == aidpdPwrStateProv)
    			{
    				if (epPDQueueCommand(EP_PD_CMD_ACCEPT))
    				{
    					setAidPDState(aidpdStateSwap);
    				}
    			}
    			else
    			{
    				DEBUG_PRINT_AIDPD("reject PR_SWAP, ps=%u, pa=%u", aidpdPwrState, (numValidAccSourceCapability > 0));
    				epPDQueueCommand(EP_PD_CMD_REJECT);
    			}
    		}
    		else
    			DEBUG_PRINT_AIDPD("ignored command %s in state %s, ps=%u", commandName[command], stateName[aidpdState], aidpdPwrState);
    		actedOn = true;
    		break;
    	}
		case EP_PD_CMD_GET_SINK_CAPABILITY:
		case EP_PD_CMD_WAIT:
		case EP_PD_CMD_GOTO_MIN:
		default:
			DEBUG_PRINT_AIDPD("ignored command %s in state %s", commandName[command], stateName[aidpdState]);
			break;
	}
	return actedOn;
}


bool aidPDReceivedData(uint8_t command, uint8_t* dataPtr, uint8_t length)
{
	bool actedOn = false;
	//DEBUG_PRINT_AIDPD("Rx command+data 0x%02x", command);
	switch(command)
	{
    	case EP_PD_RESP_SOURCE_CAPABILITY:
    	{
    		if (aidpdState == aidpdStateIdle)
    		{
    			uint32_t tmpReq = accSinkNeed & ~(USB_REQ_OBJECT_POSITION_MASK << USB_REQ_OBJECT_POSITION_SHIFT);
				memcpy(&deviceSourceCapability[0], dataPtr, length);
				numValidDeviceSourceCapability = length / 4;
				dumpPDO(&deviceSourceCapability[0], numValidDeviceSourceCapability);

				orionPowerSource_t power = getOrionPowerSource();
				if ((power != orionPowerSourceNone) && !bspPowerAvailable(power))
				{ // want to be a provider, but without power
					DEBUG_PRINT_AIDPD("reject provider !power p=%d, pa=%d", power, bspPowerAvailable(power));
					epPDQueueCommand(EP_PD_CMD_REJECT);
				}
				else if ((power != orionPowerSourceNone) && bspPowerAvailable(power))
				{ // want to be a provider, have power
					DEBUG_PRINT_AIDPD("want to be provider w/ power p=%d, pa=%d", power, bspPowerAvailable(power));
				}
				else if ((tmpReq = capabilityMet(&deviceSourceCapability[0], numValidDeviceSourceCapability, tmpReq)) != 0)
				{ // consumer met needs
					accSinkNeed = tmpReq;
					if (epPDQueueData(EP_PD_RESP_REQUEST, (uint8_t*)&accSinkNeed, 4))
					{
						dumpReq(accSinkNeed);
						setAidPDState(aidpdStateWaitAccept);
					}
				}
				else
				{ // consumer didn't meet needs
					DEBUG_PRINT_AIDPD("req > cap");
				}
    		}
    		else
    			DEBUG_PRINT_AIDPD("ignored command %s in state %s", respName[command], stateName[aidpdState]);
    		actedOn = true;
    		break;
    	}
    	case EP_PD_RESP_REQUEST:
    	{
    		if ((aidpdState == aidpdStateIdle) && (numValidAccSourceCapability > 0))
    		{
    			memcpy(&deviceSinkNeed, dataPtr, 4);
    			dumpReq(deviceSinkNeed);
    			DEBUG_PRINT_AIDPD("Req for %umA (0x%08x)", (unsigned int)((deviceSinkNeed & USB_REQ_MINMAX_CURRENT_MASK) * 10), (unsigned int)deviceSinkNeed);

    			if (capabilityMet(&accSourceCapability[0], numValidAccSourceCapability, deviceSinkNeed))
    			{
    				if (epPDQueueCommand(EP_PD_CMD_ACCEPT))
    				{
    					setAidPDState(aidpdStateConfigPower);
    				}
    			}
    			else
    			{
    				epPDQueueCommand(EP_PD_CMD_REJECT);
    			}
    		}
    		else
    			DEBUG_PRINT_AIDPD("ignored command %s in state %s", respName[command], stateName[aidpdState]);
    		actedOn = true;
    		break;
    	}
    	default:
    	{
    		DEBUG_PRINT_AIDPD("command+data ignored 0x%02x", command);
    		break;
    	}
	}
	return actedOn;
}

/**
  * @brief  This function is called periodically to service the state.
  * @param  None
  * @retval None
  */
uint8_t aidpdService(void)
{
	switch (aidpdState)
	{
    	case aidpdStateIdle:
    	{
#ifdef POLL_SOURCE_CAP
    		if ((aidpdPwrState == aidpdPwrStateNone) && (TIMER_EXPIRED(reqTimer) == true))
    		{
    			epPDQueueCommand(EP_PD_CMD_GET_SOURCE_CAPABILITY);
    			TIMER_SET(reqTimer, AIDPD_REQ_INTERVAL_MS);
    			//TIMER_CLEAR(reqTimer);
    		}
#endif
    		if ((TIMER_EXPIRED(timer) == true))
    		{
    			orionPowerSource_t power = getOrionPowerSource();
    			if ((aidpdPwrState == aidpdPwrStateProv) && !bspPowerAvailable(power))
    			{
    				DEBUG_PRINT_AIDPD("provider power fail p=%d, pa=%d", power, bspPowerAvailable(power));
    				if (epPDQueueCommand(EP_PD_CMD_POWER_SWAP))
    				{
    					setAidPDState(aidpdStateWaitSwapAccept);
    					aidpdPwrState = aidpdPwrStateNone;
    				}
    			}
    			else if ((aidpdPwrState == aidpdPwrStateCons) && bspPowerAvailable(power))
    			{
    				DEBUG_PRINT_AIDPD("provider power ready p=%d, pa=%d", power, bspPowerAvailable(power));
    				setAidPDState(aidpdStateSendHardReset);
    			}
    			else
    				TIMER_SET(timer, AIDPD_POWER_POLL_MS);
    		}
    		break;
    	}
    	case aidpdStateConfigPower:
    	{
    		if (!TIMER_RUNNING(timer) && setOrionHighPower())
    		{
    			TIMER_SET(timer, AIDPD_SOURCE_POWER_ENABLE_MS);
    		}
    		if ((TIMER_EXPIRED(timer) == true))
    		{
    			if (epPDQueueCommand(EP_PD_CMD_PS_READY))
    			{
    				aidpdPwrState = aidpdPwrStateProv;
    				DEBUG_PRINT_AIDPD("Source Ready");
    				setAidPDState(aidpdStateIdle);
    			}
    		}
    		break;
    	}
		case aidpdStateWaitAccept:
		case aidpdStateWaitReady:
		case aidpdStateWaitSwapAccept:
		{
			if ((TIMER_EXPIRED(timer) == true))
			{
				DEBUG_PRINT_AIDPD("TO %s", stateName[aidpdState]);
				setAidPDState(aidpdStateIdle);
			}
			break;
		}
		case aidpdStateSwap:
		{
			// wait for swap to complete and send reset
			if (!TIMER_RUNNING(timer))
			{
				if (epPDTxQueueEmpty() && (idioIdleTime() > AIDPD_TX_FLUSH_MS))
				{
					if (setOrionPowerSwap())
					{
						TIMER_SET(timer, AIDPD_IPS_TRANSITION_MS);
					}
					else
					{
						DEBUG_PRINT_AIDPD("swap failed at interface");
						setAidPDState(aidpdStateSendHardReset);
					}
				}
			}
			else if (TIMER_RUNNING(timer) && (getOrionConnectedAs() != orionNotConnected))
			{
				// swap complete
				setAidPDState(aidpdStateIdle);
			}
			else if ((TIMER_EXPIRED(timer) == true))
			{
				DEBUG_PRINT_AIDPD("TO: wait for swap");
				setAidPDState(aidpdStateSendHardReset);
			}
			break;
		}
		case aidpdStateReset:
		case aidpdStateSendHardReset:
		default:
		{
			break;
		}
	}
	return true;
}


void aidpdResetDevice(void)
{
	setAidPDState(aidpdStateSendHardReset);
}


/**
  * @brief  This function is called to add or replace a capability.
  * @param  capability - PDO
  * @param  position - 0 to max - 1 replace, -1, append
  * @retval true if operation successful
  */
bool aidpdSetSourceCapability(uint32_t capability, int8_t position)
{
	bool ret = false;
	if (position < 0)
		position = numValidAccSourceCapability;
	if (position < sizeof(accSourceCapability))
	{
		if (position >= numValidAccSourceCapability)
			numValidAccSourceCapability = position + 1;
		accSourceCapability[position] = capability;
		ret = true;
	}
	return ret;
}


/**
  * @brief  This function is called to set the sink's requirements.
  * @param  capability - Req
  * @retval None
  */
bool aidpdSetSinkCapability(uint32_t capability)
{
	accSinkNeed = capability;
	return true;
}


void aidpdInit(void)
{
	aidpdState = aidpdStateReset;
	aidpdPwrState = aidpdPwrStateNone;
	bspSinkEnable(false);
	TIMER_CLEAR(timer);
#ifdef POLL_SOURCE_CAP
	TIMER_SET(reqTimer, AIDPD_REQ_INTERVAL_MS);
#endif
  	epPDInit();
}
