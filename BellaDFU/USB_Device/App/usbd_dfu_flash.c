/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_dfu_flash.c
  * @brief          : Usb device for Download Firmware Update.
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

/* Includes ------------------------------------------------------------------*/
#include "usbd_dfu_flash.h"


#define FLASH_DESC_STR      "@Internal Flash   /0x08000000/10*002Ka,54*002Kg"
#define FLASH_PROGRAM_TIME 	50
#define FLASH_ERASE_TIME 	50


extern USBD_HandleTypeDef hUsbDeviceFS;


static uint16_t FLASH_If_Init(void);
static uint16_t FLASH_If_Erase(uint32_t Add);
static uint16_t FLASH_If_Write(uint8_t *src, uint8_t *dest, uint32_t Len);
static uint8_t *FLASH_If_Read(uint8_t *src, uint8_t *dest, uint32_t Len);
static uint16_t FLASH_If_DeInit(void);
static uint16_t FLASH_If_GetStatus(uint32_t Add, uint8_t Cmd, uint8_t *buffer);


#if defined ( __ICCARM__ ) /* IAR Compiler */
  #pragma data_alignment=4
#endif
__ALIGN_BEGIN USBD_DFU_MediaTypeDef USBD_DFU_Flash_fops __ALIGN_END =
{
   (uint8_t*)FLASH_DESC_STR,
    FLASH_If_Init,
    FLASH_If_DeInit,
    FLASH_If_Erase,
    FLASH_If_Write,
    FLASH_If_Read,
    FLASH_If_GetStatus
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Memory initialization routine.
  * @retval USBD_OK if operation is successful, MAL_FAIL else.
  */
uint16_t FLASH_If_Init(void)
{
	HAL_FLASH_Unlock();
	return (USBD_OK);
}

/**
  * @brief  De-Initializes Memory
  * @retval USBD_OK if operation is successful, MAL_FAIL else
  */
uint16_t FLASH_If_DeInit(void)
{
	HAL_FLASH_Lock();
	return (USBD_OK);
}

/**
  * @brief  Erase sector.
  * @param  Add: Address of sector to be erased.
  * @retval 0 if operation is successful, MAL_FAIL else.
  */
uint16_t FLASH_If_Erase(uint32_t Add)
{
	uint32_t PageError;
	HAL_StatusTypeDef status;
	FLASH_EraseInitTypeDef eraseinitstruct;

	eraseinitstruct.TypeErase 	= FLASH_TYPEERASE_PAGES;
	eraseinitstruct.Banks 		= FLASH_BANK_1;
	eraseinitstruct.Page 		= (Add - FLASH_BASE)/FLASH_PAGE_SIZE;
	eraseinitstruct.NbPages 	= 1U;

	status = HAL_FLASHEx_Erase(&eraseinitstruct, &PageError);
	if (status != HAL_OK) { return (USBD_FAIL); }
	return (USBD_OK);
}

/**
  * @brief  Memory write routine.
  * @param  src: Pointer to the source buffer. Address to be written to.
  * @param  dest: Pointer to the destination buffer.
  * @param  Len: Number of data to be written (in bytes).
  * @retval USBD_OK if operation is successful, MAL_FAIL else.
  */
uint16_t FLASH_If_Write(uint8_t *src, uint8_t *dest, uint32_t Len)
{
	uint32_t idx = 0;

	if (Len & 0x7)	//not an aligned data
	{
		for (idx=Len; idx < (Len & 0xFFF8)+8; idx++)
		{
			*(uint8_t *)(src+idx) = 0xFF;
		}
	}

	for (idx=0;idx<Len;idx+=8)
	{
		uint32_t add = (uint32_t)(dest+idx);
		uint64_t data = *(uint64_t *)(src+idx);

		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, add, data) == HAL_OK)
		{
			uint64_t v1 = data;
			uint64_t v2 = *(uint64_t *)add;
			if (v1 != v2)
			{
				return (USBD_FAIL);
			}
		}
		else
		{
			return (USBD_FAIL);
		}
	}
	return (USBD_OK);
}

/**
  * @brief  Memory read routine.
  * @param  src: Pointer to the source buffer. Address to be written to.
  * @param  dest: Pointer to the destination buffer.
  * @param  Len: Number of data to be read (in bytes).
  * @retval Pointer to the physical address where data should be read.
  */
uint8_t *FLASH_If_Read(uint8_t *src, uint8_t *dest, uint32_t Len)
{
	for (uint32_t i=0;i<Len;i++) { dest[i] = src[i]; }

	return (uint8_t *)(dest);
}

/**
  * @brief  Get status routine
  * @param  Add: Address to be read from
  * @param  Cmd: Number of data to be read (in bytes)
  * @param  buffer: used for returning the time necessary for a program or an erase operation
  * @retval USBD_OK if operation is successful
  */
uint16_t FLASH_If_GetStatus(uint32_t Add, uint8_t Cmd, uint8_t *buffer)
{
	  switch (Cmd)
	  {
	    case DFU_MEDIA_PROGRAM:
	    	buffer[1] = (uint8_t)FLASH_PROGRAM_TIME;
	    	buffer[2] = (uint8_t)(FLASH_PROGRAM_TIME<<8);
	    	buffer[3] = 0;
	    break;

	    case DFU_MEDIA_ERASE:
	    default:
	    	buffer[1] = (uint8_t)FLASH_ERASE_TIME;
	    	buffer[2] = (uint8_t)(FLASH_ERASE_TIME<<8);
	    	buffer[3] = 0;
	    break;
	  }
	  return (USBD_OK);
}


/**
  * @}
  */

/**
  * @}
  */
