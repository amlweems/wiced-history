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
 *  WICED COAP constants, data types which are common to API and library
 */

#pragma once

#include "wiced.h"
#include "wiced_crypto.h"
#include "coap_common.h"

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

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/**
 *  Defines request information needs to be used.
 */
typedef struct wiced_coap_server_request_s
{
        wiced_coap_method_t method;     /* method type requested by client */
        wiced_coap_buffer_t payload;    /* pay-load information given by client */
        void*               req_handle; /* request handle used to send response */
} wiced_coap_server_request_t;

/**
 *  Defines response information needs to be send to client.
 */
typedef struct wiced_coap_server_response_s
{
        wiced_coap_option_info_t    options;        /* Options information */
        wiced_coap_content_type_t   payload_type;   /* pay-load type needs to be sent */
        wiced_coap_buffer_t         payload;        /* pay-load buffer */
} wiced_coap_server_response_t;

typedef struct wiced_coap_server_service_s wiced_coap_server_service_t;

/** Service call-back
 *
 * @param server [in]    : Server instance.
 * @param service [in]   : Service object used for the client request.
 * @param request [in]   : Request information form client
 *
 * @return @ref wiced_result_t
 */
typedef wiced_result_t (*wiced_coap_server_callback)( void* server, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request );

#ifdef __cplusplus
} /* extern "C" */
#endif
