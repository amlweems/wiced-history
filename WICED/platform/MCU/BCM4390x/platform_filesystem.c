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
 * Defines BCM439x filesystem
 */
#include "stdint.h"
#include "string.h"
#include "platform_init.h"
#include "platform_peripheral.h"
#include "platform_mcu_peripheral.h"
#include "platform_stdio.h"
#include "platform_sleep.h"
#include "platform_config.h"
#include "platform_sflash_dct.h"
#include "platform_dct.h"
#include "wwd_constants.h"
#include "wwd_rtos.h"
#include "wwd_assert.h"
#include "RTOS/wwd_rtos_interface.h"
#include "spi_flash.h"
#include "wicedfs.h"
#include "wiced_framework.h"
#include "wiced_dct_common.h"
#include "wiced_apps_common.h"
#include "wiced_block_device.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define DDR_BASE      PLATFORM_DDR_BASE(0x0)

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

static wicedfs_usize_t read_callback ( void* user_param, void* buf, wicedfs_usize_t size, wicedfs_usize_t pos );

/******************************************************
 *               Variables Definitions
 ******************************************************/

host_semaphore_type_t sflash_mutex;
sflash_handle_t       wicedfs_sflash_handle;
wicedfs_filesystem_t  resource_fs_handle;
static  wiced_app_t   fs_app;
static  uint8_t       fs_init_done = 0;

#if (DCT_BOOTLOADER_SDK_VERSION >= DCT_BOOTLOADER_SDK_3_1_2)
static  wiced_app_t   fs_app;
#endif
/******************************************************
 *               Function Definitions
 ******************************************************/

void platform_sflash_init( void )
{
    host_rtos_init_semaphore( &sflash_mutex );

    host_rtos_set_semaphore( &sflash_mutex, WICED_FALSE );
}

/* To handle WWD applications that don't go through wiced_init() (yet need to use the file system):
 * File system initialization is called twice
 * wiced_init()->wiced_core_init()->wiced_platform_init()->platform_filesystem_init()
 * wwd_management_wifi_on()->host_platform_init()->platform_filesystem_init()
 */
platform_result_t platform_filesystem_init( void )
{
    int             result;

    if ( fs_init_done == 0)
    {
        if ( init_sflash( &wicedfs_sflash_handle, 0, SFLASH_WRITE_NOT_ALLOWED ) != WICED_SUCCESS )
        {
            return PLATFORM_ERROR;
        }

#if (DCT_BOOTLOADER_SDK_VERSION < DCT_BOOTLOADER_SDK_3_1_2)
        /* this SDK does not have apps_locations in bootloader_dct_header_t (platform_dct_header_t for the SDK) */
#else
        if ( wiced_framework_app_open( DCT_FILESYSTEM_IMAGE_INDEX, &fs_app ) != WICED_SUCCESS )
        {
            return PLATFORM_ERROR;
        }
#endif /* (DCT_BOOTLOADER_SDK_VERSION < DCT_BOOTLOADER_SDK_3_1_2) */

        result = wicedfs_init( 0, read_callback, &resource_fs_handle, &wicedfs_sflash_handle );
        wiced_assert( "wicedfs init fail", result == 0 );
        fs_init_done = ( result == 0 ) ? 1 : 0;
        return ( result == 0 ) ? PLATFORM_SUCCESS : PLATFORM_ERROR;
    }
    return PLATFORM_SUCCESS;
}

static wicedfs_usize_t read_callback( void* user_param, void* buf, wicedfs_usize_t size, wicedfs_usize_t pos )
{
#if (DCT_BOOTLOADER_SDK_VERSION < DCT_BOOTLOADER_SDK_3_1_2)
    UNUSED_PARAMETER(user_param);
    UNUSED_PARAMETER(buf);
    UNUSED_PARAMETER(size);
    UNUSED_PARAMETER(pos);
            /* this SDK does not have apps_locations in bootloader_dct_header_t (platform_dct_header_t for the SDK) */
    return 0;
#else
    wiced_result_t retval;
    (void) user_param;
    retval = wiced_framework_app_read_chunk( &fs_app, pos, (uint8_t*) buf, size );
    return ( ( WICED_SUCCESS == retval ) ? size : 0 );
#endif /* (DCT_BOOTLOADER_SDK_VERSION < DCT_BOOTLOADER_SDK_3_1_2) */
}

platform_result_t platform_get_sflash_dct_loc( sflash_handle_t* sflash_handle, uint32_t* loc )
{
    UNUSED_PARAMETER( sflash_handle );

    *loc = 0;
    return PLATFORM_SUCCESS;
}

#if !PLATFORM_NO_DDR

static wiced_result_t ddr_block_device_init( wiced_block_device_t* device, wiced_block_device_write_mode_t write_mode )
{
    ddr_block_device_specific_data_t* ddr_specific_data = (ddr_block_device_specific_data_t*) device->device_specific_data;

    if ( ! PLATFORM_FEATURE_ENAB(DDR) )
    {
        return WICED_ERROR;
    }

    if ( device->init_data->base_address_offset == (uint64_t)-1 )
    {
        ddr_specific_data->offset = PLATFORM_DDR_FREE_OFFSET;
    }
    else
    {
        ddr_specific_data->offset = (uint32_t)device->init_data->base_address_offset;
    }
    ddr_specific_data->offset = MIN( ddr_specific_data->offset, platform_ddr_get_size( ) );

    if ( device->init_data->maximum_size == 0 )
    {
        device->device_size = platform_ddr_get_size( ) - ddr_specific_data->offset;
    }
    else
    {
        device->device_size = MIN( platform_ddr_get_size( ) - ddr_specific_data->offset, device->init_data->maximum_size );
    }

    device->device_id        = 0;
    device->erase_block_size = 0;
    device->read_block_size  = 1;
    device->write_block_size = 1;

    ddr_specific_data->write_mode = write_mode;

    device->initialized = WICED_TRUE;

    return WICED_SUCCESS;
}

static wiced_result_t ddr_block_device_deinit( wiced_block_device_t* device )
{
    device->initialized = WICED_FALSE;

    return WICED_SUCCESS;
}

static wiced_result_t ddr_block_write( wiced_block_device_t* device, uint64_t start_address, const uint8_t* data, uint64_t size )
{
    ddr_block_device_specific_data_t* ddr_specific_data = (ddr_block_device_specific_data_t*) device->device_specific_data;
    if ( start_address + size > device->device_size )
    {
        return WICED_BADARG;
    }
    memcpy( (void*)(ptrdiff_t)( DDR_BASE + start_address + ddr_specific_data->offset ), data, (size_t) size );
    return WICED_SUCCESS;
}

static wiced_result_t ddr_block_flush( wiced_block_device_t* device )
{
    UNUSED_PARAMETER( device );
    return WICED_SUCCESS;
}

static wiced_result_t ddr_block_read( wiced_block_device_t* device, uint64_t start_address, uint8_t* data, uint64_t size )
{
    ddr_block_device_specific_data_t* ddr_specific_data = (ddr_block_device_specific_data_t*) device->device_specific_data;
    if ( start_address + size > device->device_size )
    {
        return WICED_BADARG;
    }
    memcpy( data, (void*)(ptrdiff_t)( DDR_BASE + start_address + ddr_specific_data->offset ), (size_t) size );
    return WICED_SUCCESS;
}

static wiced_result_t ddr_block_register_callback( wiced_block_device_t* device, wiced_block_device_status_change_callback_t callback )
{
    UNUSED_PARAMETER( device );
    UNUSED_PARAMETER( callback );
    return WICED_SUCCESS;
}

static wiced_result_t ddr_block_status( wiced_block_device_t* device, wiced_block_device_status_t* status )
{
    UNUSED_PARAMETER( device );
    *status = BLOCK_DEVICE_UP_READ_WRITE;
    return WICED_SUCCESS;
}

const wiced_block_device_driver_t ddr_block_device_driver =
{
    .init                = ddr_block_device_init,
    .deinit              = ddr_block_device_deinit,
    .erase               = NULL,
    .write               = ddr_block_write,
    .flush               = ddr_block_flush,
    .read                = ddr_block_read,
    .register_callback   = ddr_block_register_callback,
    .status              = ddr_block_status,
};

#endif /* !PLATFORM_NO_DDR */
