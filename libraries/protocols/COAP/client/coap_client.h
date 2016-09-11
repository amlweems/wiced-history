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

#include "wiced.h"
#include "coap_common.h"
#include "coap_client_common.h"
#include "coap_client_internal.h"

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

/******************************************************
 *                 Type Definitions
 ******************************************************/
/* NOTE : don't try to modify this value */
/* application has to allocate memory and give the reference and this should not be in stack and application has to free it after use */
#define WICED_COAP_OBJECT_MEMORY_SIZE_REQUIREMENT       sizeof(wiced_coap_client_t)

/** Initialize instance for CoAP client.
 *
 * @param client [in]   : Client instance to be created.
 * @param interface [in]: WLAN interface (STA,AP).
 * @param callback [in] : Event callback function needs to be registered to library.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_coap_client_init( wiced_coap_client_t* client, wiced_interface_t interface, wiced_service_callback callback );

/** This is an asynchronous API. The response of GET will be given in the event callback with event type WICED_COAP_CLIENT_EVENT_TYPE_GET_RECEIVED.
 *
 * @param client [in]   : Client instance that is already created.
 * @param request [in]  : request with required option and payload information.
 * @param msg_type [in] : message type that which client wants to send.
 * @param ip [in]       : ip address to where you want to send request.
 * @param port [in]     : Port no to which you want to send request.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_coap_client_get( wiced_coap_client_t* client, wiced_coap_client_request_t* request, wiced_coap_msgtype_t msg_type,  wiced_ip_address_t ip, uint16_t port );

/** This is an asynchronous API. The response of POST will be given in the event callback with event type WICED_COAP_CLIENT_EVENT_TYPE_POSTED.
 *
 * @param client [in]   : Client instance that is already created.
 * @param request [in]  : request with required option and payload information.
 * @param msg_type [in] : message type that which client wants to send.
 * @param ip [in]       : ip address to where you want to send request.
 * @param port [in]     : Port no to which you want to send request.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_coap_client_post( wiced_coap_client_t* client, wiced_coap_client_request_t* request, wiced_coap_msgtype_t msg_type,  wiced_ip_address_t ip, uint16_t port );

/** This is an asynchronous API. The response of OBSERVE will be given in the event callback with event type WICED_COAP_CLIENT_EVENT_TYPE_OBSERVED.
 *
 * @param client [in]   : Client instance that is already created.
 * @param request [in]  : request with required option and payload information.
 * @param msg_type [in] : message type that which client wants to send.
 * @param token_id [in] : token information with token and length.
 * @param ip [in]       : ip address to where you want to send request.
 * @param port [in]     : Port no to which you want to send request.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_coap_client_observe( wiced_coap_client_t* client, wiced_coap_client_request_t* client_request, wiced_coap_msgtype_t msg_type, wiced_coap_token_info_t* token_id, wiced_ip_address_t ip, uint16_t port );

/** DeInitialize instance for CoAP client.
 *
 * @param client [in]   : Client instance to be deInitialized.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_coap_client_deinit( wiced_coap_client_t* client );

#ifdef __cplusplus
} /* extern "C" */
#endif
