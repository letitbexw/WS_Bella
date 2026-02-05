/*
 * test.c
 *
 *  Created on: Jan 11, 2026
 *      Author: xiongwei
 */


#include "config.h"
#include "bsp.h"
#include "debug_uart.h"
#include "sn.h"
#include "timers.h"
#include "adc.h"
#include "i2c.h"
#include "orion.h"


#define TEST_HARNESS_MAX_CMD_SIZE       28
#define TEST_HARNESS_MAX_RESPONSE_SIZE  200


enum {
	CMD_VERSION = 0,
	CMD_RESET,
	CMD_ECHO,
	CMD_MLB_SER_NUM,
	CMD_LOCK_SWD,
	CMD_VI,
	CMD_SW,
	CMD_IN,
	CMD_CTRL,
	CMD_ORI,
	CMD_LAST,
	CMD_NO_MATCH = 255,
};

typedef struct {
	uint8_t 	cmdIdx;
	const char* cmdStr;
	uint8_t 	cmdStrMaxLen;
	const char* retStr;
} TestCommandTable;

static const TestCommandTable testCommandTable[] = {
	[0]={ .cmdIdx=CMD_VERSION, 		"VER", 		.cmdStrMaxLen=3, 	"<Bella-0.0.0-0>" 	},
	[1]={ .cmdIdx=CMD_RESET, 		"RST", 		.cmdStrMaxLen=3, 	"" 					},
	[2]={ .cmdIdx=CMD_ECHO, 		"ECHO-", 	.cmdStrMaxLen=5, 	"<echo-0>" 			},
	[3]={ .cmdIdx=CMD_MLB_SER_NUM, 	"MSN", 		.cmdStrMaxLen=3, 	"<msn-" 			},
	[4]={ .cmdIdx=CMD_LOCK_SWD, 	"LKS",		.cmdStrMaxLen=3, 	"<lks-0000-0" 		},
	[5]={ .cmdIdx=CMD_VI, 			"VI",		.cmdStrMaxLen=2, 	"" 					},
	[6]={ .cmdIdx=CMD_SW, 			"SW",		.cmdStrMaxLen=7, 	"" 					},
	[7]={ .cmdIdx=CMD_IN, 			"IN",		.cmdStrMaxLen=2, 	"" 					},
	[8]={ .cmdIdx=CMD_CTRL, 		"CTRL",		.cmdStrMaxLen=14, 	"" 					},
	[9]={ .cmdIdx=CMD_ORI, 			"ORI",		.cmdStrMaxLen=3, 	"" 					},
	[10]={ .cmdIdx=CMD_LAST, 		"", 		.cmdStrMaxLen=0, 	"" 					},
};

static const char respErrorStr[] = "<ERROR>";

static bool echoOn;
static uint8_t rxState;

static char testCommand[TEST_HARNESS_MAX_CMD_SIZE];
static char testResponse[TEST_HARNESS_MAX_RESPONSE_SIZE];


static uint8_t assembleCommand(void)
{
	// Get command string within [ and ]
	static char* rxCmdPtr;
	uint8_t c;

	while (uartDebugRxByte(&uartRxFifo, &c) != 0)
	{
		if (echoOn) { uartDebugTxByte(&uartTxFifo, c); }

		switch (rxState)
		{
			case 255:
				if (c == '[')
				{
					rxState = TEST_HARNESS_MAX_CMD_SIZE - 1;
					rxCmdPtr = testCommand;
				}
				break;

			default:
				rxState--;
				if (c == ']')
				{
					*rxCmdPtr = 0;
					rxState = 255;
					return 1;		// Command detected...
				}
				else { *rxCmdPtr++ = c; }
				break;
		}
	}
	return 0;
}

static uint8_t findCommand(void)
{
	uint8_t idx = 0;

	while(1)
	{
		const char *cmdStr = testCommandTable[idx].cmdStr;
		uint8_t cmdLen = testCommandTable[idx].cmdStrMaxLen;

		if (cmdStr == NULL) { return CMD_NO_MATCH; }
		if (strncmp(testCommand, cmdStr, strlen(cmdStr)) == 0)
		{
			if (strlen(testCommand) > cmdLen) { return CMD_NO_MATCH; }   // NOTE trailing excess ignored, not error
			strncpy(testResponse, testCommandTable[idx].retStr, sizeof(testResponse));
			return testCommandTable[idx].cmdIdx;
		}
		idx++;
	}
}


static void Selftest(void)
{
	HAL_GPIO_WritePin(DEBUG_OUT, GPIO_PIN_SET);
	tmrDelay_us(10);
	HAL_GPIO_WritePin(DEBUG_OUT, GPIO_PIN_RESET);
	tmrDelay_us(20);
	HAL_GPIO_WritePin(DEBUG_OUT, GPIO_PIN_SET);
}


void testInit(void)
{
	echoOn = true;
	rxState = 255;

	Selftest();
	printf("\n%s %s-%d.%d.%d-%d.%s\n",CNFG_ACCESSORY_MODEL_NUMBER, CNFG_ACCESSORY_MODEL_NUMBER, CNFG_FW_VERSION_MAJ, CNFG_FW_VERSION_MIN, CNFG_FW_REVISION, hwVersion, CNFG_BUILD_INFO);
}

void testHarnessService(void)
{
	bool err;

	if (bspReadoutProtectionIsSet()) { return; }

	while (assembleCommand())
	{
		uint8_t cmd = findCommand();
		uint8_t cmdlen = strlen(testCommand);

		err = false;
		switch (cmd)
		{
			case CMD_VERSION:
				testResponse[7] = '0' + CNFG_FW_VERSION_MAJ;
				testResponse[9] = '0' + CNFG_FW_VERSION_MIN;
				testResponse[11] = '0' + CNFG_FW_REVISION;
				testResponse[13] = 'A' + CNFG_HW_REVISION - 0;
				break;

			case CMD_RESET:
				sprintf(&testResponse[0],"Software reset -->");
				break;

			case CMD_ECHO:
				if (testCommand[5] == '0') { echoOn = false; }
				else if (testCommand[5] == '1') { echoOn = true; }
				testResponse[6] = echoOn ? '1' : '0';
				break;

			case CMD_MLB_SER_NUM:
			{
				uint8_t sn[PRODUCT_MLB_SER_NUM_SZ + 1];
				uint16_t length = 0;

				memcpy(sn, getModuleSN(&length), length);
				*(sn + length) = 0;

				testResponse[0] = '\0';

				if (strlen(testCommand) == strlen("MSN"))
				{
					if ((*sn == '\0') || (length == 0))	{ strcpy(testResponse, "<msn-**EMPTY**>"); }	// MLB SN not found
					else								{ sprintf(testResponse, "<msn-%s>", sn); }
				}
				else if (strlen(testCommand) == (PRODUCT_MLB_SER_NUM_SZ + 4))	// "MLB XXXXXXXX"
				{
					if (bspReadoutProtectionIsSet()) { err = true; break; }

					memcpy(sn, &testCommand[4], PRODUCT_MLB_SER_NUM_SZ);
					*(sn + PRODUCT_MLB_SER_NUM_SZ) = 0;
					if (setModuleSN(sn)) { sprintf(testResponse, "<msn-%s>", sn); }
					else 				 { strcpy(testResponse, "<msn-FAIL>"); }
				}
				else
				{
					strcpy(testResponse, "<msn-ERROR>");
				}
				break;
			}

			case CMD_LOCK_SWD:
				break;

			case CMD_VI:
				sprintf(&testResponse[0],
						"VBUS0 \t= %d mV\nVB0_GO \t= %d mV\nIBUS0 \t= %d mA\nVORION \t= %d mV",
						(int)ReadAdcVBUS(PORT0),
						ReadInaVoltage(),
						ReadInaCurrent(),
						(int)ReadAdcVBUS(ORION));
				break;

			case CMD_SW:
				if (cmdlen == 7)
				{
					if (testCommand[3] == '1') HAL_GPIO_WritePin(PSEN_P0, GPIO_PIN_SET);
					else if (testCommand[3] == '0') HAL_GPIO_WritePin(PSEN_P0, GPIO_PIN_RESET);
					if (testCommand[4] == '1') HAL_GPIO_WritePin(PSEN_ORION, GPIO_PIN_SET);
					else if (testCommand[4] == '0') HAL_GPIO_WritePin(PSEN_ORION, GPIO_PIN_RESET);
					if (testCommand[5] == '1') HAL_GPIO_WritePin(USBC_HPEN, GPIO_PIN_SET);
					else if (testCommand[5] == '0') HAL_GPIO_WritePin(USBC_HPEN, GPIO_PIN_RESET);
					if (testCommand[6] == '1') HAL_GPIO_WritePin(USBC_LPEN, GPIO_PIN_SET);
					else if (testCommand[6] == '0') HAL_GPIO_WritePin(USBC_LPEN, GPIO_PIN_RESET);
				}
				sprintf(&testResponse[0],
						"PSEN_P0 \t= %d\nPSEN_ORION \t= %d\nUSBC_HPEN \t= %d\nUSBC_LPEN \t= %d",
						HAL_GPIO_ReadPin(PSEN_P0),
						HAL_GPIO_ReadPin(PSEN_ORION),
						HAL_GPIO_ReadPin(USBC_HPEN),
						HAL_GPIO_ReadPin(USBC_LPEN));
				break;

			case CMD_IN:
				sprintf(&testResponse[0],"USBC_LPOC \t= %d\nREMOVAL_DET_L \t= %d",
						HAL_GPIO_ReadPin(USBC_LPOC),
						HAL_GPIO_ReadPin(REMOVAL_DET_L));
				break;

			case CMD_CTRL:
				if (cmdlen == 14)
				{
					if (testCommand[5] == '1') HAL_GPIO_WritePin(AID_PU_EN_L, GPIO_PIN_SET);
					else if (testCommand[5] == '0') HAL_GPIO_WritePin(AID_PU_EN_L, GPIO_PIN_RESET);
					if (testCommand[6] == '1') HAL_GPIO_WritePin(AID_PD_EN, GPIO_PIN_SET);
					else if (testCommand[6] == '0') HAL_GPIO_WritePin(AID_PD_EN, GPIO_PIN_RESET);
					if (testCommand[7] == '1') HAL_GPIO_WritePin(LED_RED, GPIO_PIN_SET);
					else if (testCommand[7] == '0') HAL_GPIO_WritePin(LED_RED, GPIO_PIN_RESET);
					if (testCommand[8] == '1') HAL_GPIO_WritePin(LED_GRN, GPIO_PIN_SET);
					else if (testCommand[8] == '0') HAL_GPIO_WritePin(LED_GRN, GPIO_PIN_RESET);
					if (testCommand[9] == '1') HAL_GPIO_WritePin(LED_BLU, GPIO_PIN_SET);
					else if (testCommand[9] == '0') HAL_GPIO_WritePin(LED_BLU, GPIO_PIN_RESET);
					if (testCommand[10] == '1') HAL_GPIO_WritePin(INA_EN, GPIO_PIN_SET);
					else if (testCommand[10] == '0') HAL_GPIO_WritePin(INA_EN, GPIO_PIN_RESET);
					if (testCommand[11] == '1') HAL_GPIO_WritePin(DISCH_ORION, GPIO_PIN_SET);
					else if (testCommand[11] == '0') HAL_GPIO_WritePin(DISCH_ORION, GPIO_PIN_RESET);
					if (testCommand[12] == '1') HAL_GPIO_WritePin(MAGIC_PD_DIS, GPIO_PIN_SET);
					else if (testCommand[12] == '0') HAL_GPIO_WritePin(MAGIC_PD_DIS, GPIO_PIN_RESET);
					if (testCommand[13] == '1') HAL_GPIO_WritePin(ORION_DATA_ENABLE, GPIO_PIN_SET);
					else if (testCommand[13] == '0') HAL_GPIO_WritePin(ORION_DATA_ENABLE, GPIO_PIN_RESET);
				}
				sprintf(&testResponse[0],
						"AID_PU_EN_L \t= %d\nAID_PD_EN \t= %d\nLED_RGB \t= %d%d%d\nINA_EN \t\t= %d\nDISCH_ORION \t= %d\nMAGIC_PD_DIS \t= %d\nORION_DATA_ENABLE = %d",
						HAL_GPIO_ReadPin(AID_PU_EN_L),
						HAL_GPIO_ReadPin(AID_PD_EN),
						HAL_GPIO_ReadPin(LED_RED),
						HAL_GPIO_ReadPin(LED_GRN),
						HAL_GPIO_ReadPin(LED_BLU),
						HAL_GPIO_ReadPin(INA_EN),
						HAL_GPIO_ReadPin(DISCH_ORION),
						HAL_GPIO_ReadPin(MAGIC_PD_DIS),
						HAL_GPIO_ReadPin(ORION_DATA_ENABLE));
				break;

			case CMD_ORI:
				if (getOrionDataAboveRMThreshold()) 		{ sprintf(&testResponse[0], "data > 2.4V\nVORION = %d mV", (int)ReadAdcVBUS(ORION)); }
				else {
					if (getOrionDataAboveRxThreshold()) 	{ sprintf(&testResponse[0], "1.5V < data < 2.4V\nVORION = %d mV", (int)ReadAdcVBUS(ORION)); }
					else 									{ sprintf(&testResponse[0], "data < 1.5V\nVORION = %d mV", (int)ReadAdcVBUS(ORION)); }
				}
				break;

			default:
				err = true;
				break;
		}

		if (err) { strncpy(testResponse, respErrorStr, sizeof(testResponse)); }

		if (echoOn) { printf("\n"); }
		printf("%s\n", testResponse);
		if (cmd == CMD_RESET) { HAL_Delay(100); HAL_NVIC_SystemReset(); }
	}
}



