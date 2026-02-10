/*
 * fwup_config.h
 *
 *  Created on: Feb 10, 2026
 *      Author: xiongwei
 */

#ifndef COMMON_FWUP_CONFIG_H_
#define COMMON_FWUP_CONFIG_H_


///////////////////////////////////////////////////////
// Flash Program Memory

#define CNFG_FLASH_BASE                 	0x08000000
#define CNFG_FLASH_SIZE                  	0x00010000

#define CNFG_DFU_BASE_ADDRESS            	CNFG_FLASH_BASE
// CNFG_APP_BASE_ADDRESS defined in Makefile

#define FLASH_PAGE_SIZE_BYTES            	(32 * 4)

///////////////////////////////////////////////////////
// Data EEPROM Memory

#define EEPROM_START_ADDRESS              	((uint32_t)0x08080000)
#define EEPROM_SIZE                       	((uint32_t)0x00000800)     // 2kB

#define DATA_EEPROM_MLB_SER_NUM           	EEPROM_START_ADDRESS
#define PRODUCT_MLB_SER_NUM_SZ              17    // Product MLB Serial Number Length
#define PRODUCT_FG_SER_NUM_SZ               0     // Product FG Serial Number Length (null)

#define DATA_EEPROM_ID_OFFSET             	0x20
#define DATA_EEPROM_ID_START              	(EEPROM_START_ADDRESS + DATA_EEPROM_ID_OFFSET)        // 0x08080020 - 0x0808002F
#define DATA_EEPROM_ID_IAP_OPT            	DATA_EEPROM_ID_START
#define DATA_EEPROM_HID_SIZE_0            	(DATA_EEPROM_ID_START + 1)

#define DATA_EEPROM_ROLLBACK_OFFSET       	0x30
#define DATA_EEPROM_ROLLBACK_START        	(EEPROM_START_ADDRESS + DATA_EEPROM_ROLLBACK_OFFSET)  // 0x08080030 - 0x080800FB, 4 bytes/rollback

#define DATA_EEPROM_FORCE_BOOT_DFU        	(EEPROM_START_ADDRESS + FLASH_PAGE_SIZE_BYTES - 4)    // 0x080800FC
#define FORCE_BOOT_DFU_MAGIC              	0xAA55A55A

#define DATA_EEPROM_HID_OFFSET            	0x100
#define DATA_EEPROM_HID_START             	(EEPROM_START_ADDRESS + DATA_EEPROM_HID_OFFSET)       // 0x08080100 - 0x080807FF
#define DATA_EEPROM_HID_END               	(EEPROM_START_ADDRESS + EEPROM_SIZE)

///////////////////////////////////////////////////////
// IAUP identifiers

#define CNFG_iAUP_PRODUCT_ID_CODE         	0x0045       // defined code in iAUP spec (decimal 69)
#define CNFG_PUBLIC_IAP_PROTOCOL_APP      	"com.comany1.accessory.updater.app.69"
#define CNFG_PUBLIC_IAP_PROTOCOL_DFU      	"com.comany1.accessory.updater.dfu.69"

///////////////////////////////////////////////////////
// Bootloader options

#define CNFG_DFU_RESET_DELAY_MS        		250

#define CNFG_DFU_TEST_IMAGE_SHA
//#define CNFG_DFU_TEST_STORE_SHA
#define CNFG_USE_ROLLBACK

#define CNFG_VECTOR_OFFSET_ROLLBACK    		0x20
#define CNFG_VECTOR_OFFSET_VERSION     		0x1C

#define CNFG_MAX_ROLLBACKS               	((DATA_EEPROM_FORCE_BOOT_DFU - DATA_EEPROM_ROLLBACK_START) / 4)    // end of first page, less FORCE_BOOT_MAGIC



#endif /* COMMON_FWUP_CONFIG_H_ */
