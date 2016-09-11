/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "wwd_assert.h"
#include "wwd_bus_protocol.h"
#include "internal/wwd_internal.h"
#include "internal/wifi_image/wwd_wifi_image_interface.h"
#include "internal/Bus_protocols/wwd_bus_protocol_interface.h"
#include "chip_constants.h"
#include "Platform/wwd_platform_interface.h"
#include "Network/wwd_buffer_interface.h"
#include "string.h"
#include "wiced_utilities.h"

#ifndef OTA_UPGRADE
#include "wifi_nvram_image.h"
#endif /* ifndef OTA_UPGRADE */

/******************************************************
 *             Constants
 ******************************************************/

#define MAX_TRANSFER_SIZE     (64)

/******************************************************
 *             Structures
 ******************************************************/

/******************************************************
 *             Variables
 ******************************************************/

/******************************************************
 *             Function declarations
 ******************************************************/

/******************************************************
 *             Function definitions
 ******************************************************/

wiced_result_t wiced_write_wifi_firmware_image( void )
{
    uint32_t transfer_progress;
    uint16_t transfer_size;
    uint32_t segment_size;
    uint8_t* packet = NULL;
    wiced_buffer_t buffer = NULL;
    wiced_result_t result = WICED_ERROR;

    /* Transfer firmware image into the RAM */
    transfer_progress = 0;
    segment_size = host_platform_read_wifi_firmware( transfer_progress, &buffer );
    if ( buffer != NULL )
    {
        packet = (uint8_t*) host_buffer_get_current_piece_data_pointer( buffer );
    }

    while ( segment_size != 0)
    {
        transfer_size = (uint16_t) MIN( MAX_TRANSFER_SIZE, segment_size );
        result        = wiced_set_backplane_window( transfer_progress );
        if ( result != WICED_SUCCESS )
        {
            break;
        }

        result = wiced_bus_transfer_bytes( BUS_WRITE, BACKPLANE_FUNCTION, ( transfer_progress & BACKPLANE_ADDRESS_MASK ), transfer_size, (wiced_transfer_bytes_packet_t*) (packet + sizeof(wiced_buffer_queue_ptr_t)) );
        if ( result != WICED_SUCCESS )
        {
            break;
        }

        segment_size      -= transfer_size;
        packet            += transfer_size;
        transfer_progress += transfer_size;

        if ( segment_size == 0 )
        {
            host_buffer_release( buffer, WICED_NETWORK_TX );
            buffer       = NULL;
            segment_size = host_platform_read_wifi_firmware( transfer_progress, &buffer );
            if ( buffer != NULL )
            {
                packet = (uint8_t*) host_buffer_get_current_piece_data_pointer( buffer );
            }
            else
            {
                result = WICED_ERROR;
            }
        }
    }

    if ( buffer != NULL )
    {
        host_buffer_release( buffer, WICED_NETWORK_TX );
    }

    return result;
}

wiced_result_t wiced_write_wifi_nvram_image( void )
{
    uint32_t transfer_progress;
    uint32_t device_ram_address;
    uint16_t transfer_size;
    uint32_t segment_size;
    uint8_t* data;
    wiced_buffer_t buffer;
    uint32_t image_size;
    wiced_result_t result;
    uint32_t temp_dword;

    /* Get the size of the variable image and round it up to the next 64 bytes boundary */
    image_size = ROUND_UP(host_platform_wifi_nvram_size(), (uint32_t)MAX_TRANSFER_SIZE);

    /* Transfer the variable image into the end of the RAM */
    device_ram_address = CHIP_RAM_SIZE - 4 - image_size;
    for ( transfer_progress = 0, segment_size = host_platform_read_wifi_nvram( transfer_progress, &buffer ); segment_size != 0; host_buffer_release( buffer, WICED_NETWORK_TX ), segment_size = host_platform_read_wifi_nvram( transfer_progress, &buffer ) )
    {
        data = host_buffer_get_current_piece_data_pointer( buffer );
        for ( ; segment_size != 0; segment_size -= transfer_size, data += transfer_size, transfer_progress += transfer_size, device_ram_address += transfer_size )
        {
            transfer_size = (uint16_t) MIN(MAX_TRANSFER_SIZE, segment_size);
            result = wiced_set_backplane_window( device_ram_address );
            if ( result != WICED_SUCCESS )
            {
                host_buffer_release( buffer, WICED_NETWORK_TX );
                return result;
            }
            result = wiced_bus_transfer_bytes( BUS_WRITE, BACKPLANE_FUNCTION, ( device_ram_address & BACKPLANE_ADDRESS_MASK ), transfer_size, (wiced_transfer_bytes_packet_t*) ( data + sizeof(wiced_buffer_queue_ptr_t) ) );
            if ( result != WICED_SUCCESS )
            {
                host_buffer_release( buffer, WICED_NETWORK_TX );
                return result;
            }
        }
    }
    host_buffer_release( buffer, WICED_NETWORK_TX );

    /* Write the variable image size with binary inverse */
    device_ram_address = CHIP_RAM_SIZE - 4;
    result = wiced_set_backplane_window( device_ram_address );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }
    host_buffer_get( &buffer, WICED_NETWORK_TX, 4 + WICED_BUS_HEADER_SIZE, WICED_TRUE );
    data = host_buffer_get_current_piece_data_pointer( buffer );
    temp_dword = ( ~( image_size / 4 ) << 16 ) | ( image_size / 4 );
    memcpy( data + WICED_BUS_HEADER_SIZE, &temp_dword, 4 );
    result = wiced_bus_transfer_bytes( BUS_WRITE, BACKPLANE_FUNCTION, ( device_ram_address & BACKPLANE_ADDRESS_MASK ), 4,  (wiced_transfer_bytes_packet_t*) data );
    host_buffer_release( buffer, WICED_NETWORK_TX );
    return result;
}


#ifndef OTA_UPGRADE

uint32_t host_platform_read_memory_wifi_firmware( uint32_t offset, /*@out@*/wiced_buffer_t* buffer )
{
    uint32_t buffer_size = 512;
    uint8_t* packet;
    wiced_result_t result;

    do
    {
        result = host_buffer_get( buffer, WICED_NETWORK_TX, (unsigned short) ( buffer_size + sizeof(wiced_buffer_header_t) ), WICED_FALSE );
    } while ( ( result == WICED_BUFFER_UNAVAILABLE_PERMANENT ) && ( ( buffer_size >>= 1 ) > 1 ) );

    if ( result != WICED_SUCCESS )
    {
        WPRINT_WWD_ERROR(("Fatal error: host_platform_read_firmware cannot allocate buffer"));
        return 0;
    }
    packet = (uint8_t*) host_buffer_get_current_piece_data_pointer( *buffer );

    buffer_size = MIN(host_buffer_get_current_piece_size(*buffer)-sizeof(wiced_buffer_header_t), (wifi_firmware_image_size - offset));
    memcpy( packet + sizeof(wiced_buffer_header_t), &wifi_firmware_image[offset], buffer_size );

    return buffer_size;
}

uint32_t host_platform_read_memory_wifi_nvram( uint32_t offset, /*@out@*/wiced_buffer_t* buffer )
{
    uint32_t buffer_size = 512;
    uint8_t* packet;
    wiced_result_t result;

    do
    {
        result = host_buffer_get( buffer, WICED_NETWORK_TX, (unsigned short) ( buffer_size + sizeof(wiced_buffer_header_t) ), WICED_FALSE );
    } while ( ( result == WICED_BUFFER_UNAVAILABLE_PERMANENT ) && ( ( buffer_size >>= 1 ) > 1 ) );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_WWD_ERROR(("Fatal error: host_platform_read_variables cannot allocate buffer"));
        return 0;
    }
    packet = (uint8_t*) host_buffer_get_current_piece_data_pointer( *buffer );

    buffer_size = MIN(host_buffer_get_current_piece_size(*buffer)-sizeof(wiced_buffer_header_t), (sizeof(wifi_nvram_image) - offset));
    memcpy( packet + sizeof(wiced_buffer_header_t), &wifi_nvram_image[offset], buffer_size );

    return buffer_size;
}

uint32_t host_platform_memory_wifi_nvram_size( void )
{
    return sizeof( wifi_nvram_image );
}

#endif /* ifndef OTA_UPGRADE */
