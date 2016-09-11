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
/**
 * Defines COAP Client event types received in call-back function
 */
typedef enum wiced_coap_client_event_type_s
{
    WICED_COAP_CLIENT_EVENT_TYPE_POSTED = 1,
    WICED_COAP_CLIENT_EVENT_TYPE_GET_RECEIVED,
    WICED_COAP_CLIENT_EVENT_TYPE_OBSERVED,
    WICED_COAP_CLIENT_EVENT_TYPE_NOTIFICATION,
    WICED_COAP_CLIENT_EVENT_TYPE_DELETED
} wiced_coap_client_event_type_t;


/**
 * Defines COAP Client event types received in call-back function
 */
typedef struct wiced_coap_client_event_info_s
{
    wiced_coap_client_event_type_t  type;
    wiced_coap_token_info_t         token;
    struct
    {
        char*                       uri_path;
        wiced_coap_content_type_t   payload_type;
    } opt;
    wiced_coap_buffer_t             payload;
} wiced_coap_client_event_info_t;

/******************************************************
 *                    Structures
 ******************************************************/

/**
 *  Defines response information needs to be send to client.
 */
typedef struct wiced_coap_client_request_s
{
        wiced_coap_option_info_t  options;       /* Options information received in response */
        wiced_coap_content_type_t payload_type;  /* pay-load type needs to be sent */
        wiced_coap_buffer_t       payload;       /* pay-load */
} wiced_coap_client_request_t;

/** Service call-back
 *
 * @param event_info [in] : Information received as response related to
 * payload and options and payload type.
 *
 * @return @ref wiced_result_t
 */
typedef wiced_result_t (*wiced_service_callback)( wiced_coap_client_event_info_t event_info );

#ifdef __cplusplus
} /* extern "C" */
#endif
