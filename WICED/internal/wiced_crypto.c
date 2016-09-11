/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Cryptographic functions
 *
 *  Provides cryptographic functions for use in applications
 */

#include <string.h>
#include "wiced_crypto.h"
#include "platform/wwd_platform_interface.h"

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

static uint16_t pseudo_random = 0;

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t wiced_crypto_get_random( void* buffer, uint16_t buffer_length )
{
    uint8_t* tmp_buffer_ptr;

    /* Seed the pseudo random number generator. */
    if ( pseudo_random == 0 )
    {
        pseudo_random = (uint16_t)host_platform_get_cycle_count();
    }

    /* Generate random numbers */
    tmp_buffer_ptr = (uint8_t*) buffer;
    while( buffer_length != 0 )
    {
        pseudo_random = (uint16_t)((pseudo_random * 32719 + 3) % 32749);

        *tmp_buffer_ptr = ((uint8_t*)&pseudo_random)[0];
        buffer_length--;
        tmp_buffer_ptr++;

        if ( buffer_length > 0 )
        {
            *tmp_buffer_ptr = ((uint8_t*)&pseudo_random)[1];
            buffer_length--;
            tmp_buffer_ptr++;
        }
    }

    return WICED_SUCCESS;
}

static uint16_t wiced_crypto_add_entropy_hash( const uint8_t* buffer, uint16_t buffer_length )
{
    /* CRC16. Polynomial: x^16 + x^15 + x^2 + 1 (0xa001) */

    uint16_t crc = 0xFFFF;
    int i, j;

    for ( i = 0; i < buffer_length; ++i )
    {
        crc = (uint16_t)( crc ^ buffer[i] );

        for ( j = 0; j < 8; ++j )
        {
            if (crc & 1)
            {
                crc = ( crc >> 1 ) ^ 0xA001;
            }
            else
            {
                crc = ( crc >> 1 );
            }
        }
    }

    return crc;
}

wiced_result_t wiced_crypto_add_entropy( const void* buffer, uint16_t buffer_length )
{
    pseudo_random ^= wiced_crypto_add_entropy_hash( buffer, buffer_length );
    return WICED_SUCCESS;
}
