/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include "internal/wifi_image/wwd_wifi_image_interface.h"

#ifdef OTA_UPGRADE
#include <stdio.h>
#include "stm32f2xx.h"
#include "Platform/wwd_platform_interface.h"
#include "platform.h"
#include "bootloader_app.h"
#include "wifi_nvram_image.h"
#include "wwd_assert.h"

#ifndef MIN
#define MIN(a,b) (((a) < (b))?(a):(b))
#endif

#endif /* ifdef OTA_UPGRADE */




#ifndef OTA_UPGRADE



uint32_t host_platform_read_wifi_firmware( uint32_t offset, /*@out@*/wiced_buffer_t* buffer )
{
    return host_platform_read_memory_wifi_firmware( offset, buffer );
}

uint32_t host_platform_read_wifi_nvram( uint32_t offset, /*@out@*/wiced_buffer_t* buffer )
{
    return host_platform_read_memory_wifi_nvram( offset, buffer );
}

uint32_t host_platform_wifi_nvram_size( void )
{
    return host_platform_memory_wifi_nvram_size( );
}



#else

uint32_t host_platform_read_wifi_firmware( uint32_t offset, /*@out@*/ wiced_buffer_t* buffer )
{
    uint32_t buffer_size = 512;
    uint8_t* packet;
    wiced_result_t result;
    uint32_t read_size;

    do
    {
        result = host_buffer_get( buffer, WICED_NETWORK_TX, (unsigned short) ( buffer_size + sizeof(wiced_buffer_header_t) ), WICED_FALSE );
    } while ( ( result == WICED_BUFFER_UNAVAILABLE_PERMANENT ) &&
              ( ( buffer_size >>= 1 ) > 1 ) );

    if ( result != WICED_SUCCESS )
    {
        WPRINT_PLATFORM_ERROR(("Fatal error: host_platform_read_firmware cannot allocate buffer"));
        return 0;
    }
    packet = (uint8_t*) host_buffer_get_current_piece_data_pointer( *buffer );

    if ( 0 != bootloader_api->platform_read_wifi_firmware( offset, packet + sizeof(wiced_buffer_header_t), host_buffer_get_current_piece_size(*buffer)-sizeof(wiced_buffer_header_t), &read_size ) )
    {
        return 0;
    }

    return read_size;
}

uint32_t host_platform_read_wifi_nvram( uint32_t offset, /*@out@*/ wiced_buffer_t* buffer )
{
    uint32_t buffer_size = 512;
    uint8_t* packet;
    wiced_result_t result;

    do
    {
        result = host_buffer_get( buffer, WICED_NETWORK_TX, (unsigned short) ( buffer_size + sizeof(wiced_buffer_header_t) ), WICED_FALSE );
    } while ( ( result == WICED_BUFFER_UNAVAILABLE_PERMANENT ) &&
              ( ( buffer_size >>= 1 ) > 1 ) );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_PLATFORM_ERROR(("Fatal error: host_platform_read_variables cannot allocate buffer"));
        return 0;
    }
    packet = (uint8_t*) host_buffer_get_current_piece_data_pointer( *buffer );

    buffer_size = MIN(host_buffer_get_current_piece_size(*buffer)-sizeof(wiced_buffer_header_t), (sizeof(wifi_nvram_image) - offset));
    memcpy( packet + sizeof(wiced_buffer_header_t), &wifi_nvram_image[offset], buffer_size );

    return buffer_size;
}


uint32_t host_platform_wifi_nvram_size( void )
{
    return sizeof(wifi_nvram_image);
}


/* dummy firmware array - needed to keep the bootloader app header happy*/
const unsigned char wifi_firmware_image[1] = { 0x00 };
const unsigned long wifi_firmware_image_size = (unsigned long) sizeof(wifi_firmware_image);

#endif /* ifdef OTA_UPGRADE */
