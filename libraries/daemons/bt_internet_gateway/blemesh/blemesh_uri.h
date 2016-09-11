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

#include "http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define BLE_MESH_URIS \
    { "/mesh/sendactuator/value/*", "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { process_send_actuator,    0 }, }, \
    { "/mesh/sendmultipt/value/*",  "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { process_send_multi_point, 0 }, }, \
    { "/mesh/sendp2p/value/*",      "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { process_send_p2p,         0 }, },

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
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

int32_t        process_send_actuator   ( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
int32_t        process_send_multi_point( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
int32_t        process_send_p2p        ( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
wiced_result_t ble_mesh_send_status    ( wiced_http_response_stream_t* stream, const char* status );

#ifdef __cplusplus
} /* extern "C" */
#endif
