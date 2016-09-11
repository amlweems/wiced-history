/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * Defines BCM439x WICED application framework functions
 */
#include "waf_platform.h"
#include "wiced_framework.h"
#include "wiced_apps_common.h"
#include "wiced_deep_sleep.h"
#include "wicedfs.h"

/******************************************************
 *                      Macros
 ******************************************************/

/* Application linker script ensures that tiny bootloader binary is starting from the beginning of Always-On memory.
 * Tiny bootloader linker script ensures that its configuration structure is starting from the beginning of Always-On memory.
 */
#define WICED_DEEP_SLEEP_TINY_BOOTLOADER_CONFIG    ( (volatile wiced_deep_sleep_tiny_bootloader_config_t *)PLATFORM_SOCSRAM_CH0_AON_RAM_BASE(0x0) )

/******************************************************
 *                    Constants
 ******************************************************/

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
 *              Function Definitions
 ******************************************************/

/******************************************************
 *              Global Variables
 ******************************************************/

/******************************************************
 *              Function Declarations
  ******************************************************/

/******************************************************
 *                 DCT Functions
 ******************************************************/

static wiced_bool_t platform_is_load_permitted( void* physical_address, uint32_t size, wiced_bool_t* is_aon_segment )
{
    const uint32_t     destination = (uint32_t)physical_address;
    const wiced_bool_t aon_segment = WICED_DEEP_SLEEP_IS_AON_SEGMENT( destination, size ) ? WICED_TRUE : WICED_FALSE;
    wiced_bool_t       result      = WICED_FALSE;

    if ( !aon_segment || !platform_mcu_powersave_is_warmboot() )
    {
        result          = WICED_TRUE;
        *is_aon_segment = aon_segment;
    }

    return result;
}

void platform_load_app_chunk_from_fs( const image_location_t* app_header_location, void* file_handler, void* physical_address, uint32_t size )
{
    wiced_bool_t aon_segment;

    UNUSED_PARAMETER( app_header_location );

    if ( platform_is_load_permitted( physical_address, size, &aon_segment ) )
    {
        wicedfs_file_t* stream = file_handler;

        wicedfs_fread( physical_address, size, 1, stream );

        if ( aon_segment )
        {
            WICED_DEEP_SLEEP_TINY_BOOTLOADER_CONFIG->app_address = stream->location;
        }
    }
}

void platform_erase_app_area( uint32_t physical_address, uint32_t size )
{
    /* App is in RAM, no need for erase */
    UNUSED_PARAMETER( physical_address );
    UNUSED_PARAMETER( size );
}
#if (DCT_BOOTLOADER_SDK_VERSION >= DCT_BOOTLOADER_SDK_3_1_1)
void platform_load_app_chunk(const image_location_t* app_header_location, uint32_t offset, void* physical_address, uint32_t size )
{
    wiced_bool_t aon_segment;

    if ( platform_is_load_permitted( physical_address, size, &aon_segment ) )
    {
        wiced_apps_read( app_header_location, physical_address, offset, size );

        if ( aon_segment )
        {
            WICED_DEEP_SLEEP_TINY_BOOTLOADER_CONFIG->app_address = app_header_location->detail.external_fixed.location;
        }
    }
}
#endif
