/*
 * Copyright 2014, Broadcom Corporation
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
#include "wiced_framework.h"
#include "wiced_dct_common.h"
#include "platform_peripheral.h"
#include "waf_platform.h"

/******************************************************
 *                      Macros
 ******************************************************/
#define ERASE_DCT_1()              platform_erase_flash(PLATFORM_DCT_COPY1_START_SECTOR, PLATFORM_DCT_COPY1_END_SECTOR)
#define ERASE_DCT_2()              platform_erase_flash(PLATFORM_DCT_COPY2_START_SECTOR, PLATFORM_DCT_COPY2_END_SECTOR)
/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/
#define PLATFORM_SFLASH_PERIPHERAL_ID (0)

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/
static const uint32_t DCT_section_offsets[] =
{
    [DCT_APP_SECTION]         = sizeof( platform_dct_data_t ),
    [DCT_SECURITY_SECTION]    = OFFSETOF( platform_dct_data_t, security_credentials ),
    [DCT_MFG_INFO_SECTION]    = OFFSETOF( platform_dct_data_t, mfg_info ),
    [DCT_WIFI_CONFIG_SECTION] = OFFSETOF( platform_dct_data_t, wifi_config ),
    [DCT_INTERNAL_SECTION]    = 0,
};

/******************************************************
 *               Function Definitions
 ******************************************************/
void* wiced_dct_get_current_address( dct_section_t section )
{
    static const platform_dct_header_t hdr =
    {
        .write_incomplete    = 0,
        .is_current_dct      = 1,
        .app_valid           = 1,
        .mfg_info_programmed = 0,
        .magic_number        = BOOTLOADER_MAGIC_NUMBER,
        .load_app_func       = NULL
    };

    platform_dct_header_t* dct1 = ((platform_dct_header_t*) PLATFORM_DCT_COPY1_START_ADDRESS);
    platform_dct_header_t* dct2 = ((platform_dct_header_t*) PLATFORM_DCT_COPY2_START_ADDRESS);


    if ( ( dct1->is_current_dct == 1 ) &&
         ( dct1->write_incomplete == 0 ) &&
         ( dct1->magic_number == BOOTLOADER_MAGIC_NUMBER ) )
    {
        return &((char*)dct1)[ DCT_section_offsets[section] ];
    }

    if ( ( dct2->is_current_dct == 1 ) &&
         ( dct2->write_incomplete == 0 ) &&
         ( dct2->magic_number == BOOTLOADER_MAGIC_NUMBER ) )
    {
        return &((char*)dct2)[ DCT_section_offsets[section] ];
    }

    /* No valid DCT! */
    /* Erase the first DCT and init it. */

    wiced_assert("BOTH DCTs ARE INVALID!", 0 != 0 );

    ERASE_DCT_1();

    platform_erase_flash(PLATFORM_DCT_COPY1_START_SECTOR, PLATFORM_DCT_COPY1_END_SECTOR);


    platform_write_flash_chunk( PLATFORM_DCT_COPY1_START_ADDRESS, &hdr, sizeof(hdr) );

    return &((char*)dct1)[ DCT_section_offsets[section] ];

}

wiced_result_t wiced_dct_read_with_copy    ( void* info_ptr, dct_section_t section, uint32_t offset, uint32_t size )
{
    char* curr_dct  = (char*) wiced_dct_get_current_address( section );

    memcpy( info_ptr, &curr_dct[ offset ], size );

    return WICED_SUCCESS;
}

/* Updates the boot part of the DCT header only.Currently platform_dct_write can't update header.
 * The function should be removed once platform_dct_write if fixed to write the header part as well.
 */
static int wiced_dct_update_boot( const boot_detail_t* boot_detail )
{
    platform_dct_header_t* new_dct;
    char            zero_byte          = 0;
    platform_dct_header_t* curr_dct    = &( (platform_dct_data_t*) wiced_dct_get_current_address( DCT_INTERNAL_SECTION ) )->dct_header;
    uint32_t        bytes_after_header = ( PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS ) - ( sizeof(platform_dct_header_t) );
    platform_dct_header_t hdr          =
    {
        .write_incomplete = 1,
        .is_current_dct   = 1,
        .magic_number     = BOOTLOADER_MAGIC_NUMBER
    };

    /* Erase the non-current DCT */
    if ( curr_dct == ( (platform_dct_header_t*) PLATFORM_DCT_COPY1_START_ADDRESS ) )
    {
        new_dct = (platform_dct_header_t*) PLATFORM_DCT_COPY2_START_ADDRESS;
        ERASE_DCT_2();
    }
    else
    {
        new_dct = (platform_dct_header_t*) PLATFORM_DCT_COPY1_START_ADDRESS;
        ERASE_DCT_1();
    }

    /* Write every thing other than the header */
    platform_write_flash_chunk( (uint32_t) &new_dct[ 1 ], &curr_dct[ 1 ], bytes_after_header );

    /* Verify the mfg data */
    if ( memcmp( &new_dct[ 1 ], &curr_dct[ 1 ], bytes_after_header ) != 0 )
    {
        return -2;
    }

    hdr.app_valid = curr_dct->app_valid;
    hdr.load_app_func = curr_dct->load_app_func;
    hdr.mfg_info_programmed = curr_dct->mfg_info_programmed;
    memcpy( &hdr.boot_detail, boot_detail, sizeof(boot_detail_t) );
    memcpy( hdr.apps_locations, curr_dct->apps_locations, sizeof(image_location_t) * DCT_MAX_APP_COUNT );

    /* Write the new DCT header data */
    platform_write_flash_chunk( (uint32_t) new_dct, &hdr, sizeof( hdr ) );

    /* Mark new DCT as complete and current */
    platform_write_flash_chunk( ( (uint32_t) new_dct ) + OFFSETOF(platform_dct_header_t,write_incomplete), &zero_byte, sizeof( zero_byte ) );
    platform_write_flash_chunk( ( (unsigned long) curr_dct ) + OFFSETOF(platform_dct_header_t,is_current_dct), &zero_byte, sizeof( zero_byte ) );

    return 0;
}

static int wiced_dct_update_app_header_location( uint32_t offset, const image_location_t *new_app_location )
{
    platform_dct_header_t* new_dct;
    char            zero_byte          = 0;
    platform_dct_header_t* curr_dct    = &( (platform_dct_data_t*) wiced_dct_get_current_address( DCT_INTERNAL_SECTION ) )->dct_header;
    uint32_t        bytes_after_header = ( PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS ) - ( sizeof(platform_dct_header_t) );
    platform_dct_header_t hdr          =
    {
        .write_incomplete = 1,
        .is_current_dct   = 1,
        .magic_number     = BOOTLOADER_MAGIC_NUMBER
    };

    /* Erase the non-current DCT */
    if ( curr_dct == ( (platform_dct_header_t*) PLATFORM_DCT_COPY1_START_ADDRESS ) )
    {
        new_dct = (platform_dct_header_t*) PLATFORM_DCT_COPY2_START_ADDRESS;
        ERASE_DCT_2();
    }
    else
    {
        new_dct = (platform_dct_header_t*) PLATFORM_DCT_COPY1_START_ADDRESS;
        ERASE_DCT_1();
    }

    /* Write every thing other than the header */
    platform_write_flash_chunk( (uint32_t) &new_dct[ 1 ], (uint8_t*) &curr_dct[ 1 ], bytes_after_header );

    /* Verify the mfg data */
    if ( memcmp( &new_dct[ 1 ], &curr_dct[ 1 ], bytes_after_header ) != 0 )
    {
        return -2;
    }

    hdr.app_valid           = curr_dct->app_valid;
    hdr.load_app_func       = curr_dct->load_app_func;
    hdr.mfg_info_programmed = curr_dct->mfg_info_programmed;
    memcpy( &hdr.boot_detail, &curr_dct->boot_detail, sizeof(boot_detail_t) );
    memcpy( hdr.apps_locations, curr_dct->apps_locations, sizeof(image_location_t) * DCT_MAX_APP_COUNT );
    memcpy( ( (uint8_t*) &hdr ) + offset, new_app_location, sizeof(image_location_t) );

    /* Write the new DCT header data */
    platform_write_flash_chunk( (uint32_t) new_dct, &hdr, sizeof( hdr ) );

    /* Mark new DCT as complete and current */
    platform_write_flash_chunk( ( (uint32_t) new_dct ) + OFFSETOF(platform_dct_header_t,write_incomplete), &zero_byte, sizeof( zero_byte ) );
    platform_write_flash_chunk( ( (unsigned long) curr_dct ) + OFFSETOF(platform_dct_header_t,is_current_dct), &zero_byte, sizeof( zero_byte ) );

    return 0;
}

static int wiced_write_dct( uint32_t data_start_offset, const void* data, uint32_t data_length, int8_t app_valid, void (*func)(void) )
{
    platform_dct_header_t* new_dct;
    uint32_t               bytes_after_data;
    uint8_t*               new_app_data_addr;
    uint8_t*               curr_app_data_addr;
    platform_dct_header_t* curr_dct  = &((platform_dct_data_t*)wiced_dct_get_current_address( DCT_INTERNAL_SECTION ))->dct_header;
    char                   zero_byte = 0;
    platform_dct_header_t  hdr =
    {
        .write_incomplete = 1,
        .is_current_dct   = 1,
        .magic_number     = BOOTLOADER_MAGIC_NUMBER
    };

    /* Check if the data is too big to write */
    if ( data_length + data_start_offset > ( PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS ) )
    {
        return -1;
    }

    /* Erase the non-current DCT */
    if ( curr_dct == ((platform_dct_header_t*)PLATFORM_DCT_COPY1_START_ADDRESS) )
    {
        new_dct = (platform_dct_header_t*)PLATFORM_DCT_COPY2_START_ADDRESS;
        ERASE_DCT_2();
    }
    else
    {
        new_dct = (platform_dct_header_t*)PLATFORM_DCT_COPY1_START_ADDRESS;
        ERASE_DCT_1();
    }

    data_start_offset -= sizeof( platform_dct_header_t );
    /* Write the mfg data and initial part of app data before provided data */
    platform_write_flash_chunk( ((uint32_t) &new_dct[1]), &curr_dct[1], data_start_offset );

    /* Verify the mfg data */
    if ( memcmp( &new_dct[1], &curr_dct[1], data_start_offset ) != 0 )
    {
        return -2;
    }

    /* Write the app data */
    new_app_data_addr  =  (uint8_t*) &new_dct[1]  + data_start_offset;
    curr_app_data_addr =  (uint8_t*) &curr_dct[1] + data_start_offset;

    platform_write_flash_chunk( (uint32_t) new_app_data_addr, data, data_length );

    /* Verify the app data */
    if ( memcmp( new_app_data_addr, data, data_length ) != 0 )
    {
        /* Error writing app data */
        return -3;
    }

    bytes_after_data = ( PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS ) - (sizeof(platform_dct_header_t) + data_start_offset + data_length );

    if ( bytes_after_data != 0 )
    {
        new_app_data_addr += data_length;
        curr_app_data_addr += data_length;

        platform_write_flash_chunk( (uint32_t)new_app_data_addr, curr_app_data_addr, bytes_after_data );
        /* Verify the app data */
        if ( 0 != memcmp( new_app_data_addr, curr_app_data_addr, bytes_after_data ) )
        {
            /* Error writing app data */
            return -4;
        }
    }

    hdr.app_valid           = (char) (( app_valid == -1 )? curr_dct->app_valid : app_valid);
    hdr.load_app_func       = func;
    hdr.mfg_info_programmed = curr_dct->mfg_info_programmed;
    memcpy( &hdr.boot_detail, &curr_dct->boot_detail, sizeof(boot_detail_t));
    memcpy( hdr.apps_locations, curr_dct->apps_locations, sizeof( image_location_t )* DCT_MAX_APP_COUNT );

    /* Write the header data */
    platform_write_flash_chunk( (uint32_t)new_dct, &hdr, sizeof(hdr) );

    /* Verify the header data */
    if ( memcmp( new_dct, &hdr, sizeof(hdr) ) != 0 )
    {
        /* Error writing header data */
        return -5;
    }

    /* Mark new DCT as complete and current */
    platform_write_flash_chunk( (uint32_t)&new_dct->write_incomplete, &zero_byte, sizeof(zero_byte) );
    platform_write_flash_chunk( (uint32_t)&curr_dct->is_current_dct, &zero_byte, sizeof(zero_byte) );

    return 0;
}

wiced_result_t wiced_dct_update( const void* info_ptr, dct_section_t section, uint32_t offset, uint32_t size )
{
    int retval;
    if ( ( section == DCT_INTERNAL_SECTION) &&
         ( offset == OFFSETOF( platform_dct_header_t, boot_detail ) ) &&
         ( size == sizeof(boot_detail_t))
        )
    {
        retval = wiced_dct_update_boot( info_ptr );
    }
    else if ( ( section == DCT_INTERNAL_SECTION) &&
              ( offset >= OFFSETOF( platform_dct_header_t, apps_locations ) ) &&
              ( offset < OFFSETOF( platform_dct_header_t, apps_locations ) + sizeof(image_location_t) * DCT_MAX_APP_COUNT ) &&
              ( size == sizeof(image_location_t))
           )
    {
        retval = wiced_dct_update_app_header_location( offset, info_ptr );
    }
    else
    {
        retval = wiced_write_dct( DCT_section_offsets[section] + offset, info_ptr, size, 1, NULL );
    }
    return (retval == 0)? WICED_SUCCESS : WICED_ERROR;
}

wiced_result_t wiced_dct_get_app_header_location( uint8_t app_id, image_location_t* app_header_location)
{
    if ( app_id > DCT_MAX_APP_COUNT )
    {
        return WICED_ERROR;
    }
    return wiced_dct_read_with_copy( app_header_location, DCT_INTERNAL_SECTION, DCT_APP_LOCATION_OF(app_id), sizeof(image_location_t) );
}

wiced_result_t wiced_dct_set_app_header_location( uint8_t app_id, image_location_t* app_header_location)
{
    if ( app_id > DCT_MAX_APP_COUNT )
    {
        return WICED_ERROR;
    }
    return wiced_dct_update( app_header_location, DCT_INTERNAL_SECTION, DCT_APP_LOCATION_OF(app_id), sizeof(image_location_t));
}
