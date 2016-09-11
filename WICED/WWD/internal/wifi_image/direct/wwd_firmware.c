/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "wwd_assert.h"
#include "wifi_nvram_image.h"
#include "wwd_bus_protocol.h"
#include "internal/wwd_internal.h"
#include "internal/Bus_protocols/wwd_bus_protocol_interface.h"
#include "chip_constants.h"
#include "internal/wifi_image/wwd_wifi_image_interface.h"


#ifdef WICED_BUS_HAS_HEADER
#error Direct firmware transfer cannot be used with a bus that requires a header, since buffering is needed to provide header space
#endif /* ifdef WICED_BUS_HAS_HEADER */


/******************************************************
 *             Constants
 ******************************************************/

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#define MAX_TRANSFER_SIZE     (16*1024 )

/******************************************************
 *             Structures
 ******************************************************/

/******************************************************
 *             Variables
 ******************************************************/

/******************************************************
 *             Function declarations
 ******************************************************/

static wiced_result_t write_image( const uint8_t* image, uint32_t image_size, uint32_t address );

/******************************************************
 *             Function definitions
 ******************************************************/

wiced_result_t wiced_write_wifi_firmware_image( void )
{
    return write_image( (uint8_t*) wifi_firmware_image, (uint32_t) wifi_firmware_image_size, 0 );
}

wiced_result_t wiced_write_wifi_nvram_image( void )
{
    wiced_result_t result;
    /* Round up image size to next 64 byte block */
    uint32_t temp = ( sizeof(wifi_nvram_image) + 63 ) & ( (uint32_t) ~63 );


    /* Write image */
    if ( WICED_SUCCESS != ( result = write_image( (uint8_t*) wifi_nvram_image, temp, CHIP_RAM_SIZE - 4 - temp ) ) )
    {
        return result;
    }

    /* Write the variable image size at the end */
    temp = ( ~( temp / 4 ) << 16 ) | ( temp / 4 );
    if ( WICED_SUCCESS != ( result = wiced_set_backplane_window( (uint32_t) ( CHIP_RAM_SIZE - 4 ) ) ) )
    {
        return result;
    }

    if ( WICED_SUCCESS != ( result = wiced_bus_transfer_bytes( BUS_WRITE, BACKPLANE_FUNCTION, (uint32_t) ( ( CHIP_RAM_SIZE - 4 ) & BACKPLANE_ADDRESS_MASK ), (uint16_t) 4, (wiced_transfer_bytes_packet_t*) &temp ) ) )
    {
        return result;
    }

#if 0
    {
        /* Verify NVRAM size word */
        uint32_t temp2 = 0;
        if ( WICED_SUCCESS != ( result = wiced_bus_transfer_bytes( BUS_READ, BACKPLANE_FUNCTION, (uint32_t) ( ( CHIP_RAM_SIZE - 4 ) & BACKPLANE_ADDRESS_MASK ), 4, (wiced_transfer_bytes_packet_t*) &temp2 ) ) )
        {
            return result;
        }
        if ( temp != temp2 )
        {
            /* Verify failed */
            WPRINT_WWD_ERROR(("Verify of NVRAM size failed"));
        }
    }
#endif /* if 0 */
     return WICED_SUCCESS;
}

static wiced_result_t write_image( const uint8_t* image, uint32_t image_size, uint32_t address )
{
    uint32_t transfer_progress;
    uint16_t transfer_size;
    wiced_result_t result;

    for ( transfer_progress = 0; transfer_progress < image_size; transfer_progress += transfer_size, address += transfer_size, image += transfer_size )
    {
        /* Set the backplane window */
        if ( WICED_SUCCESS != ( result = wiced_set_backplane_window( address ) ) )
        {
            return result;
        }
        transfer_size = (uint16_t) MIN( MAX_TRANSFER_SIZE, (int) ( image_size - transfer_progress ) );
        /* Round up to next 64 byte chunk */
        transfer_size = (uint16_t) ( transfer_size + 63 );
        transfer_size = (uint16_t) ( transfer_size & ~63 );
        if ( WICED_SUCCESS != ( result = wiced_bus_transfer_bytes( BUS_WRITE, BACKPLANE_FUNCTION, address & BACKPLANE_ADDRESS_MASK, transfer_size, (wiced_transfer_bytes_packet_t*) image ) ) )
        {
            return result;
        }
#if 0
        {
            /* TODO: THIS VERIFY CODE IS CURRENTLY BROKEN - ONLY CHECKS 64 BYTES, NOT 16KB */
            /* Verify download of image data */
            uint8_t tmpbuff[64];
            if ( WICED_SUCCESS != ( result = wiced_bus_transfer_bytes( BUS_READ, BACKPLANE_FUNCTION, address & BACKPLANE_ADDRESS_MASK, 64, (wiced_transfer_bytes_packet_t*)tmpbuff ) ) )
            {
                return result;
            }
            if ( 0 != memcmp( tmpbuff, image, (size_t) 64 ) )
            {
                /* Verify failed */
                WPRINT_WWD_ERROR(("Verify of firmware/NVRAM image failed"));
            }
        }
#endif /* if 0 */
    }
    return WICED_SUCCESS;
}
