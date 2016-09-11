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
 *  Cryptographic functions
 *
 *  Provides cryptographic functions for use in applications
 */

#include "wiced_crypto.h"

#include "platform/wwd_platform_interface.h"

#include "wwd_assert.h"

/******************************************************
 *                      Macros
 ******************************************************/

#ifndef WICED_CRYPTO_ADD_CYCLECNT_ENTROPY_EACH_N_BYTE
#define WICED_CRYPTO_ADD_CYCLECNT_ENTROPY_EACH_N_BYTE    1024
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                                     (sizeof(a) / sizeof(a[0]))
#endif

#ifdef WICED_CRYPTO_THREAD_NOT_SAFE
    #define WICED_CRYPTO_THREAD_SAFE_LOCK_STATE(name)
    #define WICED_CRYPTO_THREAD_SAFE_LOCK(name)
    #define WICED_CRYPTO_THREAD_SAFE_UNLOCK(name)
#elif defined( WICED_SAVE_INTERRUPTS )
    #define WICED_CRYPTO_THREAD_SAFE_LOCK_STATE(name)    uint32_t name
    #define WICED_CRYPTO_THREAD_SAFE_LOCK(name)          WICED_SAVE_INTERRUPTS( name )
    #define WICED_CRYPTO_THREAD_SAFE_UNLOCK(name)        WICED_RESTORE_INTERRUPTS( name )
#else
    #define WICED_CRYPTO_THREAD_SAFE_LOCK_STATE(name)
    #define WICED_CRYPTO_THREAD_SAFE_LOCK(name)          WICED_DISABLE_INTERRUPTS()
    #define WICED_CRYPTO_THREAD_SAFE_UNLOCK(name)        WICED_ENABLE_INTERRUPTS()
#endif

#define WICED_CRYPTO_DEFAULT_PRNG                        (&prng_well512)

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

static uint32_t prng_well512_get_random ( void );
static void     prng_well512_add_entropy( const void* buffer, uint16_t buffer_length );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static uint32_t prng_well512_state[ 16 ];
static uint32_t prng_well512_index = 0;

static uint32_t prng_add_cyclecnt_entropy_bytes = WICED_CRYPTO_ADD_CYCLECNT_ENTROPY_EACH_N_BYTE;

static wiced_crypto_prng_t prng_well512 =
{
    .get_random  = &prng_well512_get_random,
    .add_entropy = &prng_well512_add_entropy
};

static wiced_crypto_prng_t *curr_prng = WICED_CRYPTO_DEFAULT_PRNG;

/******************************************************
 *               Function Definitions
 ******************************************************/

static uint32_t crc32_calc( const uint8_t* buffer, uint16_t buffer_length, uint32_t prev_crc32 )
{
    uint32_t crc32 = ~prev_crc32;
    int i;

    for ( i = 0; i < buffer_length; i++ )
    {
        int j;

        crc32 ^= buffer[ i ];

        for ( j = 0; j < 8; j++ )
        {
            if ( crc32 & 0x1 )
            {
                crc32 = ( crc32 >> 1 ) ^ 0xEDB88320;
            }
            else
            {
                crc32 = ( crc32 >> 1 );
            }
        }
    }

    return ~crc32;
}

static uint32_t prng_well512_get_random( void )
{
    /*
     * Implementation of WELL (Well equidistributed long-period linear) pseudorandom number generator.
     * Use WELL512 source code placed by inventor to public domain.
     *
     * This is NOT cryptographically secure pseudorandom number generator (CSPRNG).
     * If need CSPRNG please use third party implementation, for example OpenSSL (it includes CPRNG),
     * FORTUNA algorithm (implementation can be found in PostgresSQL).
     * Current PRNG can be used for seeding them.
     */

    uint32_t a, b, c, d;
    uint32_t result;

    WICED_CRYPTO_THREAD_SAFE_LOCK_STATE( flags );

    WICED_CRYPTO_THREAD_SAFE_LOCK( flags );

    a = prng_well512_state[ prng_well512_index ];
    c = prng_well512_state[ ( prng_well512_index + 13 ) & 15 ];
    b = a ^ c ^ ( a << 16 ) ^ ( c << 15 );
    c = prng_well512_state[ ( prng_well512_index + 9 ) & 15 ];
    c ^= ( c >> 11 );
    a = prng_well512_state[ prng_well512_index ] = b ^ c;
    d = a ^ ( ( a << 5 ) & (uint32_t)0xDA442D24UL );
    prng_well512_index = ( prng_well512_index + 15 ) & 15;
    a = prng_well512_state[ prng_well512_index ];
    prng_well512_state[ prng_well512_index ] = a ^ b ^ d ^ ( a << 2 ) ^ ( b << 18 ) ^ ( c << 28 );

    result = prng_well512_state[ prng_well512_index ];

    WICED_CRYPTO_THREAD_SAFE_UNLOCK( flags );

    return result;
}

static void prng_well512_add_entropy( const void* buffer, uint16_t buffer_length )
{
    uint32_t crc32[ ARRAYSIZE( prng_well512_state ) ];
    uint32_t curr_crc32 = 0;
    unsigned i;

    WICED_CRYPTO_THREAD_SAFE_LOCK_STATE( flags );

    for ( i = 0; i < ARRAYSIZE( crc32 ); i++ )
    {
        curr_crc32 = crc32_calc( buffer, buffer_length, curr_crc32 );
        crc32[ i ] = curr_crc32;
    }

    WICED_CRYPTO_THREAD_SAFE_LOCK( flags );

    for ( i = 0; i < ARRAYSIZE( prng_well512_state ); i++ )
    {
        prng_well512_state[ i ] ^= crc32[ i ];
    }

    WICED_CRYPTO_THREAD_SAFE_UNLOCK( flags );
}

static wiced_bool_t prng_is_add_cyclecnt_entropy( uint16_t buffer_length )
{
    wiced_bool_t add_entropy = WICED_FALSE;

    WICED_CRYPTO_THREAD_SAFE_LOCK_STATE( flags );

    WICED_CRYPTO_THREAD_SAFE_LOCK( flags );

    if ( prng_add_cyclecnt_entropy_bytes >= WICED_CRYPTO_ADD_CYCLECNT_ENTROPY_EACH_N_BYTE )
    {
        prng_add_cyclecnt_entropy_bytes %= WICED_CRYPTO_ADD_CYCLECNT_ENTROPY_EACH_N_BYTE;
        add_entropy = WICED_TRUE;
    }

    prng_add_cyclecnt_entropy_bytes += buffer_length;

    WICED_CRYPTO_THREAD_SAFE_UNLOCK( flags );

    return add_entropy;
}

static void prng_add_cyclecnt_entropy( uint16_t buffer_length )
{
    if ( prng_is_add_cyclecnt_entropy( buffer_length ) )
    {
        uint32_t cycle_count = host_platform_get_cycle_count( );
        curr_prng->add_entropy( &cycle_count, sizeof( cycle_count ) );
    }
}

wiced_result_t wiced_crypto_get_random( void* buffer, uint16_t buffer_length )
{
    uint8_t* p = buffer;

    prng_add_cyclecnt_entropy( buffer_length );

    while ( buffer_length != 0 )
    {
        uint32_t rnd_val = curr_prng->get_random( );
        int      i;

        for ( i = 0; i < 4; i++ )
        {
            *p++ = (uint8_t)( rnd_val & 0xFF );
            if ( --buffer_length == 0 )
            {
                break;
            }
            rnd_val = ( rnd_val >> 8 );
        }
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_crypto_add_entropy( const void* buffer, uint16_t buffer_length )
{
    curr_prng->add_entropy( buffer, buffer_length );
    return WICED_SUCCESS;
}

wiced_result_t wiced_crypto_set_prng( wiced_crypto_prng_t* prng )
{
    curr_prng = ( prng == NULL ) ? WICED_CRYPTO_DEFAULT_PRNG : prng;
    return WICED_SUCCESS;
}
