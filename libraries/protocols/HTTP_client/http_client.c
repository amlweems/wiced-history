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
#include "http.h"
#include "http_client.h"
#include "wwd_assert.h"
#include "wiced_tls.h"
#include "wiced_utilities.h"
#include "linked_list.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define SIZEOF_STRING_CONST( string ) ( sizeof( string ) - 1 )

/******************************************************
 *                    Constants
 ******************************************************/

#define STATUS_LINE_PATTERN            "HTTP/* * *\r\n*"
#define HTTP_CLIENT_RECEIVE_TIMEOUT_MS ( 4000 )
#define HTTP_CLIENT_STACK_SIZE         ( 6000 )
#define HTTP_CLIENT_EVENT_QUEUE_SIZE   ( 10 )

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
 *               Function Declarations
 ******************************************************/

static wiced_result_t socket_connect_callback    ( wiced_tcp_socket_t* socket, void* arg );
static wiced_result_t socket_disconnect_callback ( wiced_tcp_socket_t* socket, void* arg );
static wiced_result_t socket_receive_callback    ( wiced_tcp_socket_t* socket, void* arg );
static wiced_result_t deferred_connect_handler   ( void* arg );
static wiced_result_t deferred_disconnect_handler( void* arg );
static wiced_result_t deinit_stream_handler      ( void* arg );
static wiced_result_t flush_stream_handler       ( void* arg );
static wiced_result_t client_receive_handler     ( void* arg );
static wiced_result_t find_status_line           ( const char* data, uint32_t data_length, char** status_line, uint32_t* status_line_length );

/******************************************************
 *               Variables Definitions
 ******************************************************/

static const char* http_methods[] =
{
    [HTTP_OPTIONS]  =  HTTP_METHOD_OPTIONS,
    [HTTP_GET    ]  =  HTTP_METHOD_GET,
    [HTTP_HEAD   ]  =  HTTP_METHOD_HEAD,
    [HTTP_POST   ]  =  HTTP_METHOD_POST,
    [HTTP_PUT    ]  =  HTTP_METHOD_PUT,
    [HTTP_DELETE ]  =  HTTP_METHOD_DELETE,
    [HTTP_TRACE  ]  =  HTTP_METHOD_TRACE,
    [HTTP_CONNECT]  =  HTTP_METHOD_CONNECT,
};

/* this structure is used to preserve information related to total data remaining to receive for particular
 * request if content_length > MTU or chunked encoding is enabled.
 */
typedef struct
{
     uint16_t   total_remaining_length;
} http_response_info_t;

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t http_client_configure(http_client_t* client, http_client_configuration_info_t* client_config)
{
    if( client_config->flag & HTTP_CLIENT_CONFIG_FLAG_SERVER_NAME )
    {
        if ( ( client_config->server_name == NULL ) || ( strlen( (char*) client_config->server_name) > MAX_EXT_DATA_LENGTH ) )
        {
            return WICED_ERROR;
        }
    }

    if(client_config->flag & HTTP_CLIENT_CONFIG_FLAG_MAX_FRAGMENT_LEN)
    {
        if( ( client_config->max_fragment_length != TLS_FRAGMENT_LENGTH_512) &&  ( client_config->max_fragment_length != TLS_FRAGMENT_LENGTH_1024) &&
            ( client_config->max_fragment_length != TLS_FRAGMENT_LENGTH_2048) && ( client_config->max_fragment_length != TLS_FRAGMENT_LENGTH_4096))
        {
            WPRINT_LIB_ERROR( ( "Wrong MAX_FRAGMENT_LENGTH value \n"));
            return WICED_ERROR;
        }
    }

    client->config = client_config;
    return WICED_SUCCESS;
}

wiced_result_t http_client_init( http_client_t* client, wiced_interface_t interface, http_event_handler_t event_handler, wiced_tls_identity_t* optional_identity )
{
    wiced_result_t result;

    wiced_assert( "bad arg", ( client != NULL ) );

    memset( client, 0, sizeof( *client ) );

    WICED_VERIFY( wiced_rtos_create_worker_thread( &client->thread, WICED_DEFAULT_LIBRARY_PRIORITY, HTTP_CLIENT_STACK_SIZE, HTTP_CLIENT_EVENT_QUEUE_SIZE ) );

    result = wiced_tcp_create_socket( &client->socket, interface );
    if ( result != WICED_SUCCESS )
    {
        wiced_rtos_delete_worker_thread( &client->thread );
        return result;
    }

    /* Enable TCP callbacks */
    if( ( result = wiced_tcp_register_callbacks( &client->socket, socket_connect_callback, socket_receive_callback, socket_disconnect_callback, (void*)client ) ) != WICED_SUCCESS )
    {
        wiced_tcp_delete_socket(&client->socket);
        wiced_rtos_delete_worker_thread( &client->thread );

        return result;
    }

    linked_list_init( &client->request_list );

    client->event_handler = (uint32_t)event_handler;
    client->tls_identity  = optional_identity;
    return result;
}

wiced_result_t http_client_deinit( http_client_t* client )
{
    wiced_assert( "bad arg", ( client != NULL ) );

    /* TLS context is freed inside TCP delete function */
    wiced_tcp_delete_socket( &client->socket );
    wiced_rtos_delete_worker_thread( &client->thread );
    linked_list_deinit( &client->request_list );
    return WICED_SUCCESS;
}

wiced_result_t http_client_connect( http_client_t* client, const wiced_ip_address_t* server_ip, uint16_t port, http_security_t security, uint32_t timeout_ms )
{
    wiced_assert( "bad arg", ( client != NULL ) && ( server_ip != NULL ) );
    wiced_tls_extension_t extension;
    wiced_result_t result;

    if ( security == HTTP_USE_TLS )
    {
        if ( client->tls_context == NULL )
        {
            client->tls_context = MALLOC_OBJECT( "HTTP Client TLS Context", wiced_tls_context_t );
            wiced_assert( "malloc fail", ( client->tls_context != NULL ) );
            if ( client->tls_context == NULL )
            {
                return WICED_OUT_OF_HEAP_SPACE;
            }
            memset( client->tls_context, 0, sizeof( *( client->tls_context ) ) );
            client->socket.context_malloced = WICED_TRUE;
        }

        /* TODO: check if peer_cn argument is needed */
        wiced_tls_init_context( client->tls_context, client->tls_identity, NULL );

        /* set TLS extensions configured by client */
        if(client->config != NULL)
        {
            if (client->config->flag & HTTP_CLIENT_CONFIG_FLAG_SERVER_NAME)
            {
                extension.type = TLS_EXTENSION_TYPE_SERVER_NAME;
                extension.extension_data.server_name = client->config->server_name;

                if ( (result = wiced_tls_set_extension(client->tls_context, &extension)) != WICED_SUCCESS )
                {
                    WPRINT_LIB_ERROR( ("Unable to set TLS extension\n") );
                    return result;
                }
            }

            if (client->config->flag & HTTP_CLIENT_CONFIG_FLAG_MAX_FRAGMENT_LEN)
            {
                extension.type = TLS_EXTENSION_TYPE_MAX_FRAGMENT_LENGTH;
                extension.extension_data.max_fragment_length = client->config->max_fragment_length;

                if ( (result = wiced_tls_set_extension(client->tls_context, &extension)) != WICED_SUCCESS )
                {
                    WPRINT_LIB_ERROR( ("Unable to set TLS extension\n") );
                    return result;
                }
            }
        }

        wiced_tcp_enable_tls( &client->socket, (void*)client->tls_context );
    }

    return wiced_tcp_connect( &client->socket, server_ip, port, timeout_ms );
}

wiced_result_t http_client_disconnect( http_client_t* client )
{
    wiced_assert( "bad arg", ( client != NULL ) );

    return wiced_tcp_disconnect( &client->socket );
}

wiced_result_t http_request_init( http_request_t* request, http_client_t* client, http_method_t method, const char* uri, http_version_t version )
{
    wiced_result_t result;

    memset( request, 0, sizeof( *request ) );

    wiced_assert( "bad arg", ( request != NULL ) && ( client != NULL ) && ( uri != NULL ) );

    request->owner = client;

    result = wiced_tcp_stream_init( &request->stream, &client->socket );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Request-Line = Method SP Request-URI SP HTTP-Version CRLF */
    WICED_VERIFY ( wiced_tcp_stream_write( &request->stream, http_methods[method], strlen( http_methods[method] ) ) );
    WICED_VERIFY ( wiced_tcp_stream_write( &request->stream, HTTP_SPACE, sizeof( HTTP_SPACE ) - 1 ) );
    WICED_VERIFY ( wiced_tcp_stream_write( &request->stream, (const void*)uri, strlen( uri ) ) );
    WICED_VERIFY ( wiced_tcp_stream_write( &request->stream, HTTP_SPACE, sizeof( HTTP_SPACE ) - 1 ) );

    switch ( version )
    {
        case HTTP_1_0: WICED_VERIFY ( wiced_tcp_stream_write( &request->stream, (const void*)HTTP_VERSION_1_0, sizeof( HTTP_VERSION_1_0 ) - 1 ) ); break;
        case HTTP_1_1: WICED_VERIFY ( wiced_tcp_stream_write( &request->stream, (const void*)HTTP_VERSION_1_1, sizeof( HTTP_VERSION_1_1 ) - 1 ) ); break;
        case HTTP_2:   WICED_VERIFY ( wiced_tcp_stream_write( &request->stream, (const void*)HTTP_VERSION_2,   sizeof( HTTP_VERSION_2   ) - 1 ) ); break;
    }

    WICED_VERIFY ( wiced_tcp_stream_write( &request->stream, HTTP_CLRF, sizeof( HTTP_CLRF ) - 1 ) );
    return result;
}

wiced_result_t http_request_deinit( http_request_t* request )
{
    wiced_assert( "bad arg", ( request != NULL ) );

    return wiced_rtos_send_asynchronous_event( &request->owner->thread, deinit_stream_handler, (void*)request );
}

wiced_result_t http_request_write_header( http_request_t* request, const http_header_field_t* header_fields, uint32_t number_of_fields )
{
    uint32_t a;

    wiced_assert( "bad arg", ( request != NULL ) );

    for ( a = 0; a < number_of_fields; a++ )
    {
        WICED_VERIFY ( wiced_tcp_stream_write( &request->stream, header_fields[a].field, header_fields[a].field_length ) );
        if ( header_fields[a].value_length != 0 )
        {
            WICED_VERIFY ( wiced_tcp_stream_write( &request->stream, header_fields[a].value, header_fields[a].value_length ) );
        }
        WICED_VERIFY ( wiced_tcp_stream_write( &request->stream, HTTP_CLRF, sizeof( HTTP_CLRF ) - 1 ) );
    }

    return WICED_SUCCESS;
}

wiced_result_t http_request_write_end_header( http_request_t* request )
{
    wiced_assert( "bad arg", ( request != NULL ) );
    return wiced_tcp_stream_write( &request->stream, (const void*)HTTP_CLRF, sizeof( HTTP_CLRF ) - 1 );
}

wiced_result_t http_request_write( http_request_t* request, const uint8_t* data, uint32_t length )
{
    wiced_assert( "bad arg", ( request != NULL ) && ( data != NULL ) );
    return wiced_tcp_stream_write( &request->stream, (const void*)data, length );
}

wiced_result_t http_request_flush( http_request_t* request )
{
    wiced_assert( "bad arg", ( request != NULL ) );
    return wiced_rtos_send_asynchronous_event( &request->owner->thread, flush_stream_handler, (void*)request );
}

static wiced_result_t socket_connect_callback( wiced_tcp_socket_t* socket, void* arg )
{
    return wiced_rtos_send_asynchronous_event( &((http_client_t*)arg)->thread, deferred_connect_handler, arg );
}

static wiced_result_t socket_disconnect_callback( wiced_tcp_socket_t* socket, void* arg )
{
    return wiced_rtos_send_asynchronous_event( &((http_client_t*)arg)->thread, deferred_disconnect_handler, arg );
}

static wiced_result_t socket_receive_callback( wiced_tcp_socket_t* socket, void* arg )
{
    return wiced_rtos_send_asynchronous_event( &((http_client_t*)arg)->thread, client_receive_handler, arg );
}

static wiced_result_t deferred_connect_handler( void* arg )
{
    http_client_t* client = (http_client_t*) arg;

    if ( client->event_handler )
    {
        ((http_event_handler_t)client->event_handler)( client, HTTP_CONNECTED, NULL );
    }
    return WICED_SUCCESS;
}

static wiced_result_t deferred_disconnect_handler( void* arg )
{
    http_client_t* client = (http_client_t*) arg;

    if ( client->event_handler )
    {
        ((http_event_handler_t)client->event_handler)( client, HTTP_DISCONNECTED, NULL );
    }
    return WICED_SUCCESS;
}

static wiced_result_t deinit_stream_handler( void* arg )
{
    http_request_t* request = (http_request_t*)arg;

    /* Remove request node from client request list */
    free(request->context);
    linked_list_remove_node( &request->owner->request_list, &request->node );

    return wiced_tcp_stream_deinit( &request->stream );
}

static wiced_result_t flush_stream_handler( void* arg )
{
    http_request_t* request = (http_request_t*)arg;
    wiced_result_t  result;
    http_response_info_t* response_info;

    /* Queue request and then flush */
    WICED_VERIFY( linked_list_insert_node_at_rear( &request->owner->request_list, &request->node ) );

    result = wiced_tcp_stream_flush( &request->stream );
    if ( result != WICED_SUCCESS )
    {
        linked_list_remove_node( &request->owner->request_list, &request->node );
        return result;
    }

    /* store response information for internal purpose */
    response_info = malloc_named ("response_information",sizeof(http_response_info_t));
    if ( response_info == NULL )
    {
       return WICED_OUT_OF_HEAP_SPACE;
    }

    memset(response_info,0,sizeof(http_response_info_t));
    request->context = response_info;

    return result;
}

static wiced_result_t client_receive_handler( void* arg )
{
    http_client_t*  client  = (http_client_t*)arg;
    wiced_packet_t* packet  = NULL;
    uint8_t*        data    = NULL;
    http_request_t* request = NULL;
    char*           parsed_data        = NULL;
    uint32_t        parsed_data_length = 0;
    uint16_t        fragment_available_data_length = 0;
    uint16_t        total_available_data_length    = 0;
    char*           message_data_length_string;
    wiced_result_t  result;
    http_response_t http_response =
    {
        .response_hdr                     = NULL,
        .response_hdr_length              = 0,
        .payload                          = NULL,
        .payload_data_length              = 0,
        .remaining_length                 = 0,
    };

    /* exhaust reading of all the available packets from socket in order
     * to handle both in-order and out-of-order arrival of packets
     */
    do
    {
        /* set timeout value to 0 as TCP stack will only notify when there are packets to consume */
        result = wiced_tcp_receive( &client->socket, &packet, WICED_NO_WAIT );
        if (result == WICED_TIMEOUT)
        {
               WPRINT_LIB_INFO (("No packet available\n\r"));
               return WICED_SUCCESS; /* treat no data as success */
        } else if (result != WICED_SUCCESS)
        {
               WPRINT_LIB_ERROR ("Failed to receive packet\n\r");
               return result;
        }

        wiced_packet_get_data( packet, 0, &data, &fragment_available_data_length, &total_available_data_length );

        /* response comes in sequence to which request being sent */
        linked_list_get_front_node( &client->request_list, (linked_list_node_t**) &request );

        if(request != NULL)
        {
            http_response_info_t* response_info = (http_response_info_t*) request->context;
            /* if more data is available for this request then just sent in case of content_length > MTU size. */
            if( response_info->total_remaining_length > 0)
            {
                http_response.request = request;
                http_response.response_hdr = NULL;
                http_response.response_hdr_length = 0;
                http_response.payload    = data;
                http_response.payload_data_length  = fragment_available_data_length;

                http_response.remaining_length = response_info->total_remaining_length - fragment_available_data_length;
                response_info->total_remaining_length = http_response.remaining_length;

                if ( client->event_handler != 0 )
                {
                     http_response.request = request;
                    ( (http_event_handler_t) client->event_handler )( client, HTTP_DATA_RECEIVED, &http_response );
                }
            }
            else
            {
                if ( find_status_line( (char*) data, fragment_available_data_length, &parsed_data, &parsed_data_length ) == WICED_SUCCESS )
                {
                    /* Now extract packet payload info such as data, data length, data type and message length */
                    http_response.payload = (uint8_t*) strnstr( (char*)data, fragment_available_data_length, HTTP_CRLF_CRLF, sizeof( HTTP_CRLF_CRLF ) - 1 );

                    /* This will have HTTP header length */
                    http_response.response_hdr_length = (http_response.payload - data);

                    /* Payload starts just after the header */
                    http_response.payload += strlen( HTTP_CRLF_CRLF );

                    if ( strnstr( (char*)data, fragment_available_data_length, HTTP_HEADER_CHUNKED, sizeof( HTTP_HEADER_CHUNKED ) - 1 ) )
                    {
                        //TODO : Implement code to handle chunked encoding response.
                    }
                    else
                    {
                        message_data_length_string = strnstr( (char*)data, fragment_available_data_length, HTTP_HEADER_CONTENT_LENGTH, sizeof( HTTP_HEADER_CONTENT_LENGTH ) - 1 );

                        if ( ( message_data_length_string != NULL ) )
                        {
                            http_response.payload_data_length = (uint16_t) ( (uint8_t*) ( data + fragment_available_data_length ) - http_response.payload );

                            message_data_length_string += ( sizeof( HTTP_HEADER_CONTENT_LENGTH ) - 1 );

                            /* if HTTP response has more paylolad data than what is mentioned in content length then take take number of bytes mentioned in content length */
                            if(strtol( message_data_length_string, NULL, 10 ) < http_response.payload_data_length)
                            {
                                http_response.payload_data_length = strtol(message_data_length_string, NULL, 10);
                            }

                            response_info->total_remaining_length = (uint16_t) ( strtol( message_data_length_string, NULL, 10 ) - http_response.payload_data_length );
                            http_response.remaining_length = response_info->total_remaining_length;
                      }
                    }

                    if ( client->event_handler != 0 )
                    {
                        http_response.request = request;
                        http_response.response_hdr = data;

                        ( (http_event_handler_t) client->event_handler )( client, HTTP_DATA_RECEIVED, &http_response );
                    }
                }
            }

            /* all the data has been received for this particular request just delete from list */
            if(response_info->total_remaining_length == 0)
            {
               free(response_info);
               linked_list_remove_node_from_front( &client->request_list, (linked_list_node_t**) &request );
            }
        }

        wiced_packet_delete( packet );
    } while ( 1 );
    return WICED_SUCCESS;
}


static wiced_result_t find_status_line( const char* data, uint32_t data_length, char** status_line, uint32_t* status_line_length )
{
    uint32_t a;

    for ( a = 0; a < data_length; a++ )
    {
        if ( match_string_with_wildcard_pattern( (char*)data + a, data_length - a, STATUS_LINE_PATTERN ) != 0 )
        {
            return WICED_SUCCESS;
        }
    }

    return WICED_NOT_FOUND;
}
