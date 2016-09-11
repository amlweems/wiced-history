/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#pragma once

#include "linked_list.h"
#include "parser/coap_parser.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define COAP_CONNECTION_PORT                 5683
#define COAP_CONNECTION_SECURITY_PORT        5684

/* Conservative size limit, as not all options have to be set at the same time. Check when Proxy-Uri option is used
 * Hdr       options (cof + observer + strings
 * Note : Please update MAX_HEADER_SIZE if you are adding any new option.
 */

#define COAP_MAX_HEADER_SIZE             (4 + COAP_TOKEN_LENGTH + 3 + 4 + 255)  /* 274 */
#define COAP_MAX_RETRANSMIT               4
#define COAP_RESPONSE_TIME                2
/******************************************************
 *                   Enumerations
 ******************************************************/
typedef enum
{
    COAP_RECEIVE_EVENT,
    COAP_SEND_EVENT,
    COAP_TIMER_EVENT,
    COAP_ADD_SERVICE_EVENT,
    COAP_DELETE_SERVICE_EVENT,
    COAP_SERVER_STOP
} coap_server_event_t;

typedef enum
{
    COAP_NOTIFICATIONTYPE_NONE   = -1,
    COAP_NOTIFICATIONTYPE_CON    = 0,
    COAP_NOTIFICATIONTYPE_NONCON = 1
} coap_notificationtype_t;

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
        linked_list_node_t  this_node; /* Linked-list node of this Observer */

        wiced_ip_address_t  addr;
        uint16_t            port;
        uint8_t             token_len;
        uint8_t             token[ WICED_COAP_MAX_TOKEN ];
        int32_t             obs_counter;
} coap_observer_t;

typedef struct coap_transaction
{
        linked_list_node_t    this_node; /* Linked-list node of this transaction */

        wiced_coap_server_service_t* service;
        uint16_t              mid;
        uint16_t              retrans_timer;
        uint16_t              reset_timer;
        uint8_t               retrans_counter;
        wiced_ip_address_t    addr;
        uint16_t              port;
        uint16_t              packet_len;
        uint8_t*              packet;
} coap_transaction_t;

typedef struct coap_response_info
{
        coap_packet_t             response;
        wiced_coap_content_type_t payload_type;
} coap_response_info_t;

typedef struct coap_request_info
{
        coap_packet_t      packet;
        wiced_ip_address_t client_ip;
        uint16_t           port;
} coap_request_info_t;

/**
 * Workspace structure for CoAP service
 * Application should not access these values - they are provided here for reference
 * to provide the compiler with data-type size information allowing static declarations
 */
struct wiced_coap_server_service_s
{
        linked_list_node_t          this_node;
        char                        service_name[ WICED_COAP_MAX_SERVICE_LENGTH ];
        wiced_coap_server_callback callback;
        wiced_coap_content_type_t   content_type;
        linked_list_t               observer_list;
};

/**
 * Workspace structure for CoAP server
 * Application should not access these values - they are provided here for reference
 * to provide the compiler with data-type size information allowing static declarations
 */
typedef struct wiced_coap_server_s
{
        wiced_thread_t              event_thread;
        wiced_queue_t               event_queue;
        wiced_udp_socket_t          socket;
        wiced_dtls_context_t        context;
        wiced_dtls_identity_t       identity;
        linked_list_t               service_list;
        linked_list_t               transaction_list;
        wiced_timed_event_t         coap_timer_event;
        wiced_coap_server_service_t discovery_service;
        int                         discovery_services_bytes_used;
} wiced_coap_server_t;

typedef struct coap_send_response
{
        wiced_coap_server_t*            server;
        wiced_coap_server_service_t*    service;
        coap_request_info_t             request;
        wiced_coap_server_response_t    response;
        coap_responsecode_t          response_code;
        wiced_coap_notification_type notification_type;
} coap_send_response_t;

typedef struct coap_server_service
{
        wiced_coap_server_t*         server;
        wiced_coap_server_service_t* service;
} coap_service_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
