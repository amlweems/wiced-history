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
 * Handle WAF (WICED Application Framework) common stuff.
 * Mainly:
 *      * Erasing Apps
 *      * Loading Apps
 *      * Setting to boot from certain App
 */

#include "wwd_assert.h"
#include "wiced_result.h"
#include "wiced_utilities.h"
#include "platform_dct.h"
#include "wiced_apps_common.h"
#include "wiced_waf_common.h"
#include "wiced_dct_common.h"
#include "platform_cmsis.h"
#include "waf_platform.h"
#include "elf.h"
#include "platform_peripheral.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define PLATFORM_SFLASH_PERIPHERAL_ID  (0)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t wiced_waf_reboot( void )
{
    /* Reset request */
    platform_mcu_reset( );

    return WICED_SUCCESS;
}

wiced_result_t wiced_waf_app_set_boot(uint8_t app_id, char load_once)
{
    boot_detail_t boot;

    boot.entry_point                 = 0;
    boot.load_details.load_once      = load_once;
    boot.load_details.valid          = 1;
    boot.load_details.destination.id = INTERNAL;

    if ( wiced_dct_get_app_header_location( app_id, &boot.load_details.source ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }
    if ( wiced_dct_update( &boot, DCT_INTERNAL_SECTION, OFFSETOF( platform_dct_header_t, boot_detail ), sizeof(boot_detail_t) ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}


wiced_result_t wiced_waf_app_open( uint8_t app_id, wiced_app_t* app )
{
    if ( wiced_dct_get_app_header_location( app_id, &app->app_header_location ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }
    app->offset = 0x00000000;
    app->app_id = app_id;
    app->last_erased_sector = 0xFFFFFFFF;

    return WICED_SUCCESS;
}

wiced_result_t wiced_waf_app_close( wiced_app_t* app )
{
    (void) app;
    return WICED_SUCCESS;
}

wiced_result_t wiced_waf_app_erase( wiced_app_t* app )
{
    return wiced_apps_erase( &app->app_header_location );
}

wiced_result_t wiced_waf_app_get_size( wiced_app_t* app, uint32_t* size )
{
    return wiced_apps_get_size( &app->app_header_location, size );
}

wiced_result_t wiced_waf_app_set_size(wiced_app_t* app, uint32_t size)
{
    if ( wiced_apps_set_size( &app->app_header_location, size ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }
    return wiced_dct_set_app_header_location( app->app_id, &app->app_header_location );
}

static wiced_result_t wiced_waf_app_area_erase(const image_location_t* app_header_location)
{
    elf_header_t header;
    uint32_t     i;
    uint32_t     start_address = 0xFFFFFFFF;
    uint32_t     end_address   = 0x00000000;

    wiced_apps_read( app_header_location, (uint8_t*) &header, 0, sizeof( header ) );

    for ( i = 0; i < header.program_header_entry_count; i++ )
    {
        elf_program_header_t prog_header;
        unsigned long offset;

        offset = header.program_header_offset + header.program_header_entry_size * (unsigned long) i;
        wiced_apps_read( app_header_location, (uint8_t*) &prog_header, offset, sizeof( prog_header ) );

        if ( ( prog_header.data_size_in_file == 0 ) || /* size is zero */
             ( ( prog_header.type & 0x1 ) == 0 ) ) /* non- loadable segment */
        {
            continue;
        }

        if ( prog_header.physical_address < start_address )
        {
            start_address = prog_header.physical_address;
        }

        if ( prog_header.physical_address + prog_header.data_size_in_file > end_address )
        {
            end_address = prog_header.physical_address + prog_header.data_size_in_file;
        }
    }
    platform_erase_app_area( start_address, end_address - start_address );
    return WICED_SUCCESS;
}

wiced_result_t wiced_waf_app_write_chunk( wiced_app_t* app, const uint8_t* data, uint32_t size)
{
    wiced_result_t  result;
    result = wiced_apps_write( &app->app_header_location, data, app->offset, size, &app->last_erased_sector );
    if ( result == WICED_SUCCESS )
    {
        app->offset += size;
    }
    return result;
}

wiced_result_t wiced_waf_app_read_chunk( wiced_app_t* app, uint32_t offset, uint8_t* data, uint32_t size )
{
    return wiced_apps_read( &app->app_header_location, data, offset, size );
}

wiced_result_t wiced_waf_app_load( const image_location_t* app_header_location, uint32_t* destination )
{
    elf_header_t header;
    uint32_t     i;

    /* Erase the application area */
    if ( wiced_waf_app_area_erase( app_header_location ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    /* Read the image header */
    wiced_apps_read( app_header_location, (uint8_t*) &header, 0, sizeof( header ) );

    for ( i = 0; i < header.program_header_entry_count; i++ )
    {
        elf_program_header_t prog_header;
        unsigned long        offset;

        offset = header.program_header_offset + header.program_header_entry_size * (unsigned long) i;
        wiced_apps_read( app_header_location, (uint8_t*) &prog_header, offset, sizeof( prog_header ) );

        if ( ( prog_header.data_size_in_file == 0 ) || /* size is zero */
             ( ( prog_header.type & 0x1 ) == 0 ) ) /* non- loadable segment */
        {
            continue;
        }
        offset = prog_header.data_offset;
        platform_load_app_chunk( app_header_location, offset, (void*) prog_header.physical_address, prog_header.data_size_in_file );
    }

    *(uint32_t *) destination = header.entry;

    return WICED_SUCCESS;
}

void wiced_waf_start_app( uint32_t entry_point )
{
    platform_start_app( entry_point );
}

wiced_result_t wiced_waf_check_factory_reset( void )
{
    return (wiced_result_t) platform_check_factory_reset( );
}
