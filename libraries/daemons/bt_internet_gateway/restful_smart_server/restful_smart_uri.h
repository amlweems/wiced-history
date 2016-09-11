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

#define RESTFUL_SMART_URIS \
    { "/gatt/nodes/*/services",                      "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { discover_services,                     0 }, }, \
    { "/gatt/nodes/*/services/*/characteristics",    "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { discover_characteristics_of_a_service, 0 }, }, \
    { "/gatt/nodes/*/characteristics",               "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { discover_characteristics_by_uuid,      0 }, }, \
    { "/gatt/nodes/*/characteristics/*/descriptors", "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { discover_descriptors,                  0 }, }, \
    { "/gatt/nodes/*/characteristics/*/value",       "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { read_characteristic_value,             0 }, }, \
    { "/gatt/nodes/*/characteristics/value",         "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { read_write_characteristic_values,      0 }, }, \
    { "/gatt/nodes/*/characteristics/*/value/*",     "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { write_characteristic_value,            0 }, }, \
    { "/gatt/nodes/*/characteristics/*",             "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { manage_characteristic,                 0 }, }, \
    { "/gatt/nodes/*/descriptors/*/value",           "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { read_descriptor_value,                 0 }, }, \
    { "/gatt/nodes/*/descriptors/*/value/*",         "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { write_descriptor_value,                0 }, }, \
    { "/management/nodes/*",                         "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { manage_node_security,                  0 }, }, \
    { "/management/nodes",                           "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { list_bonded_nodes,                     0 }, }, \
    { "/gap/nodes/*",                                "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { manage_node_connection,                0 }, }, \
    { "/gap/nodes",                                  "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { discover_nodes,                        0 }, },

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

int32_t discover_services                    ( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
int32_t discover_characteristics_of_a_service( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
int32_t discover_characteristics_by_uuid     ( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
int32_t manage_characteristic                ( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
int32_t read_characteristic_value            ( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
int32_t read_write_characteristic_values     ( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
int32_t write_characteristic_value           ( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
int32_t discover_descriptors                 ( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
int32_t read_descriptor_value                ( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
int32_t write_descriptor_value               ( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
int32_t discover_nodes                       ( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
int32_t manage_node_connection               ( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
int32_t list_bonded_nodes                    ( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );
int32_t manage_node_security                 ( const char* url_path, const char* url_query_string, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );

#ifdef __cplusplus
} /* extern "C" */
#endif
