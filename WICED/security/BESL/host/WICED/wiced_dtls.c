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
#include "wiced_tcpip.h"
#include "wiced_dtls.h"
#include "wiced_utilities.h"
#include "dtls_host_api.h"
#include "wiced_crypto.h"
#include "wwd_buffer_interface.h"
#include "crypto_constants.h"
#include "crypto_structures.h"
#include "wiced_time.h"
#include "wwd_assert.h"
#include "wwd_buffer_interface.h"
#include "besl_host_interface.h"
#include "wiced_security.h"
#include "internal/wiced_internal_api.h"
#include "wiced_udpip_dtls_api.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define MAX_HANDSHAKE_WAIT  40000
#define TARGET_PORT         5684;

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/
/* TODO : This is already there in dtls.h but to pass jenkins added here. Need to correct this  */
#define DTLS_CT_APPLICATION_DATA   23

dtls_peer_t* dtls_get_peer( dtls_context_t *ctx, const dtls_session_t *session );
void dtls_free_peer( dtls_peer_t *peer );
int dtls_prepare_record( dtls_peer_t* peer, dtls_context_t* context, dtls_security_parameters_t* security, unsigned char type, dtls_record_t *record, size_t len, int need_encrypt );
static inline dtls_security_parameters_t *dtls_security_params( dtls_peer_t *peer )
{
    return peer->security_params[ 0 ];
}

/******************************************************
 *               Static Function Declarations
 ******************************************************/
static besl_result_t wiced_dtls_load_key( wiced_dtls_key_t* key, const char* key_string );

/******************************************************
 *               Variable Definitions
 ******************************************************/
/**
 * This list of cipher suites should be in order of strength with the strongest first.
 * Do not enable cipher suites unless they meet your security requirements
 */
static const cipher_suite_t* my_ciphers[ ] =
{

#if defined( USE_DTLS_PSK_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_8_CIPHER ) && defined( USE_DTLS_AES_128_CCM_8_MAC )
        &TLS_PSK_WITH_AES_128_CCM_8, /* Reusing TLS_PSK_WITH_AES_128_CCM_8 defined in cipher_suites.h */
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_128_CCM_8_CIPHER ) && defined( USE_AES_128_CCM_8_MAC ) */

        0 /* List termination */
};

/******************************************************
 *               Function Definitions
 ******************************************************/

dtls_result_t dtls_host_get_packet_data( dtls_context_t* dtls, dtls_packet_t* packet, uint32_t offset, uint8_t** data, uint16_t* data_length, uint16_t* available_data_length )
{
    uint16_t temp_length;
    uint16_t temp_available_length;

    if ( dtls->transport_protocol == DTLS_TCP_TRANSPORT || dtls->transport_protocol == DTLS_UDP_TRANSPORT )
    {
        wiced_result_t result = wiced_packet_get_data( (wiced_packet_t*) packet, (uint16_t) offset, data, &temp_length, &temp_available_length );
        if ( result != WICED_SUCCESS )
        {
            return (dtls_result_t) result;
        }
        *data_length = temp_length;
        *available_data_length = temp_available_length;
        return DTLS_SUCCESS;
    }

    return DTLS_ERROR_BAD_INPUT_DATA;
}

dtls_result_t dtls_host_packet_get_info( uint32_t* packet, dtls_session_t* session )
{
    wiced_udp_packet_get_info( (wiced_packet_t*) packet, (wiced_ip_address_t*) &session->ip, &session->port );
    return DTLS_SUCCESS;
}

dtls_result_t dtls_host_create_buffer( dtls_context_t* context, uint8_t** buffer, uint16_t buffer_size )
{
    wiced_assert("", context->outgoing_packet == NULL);

    /* Round requested buffer size up to next 64 byte chunk (required if encryption is active) */
    buffer_size = (uint16_t) ROUND_UP(buffer_size, 64);

    /* Check if requested buffer fits within a single MTU */
    if ( ( buffer_size < 1300 ) && ( context->transport_protocol == DTLS_UDP_TRANSPORT ) ) /* TODO: Fix this */
    {
        uint16_t actual_packet_size;
        if ( wiced_packet_create_udp( context->send_context, buffer_size, (wiced_packet_t**) &context->outgoing_packet, buffer, &actual_packet_size ) != WICED_SUCCESS )
        {
            *buffer = NULL;
            return 1;
        }
        if ( context->state == DTLS_HANDSHAKE_OVER )
        {
            /* this doesn't need the extra space for the encryption header that a normal DTLS socket would use - remove it */
            *buffer -= sizeof(dtls_record_header_t);
            wiced_packet_set_data_start( (wiced_packet_t*) context->outgoing_packet, *buffer );
        }
    }
    else
    {
        /* TODO : Handle If requested size is bigger than MTU. */
    }

    return DTLS_SUCCESS;
}

void* dtls_host_malloc( const char* name, uint32_t size )
{
    (void) name;
    return malloc_named( name, size );
}

void dtls_host_free( void* p )
{
    free( p );
}

/*
 * Flush any data not yet written
 */
dtls_result_t dtls_flush_output( dtls_context_t *dtls, dtls_session_t* session, uint8_t* buffer, uint32_t length )
{
    wiced_packet_set_data_end( (wiced_packet_t*) dtls->outgoing_packet, buffer + length );

    /* Send the UDP packet */
    if ( wiced_udp_send( dtls->send_context, (wiced_ip_address_t*) &session->ip, session->port, (wiced_packet_t*) dtls->outgoing_packet ) != WICED_SUCCESS )
    {
        WPRINT_SECURITY_DEBUG( ( "UDP packet send failed\n" ) );
        wiced_packet_delete( (wiced_packet_t*) dtls->outgoing_packet ); /* Delete packet, since the send failed */
    }

    return DTLS_SUCCESS;

}

uint64_t dtls_host_get_time_ms( void )
{
    uint64_t time_ms;
    wiced_time_get_utc_time_ms( (wiced_utc_time_ms_t*) &time_ms );
    return time_ms;
}

static besl_result_t wiced_dtls_load_key( wiced_dtls_key_t* key, const char* key_string )
{
    if ( key->type == DTLS_PSK_KEY )
    {
        wiced_dtls_psk_key_t* psk_key;
        psk_key = (wiced_dtls_psk_key_t*) key;
        memcpy( psk_key->psk_key, key_string, strlen( key_string ) );
    }
    else
    {
        /* TODO : Fill ECC key */
    }
    return BESL_SUCCESS;
}

wiced_result_t wiced_dtls_init_context( wiced_dtls_context_t* context, wiced_dtls_identity_t* identity, const char* peer_cn )
{
    memset( context, 0, sizeof(wiced_dtls_context_t) );

    context->identity = identity;
    return WICED_SUCCESS;
}

wiced_result_t wiced_dtls_init_identity( wiced_dtls_identity_t* identity, const char* private_key, const uint8_t* certificate_data, uint32_t certificate_length )
{
    besl_result_t result;
    wiced_assert( "Bad args", (identity != NULL) && (private_key != NULL) && (certificate_data != NULL) );

    memset( identity, 0, sizeof( *identity ) );

    result = wiced_dtls_load_key( (wiced_dtls_key_t*) &identity->private_key, private_key );
    if ( result != BESL_SUCCESS )
    {
        return result;
    }

    memcpy( identity->private_key.psk.psk_identity, certificate_data, certificate_length );

    return WICED_SUCCESS;
}

wiced_result_t wiced_dtls_deinit_identity( wiced_dtls_identity_t* identity )
{
    identity->certificate.certificate_data = NULL;

    return WICED_SUCCESS;
}

wiced_result_t wiced_dtls_reset_context( wiced_dtls_context_t* dtls_context )
{
    free( dtls_context );

    return WICED_SUCCESS;
}

wiced_result_t wiced_udp_enable_dtls( wiced_udp_socket_t* socket, void* context )
{
    socket->dtls_context = context;
    return WICED_SUCCESS;
}

wiced_result_t wiced_dtls_deinit_context( wiced_dtls_context_t* dtls_context )
{
    if ( dtls_context->context.received_packet != NULL )
    {
        wiced_packet_delete( (wiced_packet_t*) dtls_context->context.received_packet );
    }

    dtls_free_peer( &dtls_context->context.peer );
    wiced_dtls_reset_context( dtls_context );

    memset( dtls_context, 0, sizeof(wiced_dtls_context_t) );

    return WICED_SUCCESS;
}

wiced_result_t wiced_dtls_deinit_root_ca_certificates( void )
{
    /* TODO : Deinitialize root certificates */
    return WICED_SUCCESS;
}

wiced_result_t wiced_dtls_receive_packet( wiced_udp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout )
{
    wiced_result_t result;
    dtls_session_t session;
    uint16_t record_length;
    wiced_dtls_workspace_t* context = &socket->dtls_context->context;

    wiced_assert("Bad args", (socket != NULL) && (packet != NULL));

    dtls_record_t* record;
    result = (wiced_result_t) dtls_get_next_record( context, &session, &record, timeout, DTLS_RECEIVE_PACKET_IF_NEEDED );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    dtls_record_t* temp_record;
    uint8_t* packet_data;
    uint16_t length;
    uint16_t available;
    uint8_t* end_of_data;

    /* We have a pointer to the current record so we can move on */
    dtls_skip_current_record( context );

    /* Make sure we process every record in this packet */
    record_length = ( record->record_header.length[ 0 ] ) | ( record->record_header.length[ 1 ] );
    end_of_data = ( record->data + record_length );
    while ( dtls_get_next_record( context, &session, &temp_record, timeout, DTLS_AVOID_NEW_RECORD_PACKET_RECEIVE ) == DTLS_SUCCESS )
    {
        /* Make the record data contiguous with the previous record */
        uint16_t temp_record_length = ( record->record_header.length[ 0 ] ) | ( record->record_header.length[ 1 ] );
        end_of_data = MEMCAT( end_of_data, temp_record->data, temp_record_length );
        record->record_header.length[ 0 ] = ( record_length + temp_record_length ) >> 8;
        record->record_header.length[ 1 ] = ( record_length + temp_record_length );
        dtls_skip_current_record( context );
    }

    /* Set the packet start and end */
    if ( context->received_packet == NULL )
    {
        return WICED_ERROR;
    }
    wiced_packet_get_data( (wiced_packet_t*) context->received_packet, 0, &packet_data, &length, &available );
    dtls_host_set_packet_start( context->received_packet, record->data );
    wiced_packet_set_data_end( (wiced_packet_t*) context->received_packet, end_of_data );

    *packet = (wiced_packet_t*) context->received_packet;
    context->received_packet = NULL;
    context->received_packet_length = 0;

    return WICED_SUCCESS;
}

wiced_result_t wiced_udp_start_dtls( wiced_udp_socket_t* socket, wiced_ip_address_t ip, wiced_dtls_endpoint_type_t type, wiced_dtls_certificate_verification_t verification )
{
    return wiced_generic_start_dtls_with_ciphers( socket->dtls_context, socket, ip, type, verification, my_ciphers, DTLS_UDP_TRANSPORT );
}

wiced_result_t wiced_generic_start_dtls_with_ciphers( wiced_dtls_context_t* dtls_context, void* send_context, wiced_ip_address_t ip, wiced_dtls_endpoint_type_t type, wiced_tls_certificate_verification_t verification, const cipher_suite_t* cipher_list[ ], dtls_transport_protocol_t transport_protocol )
{
    uint64_t start_time;
    dtls_result_t result;
    dtls_session_t session;

    memcpy( &session.ip, &ip, sizeof( ip ) );
    session.port = TARGET_PORT;

    memset( &dtls_context->context, 0, sizeof(wiced_dtls_workspace_t) );
    dtls_context->context.ciphers = cipher_list;
    dtls_context->context.send_context = send_context;

    start_time = dtls_host_get_time_ms( );
    dtls_context->context.transport_protocol = DTLS_UDP_TRANSPORT;
    dtls_context->context.identity = dtls_context->identity;

    do
    {
        uint64_t curr_time;
        if ( type == WICED_DTLS_AS_SERVER )
        {
            /* Implement DTLS server Handshake */
        }
        else
        {
            result = dtls_handshake_client_async( &dtls_context->context, &session );
            if ( result != DTLS_SUCCESS )
            {
                WPRINT_SECURITY_INFO(( "Error with DTLS client handshake %u\n", (unsigned int)result ));
                goto exit_with_inited_context;
            }
        }

        /* break out if stuck */
        curr_time = dtls_host_get_time_ms( );
        if ( curr_time - start_time > MAX_HANDSHAKE_WAIT )
        {
            WPRINT_SECURITY_INFO(( "Timeout in DTLS handshake\n" ));
            result = DTLS_HANDSHAKE_TIMEOUT;
            goto exit_with_inited_context;
        }

    } while ( dtls_context->context.state != DTLS_HANDSHAKE_OVER );

    return WICED_SUCCESS;

    exit_with_inited_context:
    dtls_close_notify( &dtls_context->context);
    dtls_free(&dtls_context->context,&session);
    return (wiced_result_t) result;
}

wiced_result_t wiced_dtls_close_notify( wiced_udp_socket_t* socket )
{
    dtls_close_notify( &socket->dtls_context->context);
    return WICED_SUCCESS;
}

/*
 * DTLS support functions
 */
dtls_result_t dtls_host_free_packet( uint32_t* packet )
{
    wiced_packet_delete( (wiced_packet_t*) packet );
    return DTLS_SUCCESS;
}

dtls_result_t dtls_host_receive_packet( dtls_context_t* dtls, dtls_packet_t** packet, uint32_t timeout )
{
    dtls_result_t result = DTLS_RECEIVE_FAILED;
    switch ( dtls->transport_protocol )
    {
        case DTLS_UDP_TRANSPORT:
            result = (dtls_result_t) network_udp_receive( (wiced_udp_socket_t*) dtls->send_context, (wiced_packet_t**) packet, timeout );
            break;

        default:
            wiced_assert( "unknown transport", 1 == 0 );
            break;
    }

    return result;
}

dtls_result_t dtls_host_set_packet_start( tls_packet_t* packet, uint8_t* start )
{
    wiced_packet_set_data_start( (wiced_packet_t*) packet, start );
    return DTLS_SUCCESS;
}

/*
 * Calculates the maximium amount of payload that can fit in a given sized buffer
 */
wiced_result_t wiced_dtls_calculate_overhead( wiced_dtls_workspace_t* context, uint16_t available_space, uint16_t* header, uint16_t* footer )
{
    *header = 0;
    *footer = 0;
    if ( context != NULL && context->state == DTLS_HANDSHAKE_OVER )
    {
        /* Add DTLS record header */
        *header = sizeof(dtls_record_header_t);

        /* TODO : Add MAC size based on cipher suite currently only support AES CCM so
         * made it constant 8. will change once we add MAC and cipher driver. */
        *footer += 8;

    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_dtls_encrypt_packet( wiced_dtls_workspace_t* workspace, const wiced_ip_address_t* IP, uint16_t port, wiced_packet_t* packet )
{
    uint8_t* data;
    uint16_t length;
    uint16_t available;
    wiced_result_t result;
    dtls_session_t target;
    int res;

    if ( ( workspace == NULL ) || ( packet == NULL ) )
    {
        return WICED_ERROR;
    }

    if ( wiced_packet_get_data( packet, 0, &data, &length, &available ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    data -= sizeof(dtls_record_header_t);
    result = (wiced_result_t) dtls_host_set_packet_start( (dtls_packet_t*) packet, data );

    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    memcpy( &target.ip, IP, sizeof(wiced_ip_address_t) );
    target.port = port;

    dtls_peer_t *peer = dtls_get_peer( workspace, &target );

    /* Check if peer connection already exists */
    if ( !peer )
    { /* no ==> create one */

        WPRINT_SECURITY_INFO(( "peer not found : wiced_dtls_encrypt_packet\n" ));
        return WICED_ERROR;
    }
    else
    {
        if ( workspace->state != DTLS_HANDSHAKE_OVER )
        {
            return WICED_ERROR;
        }
        else
        {
            res = dtls_prepare_record( peer, workspace, dtls_security_params( peer ), DTLS_CT_APPLICATION_DATA, (dtls_record_t*) data, length, 1 );
        }

        wiced_packet_set_data_end( packet, data + res + sizeof(dtls_record_header_t) );

    }
    return WICED_SUCCESS;
}
