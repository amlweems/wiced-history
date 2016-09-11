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
 *
 */
#include "wiced_result.h"
#include "besl_host_interface.h"
#include "wwd_structures.h"
#include "wiced_utilities.h"
#include "wwd_wifi.h"
#include "wwd_crypto.h"
#include "internal/wwd_sdpcm.h"
#include "besl_host.h"
#include "internal/wwd_bcmendian.h"
#include <string.h>
#include "network/wwd_buffer_interface.h"
#include "wwd_assert.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define is_digit(c) ((c >= '0') && (c <= '9'))
#define CHECK_IOCTL_BUFFER( buff )  if ( buff == NULL ) {  wiced_assert("Allocation failed\n", 0 == 1); return BESL_BUFFER_ALLOC_FAIL; }

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

void* besl_host_malloc( char* name, uint32_t size )
{
    BESL_DEBUG(("besl_host_malloc: %s %u\r\n", name, (unsigned int)size));
    return malloc_named( name, size );
}

void* besl_host_calloc( char* name, uint32_t num, uint32_t size )
{
    void *ptr;
    ptr = besl_host_malloc( name, num * size );
    if ( ptr != NULL )
    {
        memset(ptr, 0, num * size);
    }
    return ptr;
}

void besl_host_free( void* p )
{
    free( p );
}

besl_result_t besl_host_get_mac_address(besl_mac_t* address, uint32_t interface )
{
    wiced_buffer_t buffer;
    wiced_buffer_t response;
    wwd_result_t result;
    uint32_t*      data;

    data = wwd_sdpcm_get_iovar_buffer( &buffer, sizeof(wiced_mac_t) + sizeof(uint32_t), IOVAR_STR_BSSCFG_CUR_ETHERADDR );
    CHECK_IOCTL_BUFFER( data );
    *data = interface;

    result = wwd_sdpcm_send_iovar( SDPCM_GET, buffer, &response, WWD_STA_INTERFACE );
    if ( result != WWD_SUCCESS )
    {
        memset( address->octet, 0, sizeof(wiced_mac_t) );
        return (besl_result_t) result;
    }
    memcpy( address, host_buffer_get_current_piece_data_pointer( response ), sizeof(wiced_mac_t) );
    host_buffer_release( response, WWD_NETWORK_RX );

    return WICED_SUCCESS;
}

besl_result_t besl_host_set_mac_address(besl_mac_t* address, uint32_t interface )
{
    wiced_buffer_t buffer;
    uint32_t*      data;

    data = wwd_sdpcm_get_iovar_buffer( &buffer, sizeof(wiced_mac_t) + sizeof(uint32_t), "bsscfg:" IOVAR_STR_CUR_ETHERADDR );
    CHECK_IOCTL_BUFFER( data );
    data[0] = interface;
    memcpy(&data[1], address, sizeof(wiced_mac_t));

    return (besl_result_t) wwd_sdpcm_send_iovar( SDPCM_SET, buffer, NULL, WWD_STA_INTERFACE );
}

void besl_host_random_bytes( uint8_t* buffer, uint16_t buffer_length )
{
    wwd_wifi_get_random( buffer, buffer_length );
}

void besl_host_get_time(besl_time_t* time)
{
    *time = (besl_time_t)host_rtos_get_time();
}

uint32_t besl_host_hton32(uint32_t intlong)
{
    return htobe32(intlong);
}

uint16_t besl_host_htol16(uint16_t intshort)
{
    return intshort;
}

uint16_t besl_host_hton16_ptr(uint8_t * in, uint8_t * out)
{
    uint16_t temp;
    temp = BESL_READ_16(in);
    temp = htobe16(temp);
    BESL_WRITE_16(out, temp);
    return temp;
}

uint16_t besl_host_hton16(uint16_t intshort)
{
    return htobe16(intshort);
}

uint16_t besl_host_ltoh16(uint16_t intshort)
{
    return ntoh16(intshort);
}

uint32_t besl_host_hton32_ptr(uint8_t * in, uint8_t * out)
{
    uint32_t temp;
    temp = BESL_READ_32(in);
    temp = htobe32(temp);
    BESL_WRITE_32(out, temp);
    return temp;
}


void besl_host_hex_bytes_to_chars( char* cptr, const uint8_t* bptr, uint32_t blen )
{
    int i,j;
    uint8_t temp;

    i = 0;
    j = 0;
    while( i < blen )
    {
        // Convert first nibble of byte to a hex character
        temp = bptr[i] / 16;
        if ( temp < 10 )
        {
            cptr[j] = temp + '0';
        }
        else
        {
            cptr[j] = (temp - 10) + 'A';
        }
        // Convert second nibble of byte to a hex character
        temp = bptr[i] % 16;
        if ( temp < 10 )
        {
            cptr[j+1] = temp + '0';
        }
        else
        {
            cptr[j+1] = (temp - 10) + 'A';
        }
        i++;
        j+=2;
    }
}
