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
 *  Implements both HTTP and HTTPS servers
 *
 */

#include <string.h>
#include "http_server.h"
#include "wwd_assert.h"
#include "wiced.h"
#include "wiced_utilities.h"
#include "wiced_resource.h"
#include "strings.h"
#include "platform_resource.h"
#include "wiced_utilities.h"
#include "wiced_tls.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define HTTP_SERVER_CONNECT_THREAD_STACK_SIZE  (1500)
#define HTTPS_SERVER_CONNECT_THREAD_STACK_SIZE (4000)

#define HTTP_SERVER_THREAD_PRIORITY    (WICED_DEFAULT_LIBRARY_PRIORITY)
#define HTTP_LISTEN_PORT               (80)
#define HTTP_SERVER_RECEIVE_TIMEOUT    (1000)

/* HTTP Tokens */
#define GET_TOKEN                      "GET "
#define POST_TOKEN                     "POST "
#define PUT_TOKEN                      "PUT "

#define HTTP_1_1_TOKEN                 " HTTP/1.1"
#define FINAL_CHUNKED_PACKET           "0\r\n\r\n"

/*
 * Request-Line =   Method    SP        Request-URI           SP       HTTP-Version      CRLFCRLF
 *              = <-3 char->  <-1 char->   <-1 char->      <-1 char->  <--8 char-->    <-4char->
 *              = 18
 */
#define MINIMUM_REQUEST_LINE_LENGTH    (18)
#define EVENT_QUEUE_DEPTH              (10)
#define COMPARE_MATCH                  (0)
#define MAX_URL_LENGTH                 (100)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    SOCKET_ERROR_EVENT,
    SOCKET_DISCONNECT_EVENT,
    SOCKET_PACKET_RECEIVED_EVENT,
    SERVER_STOP_EVENT,
} http_server_event_t;

typedef enum
{
    HTTP_HEADER_AND_DATA_FRAME_STATE,
    HTTP_DATA_ONLY_FRAME_STATE,
    HTTP_ERROR_STATE
} wiced_http_packet_state_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_tcp_socket_t* socket;
    http_server_event_t event_type;
} server_event_message_t;

typedef struct
{
    linked_list_node_t           node;
    wiced_http_response_stream_t stream;
} http_response_stream_node_t;

typedef struct
{
    wiced_tcp_socket_t*       socket;
    wiced_http_packet_state_t http_packet_state;
    char                      url[ MAX_URL_LENGTH ];
    uint16_t                  url_length;
    wiced_http_message_body_t message_body_descriptor;
} wiced_http_message_context_t;

/******************************************************
 *               Static Function Declarations
 ******************************************************/

static wiced_result_t           http_server_deferred_connect_callback( void* arg );
static wiced_result_t           http_server_connect_callback         ( wiced_tcp_socket_t* socket, void* arg );
static wiced_result_t           http_server_disconnect_callback      ( wiced_tcp_socket_t* socket, void* arg );
static wiced_result_t           http_server_receive_callback         ( wiced_tcp_socket_t* socket, void* arg );
static void                     http_server_event_thread_main        ( uint32_t arg );
static wiced_result_t           http_server_parse_receive_packet     ( wiced_http_server_t* server, wiced_http_response_stream_t* stream, wiced_packet_t* packet );
static wiced_result_t           http_server_process_url_request      ( wiced_http_response_stream_t* stream, const wiced_http_page_t* page_database, char* url, uint32_t url_length, wiced_http_message_body_t* http_message_body );
static uint16_t                 http_server_remove_escaped_characters( char* output, uint16_t output_length, const char* input, uint16_t input_length );
static wiced_packet_mime_type_t http_server_get_mime_type            ( const char* request_data );
static wiced_result_t           http_server_get_request_type_and_url ( char* request, uint16_t request_length, wiced_http_request_type_t* type, char** url_start, uint16_t* url_length );
static wiced_result_t           http_server_find_url_in_page_database( char* url, uint32_t length, wiced_http_message_body_t* http_request, const wiced_http_page_t* page_database, wiced_http_page_t** page_found, wiced_packet_mime_type_t* mime_type );
static wiced_bool_t             http_server_compare_stream_socket    ( linked_list_node_t* node_to_compare, void* user_data );

/******************************************************
 *                 Static Variables
 ******************************************************/

static const char* const http_mime_array[ MIME_UNSUPPORTED ] =
{
    MIME_TABLE( EXPAND_AS_MIME_TABLE )
};

static const char* const http_status_codes[ ] =
{
    [HTTP_200_TYPE] = HTTP_HEADER_200,
    [HTTP_204_TYPE] = HTTP_HEADER_204,
    [HTTP_207_TYPE] = HTTP_HEADER_207,
    [HTTP_301_TYPE] = HTTP_HEADER_301,
    [HTTP_400_TYPE] = HTTP_HEADER_400,
    [HTTP_403_TYPE] = HTTP_HEADER_403,
    [HTTP_404_TYPE] = HTTP_HEADER_404,
    [HTTP_405_TYPE] = HTTP_HEADER_405,
    [HTTP_406_TYPE] = HTTP_HEADER_406,
    [HTTP_412_TYPE] = HTTP_HEADER_412,
    [HTTP_415_TYPE] = HTTP_HEADER_406,
    [HTTP_429_TYPE] = HTTP_HEADER_429,
    [HTTP_444_TYPE] = HTTP_HEADER_444,
    [HTTP_470_TYPE] = HTTP_HEADER_470,
    [HTTP_500_TYPE] = HTTP_HEADER_500,
    [HTTP_504_TYPE] = HTTP_HEADER_504
};

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t wiced_http_server_start( wiced_http_server_t* server, uint16_t port, uint16_t max_sockets, const wiced_http_page_t* page_database, wiced_interface_t interface, uint32_t http_thread_stack_size )
{
    http_response_stream_node_t* stream_node;
    uint32_t a;
    uint32_t server_connect_thread_stack_size;

    /* If this will be an HTTPS server then it requires a larger stack */
    if ( server->tcp_server.tls_identity == NULL )
    {
        memset( server, 0, sizeof( *server ) );
        server_connect_thread_stack_size = HTTP_SERVER_CONNECT_THREAD_STACK_SIZE;
    }
    else
    {
        server_connect_thread_stack_size = HTTPS_SERVER_CONNECT_THREAD_STACK_SIZE;
    }

    /* Store the inputs database */
    server->page_database = page_database;

    wiced_http_server_deregister_callbacks( server );

    /* Allocate space for response streams and insert them into the inactive stream list */
    linked_list_init( &server->inactive_stream_list );
    server->streams = malloc_named( "HTTP streams", sizeof(http_response_stream_node_t) * max_sockets );
    if ( server->streams == NULL )
    {
        return WICED_OUT_OF_HEAP_SPACE;
    }
    memset( server->streams, 0, sizeof(http_response_stream_node_t) * max_sockets );
    stream_node = (http_response_stream_node_t*)server->streams;
    for ( a = 0; a < max_sockets; a++ )
    {
        linked_list_set_node_data( &stream_node[a].node, (void*)&stream_node[a] );
        linked_list_insert_node_at_rear( &server->inactive_stream_list, &stream_node[a].node );
    }

    /* Create linked-list for holding active response streams */
    linked_list_init( &server->active_stream_list );

    /* Create worker thread to process connect events */
    WICED_VERIFY( wiced_rtos_create_worker_thread( &server->connect_thread, HTTP_SERVER_THREAD_PRIORITY, server_connect_thread_stack_size, EVENT_QUEUE_DEPTH ) );

    /* Initialize HTTP server event queue */
    WICED_VERIFY( wiced_rtos_init_queue( &server->event_queue, NULL, sizeof(server_event_message_t), EVENT_QUEUE_DEPTH ) );

    /* Create HTTP server connect thread */
    WICED_VERIFY( wiced_rtos_create_thread( &server->event_thread, HTTP_SERVER_THREAD_PRIORITY, "HTTPserver", http_server_event_thread_main, http_thread_stack_size, server ) );

    /* Initialise the socket state for all sockets and return */
    return wiced_tcp_server_start( &server->tcp_server, interface, port, max_sockets, http_server_connect_callback, http_server_receive_callback, http_server_disconnect_callback, (void*) server );
}

wiced_result_t wiced_http_server_stop( wiced_http_server_t* server )
{
    server_event_message_t current_event;

    current_event.event_type = SERVER_STOP_EVENT;
    current_event.socket     = 0;

    wiced_rtos_push_to_queue( &server->event_queue, &current_event, WICED_NO_WAIT );

    if ( wiced_rtos_is_current_thread( &server->event_thread ) != WICED_SUCCESS )
    {
        wiced_rtos_thread_force_awake( &server->event_thread );
        wiced_rtos_thread_join( &server->event_thread );
        wiced_rtos_delete_thread( &server->event_thread );
        wiced_rtos_delete_worker_thread( &server->connect_thread );
    }

    linked_list_deinit( &server->inactive_stream_list );
    linked_list_deinit( &server->active_stream_list );
    free( server->streams );
    server->streams = NULL;

    return WICED_SUCCESS;
}

wiced_result_t wiced_https_server_start( wiced_https_server_t* server, uint16_t port, uint16_t max_sockets, const wiced_http_page_t* page_database, wiced_tls_identity_t* identity, wiced_interface_t interface, uint32_t url_processor_stack_size )
{
    memset( server, 0, sizeof( *server ) );

    wiced_tcp_server_enable_tls( &server->tcp_server, identity );

    /* Start HTTP server */
    WICED_VERIFY( wiced_http_server_start( server, port, max_sockets, page_database, interface, url_processor_stack_size) );

    /* Enable TLS */
    return WICED_SUCCESS;
}

wiced_result_t wiced_https_server_stop( wiced_https_server_t* server )
{
    return wiced_http_server_stop( server );
}

wiced_result_t wiced_http_server_register_callbacks( wiced_http_server_t* server, http_server_callback_t receive_callback )
{
    wiced_assert( "bad arg", ( server != NULL ) );

    server->receive_callback = receive_callback;

    return WICED_SUCCESS;
}

wiced_result_t wiced_http_server_deregister_callbacks( wiced_http_server_t* server )
{
    wiced_assert( "bad arg", ( server != NULL ) );

    server->receive_callback = NULL;

    return WICED_SUCCESS;
}

wiced_result_t wiced_http_response_stream_enable_chunked_transfer( wiced_http_response_stream_t* stream )
{
    stream->chunked_transfer_enabled = WICED_TRUE;
    return WICED_SUCCESS;
}

wiced_result_t wiced_http_response_stream_disable_chunked_transfer( wiced_http_response_stream_t* stream )
{
    if ( stream->chunked_transfer_enabled == WICED_TRUE )
    {
        /* Send final chunked frame */
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, FINAL_CHUNKED_PACKET, sizeof( FINAL_CHUNKED_PACKET ) - 1 ) );
    }

    stream->chunked_transfer_enabled = WICED_FALSE;
    return WICED_SUCCESS;
}

wiced_result_t wiced_http_response_stream_write_header( wiced_http_response_stream_t* stream, http_status_codes_t status_code, uint32_t content_length, http_cache_t cache_type, wiced_packet_mime_type_t mime_type )
{
    char data_length_string[ 15 ];

    wiced_assert( "bad arg", ( stream != NULL ) );

    memset( data_length_string, 0x0, sizeof( data_length_string ) );

    /*HTTP/1.1 <status code>\r\n*/
    WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, http_status_codes[ status_code ], strlen( http_status_codes[ status_code ] ) ) );
    WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, sizeof( CRLF )-1 ) );

    /* Content-Type: xx/yy\r\n */
    WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, HTTP_HEADER_CONTENT_TYPE, strlen( HTTP_HEADER_CONTENT_TYPE ) ) );
    WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, http_mime_array[ mime_type ], strlen( http_mime_array[ mime_type ] ) ) );
    WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, strlen( CRLF ) ) );

    if ( cache_type == HTTP_CACHE_DISABLED )
    {
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, NO_CACHE_HEADER, strlen( NO_CACHE_HEADER ) ) );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, strlen( CRLF ) ) );
    }

    if ( status_code == HTTP_444_TYPE )
    {
        /* Connection: close */
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, HTTP_HEADER_CLOSE, strlen( HTTP_HEADER_CLOSE ) ) );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, strlen( CRLF ) ) );
    }
    else
    {
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, HTTP_HEADER_KEEP_ALIVE, strlen( HTTP_HEADER_KEEP_ALIVE ) ) );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, strlen( CRLF ) ) );
    }

    if ( stream->chunked_transfer_enabled == WICED_TRUE )
    {
        /* Chunked transfer encoding */
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, HTTP_HEADER_CHUNKED, strlen( HTTP_HEADER_CHUNKED ) ) );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, strlen( CRLF ) ) );
    }
    else if ( content_length != 0 )
    {
        /* Content-Length: xx\r\n */
        sprintf( data_length_string, "%lu", (long) content_length );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, HTTP_HEADER_CONTENT_LENGTH, strlen( HTTP_HEADER_CONTENT_LENGTH ) ) );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, data_length_string, strlen( data_length_string ) ) );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, strlen( CRLF ) ) );
    }

    /* Closing sequence */
    WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, strlen( CRLF ) ) );

    return WICED_SUCCESS;
}

wiced_result_t wiced_http_response_stream_write( wiced_http_response_stream_t* stream, const void* data, uint32_t length )
{
    if ( length == 0 )
    {
        return WICED_BADARG;
    }

    if ( stream->chunked_transfer_enabled == WICED_TRUE )
    {
        char data_length_string[10];
        memset( data_length_string, 0x0, sizeof( data_length_string ) );
        sprintf( data_length_string, "%lx", (long unsigned int)length );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, data_length_string, strlen( data_length_string ) ) );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, sizeof( CRLF ) - 1 ) );
    }

    WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, data, length ) );

    if ( stream->chunked_transfer_enabled == WICED_TRUE )
    {
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, sizeof( CRLF ) - 1 ) );
    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_http_response_stream_write_resource( wiced_http_response_stream_t* stream, const resource_hnd_t* res_id )
{
    wiced_result_t result;
    const void*    data;
    uint32_t       res_size;
    uint32_t       pos = 0;

    do
    {
        resource_result_t resource_result = resource_get_readonly_buffer ( res_id, pos, 0x7fffffff, &res_size, &data );
        if ( resource_result != RESOURCE_SUCCESS )
        {
            return resource_result;
        }

        result = wiced_http_response_stream_write( stream, data, (uint16_t) res_size );
        resource_free_readonly_buffer( res_id, data );
        if ( result != WICED_SUCCESS )
        {
            return result;
        }
        pos += res_size;
    } while ( res_size > 0 );

    return result;
}

wiced_result_t wiced_http_response_stream_flush( wiced_http_response_stream_t* stream )
{
    wiced_assert( "bad arg", ( stream != NULL ) );

    return wiced_tcp_stream_flush( &stream->tcp_stream );
}

wiced_result_t wiced_http_response_stream_disconnect( wiced_http_response_stream_t* stream )
{
    server_event_message_t current_event;
    wiced_http_server_t*   server;

    wiced_assert( "bad arg", ( stream != NULL ) && ( stream->tcp_stream.socket != NULL ) );

    current_event.event_type = SOCKET_DISCONNECT_EVENT;
    current_event.socket     = stream->tcp_stream.socket;

    server = (wiced_http_server_t*)stream->tcp_stream.socket->callback_arg;

    return wiced_rtos_push_to_queue( &server->event_queue, &current_event, WICED_NO_WAIT );
}

wiced_result_t wiced_http_response_stream_init( wiced_http_response_stream_t* stream, wiced_tcp_socket_t* socket )
{
    memset( stream, 0, sizeof( wiced_http_response_stream_t ) );

    stream->chunked_transfer_enabled = WICED_FALSE;

    return wiced_tcp_stream_init( &stream->tcp_stream, socket );
}

wiced_result_t wiced_http_response_stream_deinit( wiced_http_response_stream_t* stream )
{
    return wiced_tcp_stream_deinit( &stream->tcp_stream );
}

wiced_result_t wiced_http_get_query_parameter_value( const char* url_query, const char* parameter_key, char** parameter_value, uint32_t* value_length )
{
    char* iterator = (char*)url_query;

    while ( *iterator != '\0' )
    {
        char*    current_key = iterator;
        uint32_t current_key_length;

        while( ( *iterator != '\0' ) && ( *iterator != '=' ) && ( *iterator != '&' ) )
        {
            iterator++;
        }

        current_key_length = (uint32_t)( iterator - current_key );

        if ( match_string_with_wildcard_pattern( current_key, current_key_length, parameter_key ) != 0 )
        {
            if ( *iterator == '=' )
            {
                *parameter_value = iterator + 1;
                while( *iterator != '\0' && *iterator != '&' )
                {
                    iterator++;
                }
                *value_length = (uint32_t)( iterator - *parameter_value );
            }
            else
            {
                *parameter_value = NULL;
                *value_length    = 0;
            }
            return WICED_SUCCESS;
        }
        else
        {
            iterator++;
        }
    }

    *parameter_value = NULL;
    *value_length    = 0;
    return WICED_NOT_FOUND;
}

uint32_t wiced_http_get_query_parameter_count( const char* url_query )
{
    char*    current_query = (char*) url_query;
    uint32_t count;

    if ( current_query == NULL )
    {
        return 0;
    }

    /* Non-NULL URL query is considered 1 parameter */
    count = 1;

    while ( *current_query != '\0' )
    {
        /* Count up everytime '&' is found */
        if ( *current_query == '&' )
        {
            count++;
        }

        current_query++;
    }

    return count;
}

wiced_result_t wiced_http_match_query_parameter( const char* url_query, const char* parameter_key, const char* parameter_value )
{
    wiced_result_t result;
    char*          value_found;
    uint32_t       value_length;

    result = wiced_http_get_query_parameter_value( url_query, parameter_key, &value_found, &value_length );
    if ( result == WICED_SUCCESS )
    {
        if ( strncmp( parameter_value, value_found, value_length ) != 0 )
        {
            result = WICED_NOT_FOUND;
        }
    }

    return result;
}

static wiced_result_t http_server_connect_callback( wiced_tcp_socket_t* socket, void* arg )
{
    wiced_http_server_t* server = (wiced_http_server_t*)arg;

    return wiced_rtos_send_asynchronous_event( &server->connect_thread, http_server_deferred_connect_callback, (void*)socket );
}

static wiced_result_t http_server_disconnect_callback( wiced_tcp_socket_t* socket, void* arg )
{
    server_event_message_t current_event;

    current_event.event_type = SOCKET_DISCONNECT_EVENT;
    current_event.socket     = socket;
    return wiced_rtos_push_to_queue( &((wiced_http_server_t*)arg)->event_queue, &current_event, WICED_NO_WAIT );
}

static wiced_result_t http_server_receive_callback( wiced_tcp_socket_t* socket, void* arg )
{
    server_event_message_t current_event;

    current_event.event_type = SOCKET_PACKET_RECEIVED_EVENT;
    current_event.socket     = socket;
    return wiced_rtos_push_to_queue( &((wiced_http_server_t*)arg)->event_queue, &current_event, WICED_NO_WAIT );
}

static wiced_result_t http_server_parse_receive_packet( wiced_http_server_t* server, wiced_http_response_stream_t* stream, wiced_packet_t* packet )
{
    wiced_result_t result                        = WICED_SUCCESS;
    wiced_bool_t   disconnect_current_connection = WICED_FALSE;
    char*          start_of_url                  = NULL; /* Suppress compiler warning */
    uint16_t       url_length                    = 0;    /* Suppress compiler warning */
    char*          request_string;
    uint16_t       request_length;
    uint16_t       new_url_length;
    uint16_t       available_data_length;
    uint16_t       http_message_body_length;
    char*          message_data_length_string;
    char*          mime;

    wiced_http_message_body_t http_message_body =
    {
        .data                         = NULL,
        .message_data_length          = 0,
        .total_message_data_remaining = 0,
        .chunked_transfer             = WICED_FALSE,
        .mime_type                    = MIME_UNSUPPORTED,
        .request_type                 = REQUEST_UNDEFINED
    };

    if ( packet == NULL )
    {
        return WICED_ERROR;
    }

    /* Get URL request string from the receive packet */
    result = wiced_packet_get_data( packet, 0, (uint8_t**)&request_string, &request_length, &available_data_length );
    if ( result != WICED_SUCCESS )
    {
        disconnect_current_connection = WICED_TRUE;
        goto exit;
    }

    /* If application registers a receive callback, call the callback before further processing */
    if ( server->receive_callback != NULL )
    {
        result = server->receive_callback( stream, (uint8_t**)&request_string, &request_length );
        if ( result != WICED_SUCCESS )
        {
            if ( result != WICED_PARTIAL_RESULTS )
            {
                disconnect_current_connection = WICED_TRUE;
            }
            goto exit;
        }
    }

    /* Verify we have enough data to start processing */
    if ( request_length < MINIMUM_REQUEST_LINE_LENGTH )
    {
        result = WICED_ERROR;
        goto exit;
    }

    /* Check if this is a close request */
    if ( strnstr( request_string, request_length, HTTP_HEADER_CLOSE, sizeof( HTTP_HEADER_CLOSE ) - 1 ) != NULL )
    {
        disconnect_current_connection = WICED_TRUE;
    }

    /* First extract the URL from the packet */
    result = http_server_get_request_type_and_url( request_string, request_length, &http_message_body.request_type, &start_of_url, &url_length );
    if ( result == WICED_ERROR )
    {
        goto exit;
    }

    /* Remove escape strings from URL */
    new_url_length = http_server_remove_escaped_characters( start_of_url, url_length, start_of_url, url_length );

    /* Now extract packet payload info such as data, data length, data type and message length */
    http_message_body.data = (uint8_t*) strnstr( request_string, request_length, CRLF_CRLF, sizeof( CRLF_CRLF ) - 1 );

    /* This indicates start of data/end of header was not found, so exit */
    if ( http_message_body.data == NULL )
    {
        result = WICED_ERROR;
        goto exit;
    }
    else
    {
        /* Payload starts just after the header */
        http_message_body.data += strlen( CRLF_CRLF );
    }

    mime = strnstr( request_string, request_length, HTTP_HEADER_CONTENT_TYPE, sizeof( HTTP_HEADER_CONTENT_TYPE ) - 1 );
    if ( ( mime != NULL ) && ( mime < (char*) http_message_body.data ) )
    {
        mime += strlen( HTTP_HEADER_CONTENT_TYPE );
        http_message_body.mime_type = http_server_get_mime_type( mime );
    }
    else
    {
        http_message_body.mime_type = MIME_TYPE_ALL;
    }

    if ( strnstr( request_string, request_length, HTTP_HEADER_CHUNKED, sizeof( HTTP_HEADER_CHUNKED ) - 1 ) )
    {
        /* Indicate the format of this frame is chunked. Its up to the application to reassemble */
        http_message_body.chunked_transfer = WICED_TRUE;
        http_message_body_length = (uint16_t) ( (uint8_t*) ( request_string + request_length ) - http_message_body.data );
        http_message_body.message_data_length = (uint16_t) ( strnstr( (char*) http_message_body.data, http_message_body_length, FINAL_CHUNKED_PACKET, sizeof( FINAL_CHUNKED_PACKET ) - 1 ) - (char*) http_message_body.data );
    }
    else
    {
        message_data_length_string = strnstr( request_string, request_length, HTTP_HEADER_CONTENT_LENGTH, sizeof( HTTP_HEADER_CONTENT_LENGTH ) - 1 );

        if ( ( message_data_length_string != NULL ) && ( message_data_length_string < (char*) http_message_body.data ) )
        {
            http_message_body.message_data_length = (uint16_t) ( (uint8_t*) ( request_string + request_length ) - http_message_body.data );

            message_data_length_string += ( sizeof( HTTP_HEADER_CONTENT_LENGTH ) - 1 );

            http_message_body.total_message_data_remaining = (uint16_t) ( strtol( message_data_length_string, NULL, 10 ) - http_message_body.message_data_length );
        }
        else
        {
            http_message_body.message_data_length = 0;
        }
    }

    result = http_server_process_url_request( stream, server->page_database, start_of_url, new_url_length, &http_message_body );

    exit:
    if ( disconnect_current_connection == WICED_TRUE )
    {
        wiced_http_response_stream_disconnect( stream );
    }

    return result;
}

static wiced_result_t http_server_process_url_request( wiced_http_response_stream_t* stream, const wiced_http_page_t* page_database, char* url, uint32_t url_length, wiced_http_message_body_t* http_message_body )
{
    char*                    url_query_parameters = url;
    uint32_t                 query_length         = url_length;
    wiced_http_page_t*       page_found           = NULL;
    wiced_packet_mime_type_t mime_type            = MIME_TYPE_ALL;
    http_status_codes_t      status_code;

    url[ url_length ] = '\x00';

    while ( ( *url_query_parameters != '?' ) && ( query_length > 0 ) && ( *url_query_parameters != '\0' ) )
    {
        url_query_parameters++;
        query_length--;
    }

    if ( query_length != 0 )
    {
        url_length = url_length - query_length;
        *url_query_parameters = '\x00';
        url_query_parameters++;
    }
    else
    {
        url_query_parameters = NULL;
    }

    WPRINT_WEBSERVER_DEBUG( ("Processing request for: %s\n", url) );

    /* Find URL in server page database */
    if ( http_server_find_url_in_page_database( url, url_length, http_message_body, page_database, &page_found, &mime_type ) == WICED_SUCCESS )
    {
        status_code = HTTP_200_TYPE; /* OK */
    }
    else
    {
        status_code = HTTP_404_TYPE; /* Not Found */
    }

    if ( status_code == HTTP_200_TYPE )
    {
        /* Call the content handler function to write the page content into the packet and adjust the write pointers */
        switch ( page_found->url_content_type )
        {
            case WICED_DYNAMIC_URL_CONTENT:
                wiced_http_response_stream_enable_chunked_transfer( stream );
                wiced_http_response_stream_write_header( stream, status_code, CHUNKED_CONTENT_LENGTH, HTTP_CACHE_DISABLED, mime_type );
                page_found->url_content.dynamic_data.generator( url, url_query_parameters, stream, page_found->url_content.dynamic_data.arg, http_message_body );
                wiced_http_response_stream_disable_chunked_transfer( stream );
                break;

            case WICED_RAW_DYNAMIC_URL_CONTENT:
                page_found->url_content.dynamic_data.generator( url, url_query_parameters, stream, page_found->url_content.dynamic_data.arg, http_message_body );
                break;

            case WICED_STATIC_URL_CONTENT:
                wiced_http_response_stream_write_header( stream, status_code, page_found->url_content.static_data.length, HTTP_CACHE_ENABLED, mime_type );
                wiced_http_response_stream_write( stream, page_found->url_content.static_data.ptr, page_found->url_content.static_data.length );
                break;

            case WICED_RAW_STATIC_URL_CONTENT: /* This is just a Location header */
                wiced_http_response_stream_write( stream, HTTP_HEADER_301, strlen( HTTP_HEADER_301 ) );
                wiced_http_response_stream_write( stream, CRLF, strlen( CRLF ) );
                wiced_http_response_stream_write( stream, HTTP_HEADER_LOCATION, strlen( HTTP_HEADER_LOCATION ) );
                wiced_http_response_stream_write( stream, page_found->url_content.static_data.ptr, page_found->url_content.static_data.length );
                wiced_http_response_stream_write( stream, CRLF, strlen( CRLF ) );
                wiced_http_response_stream_write( stream, HTTP_HEADER_CONTENT_LENGTH, strlen( HTTP_HEADER_CONTENT_LENGTH ) );
                wiced_http_response_stream_write( stream, "0", 1 );
                wiced_http_response_stream_write( stream, CRLF_CRLF, strlen( CRLF_CRLF ) );
                break;

            case WICED_RESOURCE_URL_CONTENT:
                /* Fall through */
            case WICED_RAW_RESOURCE_URL_CONTENT:
                wiced_http_response_stream_enable_chunked_transfer( stream );
                wiced_http_response_stream_write_header( stream, status_code, CHUNKED_CONTENT_LENGTH, HTTP_CACHE_DISABLED, mime_type );
                wiced_http_response_stream_write_resource( stream, page_found->url_content.resource_data );
                wiced_http_response_stream_disable_chunked_transfer( stream );
                break;

            default:
                wiced_assert("Unknown entry in URL list", 0 != 0 );
                break;
        }
    }
    else if ( status_code >= HTTP_400_TYPE )
    {
        wiced_http_response_stream_write_header( stream, status_code, NO_CONTENT_LENGTH, HTTP_CACHE_DISABLED, MIME_TYPE_TEXT_HTML );
    }

    WICED_VERIFY( wiced_http_response_stream_flush( stream ) );

    wiced_assert( "Page Serve finished with data still in stream", stream->tcp_stream.tx_packet == NULL );
    return WICED_SUCCESS;
}

static uint16_t http_server_remove_escaped_characters( char* output, uint16_t output_length, const char* input, uint16_t input_length )
{
    uint16_t bytes_copied;
    int a;

    for ( bytes_copied = 0; ( input_length > 0 ) && ( bytes_copied != output_length ); ++bytes_copied )
    {
        if ( *input == '%' )
        {
            if ( *( input + 1 ) == '%' )
            {
                ++input;
                *output = '%';
            }
            else
            {
                *output = 0;
                for ( a = 0; a < 2; ++a )
                {
                    *output = (char) ( *output << 4 );
                    ++input;
                    if ( *input >= '0' && *input <= '9' )
                    {
                        *output = (char) ( *output + *input - '0' );
                    }
                    else if ( *input >= 'a' && *input <= 'f' )
                    {
                        *output = (char) ( *output + *input - 'a' + 10 );
                    }
                    else if ( *input >= 'A' && *input <= 'F' )
                    {
                        *output = (char) ( *output + *input - 'A' + 10 );
                    }
                }
                input_length = (uint16_t) ( input_length - 3 );
            }
        }
        else
        {
            *output = *input;
            --input_length;
        }
        ++output;
        ++input;
    }

    return bytes_copied;
}

static wiced_packet_mime_type_t http_server_get_mime_type( const char* request_data )
{
    wiced_packet_mime_type_t mime_type = MIME_TYPE_TLV;

    if ( request_data != NULL )
    {
        while ( mime_type < MIME_TYPE_ALL )
        {
            if ( strncmp( request_data, http_mime_array[ mime_type ], strlen( http_mime_array[ mime_type ] ) ) == COMPARE_MATCH )
            {
                break;
            }
            mime_type++;
        }
    }
    else
    {
        /* If MIME not specified, assumed all supported (according to rfc2616)*/
        mime_type = MIME_TYPE_ALL;
    }
    return mime_type;
}

static wiced_result_t http_server_get_request_type_and_url( char* request, uint16_t request_length, wiced_http_request_type_t* type, char** url_start, uint16_t* url_length )
{
    char* end_of_url;

    end_of_url = strnstr( request, request_length, HTTP_1_1_TOKEN, sizeof( HTTP_1_1_TOKEN ) - 1 );
    if ( end_of_url == NULL )
    {
        return WICED_ERROR;
    }

    if ( memcmp( request, GET_TOKEN, sizeof( GET_TOKEN ) - 1 ) == COMPARE_MATCH )
    {
        /* Get type  */
        *type = WICED_HTTP_GET_REQUEST;
        *url_start = request + sizeof( GET_TOKEN ) - 1;
        *url_length = (uint16_t) ( end_of_url - *url_start );
    }
    else if ( memcmp( request, POST_TOKEN, sizeof( POST_TOKEN ) - 1 ) == COMPARE_MATCH )
    {
        *type = WICED_HTTP_POST_REQUEST;
        *url_start = request + sizeof( POST_TOKEN ) - 1;
        *url_length = (uint16_t) ( end_of_url - *url_start );
    }
    else if ( memcmp( request, PUT_TOKEN, sizeof( PUT_TOKEN ) - 1 ) == COMPARE_MATCH )
    {
        *type = WICED_HTTP_PUT_REQUEST;
        *url_start = request + sizeof( PUT_TOKEN ) - 1;
        *url_length = (uint16_t) ( end_of_url - *url_start );
    }
    else
    {
        *type = REQUEST_UNDEFINED;
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

static wiced_result_t http_server_find_url_in_page_database( char* url, uint32_t length, wiced_http_message_body_t* http_request, const wiced_http_page_t* page_database, wiced_http_page_t** page_found, wiced_packet_mime_type_t* mime_type )
{
    uint32_t i = 0;

    /* Search URL list to determine if request matches one of our pages, and break out when found */
    while ( page_database[ i ].url != NULL )
    {
        if ( match_string_with_wildcard_pattern( url, length, page_database[ i ].url ) != 0 )
        {
            *mime_type = http_server_get_mime_type( page_database[ i ].mime_type );

            if ( ( *mime_type == http_request->mime_type ) || ( http_request->mime_type == MIME_TYPE_ALL ) )
            {
                *page_found = (wiced_http_page_t*)&page_database[i];
                return WICED_SUCCESS;
            }
        }
        i++;
    }

    return WICED_NOT_FOUND;
}

static void http_server_event_thread_main( uint32_t arg )
{
    wiced_http_server_t*   server = (wiced_http_server_t*) arg;
    server_event_message_t current_event;
    wiced_result_t         result;

    while ( server->quit != WICED_TRUE )
    {
        result = wiced_rtos_pop_from_queue( &server->event_queue, &current_event, WICED_NEVER_TIMEOUT );

        if ( result != WICED_SUCCESS )
        {
            current_event.socket     = NULL;
            current_event.event_type = SOCKET_ERROR_EVENT;
        }

        switch ( current_event.event_type )
        {
            case SOCKET_DISCONNECT_EVENT:
            {
                http_response_stream_node_t* stream;

                /* Search in active stream whether stream for this socket is available. If available, removed it */
                if ( linked_list_find_node( &server->active_stream_list, http_server_compare_stream_socket, (void*)current_event.socket, (linked_list_node_t**)&stream ) == WICED_SUCCESS )
                {
                    linked_list_remove_node( &server->active_stream_list, &stream->node );
                    linked_list_insert_node_at_rear( &server->inactive_stream_list, &stream->node );
                    wiced_http_response_stream_deinit( &stream->stream );
                }

                wiced_tcp_server_disconnect_socket( &server->tcp_server, current_event.socket );
                if ( current_event.socket->tls_context != NULL && current_event.socket->context_malloced == WICED_TRUE )
                {
                    ssl_free( &current_event.socket->tls_context->context );
                }

                break;
            }
            case SERVER_STOP_EVENT:
            {
                http_response_stream_node_t* stream;

                /* Deinit all response stream */
                linked_list_get_front_node( &server->active_stream_list, (linked_list_node_t**)&stream );
                while ( stream != NULL )
                {
                    linked_list_remove_node( &server->active_stream_list, &stream->node );
                    wiced_http_response_stream_deinit( &stream->stream );
                    linked_list_get_front_node( &server->active_stream_list, (linked_list_node_t**)&stream );
                }
                server->quit = WICED_TRUE;
                break;
            }

            case SOCKET_PACKET_RECEIVED_EVENT:
            {
                wiced_tcp_socket_t*          socket = (wiced_tcp_socket_t*)current_event.socket;
                wiced_packet_t*              packet = NULL;
                http_response_stream_node_t* stream = NULL;

                wiced_tcp_receive( socket, &packet, HTTP_SERVER_RECEIVE_TIMEOUT );
                if ( packet != NULL )
                {
                    /* Search if a stream has been created for this socket. If not found, create and insert one to active list */
                    if ( linked_list_find_node( &server->active_stream_list, http_server_compare_stream_socket, (void*)socket, (linked_list_node_t**)&stream ) != WICED_SUCCESS )
                    {
                        linked_list_remove_node_from_front( &server->inactive_stream_list, (linked_list_node_t**)&stream );
                        linked_list_insert_node_at_rear( &server->active_stream_list, &stream->node );
                        wiced_http_response_stream_init( &stream->stream, current_event.socket );
                    }

                    /* Process packet */
                    http_server_parse_receive_packet( server, &stream->stream, packet );
                    wiced_packet_delete( packet );
                }
                break;
            }
            case SOCKET_ERROR_EVENT: /* Fall through */
            default:
            {
                break;
            }
        }
    }

    wiced_tcp_server_stop( &server->tcp_server );
    wiced_rtos_deinit_queue( &server->event_queue );

    WICED_END_OF_CURRENT_THREAD( );
}

static wiced_bool_t http_server_compare_stream_socket( linked_list_node_t* node_to_compare, void* user_data )
{
    wiced_tcp_socket_t*          socket = (wiced_tcp_socket_t*)user_data;
    http_response_stream_node_t* stream = (http_response_stream_node_t*)node_to_compare;

    if ( stream->stream.tcp_stream.socket == socket )
    {
        return WICED_TRUE;
    }
    else
    {
        return WICED_FALSE;
    }
}

static wiced_result_t http_server_deferred_connect_callback( void* arg )
{
    wiced_tcp_socket_t*  socket = (wiced_tcp_socket_t*)arg;
    wiced_http_server_t* server = (wiced_http_server_t*)socket->callback_arg;
    wiced_tls_context_t* context;

    if ( server->tcp_server.tls_identity != NULL )
    {
        if ( socket->tls_context == NULL )
        {
            context = malloc_named("https", sizeof(wiced_tls_context_t));
            if (context == NULL)
            {
                return WICED_OUT_OF_HEAP_SPACE;
            }
            socket->context_malloced = WICED_TRUE;
        }
        else
        {
            context = socket->tls_context;
        }
        wiced_tls_init_context( context, server->tcp_server.tls_identity, NULL );
        wiced_tcp_enable_tls( socket, context );
    }
    return wiced_tcp_server_accept( &server->tcp_server, socket );
}
