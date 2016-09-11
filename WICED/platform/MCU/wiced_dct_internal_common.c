/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * Manages DCT writing to external flash
 */
#include "string.h"
#include "wwd_assert.h"
#include "wiced_result.h"
#include "spi_flash.h"
#include "platform_dct.h"
#include "waf_platform.h"
#include "wiced_framework.h"
#include "wiced_dct_common.h"
#include "wiced_apps_common.h"
#include "platform_mcu_peripheral.h"
#include "elf.h"
#include "../../../libraries/utilities/crc/crc.h"

/******************************************************
 *                      Macros
 ******************************************************/
#ifndef PLATFORM_DCT_COPY1_SIZE
#define PLATFORM_DCT_COPY1_SIZE (PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS)
#endif

#ifndef SECTOR_SIZE
#define SECTOR_SIZE ((uint32_t)(2048))
#endif

/******************************************************
 *                    Constants
 ******************************************************/
#define WICED_DCT_ADDR_MASK 0x07FFFFFF

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WICED_DCT_HEADER_NOT_IN_RANGE = 0,
    WICED_DCT_HEADER_IN_RANGE
} wiced_dct_header_in_address_range_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
static wiced_result_t wiced_dct_copy_header_info(platform_dct_header_t *dest, platform_dct_header_t *source);
static wiced_bool_t wiced_dct_check_crc_valid(uint32_t dct_start_addr, uint32_t dct_end_addr);
static uint32_t wiced_dct_generate_crc_from_flash(uint32_t dct_start_addr,
        uint32_t dct_end_addr,
        wiced_dct_header_in_address_range_t header_in_range,
        uint32_t crc_start);
static wiced_result_t wiced_dct_erase_non_current_dct( uint32_t non_current_dct );
static wiced_result_t wiced_dct_copy_sflash( uint32_t dest_loc, uint32_t src_loc, uint32_t size );

/******************************************************
 *               Variables Definitions
 ******************************************************/
static const uint32_t DCT_section_offsets[] =
{
        [DCT_APP_SECTION]             = sizeof( platform_dct_data_t ),
        [DCT_SECURITY_SECTION]        = OFFSETOF( platform_dct_data_t, security_credentials ),
        [DCT_MFG_INFO_SECTION]        = OFFSETOF( platform_dct_data_t, mfg_info ),
        [DCT_WIFI_CONFIG_SECTION]     = OFFSETOF( platform_dct_data_t, wifi_config ),
        [DCT_ETHERNET_CONFIG_SECTION] = OFFSETOF( platform_dct_data_t, ethernet_config ),
        [DCT_NETWORK_CONFIG_SECTION]  = OFFSETOF( platform_dct_data_t, network_config ),
#ifdef WICED_DCT_INCLUDE_BT_CONFIG
        [DCT_BT_CONFIG_SECTION]       = OFFSETOF( platform_dct_data_t, bt_config ),
#endif
#ifdef WICED_DCT_INCLUDE_P2P_CONFIG
        [DCT_P2P_CONFIG_SECTION]      = OFFSETOF( platform_dct_data_t, p2p_config ),
#endif
        [DCT_INTERNAL_SECTION]        = 0,
};

/******************************************************
 *               Function Definitions
 ******************************************************/
static wiced_result_t wiced_dct_copy_header_info(platform_dct_header_t *dest, platform_dct_header_t *source)
{
    if ((dest == NULL) || (source == NULL))
    {
        return WICED_BADARG;
    }

    memcpy(dest, source, sizeof(platform_dct_header_t));

    dest->full_size             = source->full_size;
    dest->used_size             = source->used_size;
    memcpy(&dest->boot_detail, &source->boot_detail, sizeof(dest->boot_detail));
    memcpy(&dest->apps_locations, &source->apps_locations, sizeof(dest->apps_locations));
    dest->load_app_func         = source->load_app_func;

    dest->magic_number          = BOOTLOADER_MAGIC_NUMBER;
    dest->write_incomplete      = 0;
    dest->app_valid             = source->app_valid;

    /* set to 0 (this is set on initial DCT write) */
    dest->initial_write         = 0;

#ifdef  DCT_HEADER_ALIGN_SIZE
    memset(&dest->padding, 0x00, sizeof(dest->padding));
#endif

    dest->crc32                 = 0;                    /* calculate in wiced_dct_finish_new_dct() */
    dest->sequence              = source->sequence + 1; /* always increment the sequence           */

    return WICED_SUCCESS;
}

static uint32_t wiced_dct_generate_crc_from_flash(uint32_t dct_start_addr,
        uint32_t dct_end_addr,
        wiced_dct_header_in_address_range_t header_in_range,
        uint32_t crc_start)
{
    /* NOTE: the dct_start_addr MUST be the start of the DCT, otherwise the structure will not be lined up !
     * we need to read the entire DCT area, but make sure we set the value of the crc32 field
     * in platform_dct_header_t to 0x00 while figuring the CRC
     */

    /* we don't want to malloc or have a big buffer on the stack */
    uint8_t                 temp_buff[64];
    uint32_t                chunk_size;
    uint32_t                calculated_dct_crc32_value;
    uint32_t                curr_addr;

    curr_addr = dct_start_addr;
    calculated_dct_crc32_value = crc_start;
    while (curr_addr < dct_end_addr)
    {
        chunk_size = dct_end_addr - curr_addr;
        if( chunk_size > sizeof(temp_buff))
        {
            chunk_size = sizeof(temp_buff);
        }

        memcpy( temp_buff, (void *)curr_addr, chunk_size );

        if (header_in_range == WICED_DCT_HEADER_IN_RANGE)
        {
            if (  ((curr_addr - dct_start_addr)               <  OFFSETOF(platform_dct_header_t, crc32)) &&
                    (((curr_addr - dct_start_addr) + chunk_size) > (OFFSETOF(platform_dct_header_t, crc32) + sizeof(uint32_t))) )
            {
                uint32_t*   crc_ptr;

                /* we want the crc32 value to be 0x00 when computing the value! */
                crc_ptr = (uint32_t*)(&temp_buff[(curr_addr - dct_start_addr) + OFFSETOF(platform_dct_header_t, crc32)]);
                *crc_ptr = 0;


                header_in_range = WICED_DCT_HEADER_NOT_IN_RANGE;
            }
        }

        /* compute the crc */
        calculated_dct_crc32_value = crc32(temp_buff, chunk_size, calculated_dct_crc32_value);
        curr_addr += chunk_size;
    }

    return calculated_dct_crc32_value;
}

static wiced_bool_t wiced_dct_check_crc_valid(uint32_t dct_start_addr, uint32_t dct_end_addr)
{
    /* NOTE: the dct_start_addr MUST be the start of the DCT, otherwise the structure will not be lined up !
     * we need to read the entire DCT area, but make sure we set the value of the crc32 field
     * in platform_dct_header_t to 0x00 while figuring the CRC
     */

    uint32_t    calculated_dct_crc32_value;
    uint32_t    read_dct_crc32_value;

    /* get the CRC from the header to compare after we generate a new CRC */
    read_dct_crc32_value = *((uint32_t*)(dct_start_addr + OFFSETOF(platform_dct_header_t, crc32)));

    calculated_dct_crc32_value = wiced_dct_generate_crc_from_flash(dct_start_addr, dct_end_addr, WICED_DCT_HEADER_IN_RANGE, CRC32_INIT_VALUE);

    /* check the calculated vs. the read crc values */
    if (calculated_dct_crc32_value == read_dct_crc32_value)
    {
        /* valid! */
        return WICED_TRUE;
    }
    return WICED_FALSE;
}

#if defined(OTA2_SUPPORT)
wiced_bool_t wiced_dct_ota2_check_crc_valid(uint32_t dct_start_addr, uint32_t dct_end_addr)
{
     return wiced_dct_check_crc_valid( dct_start_addr, dct_end_addr);
}

wiced_result_t wiced_dct_ota2_erase_save_area_and_copy_dct(uint32_t dst_dct)
{
    wiced_result_t          result;
    uint32_t                src_dct;

     /* Erase the first DCT and init it. */
     if (wiced_dct_erase_non_current_dct( dst_dct ) != WICED_SUCCESS)
     {
         return WICED_ERROR;
     }

     /* copy the current DCT to the save area */
     src_dct =  (uint32_t)wiced_dct_get_current_address( DCT_INTERNAL_SECTION );
     result = wiced_dct_copy_sflash( dst_dct, src_dct, PLATFORM_DCT_COPY1_SIZE);

     return result;
}

#endif

void* wiced_dct_get_current_address( dct_section_t section )
{

    platform_dct_header_t   dct1_hdr,   dct2_hdr;
    wiced_bool_t            dct1_valid, dct2_valid;
    uint32_t                new_crc32;
    platform_dct_header_t   new_hdr = { 0 };
    char                    zero_byte = 0;

    memcpy(&dct1_hdr, (char *)PLATFORM_DCT_COPY1_START_ADDRESS, sizeof(platform_dct_header_t));
    memcpy(&dct2_hdr, (char *)PLATFORM_DCT_COPY2_START_ADDRESS, sizeof(platform_dct_header_t));

    dct1_valid = WICED_FALSE;
    dct2_valid = WICED_FALSE;

    /* On the initial write (debugging or manuf.), the initial_write flag will be 1, and the crc32 will be 0x00
     * We will accept the DCT as valid (if the other fields are ok). This can only happen with DCT1.
     */
    if ( ( dct1_hdr.write_incomplete == 0 ) &&
            ( dct1_hdr.magic_number == BOOTLOADER_MAGIC_NUMBER ) &&
            ( ((dct1_hdr.initial_write == 1) && (dct1_hdr.crc32 == 0x00)) ||
                    (wiced_dct_check_crc_valid(PLATFORM_DCT_COPY1_START_ADDRESS, PLATFORM_DCT_COPY1_END_ADDRESS) == WICED_TRUE)) )
    {
        dct1_valid = WICED_TRUE;
    }

    if ( ( dct2_hdr.write_incomplete == 0 ) &&
            ( dct2_hdr.magic_number == BOOTLOADER_MAGIC_NUMBER ) &&
            ( wiced_dct_check_crc_valid(PLATFORM_DCT_COPY2_START_ADDRESS, PLATFORM_DCT_COPY2_END_ADDRESS) == WICED_TRUE))
    {
        dct2_valid = WICED_TRUE;
    }

    /* If the power was removed during writing, both may be valid! */
    if ( ( dct1_valid == WICED_TRUE ) && ( dct2_valid == WICED_TRUE ))
    {
        /* if dct1 was manuf. programmed, use that */
        if (dct1_hdr.initial_write == 1)
        {
            /* DCT1 is latest, set 2 to FALSE */
            dct2_valid = WICED_FALSE;
        }
        else if (dct1_hdr.sequence > dct2_hdr.sequence)
        {
            /* DCT1 is latest, set 2 to FALSE */
            dct2_valid = WICED_FALSE;
        }
        else if (dct1_hdr.sequence < dct2_hdr.sequence)
        {
            /* DCT2 is latest, set 1 to FALSE */
            dct1_valid = WICED_FALSE;
        }
        else
        {
            /* Both valid, same sequence - this should not happen! Use DCT1 here */
            dct2_valid = WICED_FALSE;
        }
    }

    if ( dct1_valid == WICED_TRUE )
    {
        return (void*) ( PLATFORM_DCT_COPY1_START_ADDRESS + DCT_section_offsets[section] );
    }

    if ( dct2_valid == WICED_TRUE )
    {
        return (void*) ( PLATFORM_DCT_COPY2_START_ADDRESS + DCT_section_offsets[section] );
    }

    wiced_assert("BOTH DCTs ARE INVALID!", 0 != 0 );

    /* Erase the first DCT and init it. */
    wiced_dct_erase_non_current_dct( PLATFORM_DCT_COPY1_START_ADDRESS );

    memset(&new_hdr, 0x00, sizeof(platform_dct_header_t));
    new_hdr.app_valid = 1;
    new_hdr.magic_number        = BOOTLOADER_MAGIC_NUMBER;
    new_hdr.write_incomplete  = 0;
    new_hdr.crc32             = 0;
    new_crc32 = crc32((uint8_t*)&new_hdr, sizeof(platform_dct_header_t), CRC32_INIT_VALUE);
    new_crc32 = wiced_dct_generate_crc_from_flash( PLATFORM_DCT_COPY1_START_ADDRESS + sizeof(platform_dct_header_t),
            PLATFORM_DCT_COPY1_END_ADDRESS,
            WICED_DCT_HEADER_NOT_IN_RANGE, new_crc32);
    new_hdr.crc32 = new_crc32;
    new_hdr.write_incomplete  = 1;

    /* write out the header with write_incomplete set */
    platform_write_flash_chunk( PLATFORM_DCT_COPY1_START_ADDRESS, &new_hdr, sizeof(platform_dct_header_t) );
    /* Mark new DCT as complete */
    platform_write_flash_chunk( PLATFORM_DCT_COPY1_START_ADDRESS + OFFSETOF(platform_dct_header_t,write_incomplete), &zero_byte, sizeof( zero_byte ) );

    return (void*) ( PLATFORM_DCT_COPY1_START_ADDRESS + DCT_section_offsets[ section ] );
}

wiced_result_t wiced_dct_read_with_copy ( void* info_ptr, dct_section_t section, uint32_t offset, uint32_t size )
{
    void*        curr_dct;

    curr_dct = wiced_dct_get_current_address( section );
    if (curr_dct == GET_CURRENT_ADDRESS_FAILED)
    {
        return WICED_ERROR;
    }

    if (memcpy( info_ptr, &((uint8_t*)curr_dct)[offset], size ) != info_ptr)
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

static wiced_result_t wiced_dct_copy_sflash( uint32_t dest_loc_in, uint32_t src_loc_in, uint32_t size )
{
    int result;
    uint32_t    dest = dest_loc_in;
    uint32_t    src  = src_loc_in;
    result = platform_write_flash_chunk( dest, (void*)src, size);
    if (result != 0)
    {
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}

static wiced_result_t wiced_dct_erase_non_current_dct( uint32_t non_current_dct )
{
    wiced_result_t result;

    /* convert the dct address to a sector number! */
    uint16_t start_sector = (uint16_t)((non_current_dct & WICED_DCT_ADDR_MASK) / SECTOR_SIZE);
    uint16_t end_sector   = (uint16_t)((((non_current_dct + DCT1_SIZE) & WICED_DCT_ADDR_MASK) / SECTOR_SIZE) - 1);
    result = platform_erase_flash( start_sector, end_sector );
    return result;
}


static wiced_result_t wiced_dct_start_new_dct(uint32_t *new_dct,
        platform_dct_header_t *new_hdr )
{
    platform_dct_header_t*  curr_dct;

    if ((new_dct == NULL) || (new_hdr == NULL))
    {
        return WICED_ERROR;
    }

    /* Erase the non-current DCT */
    curr_dct  = (platform_dct_header_t *)wiced_dct_get_current_address( DCT_INTERNAL_SECTION );
    if (curr_dct == GET_CURRENT_ADDRESS_FAILED)
    {
        return WICED_ERROR;
    }
    *new_dct = ((uint32_t)curr_dct == PLATFORM_DCT_COPY1_START_ADDRESS ) ?
            PLATFORM_DCT_COPY2_START_ADDRESS : PLATFORM_DCT_COPY1_START_ADDRESS;
    wiced_dct_erase_non_current_dct( *new_dct );

    /* read the header from the old DCT */
    if (wiced_dct_copy_header_info(new_hdr, curr_dct) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

static wiced_result_t wiced_dct_finish_new_dct(uint32_t new_dct, platform_dct_header_t *new_hdr )
{
    uint32_t        new_crc32;
    char            zero_byte = 0;
    wiced_result_t  result = WICED_SUCCESS;

    /* generate the new CRC value
     * save these values - we need to calculate the CRC with them as 0x00
     */
    new_hdr->write_incomplete  = 0;             /* compute CRC with this as 0x00 */
    new_hdr->crc32             = 0;             /* compute CRC with this as 0x00 */
    new_crc32 = crc32((uint8_t*)new_hdr, sizeof(platform_dct_header_t), CRC32_INIT_VALUE);
    new_crc32 = wiced_dct_generate_crc_from_flash( new_dct + sizeof(platform_dct_header_t),
            new_dct + PLATFORM_DCT_COPY1_SIZE,
            WICED_DCT_HEADER_NOT_IN_RANGE, new_crc32);
    new_hdr->crc32 = new_crc32;
    new_hdr->write_incomplete = 1;

    /* Write the new DCT header data, note, and we know that the CRC is wrong (write_incomplete written to 0 to fix CRC below) */
    if ( platform_write_flash_chunk( new_dct, new_hdr, sizeof(platform_dct_header_t)) != PLATFORM_SUCCESS )
    {
        result = WICED_ERROR;
    }

    /* if we were successful, clear write_incomplete (CRC is now correct) */
    if ((result == WICED_SUCCESS) &&
            (platform_write_flash_chunk( (uint32_t)new_dct + OFFSETOF(platform_dct_header_t, write_incomplete),
                                         &zero_byte, sizeof( zero_byte )) != PLATFORM_SUCCESS) )
    {
        result = WICED_ERROR;
    }

    /* clear the initial_write bit of DCT 1, which overrides sequence numbers */
    if ((result == WICED_SUCCESS) && (new_dct != PLATFORM_DCT_COPY1_START_ADDRESS))
    {
        if (platform_write_flash_chunk( PLATFORM_DCT_COPY1_START_ADDRESS + OFFSETOF(platform_dct_header_t, initial_write),
                                        &zero_byte, sizeof( zero_byte )) != PLATFORM_SUCCESS)
        {
            result = WICED_ERROR;
        }
    }
    return result;
}


wiced_result_t wiced_dct_write_boot_details( const boot_detail_t* new_boot_details )
{
    uint32_t                bytes_after_header;
    platform_dct_header_t   new_dct_header;
    uint32_t                new_dct, curr_dct;

    if (new_boot_details == NULL)
    {
        return WICED_ERROR;
    }

    curr_dct = (uint32_t)wiced_dct_get_current_address(DCT_INTERNAL_SECTION);
    if (curr_dct == (uint32_t)GET_CURRENT_ADDRESS_FAILED)
    {
        return WICED_ERROR;
    }

    /* set up the new DCT (and get new_dct address) */
    if (wiced_dct_start_new_dct( (uint32_t*)&new_dct, &new_dct_header ) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    /* Write every thing other than the header */
    bytes_after_header = PLATFORM_DCT_COPY1_SIZE - sizeof(platform_dct_header_t);
    if (wiced_dct_copy_sflash(new_dct + sizeof(platform_dct_header_t), curr_dct + sizeof(platform_dct_header_t), bytes_after_header) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    /* copy new boot_detail info */
    new_dct_header.boot_detail      = *new_boot_details;

    return wiced_dct_finish_new_dct(new_dct, &new_dct_header);

}

wiced_result_t wiced_dct_write_app_location( image_location_t *new_app_location, uint32_t app_index )
{
    uint32_t                bytes_after_header;
    platform_dct_header_t   new_dct_header;
    uint32_t                new_dct, curr_dct;

    if ((app_index >= DCT_MAX_APP_COUNT) || (new_app_location == NULL))
    {
        return WICED_ERROR;
    }

    curr_dct = (uint32_t)wiced_dct_get_current_address( DCT_INTERNAL_SECTION );
    if (curr_dct == (uint32_t)GET_CURRENT_ADDRESS_FAILED)
    {
        return WICED_ERROR;
    }

    if (wiced_dct_start_new_dct(&new_dct, &new_dct_header) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    /* use new app location info */
    memcpy( &new_dct_header.apps_locations[app_index], new_app_location, sizeof(image_location_t));

    /* Write every thing other than the header */
    bytes_after_header = PLATFORM_DCT_COPY1_SIZE - sizeof(platform_dct_header_t);
    if (wiced_dct_copy_sflash(new_dct + sizeof(platform_dct_header_t), curr_dct + sizeof(platform_dct_header_t), bytes_after_header) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    return wiced_dct_finish_new_dct(new_dct, &new_dct_header);

}

wiced_result_t wiced_dct_write( const void* data, dct_section_t section, uint32_t offset, uint32_t data_length )
{
    platform_dct_header_t   new_dct_header;
    uint32_t                bytes_to_copy;
    uint32_t                section_start = DCT_section_offsets[ section ] + offset;
    uint32_t                new_dct, curr_dct;

    /* Check if the data is too big to write */
    if ( (section_start < sizeof(platform_dct_header_t)) ||
            ((section_start + data_length) > PLATFORM_DCT_COPY1_SIZE))
    {
        return WICED_ERROR;
    }

    curr_dct = (uint32_t) wiced_dct_get_current_address( DCT_INTERNAL_SECTION );
    if (curr_dct == (uint32_t)GET_CURRENT_ADDRESS_FAILED)
    {
        return WICED_ERROR;
    }

    if (wiced_dct_start_new_dct(&new_dct, &new_dct_header) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    /* copy from after the platform_dct_header_t to the section */
    bytes_to_copy = section_start - sizeof(platform_dct_header_t);
    if ( bytes_to_copy != 0 )
    {
        if (wiced_dct_copy_sflash( new_dct + sizeof(platform_dct_header_t), curr_dct + sizeof(platform_dct_header_t), bytes_to_copy ) != WICED_SUCCESS)
        {
            return WICED_ERROR;
        }
    }


    /* write the new data */
    if (platform_write_flash_chunk( (uint32_t)new_dct + section_start, data, data_length ) != 0)
    {
        return WICED_ERROR;
    }

    /* Calculate how many bytes need to be written after the end of the requested data write */
    bytes_to_copy = ( PLATFORM_DCT_COPY1_SIZE ) - ( section_start + data_length );
    if ( bytes_to_copy != 0 )
    {
        /* There is data after end of requested write - copy it from old DCT to new DCT */
        if (wiced_dct_copy_sflash(new_dct + section_start + data_length, curr_dct + section_start + data_length, bytes_to_copy ) != WICED_SUCCESS)
        {
            return WICED_ERROR;
        }
    }

    /* update any header info and finish the new DCT */
    new_dct_header.app_valid = 1;

    if (wiced_dct_finish_new_dct(new_dct, &new_dct_header) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }
    return WICED_SUCCESS;

}

static wiced_result_t wiced_dct_check_apps_locations_valid( image_location_t* app_header_locations )
{
    if (   ( /* No FR APP */                    app_header_locations[DCT_FR_APP_INDEX].id  != EXTERNAL_FIXED_LOCATION )
            || ( /* FR App address incorrect */     app_header_locations[DCT_FR_APP_INDEX].detail.external_fixed.location != (SFLASH_APPS_HEADER_LOC + sizeof(app_header_t) * DCT_FR_APP_INDEX))
            || ( /* No FR DCT */                    app_header_locations[DCT_DCT_IMAGE_INDEX].id  != EXTERNAL_FIXED_LOCATION )
            || ( /* DCT address is incorrect */     app_header_locations[DCT_DCT_IMAGE_INDEX].detail.external_fixed.location != (SFLASH_APPS_HEADER_LOC + sizeof(app_header_t) * DCT_DCT_IMAGE_INDEX))
    )
    {
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}

static wiced_result_t wiced_dct_load( const image_location_t* dct_location )
{
    elf_header_t         header;
    elf_program_header_t prog_header;
    uint32_t             size;
    uint32_t             offset;
    uint32_t             dest_loc = PLATFORM_DCT_COPY1_START_ADDRESS;
    uint8_t              buff[64];

    /* Erase the application area */
    wiced_dct_erase_non_current_dct( dest_loc );

    /* Read the image header */
    if (wiced_apps_read( dct_location, (uint8_t*) &header, 0, sizeof( header ) ) != 0)
    {
        return WICED_ERROR;
    }

    if ( header.program_header_entry_count != 1 )
    {
        return WICED_ERROR;
    }

    if (wiced_apps_read( dct_location, (uint8_t*) &prog_header, header.program_header_offset, sizeof( prog_header ) ) != 0 )
    {
        return WICED_ERROR;
    }

    size   = prog_header.data_size_in_file;
    offset = prog_header.data_offset;

    while ( size > 0 )
    {
        uint32_t write_size = MIN( sizeof(buff), size);
        if (wiced_apps_read( dct_location, buff, offset, write_size) != 0)
        {
            if (platform_write_flash_chunk( (uint32_t)dest_loc, buff, write_size ) != 0)
            {
                return WICED_ERROR;
            }
        }

        offset   += write_size;
        dest_loc += write_size;
        size     -= write_size;
    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_dct_restore_factory_reset( void )
{
    image_location_t app_header_locations[ DCT_MAX_APP_COUNT ];

    if (wiced_dct_read_with_copy( app_header_locations, DCT_INTERNAL_SECTION, DCT_APP_LOCATION_OF(DCT_FR_APP_INDEX),
                              sizeof(image_location_t)* DCT_MAX_APP_COUNT ) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }
    if (wiced_dct_check_apps_locations_valid ( app_header_locations ) == WICED_ERROR )
    {
        /* DCT was corrupted. Restore only FR Application address */
        app_header_locations[DCT_FR_APP_INDEX].id                                = EXTERNAL_FIXED_LOCATION;
        app_header_locations[DCT_FR_APP_INDEX].detail.external_fixed.location    = SFLASH_APPS_HEADER_LOC + sizeof(app_header_t) * DCT_FR_APP_INDEX;
        app_header_locations[DCT_DCT_IMAGE_INDEX].id                             = EXTERNAL_FIXED_LOCATION;
        app_header_locations[DCT_DCT_IMAGE_INDEX].detail.external_fixed.location = SFLASH_APPS_HEADER_LOC + sizeof(app_header_t) * DCT_DCT_IMAGE_INDEX;
    }
    /* OK Current DCT seems decent, lets keep apps locations. */
    if (wiced_dct_load( &app_header_locations[DCT_DCT_IMAGE_INDEX] ) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }
    if (wiced_dct_write_app_location( &app_header_locations[DCT_FR_APP_INDEX], DCT_FR_APP_INDEX ) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }
    if (wiced_dct_write_app_location( &app_header_locations[DCT_DCT_IMAGE_INDEX], DCT_DCT_IMAGE_INDEX ) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_dct_get_app_header_location( uint8_t app_id, image_location_t* app_header_location )
{
    if ( app_id >= DCT_MAX_APP_COUNT )
    {
        return WICED_ERROR;
    }
    return wiced_dct_read_with_copy( app_header_location, DCT_INTERNAL_SECTION, DCT_APP_LOCATION_OF(app_id), sizeof(image_location_t) );
}

wiced_result_t wiced_dct_set_app_header_location( uint8_t app_id, image_location_t* app_header_location )
{
    if ( app_id >= DCT_MAX_APP_COUNT )
    {
        return WICED_ERROR;
    }
    return wiced_dct_write_app_location(app_header_location, app_id);
}
