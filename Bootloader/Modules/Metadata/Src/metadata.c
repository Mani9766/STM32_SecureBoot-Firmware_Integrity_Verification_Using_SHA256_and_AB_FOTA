/*
 * metadata.c
 *
 *  Created on: Sep 12, 2026
 *      Author: Manisha Daigavhane
 */

#include "metadata.h"
#include "flash_storage.h"

#include <stddef.h>
#include <string.h>

extern CRC_HandleTypeDef hcrc;

bool Metadata_Validate(
    const firmware_metadata_t *metadata,
    uintptr_t image_start,
    uintptr_t image_region_end)
{
    uintptr_t image_end;

    if (metadata == NULL)
    {
        return false;
    }

    if (metadata->magic != FIRMWARE_METADATA_MAGIC)
    {
        return false;
    }

    if (metadata->image_size == 0U)
    {
        return false;
    }

    image_end =
        image_start + (uintptr_t)metadata->image_size;

    if (image_end < image_start)
    {
        return false;
    }

    if (image_end > image_region_end)
    {
        return false;
    }

    return true;
}

uint32_t Metadata_CalculateRecordCRC(
    const firmware_metadata_record_t *record)
{
    size_t crc_data_length;

    if (record == NULL)
    {
        return 0U;
    }

    /*
     * CRC covers sequence + firmware metadata.
     * metadata_crc and commit_marker are excluded.
     */
    crc_data_length =
        offsetof(
            firmware_metadata_record_t,
            metadata_crc);

    return HAL_CRC_Calculate(
        &hcrc,
        (uint32_t *)record,
        (uint32_t)(crc_data_length / sizeof(uint32_t)));
}

bool Metadata_ValidateRecord(
    const firmware_metadata_record_t *record)
{
    uint32_t calculated_crc;

    if (record == NULL)
    {
        return false;
    }

    /*
     * Commit marker is written last.
     */
    if (record->commit_marker != METADATA_COMMIT_MARKER)
    {
        return false;
    }

    calculated_crc =
        Metadata_CalculateRecordCRC(record);

    if (calculated_crc != record->metadata_crc)
    {
        return false;
    }

    return true;
}

bool Metadata_IsSequenceNewer(
    uint32_t current_sequence,
    uint32_t new_sequence)
{
    return ((int32_t)(new_sequence - current_sequence) > 0);
}

static bool Metadata_IsRecordErased(
    const firmware_metadata_record_t *record)
{
    const uint32_t *data;

    if (record == NULL)
    {
        return false;
    }

    data = (const uint32_t *)record;

    for (size_t i = 0U;
         i < (sizeof(firmware_metadata_record_t) /
              sizeof(uint32_t));
         i++)
    {
        if (data[i] != 0xFFFFFFFFU)
        {
            return false;
        }
    }

    return true;
}

static bool Metadata_FindNextRecordAddress(
    uintptr_t sector_start,
    uintptr_t sector_end,
    uintptr_t *record_address)
{
    firmware_metadata_record_t record;
    uintptr_t address;

    if ((record_address == NULL) ||
        (sector_start >= sector_end))
    {
        return false;
    }

    for (address = sector_start;
         (address + sizeof(firmware_metadata_record_t)) <=
         sector_end;
         address += sizeof(firmware_metadata_record_t))
    {
        record =
            *(const firmware_metadata_record_t *)address;

        if (Metadata_IsRecordErased(&record))
        {
            *record_address = address;
            return true;
        }
    }

    return false;
}

bool Metadata_ReadLatestRecord(
    uintptr_t sector_start,
    uintptr_t sector_end,
    firmware_metadata_record_t *latest_record)
{
    firmware_metadata_record_t record;
    bool found_valid_record = false;
    uintptr_t address;

    if ((latest_record == NULL) ||
        (sector_start >= sector_end))
    {
        return false;
    }

    for (address = sector_start;
         (address + sizeof(firmware_metadata_record_t)) <=
         sector_end;
         address += sizeof(firmware_metadata_record_t))
    {
        record =
            *(const firmware_metadata_record_t *)address;

        if (!Metadata_ValidateRecord(&record))
        {
            continue;
        }

        if (!found_valid_record)
        {
            *latest_record = record;
            found_valid_record = true;
            continue;
        }

        if (Metadata_IsSequenceNewer(
                latest_record->sequence,
                record.sequence))
        {
            *latest_record = record;
        }
    }

    return found_valid_record;
}

HAL_StatusTypeDef Metadata_WriteRecord(
    uintptr_t sector_start,
    uintptr_t sector_end,
    uint32_t flash_sector,
    uint32_t voltage_range,
    const firmware_metadata_t *metadata)
{
    firmware_metadata_record_t latest_record;
    firmware_metadata_record_t new_record;

    uintptr_t record_address;

    bool latest_record_found;
    bool sector_full = false;

    uint32_t next_sequence;
    uint32_t data;

    HAL_StatusTypeDef status;

    if ((metadata == NULL) ||
        (sector_start >= sector_end))
    {
        return HAL_ERROR;
    }

    /*
     * Find the latest valid metadata record.
     */
    latest_record_found =
        Metadata_ReadLatestRecord(
            sector_start,
            sector_end,
            &latest_record);

    if (latest_record_found)
    {
        next_sequence =
            latest_record.sequence + 1U;
    }
    else
    {
        /*
         * No valid metadata record exists.
         */
        next_sequence = 1U;
    }

    /*
     * Try to find an erased record location.
     */
    if (!Metadata_FindNextRecordAddress(
            sector_start,
            sector_end,
            &record_address))
    {
        sector_full = true;
    }

    /*
     * If the metadata sector is full, erase it and
     * restart the metadata journal from Record 0.
     *
     * Power-loss handling during this erase/rebuild
     * is outside the current project scope.
     */
    if (sector_full)
    {
        status =
            FlashStorage_EraseSector(
                flash_sector,
                voltage_range);

        if (status != HAL_OK)
        {
            return status;
        }

        /*
         * After erase, the first record is at
         * the beginning of the sector.
         */
        record_address = sector_start;

        /*
         * Preserve sequence continuity across the
         * sector rollover.
         */
        if (latest_record_found)
        {
            next_sequence =
                latest_record.sequence + 1U;
        }
        else
        {
            next_sequence = 1U;
        }
    }

    /*
     * Build new metadata record in RAM.
     */
    memset(
        &new_record,
        0xFF,
        sizeof(new_record));

    new_record.sequence = next_sequence;
    new_record.metadata = *metadata;

    /*
     * Calculate CRC over sequence + metadata.
     */
    new_record.metadata_crc =
        Metadata_CalculateRecordCRC(
            &new_record);

    /*
     * commit_marker remains erased.
     * It is programmed last.
     */

    status = HAL_FLASH_Unlock();

    if (status != HAL_OK)
    {
        return status;
    }

    /*
     * Program sequence + metadata + CRC.
     */
    for (uint32_t offset = 0U;
         offset < offsetof(
             firmware_metadata_record_t,
             commit_marker);
         offset += sizeof(uint32_t))
    {
        memcpy(
            &data,
            ((const uint8_t *)&new_record) + offset,
            sizeof(uint32_t));

        status = HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_WORD,
            (uint32_t)(record_address + offset),
            data);

        if (status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return status;
        }

        /*
         * Verify each programmed word.
         */
        if (*(volatile uint32_t *)
                (record_address + offset) != data)
        {
            HAL_FLASH_Lock();
            return HAL_ERROR;
        }
    }

    /*
     * Commit marker is programmed LAST.
     */
    status = HAL_FLASH_Program(
        FLASH_TYPEPROGRAM_WORD,
        (uint32_t)(
            record_address +
            offsetof(
                firmware_metadata_record_t,
                commit_marker)),
        METADATA_COMMIT_MARKER);

    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return status;
    }

    /*
     * Verify commit marker.
     */
    if (*(volatile uint32_t *)
            (record_address +
             offsetof(
                 firmware_metadata_record_t,
                 commit_marker))
        != METADATA_COMMIT_MARKER)
    {
        HAL_FLASH_Lock();
        return HAL_ERROR;
    }

    status = HAL_FLASH_Lock();

    if (status != HAL_OK)
    {
        return status;
    }

    return HAL_OK;
}
