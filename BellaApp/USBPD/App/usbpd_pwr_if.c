/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usbpd_pwr_if.c
  * @author  MCD Application Team
  * @brief   This file contains power interface control functions.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#define __USBPD_PWR_IF_C

/* Includes ------------------------------------------------------------------*/
#include "usbpd_pwr_if.h"
#include "usbpd_hw_if.h"
#include "usbpd_dpm_core.h"
#include "usbpd_dpm_conf.h"
#include "usbpd_pdo_defs.h"
#include "usbpd_core.h"
#if defined(_TRACE)
#include "usbpd_trace.h"
#endif /* _TRACE */
#include "string.h"
#include "adc.h"
#include "ina219.h"


/* Private macros ------------------------------------------------------------*/
/** @addtogroup STM32_USBPD_APPLICATION_POWER_IF_Private_Macros
  * @{
  */
#if defined(_TRACE)
#define PWR_IF_DEBUG_TRACE(_PORT_, __MESSAGE__)  USBPD_TRACE_Add(USBPD_TRACE_DEBUG, (_PORT_), 0u, (uint8_t*)(__MESSAGE__), sizeof(__MESSAGE__) - 1u)
#else
#define PWR_IF_DEBUG_TRACE(_PORT_, __MESSAGE__)
#endif /* _TRACE */


USBPD_PWR_Port_PDO_Storage_TypeDef PWR_Port_PDO_Storage[USBPD_PORT_COUNT];


USBPD_StatusTypeDef USBPD_PWR_IF_Init(void)
{
	PWR_Port_PDO_Storage[USBPD_PORT_0].SinkPDO.ListOfPDO   = (uint32_t *) PORT0_PDO_ListSNK;
	PWR_Port_PDO_Storage[USBPD_PORT_0].SinkPDO.NumberOfPDO = &USBPD_NbPDO[0];
	return USBPD_OK;
}

/**
  * @brief  Checks if the power on a specified port is ready
  * @param  PortNum Port number
  * @param  Vsafe   Vsafe status based on @ref USBPD_VSAFE_StatusTypeDef
  * @retval USBPD status
  */
USBPD_StatusTypeDef USBPD_PWR_IF_SupplyReady(uint8_t PortNum, USBPD_VSAFE_StatusTypeDef Vsafe)
{
	USBPD_StatusTypeDef status = USBPD_ERROR;
	uint32_t _voltage;

	/* check for valid port */
	if (!USBPD_PORT_IsValid(PortNum)) { return USBPD_ERROR; }

	_voltage = ReadAdcVBUS(PortNum);
//	BSP_USBPD_PWR_VBUSGetVoltage(PortNum, &_voltage);
	if (USBPD_VSAFE_0V == Vsafe) 	{ status = ((_voltage < USBPD_PWR_LOW_VBUS_THRESHOLD) ? USBPD_OK : USBPD_ERROR); }	/* Vsafe0V */
	else							{ status = ((_voltage > USBPD_PWR_HIGH_VBUS_THRESHOLD) ? USBPD_OK : USBPD_ERROR);}	/* Vsafe5V */

	return status;
}

/**
  * @brief  Reads the voltage and the current on a specified port
  * @param  PortNum Port number
  * @param  pVoltage: The Voltage in mV
  * @param  pCurrent: The Current in mA
  * @retval USBPD_ERROR or USBPD_OK
  */
USBPD_StatusTypeDef USBPD_PWR_IF_ReadVA(uint8_t PortNum, uint16_t *pVoltage, uint16_t *pCurrent)
{
	// static uint16_t vol, cur;
	// vol = (uint16_t)ReadAdcVBUS(PortNum);
	// cur = (uint16_t)ReadInaCurrent();
	// pVoltage = &vol;
	// pCurrent = &cur;
	// return USBPD_OK;
  return USBPD_ERROR;
}

/**
  * @brief  Enables the VConn on the port.
  * @param  PortNum Port number
  * @param  CC      Specifies the CCx to be selected based on @ref CCxPin_TypeDef structure
  * @retval USBPD status
  */
USBPD_StatusTypeDef USBPD_PWR_IF_Enable_VConn(uint8_t PortNum, CCxPin_TypeDef CC)
{
	return USBPD_ERROR;
}

/**
  * @brief  Disable the VConn on the port.
  * @param  PortNum Port number
  * @param  CC      Specifies the CCx to be selected based on @ref CCxPin_TypeDef structure
  * @retval USBPD status
  */
USBPD_StatusTypeDef USBPD_PWR_IF_Disable_VConn(uint8_t PortNum, CCxPin_TypeDef CC)
{
	return USBPD_ERROR;
}

/**
  * @brief  Allow PDO data reading from PWR_IF storage.
  * @param  PortNum Port number
  * @param  DataId Type of data to be read from PWR_IF
  *         This parameter can be one of the following values:
  *           @arg @ref USBPD_CORE_DATATYPE_SRC_PDO Source PDO reading requested
  *           @arg @ref USBPD_CORE_DATATYPE_SNK_PDO Sink PDO reading requested
  * @param  Ptr Pointer on address where PDO values should be written (u8 pointer)
  * @param  Size Pointer on nb of u32 written by PWR_IF (nb of PDOs)
  * @retval None
  */
void USBPD_PWR_IF_GetPortPDOs(uint8_t PortNum, USBPD_CORE_DataInfoType_TypeDef DataId, uint8_t *Ptr, uint32_t *Size)
{
	if (USBPD_PORT_0 == PortNum)
    {
		if (DataId == USBPD_CORE_DATATYPE_SRC_PDO)
		{

		}
		else
		{
			*Size = PORT0_NB_SINKPDO;
			memcpy(Ptr,PORT0_PDO_ListSNK, sizeof(uint32_t) * PORT0_NB_SINKPDO);
		}
    }
}

/**
  * @brief  Find out SRC PDO pointed out by a position provided in a Request DO (from Sink).
  * @param  PortNum Port number
  * @param  RdoPosition RDO Position in list of provided PDO
  * @param  Pdo Pointer on PDO value pointed out by RDO position (u32 pointer)
  * @retval Status of search
  *         USBPD_OK : Src PDO found for requested DO position (output Pdo parameter is set)
  *         USBPD_FAIL : Position is not compliant with current Src PDO for this port (no corresponding PDO value)
  */
USBPD_StatusTypeDef USBPD_PWR_IF_SearchRequestedPDO(uint8_t PortNum, uint32_t RdoPosition, uint32_t *Pdo)
{
	return USBPD_FAIL;
}

/**
  * @brief  Function called in case of critical issue is detected to switch in safety mode.
  * @param  ErrorType Type of error detected by monitoring (based on @ref USBPD_PWR_IF_ERROR)
  * @retval None
  */
void USBPD_PWR_IF_AlarmType(USBPD_PWR_IF_ERROR ErrorType) {}

/**
  * @brief  the function is called in case of critical issue is detected to switch in safety mode.
  * @retval None
  */
void USBPD_PWR_IF_Alarm() {}

/**
  * @brief Function is called to get VBUS power status.
  * @param PortNum Port number
  * @param PowerTypeStatus  Power type status based on @ref USBPD_VBUSPOWER_STATUS
  * @retval UBBPD_TRUE or USBPD_FALSE
  */
uint8_t USBPD_PWR_IF_GetVBUSStatus(uint8_t PortNum, USBPD_VBUSPOWER_STATUS PowerTypeStatus)
{
	uint8_t _status = USBPD_FALSE;
	uint32_t _vbus = HW_IF_PWR_GetVoltage(PortNum);

	switch(PowerTypeStatus)
	{
		case USBPD_PWR_BELOWVSAFE0V :
			if (_vbus < USBPD_PWR_LOW_VBUS_THRESHOLD) _status = USBPD_TRUE;
			break;
		case USBPD_PWR_VSAFE5V :
			if (_vbus >= USBPD_PWR_HIGH_VBUS_THRESHOLD) _status = USBPD_TRUE;
			break;
		case USBPD_PWR_SNKDETACH:
			if (_vbus < USBPD_PWR_HIGH_VBUS_THRESHOLD) _status = USBPD_TRUE;
			break;
		default :
			break;
	}
	return _status;
}

/**
  * @brief Function is called to set the VBUS threshold when a request has been accepted.
  * @param PortNum Port number
  * @retval None
  */
void USBPD_PWR_IF_UpdateVbusThreshold(uint8_t PortNum) {}

/**
  * @brief Function is called to reset the VBUS threshold when there is a power reset.
  * @param PortNum Port number
  * @retval None
  */
void USBPD_PWR_IF_ResetVbusThreshold(uint8_t PortNum) {}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
