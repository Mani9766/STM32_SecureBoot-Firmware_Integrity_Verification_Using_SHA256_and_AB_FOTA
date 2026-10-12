/*
 * flash_storage.c
 *
 *  Created on: Sep 15, 2026
 *      Author: Manisha Daigavhane
 */

#include "flash_storage.h"
#include "main.h"
#include <string.h>

HAL_StatusTypeDef FlashStorage_EraseSector(
    uint32_t sector,
    uint32_t voltage_range)
{
    FLASH_EraseInitTypeDef erase_init = {0};
    uint32_t sector_error = 0xFFFFFFFFU;
    HAL_StatusTypeDef status;
    HAL_StatusTypeDef lock_status;

    /* Protect bootloader sectors; runtime updates use sectors 2-6 */
    if ((sector < FLASH_SECTOR_2) ||
        (sector > FLASH_SECTOR_6))
    {
        return HAL_ERROR;
    }

    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.Sector = sector;
    erase_init.NbSectors = 1U;
    erase_init.VoltageRange = voltage_range;

    /* Check flash unlock result */
    status = HAL_FLASH_Unlock();

    if (status != HAL_OK)
    {
        return status;
    }

    /* Erase the requested sector */
    status = HAL_FLASHEx_Erase(&erase_init, &sector_error);

    /* Always attempt to lock flash after the erase attempt */
    lock_status = HAL_FLASH_Lock();

    /* Preserve the erase failure status */
    if (status != HAL_OK)
    {
        return status;
    }

    /* Check the reported sector error */
    if (sector_error != 0xFFFFFFFFU)
    {
        return HAL_ERROR;
    }

    /* Report a flash-lock failure if erasing succeeded */
    if (lock_status != HAL_OK)
    {
        return lock_status;
    }

    return HAL_OK;
}

HAL_StatusTypeDef FlashStorage_ProgramMetadata(
    uint32_t address,
    const firmware_metadata_t *metadata)
{
    uint32_t data;
    HAL_StatusTypeDef status;

    if (metadata == NULL)
    {
        return HAL_ERROR;
    }

    HAL_FLASH_Unlock();

    for (uint32_t offset = 0U;
         offset < sizeof(firmware_metadata_t);
         offset += sizeof(uint32_t))
    {
        memcpy(&data,
               ((const uint8_t *)metadata) + offset,
               sizeof(uint32_t));

        status = HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_WORD,
            address + offset,
            data);

        if (status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return status;
        }
    }

    HAL_FLASH_Lock();

    return HAL_OK;
}

HAL_StatusTypeDef FlashStorage_ReadMetadata(
    uint32_t address,
    firmware_metadata_t *metadata)
{
    if (metadata == NULL)
    {
        return HAL_ERROR;
    }

    memcpy(metadata,
           (const void *)address,
           sizeof(firmware_metadata_t));

    return HAL_OK;
}

HAL_StatusTypeDef FlashStorage_ProgramImage(
    uint32_t destination_address,
    uint32_t source_address,
    uint32_t image_size)
{
    uint32_t data;
    HAL_StatusTypeDef status;
    uint32_t destination_capacity;
    uint32_t programmed_size;

    if ((image_size == 0U) ||
        (destination_address == 0U) ||
        (source_address == 0U))
    {
        return HAL_ERROR;
    }

    /* Determine destination region capacity */
    if (destination_address == DOWNLOAD_START_ADDRESS)
    {
        destination_capacity = 0x10000U;  /* 64 KB staging */
    }
    else if ((destination_address == SLOT_A_IMAGE_START) ||
             (destination_address == SLOT_B_IMAGE_START))
    {
        destination_capacity = 0x20000U;  /* 128 KB slot */
    }
    else if ((destination_address == SLOT_A_METADATA_ADDRESS) ||
             (destination_address == SLOT_B_METADATA_ADDRESS))
    {
        destination_capacity = 0x4000U;  /* 16 KB metadata sector */
    }
    else
    {
        return HAL_ERROR;
    }

    /* Prevent overflow when rounding to 32-bit words as if image size is 61 bytes then actual image size becomes 64 bytes for 32 byte alignment*/
    if (image_size > (UINT32_MAX - 3U))
    {
        return HAL_ERROR;
    }

    /* Actual number of bytes programmed */
    programmed_size = (image_size + 3U) & ~0x03U;

    /* Reject writes exceeding destination capacity */
    if (programmed_size > destination_capacity)
    {
        return HAL_ERROR;
    }

    /* Staging source must also fit within 64 KB */
    if ((source_address == DOWNLOAD_START_ADDRESS) &&
        (image_size > 0x10000U))
    {
        return HAL_ERROR;
    }

    status = HAL_FLASH_Unlock();

    if (status != HAL_OK)
    {
        return status;
    }

    for (uint32_t offset = 0U;
         offset < image_size;
         offset += sizeof(uint32_t))
    {
        data = 0xFFFFFFFFU;

        uint32_t remaining = image_size - offset;
        uint32_t copy_size =
            (remaining >= sizeof(uint32_t)) ?
            sizeof(uint32_t) :
            remaining;

        memcpy(
            &data,
            (const void *)(source_address + offset),
            copy_size);

        status = HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_WORD,
            destination_address + offset,
            data);

        if (status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return status;
        }
    }

    status = HAL_FLASH_Lock();

    if (status != HAL_OK)
    {
        return status;
    }

    return HAL_OK;
}
