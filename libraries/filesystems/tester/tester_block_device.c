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
 *  Tester tool for WicedFS
 *
 *  Creates an WicedFS image file from a specified host PC
 *  directory, then uses the WicedFS API to read back
 *  the data and compare it to the files/directories
 *  on the ost PC
 */

#include <errno.h>
#include <string.h>
#include "wiced_filesystem.h"
#include "tester.h"


#define TESTER_DEVICE_SIZE ((uint64_t) 0x0fffffff)

static wicedfs_usize_t hostfile_wicedfs_read( void* user_param, void* buf, wicedfs_usize_t size, wicedfs_usize_t pos )
{
    /* Seek to the requested hardware location in the image file */
    if ( 0 != fseeko64( (FILE*)user_param, (off64_t)(pos), SEEK_SET) )
    {
        return 0;
    }

    /* Read the requested data from the image file */
    return (wicedfs_usize_t) fread( buf, 1, (size_t)size, (FILE*)user_param );
}



static wiced_result_t tester_block_device_init( wiced_block_device_t* device, wiced_block_device_write_mode_t write_mode )
{
    tester_block_device_specific_data_t* tester_specific_data = (tester_block_device_specific_data_t*) device->device_specific_data;

    if ( device->initialized == WICED_TRUE )
    {
        return WICED_SUCCESS;
    }

    /* Open the WicedFS image file for reading back */
    tester_specific_data->image_file_handle = fopen( tester_specific_data->filename, "rb+" );
    if ( tester_specific_data->image_file_handle == NULL )
    {
        /* Does not exist - try creating it */
        tester_specific_data->image_file_handle = fopen( tester_specific_data->filename, "wb+" );
        if ( tester_specific_data->image_file_handle == NULL )
        {
            printf( "Error opening file system image file %s %d\n", tester_specific_data->filename, errno );
            return WICED_ERROR;
        }
    }

    device->device_id        = 0;
    if ( device->init_data->maximum_size == 0 )
    {
        device->device_size = TESTER_DEVICE_SIZE;
    }
    else
    {
        device->device_size = MIN( TESTER_DEVICE_SIZE, device->init_data->maximum_size );
    }

    device->erase_block_size = 0;
    device->read_block_size  = 1;
    device->write_block_size = 1;

    tester_specific_data->write_mode = write_mode;

    device->initialized = WICED_TRUE;

    return WICED_SUCCESS;
}

static wiced_result_t tester_block_device_deinit( wiced_block_device_t* device )
{
    tester_block_device_specific_data_t* tester_specific_data = (tester_block_device_specific_data_t*) device->device_specific_data;

    fclose( tester_specific_data->image_file_handle );

    device->initialized = WICED_FALSE;

    return WICED_SUCCESS;
}

static wiced_result_t tester_block_write( wiced_block_device_t* device, uint64_t start_address, const uint8_t* data, uint64_t size )
{
    size_t items_written;
    tester_block_device_specific_data_t* tester_specific_data = (tester_block_device_specific_data_t*) device->device_specific_data;

    if ( start_address + size > device->device_size )
    {
        return WICED_BADARG;
    }

    /* Seek to the requested hardware location in the image file */
    if ( 0 != fseeko64( tester_specific_data->image_file_handle, (off64_t)(start_address), SEEK_SET) )
    {
        return 0;
    }

    /* Write the requested data to the image file */
    items_written = fwrite( data, 1, (size_t)size, tester_specific_data->image_file_handle );

    if ( items_written != size )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

static wiced_result_t tester_block_flush( wiced_block_device_t* device )
{
    tester_block_device_specific_data_t* tester_specific_data = (tester_block_device_specific_data_t*) device->device_specific_data;

    fflush( tester_specific_data->image_file_handle );

    return WICED_SUCCESS;
}

static wiced_result_t tester_block_read( wiced_block_device_t* device, uint64_t start_address, uint8_t* data, uint64_t size )
{
    size_t items_read;
    tester_block_device_specific_data_t* tester_specific_data = (tester_block_device_specific_data_t*) device->device_specific_data;

    if ( start_address + size > device->device_size )
    {
        return WICED_BADARG;
    }

    /* Seek to the requested hardware location in the image file */
    if ( 0 != fseeko64( tester_specific_data->image_file_handle, (off64_t)(start_address), SEEK_SET) )
    {
        return 0;
    }

    /* Read the requested data from the image file */
    items_read = fread( data, 1, (size_t)size, tester_specific_data->image_file_handle );

    if ( feof( tester_specific_data->image_file_handle ) )
    {
        /* File has not been expanded to this size yet - just return blank data */
        memset( data, 0, size );
        return WICED_SUCCESS;
    }

    if ( items_read != size )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

static wiced_result_t tester_block_register_callback( wiced_block_device_t* device, wiced_block_device_status_change_callback_t callback )
{
    UNUSED_PARAMETER( device );
    UNUSED_PARAMETER( callback );
    return WICED_SUCCESS;
}

static wiced_result_t tester_block_status( wiced_block_device_t* device, wiced_block_device_status_t* status )
{
    UNUSED_PARAMETER( device );
    *status = BLOCK_DEVICE_UP_READ_WRITE;
    return WICED_SUCCESS;
}

const wiced_block_device_driver_t tester_block_device_driver =
{
    .init                = tester_block_device_init,
    .deinit              = tester_block_device_deinit,
    .erase               = NULL,
    .write               = tester_block_write,
    .flush               = tester_block_flush,
    .read                = tester_block_read,
    .register_callback   = tester_block_register_callback,
    .status              = tester_block_status,
};
