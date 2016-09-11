/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once
#include "wiced_result.h"
#include "wiced_tcpip.h"
#include "wiced_rtos.h"
#include "linked_list.h"
#include "http.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 * @cond               Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    HTTP_NO_SECURITY, /* Standard HTTP over TCP */
    HTTP_USE_TLS,     /* HTTPS over secure TLS tunnel */
} http_security_t;

typedef enum
{
    HTTP_NO_EVENT,
    HTTP_CONNECTED,     /* Connection for HTTP transaction is established */
    HTTP_DISCONNECTED,  /* Connection for HTTP transaction is dropped */
    HTTP_DATA_RECEIVED, /* Marks beginning of the header or the beginning of a segment if the header is fragmented over several packets */
} http_event_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_tcp_socket_t    socket;
    wiced_tls_identity_t* tls_identity;
    wiced_tls_context_t*  tls_context;
    uint32_t              event_handler;
    linked_list_t         request_list;
    wiced_worker_thread_t thread;
    wiced_bool_t          response_received;
} http_client_t;

typedef struct
{
    linked_list_node_t node;
    wiced_tcp_stream_t stream;
    http_client_t*     owner;
} http_request_t;

typedef struct
{
    http_request_t* request;
    uint8_t*        data;
    uint16_t        length;
} http_response_t;

typedef void (*http_event_handler_t)( http_client_t* client, http_event_t event, http_response_t* response );

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 * @endcond
 ******************************************************/

/*****************************************************************************/
/** @addtogroup http      HTTP Client
 *  @ingroup ipcoms
 *
 * Communication functions for HTTP (Hypertext Transfer Protocol) Client
 *
 *  @{
 */
/*****************************************************************************/

/**
 * Initialize HTTP client
 *
 * @param[in] client                : HTTP client
 * @param[in] interface             : WICED interface
 * @param[in] event_handler         : Event callback function
 * @param[in] optional_tls_identity : Pointer to client TLS identity, if available
 *
 * @return @ref wiced_result_t
 */
wiced_result_t http_client_init( http_client_t* client, wiced_interface_t interface, http_event_handler_t event_handler, wiced_tls_identity_t* optional_identity );

/**
 * De-initialize HTTP client
 *
 * @param[in] client : HTTP client
 *
 * @return @ref wiced_result_t
 */
wiced_result_t http_client_deinit( http_client_t* client );

/**
 * Connect to a HTTP server
 *
 * @param[in] client     : HTTP client
 * @param[in] server_ip  : HTTP server IP address
 * @param[in] port       : TCP port
 * @param[in] security   : Security type i.e. HTTP or HTTPS
 * @param[in] timeout_ms : Connection timeout in milliseconds
 *
 * @return @ref wiced_result_t
 */
wiced_result_t http_client_connect( http_client_t* client, const wiced_ip_address_t* server_ip, uint16_t port, http_security_t security, uint32_t timeout_ms );

/**
 * Disconnect client from HTTP server
 *
 * @param[in] client : HTTP client
 *
 * @return @ref wiced_result_t
 */
wiced_result_t http_client_disconnect( http_client_t* client );

/**
 * Initialize a HTTP request
 *
 * @param[in] request : HTTP request
 * @param[in] client  : HTTP client
 * @param[in] method  : HTTP request method
 * @param[in] uri     : Universal Resource Identifier or URI (normally starts with '/')
 * @param[in] version : HTTP version
 *
 * @return @ref wiced_result_t
 */
wiced_result_t http_request_init( http_request_t* request, http_client_t* client, http_method_t method, const char* uri, http_version_t version );

/**
 * De-initialize a HTTP request
 *
 * @param[in] request : HTTP request
 *
 * @return @ref wiced_result_t
 */
wiced_result_t http_request_deinit( http_request_t* request );

/**
 * Write header to the HTTP request
 *
 * @param[in] request          : HTTP request
 * @param[in] header_fields    : Pointer to an array containing header fields and they values
 * @param[in] number_of_fields : Total number of the header fields i.e. the size of the array
 *
 * @return @ref wiced_result_t
 */
wiced_result_t http_request_write_header( http_request_t* request, const http_header_field_t* header_fields, uint32_t number_of_fields );

/**
 * Write end of header (blank line) to the HTTP request
 *
 * @param[in] request : HTTP request
 *
 * @return @ref wiced_result_t
 */
wiced_result_t http_request_write_end_header( http_request_t* request );

/**
 * Write data to the HTTP request
 *
 * @param[in] request : HTTP request
 * @param[in] data    : Data to write
 * @param[in] length  : Data length in bytes
 *
 * @return @ref wiced_result_t
 */
wiced_result_t http_request_write( http_request_t* request, const uint8_t* data, uint32_t length );

/**
 * Flush (send) the HTTP request to the server. The response will be received via the HTTP event handler specified in http_client_init()
 *
 * @param[in] request : HTTP request
 *
 * @return @ref wiced_result_t
 */
wiced_result_t http_request_flush( http_request_t* request );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif
