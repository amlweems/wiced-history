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
#include "x509.h"


#include "wiced_network.h"
#include "dtls.h"
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
/* TODO : This is already there in dtls.h but to pass jenkins added here. Need to correct this  */
#define DTLS_CT_APPLICATION_DATA   23

dtls_result_t dtls_handshake_message(wiced_udp_socket_t* socket, void* args );

dtls_peer_t* dtls_get_peer( dtls_context_t *ctx, const dtls_session_t *session );
void dtls_free_peer( dtls_peer_t *peer );
int dtls_prepare_record( dtls_peer_t* peer, dtls_context_t* context, dtls_security_parameters_t* security, unsigned char type, dtls_record_t *record, size_t len, int need_encrypt );

/******************************************************
 *               Static Function Declarations
 ******************************************************/
static besl_result_t wiced_dtls_load_key( wiced_dtls_key_t* key, const char* key_string, const uint32_t key_length );
static besl_result_t wiced_dtls_load_certificate( wiced_dtls_certificate_t* certificate, const uint8_t* certificate_data, uint32_t certificate_length, wiced_tls_certificate_format_t certificate_format );

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
        &DTLS_PSK_WITH_AES_128_CCM_8,
#endif /* if defined( USE_PSK_KEYSCHEME ) && defined( USE_AES_128_CCM_8_CIPHER ) && defined( USE_AES_128_CCM_8_MAC ) */

#if defined( USE_DTLS_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_DTLS_AES_128_CCM_8_CIPHER ) && defined( USE_DTLS_AES_128_CCM_8_MAC )
        &DTLS_ECDHE_ECDSA_WITH_AES_128_CCM_8,
#endif /* if defined( USE_ECDHE_ECDSA_KEYSCHEME ) && defined( USE_AES_128_CCM_8_CIPHER ) && defined( USE_AES_128_CCM_8_MAC ) */

        0 /* List termination */
};

/******************************************************
 *               Function Definitions
 ******************************************************/

dtls_result_t dtls_host_get_packet_data( dtls_context_t* dtls, dtls_packet_t* packet, uint32_t offset, uint8_t** data, uint16_t* data_length, uint16_t* available_data_length )
{
    uint16_t temp_length;
    uint16_t temp_available_length;
    wiced_result_t result = wiced_packet_get_data( (wiced_packet_t*) packet, (uint16_t) offset, data, &temp_length, &temp_available_length );
    if ( result != WICED_SUCCESS )
    {
        return (dtls_result_t) result;
    }
    *data_length = temp_length;
    *available_data_length = temp_available_length;
    return DTLS_SUCCESS;
}

dtls_result_t dtls_host_packet_get_info( uint32_t* packet, dtls_session_t* session )
{
    wiced_udp_packet_get_info( (wiced_packet_t*) packet, (wiced_ip_address_t*) &session->ip, &session->port );
    return DTLS_SUCCESS;
}

dtls_result_t dtls_host_create_buffer( dtls_context_t* dtls_context, dtls_peer_t* peer, uint8_t** buffer, uint16_t buffer_size )
{
    wiced_assert("", dtls_context->outgoing_packet == NULL);

    /* Round requested buffer size up to next 64 byte chunk (required if encryption is active) */
    buffer_size = (uint16_t) ROUND_UP(buffer_size, 64);

    /* Check if requested buffer fits within a single MTU */
    if ( ( buffer_size < 1300 ) ) /* TODO: Fix this */
    {
        uint16_t actual_packet_size;

        if ( wiced_packet_create_udp( dtls_context->send_context, buffer_size, (wiced_packet_t**) &dtls_context->outgoing_packet, buffer, &actual_packet_size ) != WICED_SUCCESS )
        {
            *buffer = NULL;
            return DTLS_ERROR;
        }

        *buffer -= sizeof(dtls_record_header_t);
        wiced_packet_set_data_start( (wiced_packet_t*) dtls_context->outgoing_packet, *buffer );
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
dtls_result_t dtls_flush_output( dtls_context_t* context, dtls_session_t* session, uint8_t* buffer, uint32_t length )
{
    wiced_packet_set_data_end( (wiced_packet_t*) context->outgoing_packet, buffer + length );

    /* Send the UDP packet */
    if ( wiced_udp_send( context->send_context, (wiced_ip_address_t*) &session->ip, session->port, (wiced_packet_t*) context->outgoing_packet) != WICED_SUCCESS )
    {
        wiced_packet_delete( (wiced_packet_t*) context->outgoing_packet ); /* Delete packet, since the send failed */
        return DTLS_ERROR;
    }

    return DTLS_SUCCESS;

}

uint64_t dtls_host_get_time_ms( void )
{
    uint64_t time_ms;
    wiced_time_get_utc_time_ms( (wiced_utc_time_ms_t*) &time_ms );
    return time_ms;
}

static besl_result_t wiced_dtls_load_key( wiced_dtls_key_t* key, const char* key_string, const uint32_t key_length )
{
    if ( key->type == TLS_PSK_KEY )
    {
        wiced_dtls_psk_key_t* psk_key;
        psk_key = (wiced_dtls_psk_key_t*) key;
        memcpy( psk_key->psk_key, key_string, strlen( key_string ) );
    }
    else if ( key->type == TLS_ECC_KEY )
    {
        if ( x509parse_key_ecc( (wiced_dtls_ecc_key_t*) key, (unsigned char *) key_string, key_length, NULL, 0 ) != 0 )
        {
            wiced_assert( "Key parse error", 0 != 0 );
            return BESL_KEY_PARSE_FAIL;
        }
    }
    else
    {
        wiced_assert( "Key type not found", 0 != 0 );
        return BESL_KEY_PARSE_FAIL;
    }

    return BESL_SUCCESS;
}

static besl_result_t wiced_dtls_load_certificate( wiced_dtls_certificate_t* certificate, const uint8_t* certificate_data, uint32_t certificate_length, wiced_tls_certificate_format_t certificate_format )
{
    besl_result_t result;
    uint32_t der_certificate_length;
    uint32_t total_der_bytes;
    x509_name* name_iter = NULL;

    /* Allocate space for temporary processing */
    certificate->processed_certificate_data = malloc_named( "cert", sizeof(x509_cert) );
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

    name_iter = &certificate->processed_certificate_data->subject;
    for (; name_iter != NULL; name_iter = name_iter->next)
    {
        if (name_iter->val.p[name_iter->val.len] == 0x30)
        {
            char* temp = malloc_named("tls", name_iter->val.len+1);
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
    free( certificate->processed_certificate_data );
    certificate->processed_certificate_data = NULL;

    return BESL_SUCCESS;

end:
    x509_free( certificate->processed_certificate_data );
    free( certificate->processed_certificate_data );
    certificate->processed_certificate_data = NULL;

    if ( certificate->certificate_data_malloced == WICED_TRUE )
    {
        free( (void*)certificate->certificate_data );
        certificate->certificate_data = NULL;
    }

    return result;
}

wiced_result_t wiced_dtls_init_context( wiced_dtls_context_t* context, wiced_dtls_identity_t* identity, const char* peer_cn )
{
    memset( context, 0, sizeof(wiced_dtls_context_t) );

    context->identity = identity;
    return WICED_SUCCESS;
}

wiced_result_t wiced_dtls_init_identity( wiced_dtls_identity_t* identity, const char* private_key, const uint32_t key_length, const uint8_t* certificate_data, uint32_t certificate_length )
{
    besl_result_t result;
    wiced_assert( "Bad args", (identity != NULL) && (private_key != NULL) && (certificate_data != NULL) );

    memset( identity, 0, sizeof( *identity ) );

    /* TODO : Need to find way to load PSK identity and key, currently to differentiate ECC and PSK key scheme taking certificate length which should be atleast 256 if certificate is present else
     * consider it as a PSK identity.
     */
    if ( certificate_length < 256 )
    {
        memcpy( identity->private_key.psk.psk_identity, certificate_data, certificate_length );
        identity->private_key.common.type = TLS_PSK_KEY;
    }
    else
    {
        identity->private_key.common.type = TLS_ECC_KEY;
        result = wiced_dtls_load_certificate( &identity->certificate, certificate_data, certificate_length, TLS_CERTIFICATE_IN_PEM_FORMAT );
        if ( result != BESL_SUCCESS )
        {
            return ( wiced_result_t )result;
        }
    }

    result = wiced_dtls_load_key( (wiced_dtls_key_t*)&identity->private_key, private_key, key_length  );
    if ( result != BESL_SUCCESS )
    {
        return result;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_dtls_deinit_identity( wiced_dtls_identity_t* identity )
{

    if ( identity->certificate.processed_certificate_data != NULL )
    {
        x509_free( identity->certificate.processed_certificate_data );
        identity->certificate.processed_certificate_data = NULL;
    }


   if ( identity->certificate.public_key != NULL )
   {
       dtls_host_free( identity->certificate.public_key );
       identity->certificate.public_key = NULL;
   }

   if ( identity->certificate.common_name != NULL )
   {
       dtls_host_free( (void*)identity->certificate.common_name );
       identity->certificate.common_name = NULL;
   }

   if ( identity->certificate.certificate_data_malloced == WICED_TRUE )
   {
       dtls_host_free( (void*)identity->certificate.certificate_data );
       identity->certificate.certificate_data = NULL;
   }

    return WICED_SUCCESS;
}

wiced_result_t wiced_dtls_reset_context( wiced_dtls_context_t* dtls_context )
{
    dtls_free( (dtls_context_t*)&dtls_context->context, NULL );
    return WICED_SUCCESS;
}

wiced_result_t wiced_udp_enable_dtls( wiced_udp_socket_t* socket, void* context )
{
    socket->dtls_context = context;
    return WICED_SUCCESS;
}

wiced_result_t wiced_dtls_deinit_context( wiced_dtls_context_t* dtls_context )
{
    dtls_peer_t *current_peer, *peer;

    linked_list_get_front_node( &dtls_context->context.peer_list, (linked_list_node_t**) &current_peer );
    while ( current_peer != NULL )
    {
        peer = current_peer;

        current_peer = (dtls_peer_t*) peer->this_node.next;
        linked_list_remove_node( &dtls_context->context.peer_list, &peer->this_node );

        dtls_free_peer (peer);
    }

    wiced_dtls_reset_context( dtls_context );
    return WICED_SUCCESS;
}

wiced_result_t wiced_dtls_receive_packet( wiced_udp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout )
{
    return WICED_SUCCESS;
}

wiced_result_t wiced_udp_start_dtls( wiced_udp_socket_t* socket, wiced_ip_address_t ip, wiced_dtls_endpoint_type_t type, wiced_dtls_certificate_verification_t verification )
{
    return wiced_generic_start_dtls_with_ciphers( socket->dtls_context, socket, ip, type, ( wiced_tls_certificate_verification_t )verification, my_ciphers, DTLS_UDP_TRANSPORT );
}

static wiced_result_t server_receive_callback( wiced_udp_socket_t* socket, void *args )
{
    /* TODO : Instead of processing handshake packets from client in network worker thread context, will create thread for processing
              handshake packets so we can give context back to network worker thread immediately. */
    return dtls_handshake_message (socket, args);
}

wiced_result_t wiced_generic_start_dtls_with_ciphers( wiced_dtls_context_t* dtls_context, void* send_context, wiced_ip_address_t ip, wiced_dtls_endpoint_type_t type, wiced_dtls_certificate_verification_t verification, const cipher_suite_t* cipher_list[ ], dtls_transport_protocol_t transport_protocol )
{
    wiced_result_t result;
    microrng_state  rngstate;

    memset( &dtls_context->context, 0, sizeof(wiced_dtls_workspace_t) );
    dtls_context->context.ciphers = cipher_list;

    wiced_crypto_get_random(dtls_context->context.cookie_secret, 32);

    /* Intialize Peer list */
    linked_list_init (&dtls_context->context.peer_list);

    /* Register callback function to process handshake messages and application data */
    if ( ( result = wiced_udp_register_callbacks( (wiced_udp_socket_t*)send_context, server_receive_callback, dtls_context->callback_arg ) ) != WICED_SUCCESS )
    {
        WPRINT_LIB_ERROR(("Error in registering udp callback\n"));
        return result;
    }

    wiced_crypto_get_random( &rngstate.entropy, 4 );

    dtls_context->context.f_rng = microrng_rand;
    dtls_context->context.p_rng = &rngstate;

    dtls_context->context.identity = dtls_context->identity;

    /* TODO : Add DTLS client code here */

    return WICED_SUCCESS;

}

wiced_result_t wiced_dtls_close_notify( wiced_udp_socket_t* socket )
{
    dtls_close_notify( &socket->dtls_context->context);
    return WICED_SUCCESS;
}

dtls_result_t dtls_host_free_packet( uint32_t* packet )
{
    wiced_packet_delete( (wiced_packet_t*) packet );
    return DTLS_SUCCESS;
}

dtls_result_t dtls_host_receive_packet( dtls_context_t* context, dtls_packet_t** packet, uint32_t timeout )
{
    dtls_result_t result = DTLS_RECEIVE_FAILED;
    result = (dtls_result_t) network_udp_receive( (wiced_udp_socket_t*) context->send_context, (wiced_packet_t**) packet, timeout );

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

    *header = sizeof(dtls_record_header_t);

    /* TODO : Add MAC size based on cipher suite currently only support AES CCM so
     * made it constant 8. will change once we add MAC and cipher driver. */
    *footer += 8;

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
        if ( peer->context.state != DTLS_HANDSHAKE_OVER )
        {
            return WICED_ERROR;
        }
        else
        {
            res = dtls_prepare_record( peer, workspace, dtls_security_params( peer ), DTLS_CT_APPLICATION_DATA, (dtls_record_t*) data, length, 1 );
            if ( res < 0 )
            {
                return WICED_ERROR;
            }
        }

        wiced_packet_set_data_end( packet, data + res + sizeof(dtls_record_header_t) );

    }
    return WICED_SUCCESS;
}

dtls_result_t dtls_handshake_message(wiced_udp_socket_t* socket, void* args )
{
    wiced_packet_t*             packet = NULL;
    dtls_peer_t                 *peer = NULL;
    uint8_t *data;              /* (decrypted) payload */
    dtls_handshake_message_t*   handshake_msg;
    dtls_record_t*              record;
    uint16_t                    record_length;
    uint8_t*                    end_data;
    uint8_t*                    request_string = NULL;
    uint16_t                    request_length;
    uint16_t                    available_data_length;
    dtls_session_t              peer_session;
    dtls_peer_data              peer_data;
    //int                         i = 0;
    dtls_result_t               result;

    UNUSED_PARAMETER(handshake_msg);

    /* Receive Packet from client */
    wiced_udp_receive (socket, &packet, WICED_NO_WAIT);

    dtls_host_get_packet_data(&socket->dtls_context->context, (dtls_packet_t*)packet, 0, &request_string, &request_length, &available_data_length);

    dtls_host_packet_get_info( (uint32_t*)packet, &peer_session);

    socket->dtls_context->context.send_context = socket;

    WPRINT_SECURITY_DEBUG ( ("UDP Rx from IP %u.%u.%u.%u:%d\n",
              (unsigned char) ( ( GET_IPV4_ADDRESS(peer_session.ip) >> 24 ) & 0xff ),
              (unsigned char) ( ( GET_IPV4_ADDRESS(peer_session.ip) >> 16 ) & 0xff ),
              (unsigned char) ( ( GET_IPV4_ADDRESS(peer_session.ip) >> 8 ) & 0xff ),
              (unsigned char) ( ( GET_IPV4_ADDRESS(peer_session.ip) >> 0 ) & 0xff ),
               peer_session.port ));


    /* check if we have DTLS state for addr/port */
    peer = dtls_get_peer(&socket->dtls_context->context, &peer_session);

    /* If Peer is already available and handshake has been over then decrypt data and deliver to application */
    if( peer && peer->context.state ==  DTLS_HANDSHAKE_OVER )
    {
        record = (dtls_record_t*) request_string;
        record_length = ( record->record_header.length[ 0 ] << 8 ) | ( record->record_header.length[ 1 ] );

        if (record->record_header.content_type != DTLS_CT_APPLICATION_DATA)
        {
            wiced_packet_delete (packet);
            return DTLS_SUCCESS;
        }

        int hdr_len = decrypt_verify( &socket->dtls_context->context, &peer_session, record, record_length, &data );
        if ( hdr_len < 0 )
        {
            wiced_packet_delete (packet);
            return DTLS_ERROR_DECRYPTION_FAIL;
        }

        memcpy( record->data, data, hdr_len );
        record->record_header.length[ 0 ] = hdr_len;
        record->record_header.length[ 1 ] = hdr_len >> 8;

        end_data = ( record->data + hdr_len );

        dtls_host_set_packet_start( (tls_packet_t*)packet, record->data );
        wiced_packet_set_data_end( (wiced_packet_t*) packet, end_data );

        peer_data.callback_args = args;
        peer_data.packet = (tls_packet_t*)packet;
        memcpy(&peer_data.session, &peer_session, sizeof(dtls_session_t));

        /* its upto application to delete packet */
        socket->dtls_context->receive_callback (socket, &peer_data);

        return DTLS_SUCCESS;
    }

    /* Peer is not available or handshake has not been finished, process handshake packets */
    while( request_length > 0 )
    {

        record = (dtls_record_t*) request_string;
        record_length = ( record->record_header.length[ 0 ] << 8 ) | ( record->record_header.length[ 1 ] );

        if( ( result = dtls_handshake_server_async (&socket->dtls_context->context, &peer_session, peer, record, record_length + sizeof(dtls_record_header_t)) ) != DTLS_SUCCESS )
        {
            wiced_packet_delete ( packet );
            return DTLS_ERROR;
        }

        request_length -= record_length + sizeof(dtls_record_header_t);
        request_string   +=  record_length + sizeof(dtls_record_header_t);
    }

    wiced_packet_delete ( packet );

    return DTLS_SUCCESS;

}
