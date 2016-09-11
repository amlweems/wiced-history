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
 *  Block device driver for FileX
 *  Provides the functions which are called by the bottom
 *  of FileX
 */

#include <stddef.h>
#include "tx_api.h"
#include "fx_api.h"
#include "wiced_block_device.h"
#include "wiced_filex.h"

/******************************************************
 *                      Macros
 ******************************************************/

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
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

VOID  wiced_filex_driver( FX_MEDIA* media_ptr )
{
    /* Get a handle to the block device */
    wiced_block_device_t* device = (wiced_block_device_t*) media_ptr -> fx_media_driver_info;

    media_ptr -> fx_media_driver_status =  FX_ACCESS_ERROR;

    /* Process the driver request specified in the media control block.  */
    switch(media_ptr -> fx_media_driver_request)
    {

        case FX_DRIVER_INIT:
        {
            /* FileX is requesting to init the underlying block device */
            wiced_result_t result;

            result = device->driver->init( device, FILEX_WRITE_STRATEGY );
            if ( result != WICED_SUCCESS )
            {
                break;
            }

            /* Inform FileX whether the block device needs to know when sectors become free */
            if ( device->erase_block_size != BLOCK_DEVICE_ERASE_NOT_REQUIRED )
            {
                media_ptr ->fx_media_driver_free_sector_update = FX_TRUE;
            }

            /* Inform FileX whether the block device is write protected */
            if ( device->write_block_size == BLOCK_DEVICE_WRITE_NOT_ALLOWED )
            {
                media_ptr ->fx_media_driver_write_protect = FX_TRUE;
            }

            media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            break;
        }

        case FX_DRIVER_UNINIT:
        {
            /* FileX is requesting to de-init the underlying block device */
            wiced_result_t result;

            result = device->driver->deinit( device );
            if ( result == WICED_SUCCESS )
            {
                media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            }

            break;
        }

        case FX_DRIVER_READ:
        {
            /* FileX is requesting to read from the underlying block device */
            wiced_result_t result;
            uint64_t requested_address = (uint64_t)(media_ptr -> fx_media_driver_logical_sector + media_ptr -> fx_media_hidden_sectors) * media_ptr -> fx_media_bytes_per_sector;
            uint64_t requested_size    = (uint64_t)(media_ptr -> fx_media_driver_sectors) * media_ptr -> fx_media_bytes_per_sector;

            result = device->driver->read( device, requested_address, media_ptr -> fx_media_driver_buffer, requested_size );

            if ( result == WICED_SUCCESS )
            {
                /* Successful driver request.  */
                media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            }
            break;
        }

        case FX_DRIVER_WRITE:
        {
            /* FileX is requesting to write to the underlying block device */
            wiced_result_t result;
            uint64_t requested_address = (uint64_t)(media_ptr -> fx_media_driver_logical_sector + media_ptr -> fx_media_hidden_sectors) * media_ptr -> fx_media_bytes_per_sector;
            uint64_t requested_size    = (uint64_t)(media_ptr -> fx_media_driver_sectors) * media_ptr -> fx_media_bytes_per_sector;

            result = device->driver->write( device, requested_address, media_ptr -> fx_media_driver_buffer, requested_size );

            if ( result == WICED_SUCCESS )
            {
                /* Successful driver request.  */
                media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            }
            break;
        }

        case FX_DRIVER_FLUSH:
        {
            /* FileX is requesting to flush the underlying block device */
            wiced_result_t result;

            /* Check whether flush is implemented on the device */
            if ( device->driver->flush == NULL )
            {
                /* Flush not implemented on this block device */
                media_ptr -> fx_media_driver_status =  FX_SUCCESS;
                break;
            }

            /* Perform the flush */
            result = device->driver->flush( device );

            if ( result == WICED_SUCCESS )
            {
                /* Successful driver request.  */
                media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            }
            break;
        }

        case FX_DRIVER_ABORT:
        {
            /* Abort is not implemented.  */
            media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            break;
        }

        case FX_DRIVER_BOOT_READ:
        {
            /* FileX is requesting to read the boot sector of the underlying block device */
            wiced_result_t result;
            UINT        bytes_per_sector;

            /* Read the first 16 bytes (at least)  */
            result = device->driver->read( device, 0, media_ptr -> fx_media_driver_buffer, MAX(16, device->read_block_size) );
            if ( result != WICED_SUCCESS )
            {
                break;
            }

            /* Check whether the boot record is valid.  */
            if ((media_ptr -> fx_media_driver_buffer[0] != (UCHAR) 0xEB) ||
                (media_ptr -> fx_media_driver_buffer[1] != (UCHAR) 0x34) ||
                (media_ptr -> fx_media_driver_buffer[2] != (UCHAR) 0x90))
            {

                /* Invalid boot record, return an error!  */
                media_ptr -> fx_media_driver_status =  FX_MEDIA_INVALID;
                return;
            }

            /* Pickup the bytes per sector.  */
            bytes_per_sector =  _fx_utility_16_unsigned_read(&media_ptr -> fx_media_driver_buffer[FX_BYTES_SECTOR]);

            /* Ensure this is less than the destination.  */
            /* Read the boot sector into the destination.  */
            result = device->driver->read( device, 0, media_ptr -> fx_media_driver_buffer, MIN( bytes_per_sector, media_ptr -> fx_media_memory_size ) );
            if ( result != WICED_SUCCESS )
            {
                break;
            }

            /* Successful driver request.  */
            media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            break;
        }

        case FX_DRIVER_BOOT_WRITE:
        {
            /* FileX is requesting to write to the underlying block device */
            wiced_result_t result;

            /* Write the boot record and return to the caller.  */
            result = device->driver->write( device, 0, media_ptr -> fx_media_driver_buffer, media_ptr -> fx_media_bytes_per_sector );
            if ( result == WICED_SUCCESS )
            {
                /* Successful driver request.  */
                media_ptr -> fx_media_driver_status =  FX_SUCCESS;
            }

            break ;
        }

        default:
        {

            /* Invalid driver request.  */
            media_ptr -> fx_media_driver_status =  FX_IO_ERROR;
            break;
        }
    }
}

