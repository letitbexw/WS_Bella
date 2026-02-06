
/**
  ******************************************************************************
  * @file    usbpd_pdo_defs.h
  * @author  MCD Application Team
  * @brief   Header file for definition of PDO/APDO values for 2 ports(DRP/SNK) configuration
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


#ifndef __USBPD_PDO_DEF_H_
#define __USBPD_PDO_DEF_H_

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbpd_def.h"
//#include "usbpd_pwr_if.h"

/* Define   ------------------------------------------------------------------*/
#define PORT0_NB_SOURCEPDO         0U   /* Number of Source PDOs (applicable for port 0)   */
#define PORT0_NB_SINKPDO           4U   /* Number of Sink PDOs (applicable for port 0)     */

/* Exported typedef ----------------------------------------------------------*/
typedef struct
{
	uint32_t *ListOfPDO;        /*!< Pointer on Power Data Objects list, defining port capabilities */
	uint8_t  *NumberOfPDO;  	/*!< Number of Power Data Objects defined in ListOfPDO. This parameter must be set at max to @ref USBPD_MAX_NB_PDO value */
} USBPD_PortPDO_TypeDef;

typedef struct
{
	USBPD_PortPDO_TypeDef SinkPDO;
	USBPD_PortPDO_TypeDef SrcPDO;
} USBPD_PWR_Port_PDO_Storage_TypeDef;

/* Exported define -----------------------------------------------------------*/
#define USBPD_CORE_PDO_SRC_FIXED_MAX_CURRENT 3
#define USBPD_CORE_PDO_SNK_FIXED_MAX_CURRENT 1500


#ifdef __USBPD_PWR_IF_C
uint8_t USBPD_NbPDO[2] = { (PORT0_NB_SINKPDO), (PORT0_NB_SOURCEPDO) };
#endif

#ifndef __USBPD_PWR_IF_C
extern uint32_t PORT0_PDO_ListSRC[USBPD_MAX_NB_PDO];
extern uint32_t PORT0_PDO_ListSNK[USBPD_MAX_NB_PDO];
#else
/* Definition of Source PDO for Port 0 */
uint32_t PORT0_PDO_ListSRC[USBPD_MAX_NB_PDO] =
{
	/* PDO 1 */ (0x00000000U),
	/* PDO 2 */ (0x00000000U),
	/* PDO 3 */ (0x00000000U),
	/* PDO 4 */ (0x00000000U),
	/* PDO 5 */ (0x00000000U),
	/* PDO 6 */ (0x00000000U),
	/* PDO 7 */ (0x00000000U),
};

/* Definition of Sink PDO for Port 0 */
uint32_t PORT0_PDO_ListSNK[USBPD_MAX_NB_PDO] =
{
	/* PDO 1 */
	(
		USBPD_PDO_TYPE_FIXED                 			| 	/* Fixed supply PDO            	*/
		USBPD_PDO_SNK_FIXED_SET_VOLTAGE(5000U)      	| 	/* Voltage in mV               	*/
		USBPD_PDO_SNK_FIXED_SET_OP_CURRENT(1000U)   	| 	/* Operating current in  mA  	*/
		/* Common definitions applicable to all PDOs, defined only in PDO 1 */
		USBPD_PDO_SNK_FIXED_FRS_NOT_SUPPORTED          	| 	/* Fast Role Swap				*/
		USBPD_PDO_SNK_FIXED_DRD_NOT_SUPPORTED          	| 	/* Dual-Role Data              	*/
		USBPD_PDO_SNK_FIXED_USBCOMM_NOT_SUPPORTED      	| 	/* USB Communications          	*/
		USBPD_PDO_SNK_FIXED_EXT_POWER_NOT_AVAILABLE    	| 	/* External Power              	*/
		USBPD_PDO_SNK_FIXED_HIGHERCAPAB_NOT_SUPPORTED   | 	/* Higher Capability           	*/
		USBPD_PDO_SNK_FIXED_DRP_NOT_SUPPORTED            	/* Dual-Role Power             	*/
	),
	/* PDO 2 */
	(
		USBPD_PDO_TYPE_FIXED                        	| 	/* Fixed supply                	*/
		USBPD_PDO_SNK_FIXED_SET_VOLTAGE(9000U)         	| 	/* Voltage in mV               	*/
		USBPD_PDO_SNK_FIXED_SET_OP_CURRENT(1000U)       	/* Operating current in  mA    	*/
	),
	/* PDO 3 */
	(
		USBPD_PDO_TYPE_FIXED                        	| 	/* Fixed supply                	*/
		USBPD_PDO_SNK_FIXED_SET_VOLTAGE(15000U)         | 	/* Voltage in mV               	*/
		USBPD_PDO_SNK_FIXED_SET_OP_CURRENT(1000U)       	/* Operating current in  mA  	*/
	),
	/* PDO 4 */
	(
		USBPD_PDO_TYPE_FIXED                        	| 	/* Fixed supply                	*/
		USBPD_PDO_SNK_FIXED_SET_VOLTAGE(20000U)         | 	/* Voltage in mV               	*/
		USBPD_PDO_SNK_FIXED_SET_OP_CURRENT(1000U)       	/* Operating current in  mA     */
	),
	/* PDO 5 */ (0x00000000U),
	/* PDO 6 */ (0x00000000U),
	/* PDO 7 */ (0x00000000U),
};
#endif

/* Exported functions --------------------------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif /* __USBPD_PDO_DEF_H_ */
