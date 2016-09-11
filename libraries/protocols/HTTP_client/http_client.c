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

#define STATUS_LINE_PATTERN  "HTTP/* * *\r\n*"

#define HTTP_CLIENT_RECEIVE_TIMEOUT_MS ( 4000 )
#define HTTP_CLIENT_STACK_SIZE         ( 6000 )
#define HTTP_CLIENT_EVENT_QUEUE_SIZE     ( 10 )

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

/******************************************************
 *               Function Definitions
 ******************************************************/

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
    wiced_tcp_register_callbacks( &client->socket, socket_connect_callback, socket_receive_callback, socket_disconnect_callback, (void*)client );

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
    wiced_tcp_stream_write( &request->stream, http_methods[method], strlen( http_methods[method] ) );
    wiced_tcp_stream_write( &request->stream, HTTP_SPACE, sizeof( HTTP_SPACE ) - 1 );
    wiced_tcp_stream_write( &request->stream, (const void*)uri, strlen( uri ) );
    wiced_tcp_stream_write( &request->stream, HTTP_SPACE, sizeof( HTTP_SPACE ) - 1 );

    switch ( version )
    {
        case HTTP_1_0: wiced_tcp_stream_write( &request->stream, (const void*)HTTP_VERSION_1_0, sizeof( HTTP_VERSION_1_0 ) - 1 ); break;
        case HTTP_1_1: wiced_tcp_stream_write( &request->stream, (const void*)HTTP_VERSION_1_1, sizeof( HTTP_VERSION_1_1 ) - 1 ); break;
        case HTTP_2:   wiced_tcp_stream_write( &request->stream, (const void*)HTTP_VERSION_2,   sizeof( HTTP_VERSION_2   ) - 1 ); break;
    }

    wiced_tcp_stream_write( &request->stream, HTTP_CLRF, sizeof( HTTP_CLRF ) - 1 );
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
        wiced_tcp_stream_write( &request->stream, header_fields[a].field, header_fields[a].field_length );
        if ( header_fields[a].value_length != 0 )
        {
            wiced_tcp_stream_write( &request->stream, HTTP_COLON, sizeof( HTTP_COLON ) - 1 );
            wiced_tcp_stream_write( &request->stream, HTTP_SPACE, sizeof( HTTP_SPACE ) - 1 );
            wiced_tcp_stream_write( &request->stream, header_fields[a].value, header_fields[a].value_length );
        }
        wiced_tcp_stream_write( &request->stream, HTTP_CLRF, sizeof( HTTP_CLRF ) - 1 );
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
    linked_list_remove_node( &request->owner->request_list, &request->node );

    return wiced_tcp_stream_deinit( &request->stream );
}

static wiced_result_t flush_stream_handler( void* arg )
{
    http_request_t* request = (http_request_t*)arg;
    wiced_result_t  result;

    /* Queue request and then flush */
    WICED_VERIFY( linked_list_insert_node_at_rear( &request->owner->request_list, &request->node ) );

    result = wiced_tcp_stream_flush( &request->stream );
    if ( result != WICED_SUCCESS )
    {
        linked_list_remove_node( &request->owner->request_list, &request->node );
    }
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

    WICED_VERIFY( wiced_tcp_receive( &client->socket, &packet, HTTP_CLIENT_RECEIVE_TIMEOUT_MS ) );

    wiced_packet_get_data( packet, 0, &data, &fragment_available_data_length, &total_available_data_length );

    if ( find_status_line( (char*) data, fragment_available_data_length, &parsed_data, &parsed_data_length ) == WICED_SUCCESS )
    {
        /* Status line is found i.e. first data fragment */
        if ( client->response_received == WICED_TRUE )
        {
            /* Remove obsolete element from the list */
            linked_list_remove_node_from_front( &client->request_list, (linked_list_node_t**) &request );
            linked_list_get_front_node( &client->request_list, (linked_list_node_t**) &request );
        }
        else
        {
            linked_list_get_front_node( &client->request_list, (linked_list_node_t**) &request );
            client->response_received = WICED_TRUE;
        }
    }
    else
    {
        linked_list_get_front_node( &client->request_list, (linked_list_node_t**) &request );
    }

    if ( request != NULL )
    {
        http_response_t response;

        response.request = request;
        response.data    = data;
        response.length  = fragment_available_data_length;

        if ( client->event_handler != 0 )
        {
            ( (http_event_handler_t) client->event_handler )( client, HTTP_DATA_RECEIVED, &response );
        }
    }

    /* Parse packet and call callback here */
    wiced_packet_delete( packet );
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
