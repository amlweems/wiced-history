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
#include "coap_client_common.h"
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

/* Conservative size limit, as not all options have to be set at the same time. Check when Proxy-Uri option is used
 * Hdr       options (cof + observer + strings
 * Note : Please update MAX_HEADER_SIZE if you are adding any new option.
 */

#define COAP_MAX_HEADER_SIZE             ( 4 + COAP_TOKEN_LENGTH + 3 + 4 + 255)  /* 274 */
#define COAP_MAX_RETRANSMIT               4
#define COAP_RESPONSE_TIME                2
/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct wiced_coap_client_s
{
        wiced_thread_t          event_thread;
        wiced_queue_t           event_queue;
        wiced_udp_socket_t      socket;
        wiced_service_callback  callback;
} wiced_coap_client_t;

typedef struct
{
        coap_packet_t             request;
        wiced_coap_content_type_t payload_type;
} coap_client_request_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
