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
 *
 */
#include "wwd_constants.h"
#include "wiced_tls.h"
#include "wiced_utilities.h"
#include "tls_host_api.h"
#include "wiced_crypto.h"
#include "crypto_constants.h"
#include "crypto_structures.h"
#include "wiced_time.h"
#include "wwd_assert.h"
#include "network/wwd_buffer_interface.h"
#include "besl_host_interface.h"
#include "wiced_security.h"
#include "internal/wiced_internal_api.h"
#include "wiced_tcpip_tls_api.h"
#include "wiced_tcpip.h"
#include "x509.h"
#include <string.h>
#include "tls_cipher_suites.h"

#ifndef DISABLE_EAP_TLS
#include "wiced_supplicant.h"
#endif /* DISABLE_EAP_TLS */

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#ifndef WICED_TLS_MAX_RESUMABLE_SESSIONS
#define WICED_TLS_MAX_RESUMABLE_SESSIONS    4
#endif

/* Maximum number of session reconnects */
#define MAX_TLS_SESSION_AGE     32

#define SSL_IS_CLIENT            0
#define SSL_IS_SERVER            1
#define SESSION_CAN_BE_RESUMED   1
#define SESSION_NO_TIMEOUT       0

#define MAX_HANDSHAKE_WAIT  20000

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

static int32_t tls_get_session( ssl_context *ssl );
static int32_t tls_set_session( ssl_context *ssl );
static wiced_result_t tls_packetize_buffered_data ( wiced_tls_workspace_t* context, wiced_packet_t** packet );
#ifndef DISABLE_EAP_TLS
static wiced_result_t tls_eap_buffered_data       ( wiced_tls_workspace_t* context, besl_packet_t* packet );
#endif /* DISABLE_EAP_TLS */


static besl_result_t wiced_tls_load_certificate( wiced_tls_certificate_t* certificate, const uint8_t* certificate_data, uint32_t certificate_length, wiced_tls_certificate_format_t certificate_format );
static besl_result_t wiced_tls_load_key( wiced_tls_key_t* key, const char* key_string );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static ssl_session session_list[WICED_TLS_MAX_RESUMABLE_SESSIONS];
static int session_list_initialized = 0;

/**
 * This list of cipher suites should be in order of strength with the strongest first.
 * Do not enable cipher suites unless they meet your security requirements
 */
static const cipher_suite_t* my_ciphers[] =
{
        /* First: Ephemeral Diffie Hellman with 256 bit cipher, in order of MAC strength */
#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA256_MAC )
        &TLS_DHE_RSA_WITH_AES_256_CBC_SHA256,
#endif
#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA256_MAC )
        &TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA256,
#endif
#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
        &TLS_DHE_RSA_WITH_AES_256_CBC_SHA,
#endif
#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
        &TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA,
#endif

        /* Next: Ephemeral Diffie Hellman with 128 bit cipher, in order of MAC strength */
#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
        &TLS_DHE_RSA_WITH_AES_128_CBC_SHA256,
#endif
#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
        &TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA256,
#endif
#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
        &TLS_DHE_RSA_WITH_AES_128_CBC_SHA,
#endif
#if defined( USE_DHE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
        &TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA,
#endif

        /* Next: Regular Diffie Hellman with 256 bit cipher, in order of MAC strength */
#if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA256_MAC )
        &TLS_RSA_WITH_AES_256_CBC_SHA256,
#endif
#if defined( USE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA256_MAC )
        &TLS_RSA_WITH_CAMELLIA_256_CBC_SHA256,
#endif
#if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
        &TLS_RSA_WITH_AES_256_CBC_SHA,
#endif
#if defined( USE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
        &TLS_RSA_WITH_CAMELLIA_256_CBC_SHA,
#endif

        /* Next: Regular Diffie Hellman with 128 bit cipher, in order of MAC strength */
#if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
        &TLS_RSA_WITH_AES_128_CBC_SHA256,
#endif
#if defined( USE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
        &TLS_RSA_WITH_CAMELLIA_128_CBC_SHA256,
#endif
#if defined( USE_RSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
        &TLS_RSA_WITH_AES_128_CBC_SHA,
#endif
#if defined( USE_RSA_KEYSCHEME ) && defined( USE_CAMELLIA_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
        &TLS_RSA_WITH_CAMELLIA_128_CBC_SHA,
#endif

        /* Finally: Other ciphers */
#if defined( USE_RSA_KEYSCHEME ) && defined( USE_SEED_CBC_CIPHER ) && defined( USE_SHA_MAC )
        &TLS_RSA_WITH_SEED_CBC_SHA,
#endif

//        &TLS_RSA_WITH_RC4_128_MD5,         /* Insecure */
//        &TLS_RSA_WITH_RC4_128_SHA,         /* Insecure */
//        &TLS_RSA_WITH_3DES_EDE_CBC_SHA,    /* Insecure */
//        &TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA /* Insecure */

#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA_MAC )
        &TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA,
#endif
#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA_MAC )
        &TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA,
#endif
#if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
        &TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256,
#endif
#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_AES_128_CBC_CIPHER ) && defined( USE_SHA256_MAC )
        &TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA,
#endif
#if defined( USE_ECDH_ECDSA_KEYSCHEME ) && defined( USE_AES_256_CBC_CIPHER ) && defined( USE_SHA384_MAC )
        &TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA,
#endif

        0   /* List termination */
};

x509_cert* root_ca_certificates = NULL;

/* Diffie-Hellman Prime: 1024-bit MODP Group with 160-bit Prime Order Subgroup from RFC 5114 */
static const unsigned char diffie_hellman_prime_P[] =
{
        0xB1, 0x0B, 0x8F, 0x96, 0xA0, 0x80, 0xE0, 0x1D,
        0xDE, 0x92, 0xDE, 0x5E, 0xAE, 0x5D, 0x54, 0xEC,
        0x52, 0xC9, 0x9F, 0xBC, 0xFB, 0x06, 0xA3, 0xC6,
        0x9A, 0x6A, 0x9D, 0xCA, 0x52, 0xD2, 0x3B, 0x61,
        0x60, 0x73, 0xE2, 0x86, 0x75, 0xA2, 0x3D, 0x18,
        0x98, 0x38, 0xEF, 0x1E, 0x2E, 0xE6, 0x52, 0xC0,
        0x13, 0xEC, 0xB4, 0xAE, 0xA9, 0x06, 0x11, 0x23,
        0x24, 0x97, 0x5C, 0x3C, 0xD4, 0x9B, 0x83, 0xBF,
        0xAC, 0xCB, 0xDD, 0x7D, 0x90, 0xC4, 0xBD, 0x70,
        0x98, 0x48, 0x8E, 0x9C, 0x21, 0x9A, 0x73, 0x72,
        0x4E, 0xFF, 0xD6, 0xFA, 0xE5, 0x64, 0x47, 0x38,
        0xFA, 0xA3, 0x1A, 0x4F, 0xF5, 0x5B, 0xCC, 0xC0,
        0xA1, 0x51, 0xAF, 0x5F, 0x0D, 0xC8, 0xB4, 0xBD,
        0x45, 0xBF, 0x37, 0xDF, 0x36, 0x5C, 0x1A, 0x65,
        0xE6, 0x8C, 0xFD, 0xA7, 0x6D, 0x4D, 0xA7, 0x08,
        0xDF, 0x1F, 0xB2, 0xBC, 0x2E, 0x4A, 0x43, 0x71,
};

static const unsigned char diffie_hellman_prime_G[] =
{
        0xA4, 0xD1, 0xCB, 0xD5, 0xC3, 0xFD, 0x34, 0x12,
        0x67, 0x65, 0xA4, 0x42, 0xEF, 0xB9, 0x99, 0x05,
        0xF8, 0x10, 0x4D, 0xD2, 0x58, 0xAC, 0x50, 0x7F,
        0xD6, 0x40, 0x6C, 0xFF, 0x14, 0x26, 0x6D, 0x31,
        0x26, 0x6F, 0xEA, 0x1E, 0x5C, 0x41, 0x56, 0x4B,
        0x77, 0x7E, 0x69, 0x0F, 0x55, 0x04, 0xF2, 0x13,
        0x16, 0x02, 0x17, 0xB4, 0xB0, 0x1B, 0x88, 0x6A,
        0x5E, 0x91, 0x54, 0x7F, 0x9E, 0x27, 0x49, 0xF4,
        0xD7, 0xFB, 0xD7, 0xD3, 0xB9, 0xA9, 0x2E, 0xE1,
        0x90, 0x9D, 0x0D, 0x22, 0x63, 0xF8, 0x0A, 0x76,
        0xA6, 0xA2, 0x4C, 0x08, 0x7A, 0x09, 0x1F, 0x53,
        0x1D, 0xBF, 0x0A, 0x01, 0x69, 0xB6, 0xA2, 0x8A,
        0xD6, 0x62, 0xA4, 0xD1, 0x8E, 0x73, 0xAF, 0xA3,
        0x2D, 0x77, 0x9D, 0x59, 0x18, 0xD0, 0x8B, 0xC8,
        0x85, 0x8F, 0x4D, 0xCE, 0xF9, 0x7C, 0x2A, 0x24,
        0x85, 0x5E, 0x6E, 0xEB, 0x22, 0xB3, 0xB2, 0xE5,
};

/******************************************************
 *               Function Definitions
 ******************************************************/

tls_result_t tls_host_create_buffer( ssl_context* ssl, uint8_t** buffer, uint16_t buffer_size )
{
    wiced_assert("", ssl->outgoing_packet == NULL);

    /* Round requested buffer size up to next 64 byte chunk (required if encryption is active) */
    buffer_size = (uint16_t) ROUND_UP(buffer_size, 64);

    /* Check if requested buffer fits within a single MTU */
    if ( ( buffer_size < 1300) && ( ssl->transport_protocol == TLS_TCP_TRANSPORT ) ) /* TODO: Fix this */
    {
        uint16_t actual_packet_size;
        if ( wiced_packet_create_tcp( ssl->send_context, buffer_size, (wiced_packet_t**) &ssl->outgoing_packet, buffer, &actual_packet_size ) != WICED_SUCCESS )
        {
            *buffer = NULL;
            return 1;
        }
        if ( ssl->state == SSL_HANDSHAKE_OVER )
        {
            /* this doesn't need the extra space for the encryption header that a normal TLS socket would use - remove it */
            *buffer -= sizeof(tls_record_header_t);
            wiced_packet_set_data_start((wiced_packet_t*) ssl->outgoing_packet, *buffer);
        }
    }
    else
    {
        /* Requested size bigger than a MTU or TLS_EAP_TRANSPORT */
        /* Malloc space */
        *buffer = tls_host_malloc("tls", buffer_size);
        if ( *buffer == NULL )
        {
            return TLS_ERROR_OUT_OF_MEMORY;
        }
        memset( *buffer, 0, buffer_size );
        ssl->out_buffer_size = buffer_size;
    }

    return 0;
}

tls_result_t tls_host_get_packet_data( ssl_context* ssl, tls_packet_t* packet, uint32_t offset, uint8_t** data, uint16_t* data_length, uint16_t* available_data_length )
{
    uint16_t temp_length;
    uint16_t temp_available_length;

    if ( packet == NULL )
    {
        return TLS_ERROR_BAD_INPUT_DATA;
    }

    if ( ssl->transport_protocol == TLS_TCP_TRANSPORT )
    {
        wiced_result_t result = wiced_packet_get_data((wiced_packet_t*)packet, (uint16_t)offset, data, &temp_length, &temp_available_length);
        if ( result != WICED_SUCCESS)
        {
            return (tls_result_t) result;
        }
        *data_length = temp_length;
        *available_data_length = temp_available_length;
        return TLS_SUCCESS;
    }
#ifndef DISABLE_EAP_TLS
    else if ( ssl->transport_protocol == TLS_EAP_TRANSPORT )
    {
        besl_result_t result = supplicant_host_get_tls_data( (besl_packet_t) packet, (uint16_t)offset, data, &temp_length, &temp_available_length );
        if ( result != BESL_SUCCESS )
        {
            return (tls_result_t) result;
        }

        *data_length = temp_length;
        *available_data_length = temp_available_length;
        return TLS_SUCCESS;
    }
#endif /* DISABLE_EAP_TLS */
    return TLS_ERROR_BAD_INPUT_DATA;
}

void* tls_host_malloc( const char* name, uint32_t size)
{
    (void) name;
    return malloc_named( name, size );
}

extern void* tls_host_calloc( const char* name, size_t nelem, size_t elsize )
{
    (void) name;
    return calloc_named( name, nelem, elsize );
}

void tls_host_free(void* p)
{
    free( p );
}

void* tls_host_get_defragmentation_buffer ( uint16_t size )
{
    return malloc_named( "tls", size );
}

void  tls_host_free_defragmentation_buffer( void* buffer )
{
    tls_host_free( buffer );
    buffer = NULL;
}

/*
 * Flush any data not yet written
 */
tls_result_t ssl_flush_output( ssl_context *ssl, uint8_t* buffer, uint32_t length )
{
    if ( ssl->transport_protocol == TLS_TCP_TRANSPORT )
    {
        if (ssl->outgoing_packet != NULL)
        {
            wiced_packet_set_data_end((wiced_packet_t*)ssl->outgoing_packet, buffer + length);
            tls_host_send_tcp_packet(ssl->send_context, ssl->outgoing_packet);
            ssl->outgoing_packet = NULL;
            ssl->out_buffer_size = 0;
        }
        else
        {
            uint16_t      actual_packet_size;
            tls_packet_t* temp_packet;
            uint8_t*      packet_buffer;
            uint8_t*      data = buffer;

            while (length != 0)
            {
                uint16_t amount_to_copy;
                if ( wiced_packet_create_tcp( ssl->send_context, (uint16_t)length, (wiced_packet_t**) &temp_packet, &packet_buffer, &actual_packet_size ) != WICED_SUCCESS )
                {
                    tls_host_free(buffer);
                    buffer = NULL;
                    return 1;
                }
                if ( ssl->state == SSL_HANDSHAKE_OVER )
                {
                    /* this doesn't need the extra space for the encryption header that a normal TLS socket would use - remove it */
                    packet_buffer -= sizeof(tls_record_header_t);
                    wiced_packet_set_data_start((wiced_packet_t*) temp_packet, packet_buffer);
                }
                amount_to_copy = (uint16_t) MIN(length, actual_packet_size);
                packet_buffer = MEMCAT(packet_buffer, data, amount_to_copy);
                data   += amount_to_copy;
                length -= amount_to_copy;
                wiced_packet_set_data_end((wiced_packet_t*)temp_packet, packet_buffer );
                tls_host_send_tcp_packet(ssl->send_context, temp_packet);
            }

            tls_host_free(buffer);
            buffer = NULL;
        }
    }
#ifndef DISABLE_EAP_TLS
    else if ( ssl->transport_protocol == TLS_EAP_TRANSPORT )
    {
        supplicant_host_send_eap_tls_fragments( (supplicant_workspace_t*) ssl->send_context, buffer, length );
        ssl->out_buffer_size = 0;
        tls_host_free( buffer );
        buffer = NULL;
    }
#endif /* DISABLE_EAP_TLS */

    return TLS_SUCCESS;
}

uint64_t tls_host_get_time_ms( void )
{
    uint64_t time_ms;
    wiced_time_get_utc_time_ms( (wiced_utc_time_ms_t*) &time_ms );
    return time_ms;
}

static int32_t tls_get_session( ssl_context *ssl )
{
    int idx;
    ssl_session *cur;
    uint64_t t = tls_host_get_time_ms( );

    if ( ssl->resume == 0 )
        return ( 1 );

    /* self-initializing */
    if ( !session_list_initialized )
    {
        memset( session_list, 0, sizeof( session_list ) );
        session_list_initialized = 1;
    }

    for ( idx = 0; idx < WICED_TLS_MAX_RESUMABLE_SESSIONS; idx++ )
    {
        cur = &( session_list[idx] );

        if ( ( !cur->length ) || ( ssl->session->cipher != cur->cipher ) || ( ssl->session->length != cur->length ) )
            continue;

        if ( memcmp( ssl->session->id, cur->id, (size_t) MIN(cur->length, sizeof( cur->id )) ) != 0 )
            continue;

        /* if timeout is non-zero, timeout frees matched session slot after t=timeout.
         * zero timeout has no effect (never expires) */
        if ( (ssl->timeout != 0) && ((time_t)t - cur->start > ssl->timeout ))
        {
            goto done;
        }

        /* if age is non-zero,
         * age frees matched session slot after it has been
         * resumed age number of times, regardless
         * of whether timeout is enabled and working.*/
        if ( cur->age )
        {
            cur->age--;
            /* if aged out then reuse this slot */
            if ( !cur->age )
            {
                goto done;
            }
        }

        /* session matched, restore master key */
        memcpy( ssl->session->master, cur->master, 48 );

        /* refresh the expiration timeout */
        cur->start = (time_t)t;

        /* resuming session is true */
        return ( 0 );
    }

    done:

    /* resuming session is false */
    return ( 1 );
}

static int32_t tls_set_session( ssl_context *ssl )
{
    int idx;
    ssl_session *cur;
    uint64_t t = tls_host_get_time_ms( );
    int age = MAX_TLS_SESSION_AGE; /* oldest age */
    int old = 0; /* oldest slot */

    /* self-initializing */
    if ( !session_list_initialized )
    {
        memset( session_list, 0, sizeof( session_list ) );
        session_list_initialized = 1;
    }

    /* search static list */
    for ( idx = 0; idx < WICED_TLS_MAX_RESUMABLE_SESSIONS; idx++ )
    {
        cur = &( session_list[idx] );

        /* unused? */
        if ( !cur->age )
            break;

        /* keeps track of oldest entry if a slot needs to be taken away. */
        if ( cur->age < age )
        {
            age = cur->age;
            old = idx;
        }

    } /* for session_list */

    /* check if all slots in use, then take away oldest slot */
    if ( WICED_TLS_MAX_RESUMABLE_SESSIONS == idx )
    {
        idx = old;
        cur = &( session_list[idx] );
    }

    /* save session */
    memcpy( cur, ssl->session, sizeof(ssl_session) );
    /* set expiration timer */
    cur->start = (time_t)t;
    cur->age   = MAX_TLS_SESSION_AGE;

    /* session was saved */
    return ( 0 );
}

static besl_result_t wiced_tls_load_certificate( wiced_tls_certificate_t* certificate, const uint8_t* certificate_data, uint32_t certificate_length, wiced_tls_certificate_format_t certificate_format )
{
    besl_result_t result;
    uint32_t der_certificate_length;
    uint32_t total_der_bytes;

    /* Allocate space for temporary processing */
    certificate->processed_certificate_data = tls_host_malloc( "cert", sizeof(x509_cert) );
    if ( certificate->processed_certificate_data == NULL )
    {
        return BESL_TLS_ERROR_OUT_OF_MEMORY;
    }

    memset( certificate->processed_certificate_data, 0, sizeof(x509_cert) );

    switch ( certificate_format )
    {
        case TLS_CERTIFICATE_IN_DER_FORMAT:
            if ( x509_parse_certificate_data( certificate->processed_certificate_data, certificate_data, certificate_length ) != 0 )
            {
                result = BESL_CERT_PARSE_FAIL;
                goto end;
            }
            certificate->certificate_data        = certificate_data;
            certificate->certificate_data_length = certificate_length;
            break;

        case TLS_CERTIFICATE_IN_PEM_FORMAT:
            x509_convert_pem_to_der( certificate_data, certificate_length, &certificate->certificate_data, &total_der_bytes );
            certificate->certificate_data_malloced = WICED_TRUE;
            certificate->certificate_data_length = total_der_bytes;

            der_certificate_length = x509_read_cert_length( certificate->certificate_data );
            if ( x509_parse_certificate_data( certificate->processed_certificate_data, certificate->certificate_data, der_certificate_length ) != 0 )
            {
                result = BESL_CERT_PARSE_FAIL;
                goto end;
            }
            break;
    }

    /* Take ownership of the public_key and common_name */
    certificate->public_key  = certificate->processed_certificate_data->public_key;

    x509_name* name_iter = &certificate->processed_certificate_data->subject;
    for (; name_iter != NULL; name_iter = name_iter->next)
    {
        if (name_iter->val.p[name_iter->val.len] == 0x30)
        {
            char* temp = tls_host_malloc("tls", name_iter->val.len+1);
            if (temp == NULL)
            {
                result = BESL_TLS_ERROR_OUT_OF_MEMORY;
                goto end;
            }
            memcpy(temp, name_iter->val.p, name_iter->val.len);
            temp[name_iter->val.len] = 0;

            certificate->common_name = temp;
            break;
        }
    }
    //certificate->common_name = certificate->processed_certificate_data->subject;

    /* NULL the public_key pointer in the temporary certificate so x509_free() doesn't free the memory */
    certificate->processed_certificate_data->public_key = NULL;

    x509_free( certificate->processed_certificate_data );
    tls_host_free( certificate->processed_certificate_data );
    certificate->processed_certificate_data = NULL;

    return BESL_SUCCESS;

end:
    x509_free( certificate->processed_certificate_data );
    tls_host_free( certificate->processed_certificate_data );
    certificate->processed_certificate_data = NULL;

    if ( certificate->certificate_data_malloced == WICED_TRUE )
    {
        tls_host_free( (void*)certificate->certificate_data );
        certificate->certificate_data = NULL;
    }

    return result;
}

static besl_result_t wiced_tls_load_key( wiced_tls_key_t* key, const char* key_string )
{
    if ( key->type == TLS_RSA_KEY )
    {
        if ( x509parse_key( (rsa_context*)key, (unsigned char *) key_string, (uint32_t) strlen( key_string ), NULL, 0 ) != 0 )
        {
            wiced_assert( "Key parse error", 0 != 0 );
            return BESL_KEY_PARSE_FAIL;
        }
    }
    else
    {
        // Fill ECC key
    }

    return BESL_SUCCESS;
}

wiced_result_t wiced_tls_init_context( wiced_tls_context_t* context, wiced_tls_identity_t* identity, const char* peer_cn )
{
    memset( context, 0, sizeof(wiced_tls_context_t) );

    context->context_id      = WICED_TLS_CONTEXT_ID;
    context->identity        = identity;
    context->context.peer_cn = peer_cn;

    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_init_identity( wiced_tls_identity_t* identity, const char* private_key, const uint8_t* certificate_data, uint32_t certificate_length )
{
    besl_result_t result;

    wiced_assert( "Bad args", (identity != NULL) && (private_key != NULL) && (certificate_data != NULL) );

    memset( identity, 0, sizeof( *identity ) );

    result = wiced_tls_load_key( (wiced_tls_key_t*)&identity->private_key, private_key );
    if ( result != BESL_SUCCESS )
    {
        return result;
    }

    result = wiced_tls_load_certificate( &identity->certificate, certificate_data, certificate_length, TLS_CERTIFICATE_IN_PEM_FORMAT );
    if ( result != BESL_SUCCESS )
    {
        return result;
    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_deinit_identity( wiced_tls_identity_t* tls_identity)
{
    if ( tls_identity->certificate.processed_certificate_data != NULL )
    {
        x509_free( tls_identity->certificate.processed_certificate_data );
        tls_identity->certificate.processed_certificate_data = NULL;
    }

    if ( tls_identity->private_key.common.type == TLS_RSA_KEY )
    {
        rsa_free( (rsa_context*)&tls_identity->private_key.rsa );
    }

    if ( tls_identity->certificate.public_key != NULL )
    {
        if ( tls_identity->certificate.public_key->type == TLS_RSA_KEY )
        {
            rsa_free( (rsa_context*)tls_identity->certificate.public_key );
        }
        tls_host_free( tls_identity->certificate.public_key );
        tls_identity->certificate.public_key = NULL;
    }

    if ( tls_identity->certificate.common_name != NULL )
    {
        tls_host_free( (void*)tls_identity->certificate.common_name );
        tls_identity->certificate.common_name = NULL;
    }

    if ( tls_identity->certificate.certificate_data_malloced == WICED_TRUE )
    {
        tls_host_free( (void*)tls_identity->certificate.certificate_data );
        tls_identity->certificate.certificate_data = NULL;
    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_reset_context( wiced_tls_context_t* tls_context )
{
    ssl_free( &tls_context->context);

    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_deinit_context( wiced_tls_context_t* tls_context )
{
    ssl_free( &tls_context->context);

    memset( tls_context, 0, sizeof( *tls_context ) );
    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_init_root_ca_certificates( const char* trusted_ca_certificates )
{
    int result;

    wiced_tls_deinit_root_ca_certificates( );

    root_ca_certificates = tls_host_malloc( "tls", sizeof(*root_ca_certificates) );
    if ( root_ca_certificates == NULL )
    {
        return WICED_OUT_OF_HEAP_SPACE;
    }

    memset( root_ca_certificates, 0, sizeof(*root_ca_certificates) );

    result = x509_parse_certificate( root_ca_certificates, (const uint8_t*)trusted_ca_certificates, strlen( (const char*)trusted_ca_certificates ) );
    if ( result != 0 )
    {
        tls_host_free( root_ca_certificates );
        root_ca_certificates = NULL;
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_deinit_root_ca_certificates( void )
{
    if ( root_ca_certificates != NULL )
    {
        x509_free( root_ca_certificates );
        tls_host_free( root_ca_certificates );
        root_ca_certificates = NULL;
    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_tcp_enable_tls( wiced_tcp_socket_t* socket, void* context )
{
    socket->tls_context = context;
    return WICED_SUCCESS;
}


wiced_result_t wiced_tcp_start_tls( wiced_tcp_socket_t* socket, wiced_tls_endpoint_type_t type, wiced_tls_certificate_verification_t verification )
{
    return wiced_generic_start_tls_with_ciphers( socket->tls_context, socket, type, verification, my_ciphers, TLS_TCP_TRANSPORT );
}

#ifndef DISABLE_EAP_TLS
wiced_result_t wiced_supplicant_start_tls( supplicant_workspace_t* supplicant, wiced_tls_endpoint_type_t type, wiced_tls_certificate_verification_t verification )
{
    host_rtos_delay_milliseconds( 10 );
    return wiced_generic_start_tls_with_ciphers( supplicant->tls_context, supplicant, type, verification, my_ciphers, TLS_EAP_TRANSPORT );
}

wiced_result_t wiced_supplicant_enable_tls( supplicant_workspace_t* supplicant, void* context )
{
    supplicant->tls_context = context;
    return WICED_SUCCESS;
}

#endif /* DISABLE_EAP_TLS */

wiced_result_t wiced_generic_start_tls_with_ciphers( wiced_tls_context_t* tls_context, void* referee, wiced_tls_endpoint_type_t type, wiced_tls_certificate_verification_t verification, const cipher_suite_t* cipher_list[], tls_transport_protocol_t transport_protocol )
{
    microrng_state  rngstate;
    int             prev_state;
    uint64_t        start_time;
    tls_result_t    result;

    if ( tls_context->context_id != WICED_TLS_CONTEXT_ID )
    {
        return WICED_TLS_ERROR_UNITIALIZED_CONTEXT;
    }

    /* Initialize the session data */
    if ( transport_protocol != TLS_EAP_TRANSPORT )
    {
        memset( &tls_context->session, 0, sizeof(wiced_tls_session_t) );
    }

    if ( ssl_init( &tls_context->context ) != 0 )
    {
        wiced_assert( "Error initialising SSL", 0 != 0 );
        return WICED_TLS_INIT_FAIL;
    }

    /* Prepare session and entropy */
    tls_context->session.age = MAX_TLS_SESSION_AGE;
    wiced_crypto_get_random( &rngstate.entropy, 4 );

    tls_context->context.transport_protocol = transport_protocol;

    microrng_init( &rngstate );

    ssl_set_endpoint( &tls_context->context, type );
    ssl_set_rng     ( &tls_context->context, microrng_rand, &rngstate );
    tls_context->context.receive_context = referee;
    tls_context->context.send_context    = referee;
    tls_context->context.get_session     = tls_get_session;
    tls_context->context.set_session     = tls_set_session;
    tls_context->context.ciphers         = cipher_list;
    ssl_set_session ( &tls_context->context, SESSION_CAN_BE_RESUMED, 1000000, &tls_context->session );

    /* Assert if user has not created correct TLS context for the TLS endpoint type */
    wiced_assert("TLS servers must have an advanced TLS context", !((type == WICED_TLS_AS_SERVER) && (tls_context->identity == NULL)));

    if ( root_ca_certificates != NULL )
    {
        ssl_set_ca_chain( &tls_context->context, root_ca_certificates, tls_context->context.peer_cn );
        ssl_set_authmode( &tls_context->context, verification );
    }
    else
    {
        ssl_set_authmode( &tls_context->context, SSL_VERIFY_NONE );
    }

    if ( tls_context->identity != NULL )
    {
        tls_context->context.identity = tls_context->identity;
        if ( tls_context->context.identity->private_key.common.type == TLS_RSA_KEY )
        {
            tls_context->context.identity->private_key.rsa.f_rng = microrng_rand;
            tls_context->context.identity->private_key.rsa.p_rng = &rngstate;
        }
        ssl_set_dh_param( &tls_context->context, diffie_hellman_prime_P, sizeof( diffie_hellman_prime_P ), diffie_hellman_prime_G, sizeof( diffie_hellman_prime_G ) );
    }

    prev_state = 0;
    start_time = tls_host_get_time_ms();
    do
    {
        uint64_t curr_time;
        if (type == WICED_TLS_AS_SERVER)
        {
            result = ssl_handshake_server_async( &tls_context->context );
            if ( result != TLS_SUCCESS )
            {
                WPRINT_SECURITY_INFO(( "Error with TLS server handshake\n" ));
                goto exit_with_inited_context;
            }
        }
        else
        {
            result = ssl_handshake_client_async( &tls_context->context );
            if ( result != TLS_SUCCESS )
            {
                WPRINT_SECURITY_INFO(( "Error with TLS client handshake %u\n", (unsigned int)result ));
                goto exit_with_inited_context;
            }
        }

        /* break out if stuck */
        curr_time = tls_host_get_time_ms();
        if ( curr_time - start_time > MAX_HANDSHAKE_WAIT )
        {
            WPRINT_SECURITY_INFO(( "Timeout in SSL handshake\n" ));
            result = TLS_HANDSHAKE_TIMEOUT;
            goto exit_with_inited_context;
        }

        /* if no state change then wait on client */
        if ( prev_state == tls_context->context.state )
        {
            host_rtos_delay_milliseconds( 10 );
        }
        else /* otherwise process next state with no delay */
        {
            prev_state = tls_context->context.state;
        }
    } while ( tls_context->context.state != SSL_HANDSHAKE_OVER );

    return WICED_SUCCESS;

exit_with_inited_context:
    ssl_close_notify( &tls_context->context );
    ssl_free(&tls_context->context);
    return (wiced_result_t) result;
}

/*
 * Calculates the maximium amount of payload that can fit in a given sized buffer
 */
wiced_result_t wiced_tls_calculate_overhead( wiced_tls_workspace_t* context, uint16_t available_space, uint16_t* header, uint16_t* footer )
{
    *header = 0;
    *footer = 0;
    if ( context != NULL && context->state == SSL_HANDSHAKE_OVER )
    {
        /* Add TLS record header */
        *header = sizeof(tls_record_header_t);

        /* Compensate for padding */
        if ( context->ivlen != 0 )
        {
            /* Note: There must be at least one byte of padding so align to block size and add an extra 1 */
            *footer += ( available_space - *header - *footer ) % context->ivlen + 1;
        }

        /* MAC */
        *footer += context->maclen;

        /* TLS 1.1+ uses explicit IV in footer */
        if ( context->minor_ver > 1 )
        {
            *footer += context->ivlen;
        }
    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_encrypt_packet( wiced_tls_workspace_t* workspace, wiced_packet_t* packet )
{
    uint8_t* data;
    uint16_t length;
    uint16_t available;
    wiced_result_t result;
    tls_record_t* record;

    if ( ( workspace == NULL ) || ( packet == NULL ) )
    {
        return WICED_ERROR;
    }
    if ( wiced_packet_get_data(packet, 0, &data, &length, &available) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }
    data -= sizeof(tls_record_header_t);
    result = (wiced_result_t) tls_host_set_packet_start((tls_packet_t*)packet, data);
    if ( result != WICED_SUCCESS)
    {
        return result;
    }

    record                = (tls_record_t*) data;
    record->type          = SSL_MSG_APPLICATION_DATA;
    record->major_version = (uint8_t)workspace->major_ver;
    record->minor_version = (uint8_t)workspace->minor_ver;
    record->length        = htobe16( length );

    result = (wiced_result_t) tls_encrypt_record( workspace, record, length );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    wiced_packet_set_data_end(packet, data + htobe16(record->length) + sizeof(tls_record_header_t));
    return WICED_SUCCESS;
}

#if 0
wiced_result_t wiced_tls_send_buffer(wiced_tcp_socket_t* socket, void* buffer, uint16_t buffer_length)
{
    UNUSED_PARAMETER(socket);
    UNUSED_PARAMETER(buffer);
    UNUSED_PARAMETER(buffer_length);
    return WICED_UNSUPPORTED;
}
#endif

wiced_result_t wiced_tls_receive_packet( wiced_tcp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout )
{
    wiced_result_t result;
    wiced_tls_workspace_t* context = &socket->tls_context->context;

    wiced_assert("Bad args", (socket != NULL) && (packet != NULL));


    /* Check if we already have a record which should only happen if it was larger than a packet which means it's stored in the defragmentation buffer */
    if ( context->current_record != NULL )
    {
        wiced_assert( "Something wrong", (void*)context->current_record == context->defragmentation_buffer );
        return tls_packetize_buffered_data( context, packet );
    }
    else
    {
        tls_record_t* record;
        result = (wiced_result_t) tls_get_next_record( context, &record, timeout, TLS_RECEIVE_PACKET_IF_NEEDED );
        if ( result != WICED_SUCCESS )
        {
            return result;
        }

        /* Check if this record has been defragmented */
        if ( (void*)record == context->defragmentation_buffer )
        {
            return tls_packetize_buffered_data( context, packet );
        }
        else
        {
            tls_record_t* temp_record;
            uint8_t* packet_data;
            uint16_t length;
            uint16_t available;
            uint8_t* end_of_data;

            /* We have a pointer to the current record so we can move on */
            tls_skip_current_record(context);

            /* Make sure we process every record in this packet */
            end_of_data = record->message + htobe16( record->length );
            while ( tls_get_next_record( context, &temp_record, timeout, TLS_AVOID_NEW_RECORD_PACKET_RECEIVE ) == TLS_SUCCESS )
            {
                /* Make the record data contiguous with the previous record */
                uint16_t temp_record_length = htobe16( temp_record->length );
                end_of_data = MEMCAT( end_of_data, temp_record->message, temp_record_length );
                record->length = htobe16( htobe16(record->length) + temp_record_length );
                tls_skip_current_record( context );
            }

            /* Set the packet start and end */
            if ( context->received_packet == NULL )
            {
                return WICED_ERROR;
            }
            wiced_packet_get_data( (wiced_packet_t*)context->received_packet, 0, &packet_data, &length, &available );
            tls_host_set_packet_start( context->received_packet, record->message );
            wiced_packet_set_data_end( (wiced_packet_t*)context->received_packet, end_of_data );

            *packet = (wiced_packet_t*)context->received_packet;
            context->received_packet        = NULL;
            context->received_packet_length = 0;
        }
    }

    return WICED_SUCCESS;
}

#ifndef DISABLE_EAP_TLS
wiced_result_t wiced_tls_receive_eap_packet( supplicant_workspace_t* supplicant, besl_packet_t* packet, uint32_t timeout )
{
    wiced_result_t result;
    wiced_tls_workspace_t* context = &supplicant->tls_context->context;

    wiced_assert( "Bad args", (packet != NULL) );


    /* Check if we already have a record which should only happen if it was larger than a packet which means it's stored in the defragmentation buffer */
    if ( context->current_record != NULL )
    {
        wiced_assert( "Something wrong", (void*)context->current_record == context->defragmentation_buffer );
        return tls_eap_buffered_data( context, packet );
    }
    else
    {
        tls_record_t* record;
        result = (wiced_result_t) tls_get_next_record( context, &record, timeout, TLS_RECEIVE_PACKET_IF_NEEDED );
        if ( result != WICED_SUCCESS )
        {
            return result;
        }
        /* Check if this record has been defragmented */
        if ( (void*)record == context->defragmentation_buffer )
        {
            return tls_eap_buffered_data( context, packet );
        }
        else
        {
            tls_record_t* temp_record;
            uint8_t* end_of_data;

            /* We have a pointer to the current record so we can move on */
            tls_skip_current_record(context);

            /* Make sure we process every record in this packet */
            end_of_data = besl_host_get_data( (besl_packet_t)context->received_packet );
            end_of_data = MEMCAT( end_of_data, record->message, htobe16(record->length) );
            while ( tls_get_next_record( context, &temp_record, timeout, TLS_AVOID_NEW_RECORD_PACKET_RECEIVE ) == TLS_SUCCESS )
            {
                /* Make the record data contiguous with the previous record */
                uint16_t temp_record_length = htobe16( temp_record->length );
                end_of_data = MEMCAT( end_of_data, temp_record->message, temp_record_length );
                tls_skip_current_record( context );
            }

            (*packet) = (besl_packet_t)context->received_packet;
            context->received_packet        = NULL;
            context->received_packet_length = 0;
        }
    }

    return WICED_SUCCESS;
}
#endif /* DISABLE_EAP_TLS */


wiced_result_t wiced_tls_close_notify( wiced_tcp_socket_t* socket )
{
    if ( socket->tls_context != NULL )
    {
        ssl_close_notify( &socket->tls_context->context );
    }
    return WICED_SUCCESS;
}


static wiced_result_t tls_packetize_buffered_data( wiced_tls_workspace_t* context, wiced_packet_t** packet )
{
    uint8_t*       data;
    uint16_t       length;
    uint16_t       available_length;
    uint16_t       record_length;
    wiced_result_t result;
    uint32_t       amount_to_copy;

    (void)available_length;

    record_length = (uint16_t)( htobe16(context->current_record->length) + sizeof(tls_record_header_t) );

    /* Get a buffer and fill with decrypted content */
    result = wiced_packet_create_tcp( context->send_context, (uint16_t) MIN(1024, record_length - context->defragmentation_buffer_bytes_processed), (wiced_packet_t**) packet, &data, &length );
    if ( result  != WICED_SUCCESS )
    {
        *packet = NULL;
        return result;
    }
    if ( context->state == SSL_HANDSHAKE_OVER )
    {
        /* this doesn't need the extra space for the encryption header that a normal TLS socket would use - remove it */
        data -= sizeof(tls_record_header_t);
        wiced_packet_set_data_start((wiced_packet_t*) *packet, data);
    }

    amount_to_copy = (uint32_t) MIN( length, record_length - context->defragmentation_buffer_bytes_processed );
    memcpy( data, &context->defragmentation_buffer[context->defragmentation_buffer_bytes_processed], amount_to_copy );
    wiced_packet_set_data_end( *packet, data + amount_to_copy );

    context->defragmentation_buffer_bytes_processed = (uint16_t) ( context->defragmentation_buffer_bytes_processed + amount_to_copy );

    /* Check if we've returned all the data to the user */
    if ( context->defragmentation_buffer_bytes_processed >= record_length )
    {
        tls_host_free_defragmentation_buffer(context->defragmentation_buffer);
        context->defragmentation_buffer                 = NULL;
        context->defragmentation_buffer_bytes_processed = 0;
        context->defragmentation_buffer_bytes_received  = 0;
        context->defragmentation_buffer_length          = 0;
        context->current_record                         = NULL;
    }

    return WICED_SUCCESS;
}

#ifndef DISABLE_EAP_TLS
static wiced_result_t tls_eap_buffered_data( wiced_tls_workspace_t* context, besl_packet_t* packet )
{
    uint16_t       length;
    uint16_t       available_length;
    uint16_t       record_length;
    besl_result_t  result;
    uint32_t       amount_to_copy;
    eap_packet_t*  header;

    (void)available_length;

    record_length = (uint16_t)( htobe16(context->current_record->length) + sizeof(tls_record_header_t) );

    result = besl_host_create_packet( packet, (uint16_t) MIN(1024, record_length - context->defragmentation_buffer_bytes_processed) );
    if ( result != BESL_SUCCESS )
    {
        return result;
    }
    header = ( eap_packet_t* ) besl_host_get_data( *packet );
    length = besl_host_get_packet_size( *packet );

    amount_to_copy = (uint32_t) MIN( length, record_length - context->defragmentation_buffer_bytes_processed );
    memcpy( header, &context->defragmentation_buffer[context->defragmentation_buffer_bytes_processed], amount_to_copy );

    context->defragmentation_buffer_bytes_processed = (uint16_t) ( context->defragmentation_buffer_bytes_processed + amount_to_copy );

    /* Check if we've returned all the data to the user */
    if ( context->defragmentation_buffer_bytes_processed >= record_length )
    {
        tls_host_free_defragmentation_buffer(context->defragmentation_buffer);
        context->defragmentation_buffer = 0;
        context->current_record = NULL;
    }

    return WICED_SUCCESS;
}
#endif /* DISABLE_EAP_TLS */


wiced_bool_t wiced_tls_is_encryption_enabled( wiced_tcp_socket_t* socket )
{
    if ( socket->tls_context != NULL && socket->tls_context->context.state == SSL_HANDSHAKE_OVER )
    {
        return WICED_TRUE;
    }
    else
    {
        return WICED_FALSE;
    }
}

/*
 * TLS support functions
 */
tls_result_t tls_host_free_packet( tls_packet_t* packet )
{
    wiced_packet_delete((wiced_packet_t*)packet);
    return TLS_SUCCESS;
}

tls_result_t tls_host_send_tcp_packet( void* context, tls_packet_t* packet )
{
    if ( network_tcp_send_packet( (wiced_tcp_socket_t*)context, (wiced_packet_t*)packet) != WICED_SUCCESS )
    {
        wiced_packet_delete((wiced_packet_t*)packet);
    }
    return TLS_SUCCESS;
}

tls_result_t tls_host_receive_packet(ssl_context* ssl, tls_packet_t** packet, uint32_t timeout)
{
    tls_result_t              result = TLS_RECEIVE_FAILED;
    switch(ssl->transport_protocol)
    {
        case TLS_TCP_TRANSPORT:
            result = (tls_result_t) network_tcp_receive( (wiced_tcp_socket_t*) ssl->receive_context, (wiced_packet_t**) packet, timeout );
            break;

        case TLS_EAP_TRANSPORT:
#ifndef DISABLE_EAP_TLS
            result = (tls_result_t) supplicant_receive_eap_tls_packet( ssl->receive_context, packet, timeout );
            break;
#endif /* DISABLE_EAP_TLS */

        case TLS_UDP_TRANSPORT:
        default:
            wiced_assert( "unknown transport", 1 == 0 );
            break;
    }

    return result;
}

tls_result_t tls_host_set_packet_start( tls_packet_t* packet, uint8_t* start )
{
    wiced_packet_set_data_start((wiced_packet_t*)packet, start);
    return TLS_SUCCESS;
}
