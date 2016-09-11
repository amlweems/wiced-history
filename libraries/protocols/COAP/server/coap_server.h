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
#include "wiced_crypto.h"
#include "coap_server_common.h"
#include "coap_server_internal.h"

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
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                 Function Definitions
 ******************************************************/

/** Create and Initialize COAP server
 *
 * @param server [in]   : Server instance to be created
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_coap_server_init( wiced_coap_server_t* server );

/** Destroy and de-initialize COAP server
 *
 * @param server [in]   : Server instance to be destroyed
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_coap_server_deinit( wiced_coap_server_t* server );

/** Start COAP Server
 *
 * @param server [in]    : Server instance
 * @param interface [in] : Network interface
 * @param port [in]      : Port no to which you want server to listen for requests.
 * @param security [in]  : security parameters to be used. Includes RSA public certificate, private
 *                         key and root certificate (usually self certified)
 *                         If set to NULL, no security is going to be used
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_coap_server_start( wiced_coap_server_t* server, wiced_interface_t interface, uint16_t port, wiced_coap_security_t *security );

/** Stop COAP Server
 *
 * @param server [in]   : Server instance that needs to be stopped
 *
 *  @return @ref wiced_result_t
 */
wiced_result_t wiced_coap_server_stop( wiced_coap_server_t* server );

/** Register or Add new service with COAP server
 *
 * @param server             : Server instance
 * @param service            : Service object to be added to server
 * @param service_name[in]   : Name of the service it should be 'null' terminated
 * @param callback [in]      : Service call-back
 * @param content_type [in]  : Content type
 *
 *  @return @ref wiced_result_t
 */
wiced_result_t wiced_coap_server_add_service( wiced_coap_server_t* server, wiced_coap_server_service_t* service, char* service_name, wiced_coap_server_callback callback, wiced_coap_content_type_t type );

/** De-register or delete particular service with COAP server
 *
 * @param server[in]     : Server instance
 * @param service[in]    : Service object that needs to be delete
 *
 *  @return @ref wiced_result_t
 */
wiced_result_t wiced_coap_server_delete_service( wiced_coap_server_t* server, wiced_coap_server_service_t* service );

/** Send response back to COAP client
 *
 * @param server[in]            : server instance to be used to send response
 * @param service [in]          : service object to be used to send response
 * @param req_handle [in]       : request handle
 * @param response [in]         : Response that will be sent from callback
 * @param notification_type[in] : Notification type
 *
 *  @return @ref wiced_result_t
 */
wiced_result_t wiced_coap_server_send_response( void* server, wiced_coap_server_service_t* service, void* req_handle, wiced_coap_server_response_t* response, wiced_coap_notification_type notification_type );

#ifdef __cplusplus
} /* extern "C" */
#endif
