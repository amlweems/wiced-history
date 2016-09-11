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

#include "wiced_result.h"
#include "wiced_tcpip.h"
#include "wiced_rtos.h"
#include "linked_list.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define HTTP_CLRF        "\r\n"
#define HTTP_CRLF_CRLF   "\r\n\r\n"
#define HTTP_SPACE       " "
#define HTTP_COLON       ":"

#define HTTP_VERSION_1_0  "HTTP/1.0"
#define HTTP_VERSION_1_1  "HTTP/1.1"
#define HTTP_VERSION_2    "HTTP/2"

#define HTTP_METHOD_OPTIONS  "OPTIONS"
#define HTTP_METHOD_GET      "GET"
#define HTTP_METHOD_HEAD     "HEAD"
#define HTTP_METHOD_POST     "POST"
#define HTTP_METHOD_PUT      "PUT"
#define HTTP_METHOD_DELETE   "DELETE"
#define HTTP_METHOD_TRACE    "TRACE"
#define HTTP_METHOD_CONNECT  "CONNECT"

#define HTTP_HEADER_HOST            "Host: "
#define HTTP_HEADER_DATE            "Date: "
#define HTTP_HEADER_CONTENT_LENGTH  "Content-Length: "
#define HTTP_HEADER_CONTENT_TYPE    "Content-Type: "
#define HTTP_HEADER_CHUNKED         "Transfer-Encoding: chunked"

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    HTTP_1_0,
    HTTP_1_1,
    HTTP_2,
} http_version_t;

/* rfc2616 */
typedef enum
{
    HTTP_UNKNOWN  = -1,
    HTTP_OPTIONS  =  0,
    HTTP_GET      =  1,
    HTTP_HEAD     =  2,
    HTTP_POST     =  3,
    HTTP_PUT      =  4,
    HTTP_DELETE   =  5,
    HTTP_TRACE    =  6,
    HTTP_CONNECT  =  7,

    HTTP_METHODS_MAX,   /* must be last! */
} http_method_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    char*    field;
    uint16_t field_length;
    char*    value;
    uint16_t value_length;
} http_header_field_t;

typedef struct
{
    http_version_t version;
    uint16_t       code;
} http_status_line_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t http_parse_header( const uint8_t* data, uint16_t length, http_header_field_t* header, uint32_t number_of_header_fields );

wiced_result_t http_get_status_line( const uint8_t* data, uint16_t length, http_status_line_t* status_line );

wiced_result_t http_split_line( const char* line, uint16_t max_length, char** next_line );

wiced_result_t http_get_next_line( const char* line, uint16_t max_length, char** next_line );

wiced_result_t http_get_line_length( const char* line, uint32_t max_line_length, uint32_t* actual_length );

wiced_result_t http_get_next_line_with_length( const char* data, uint16_t data_length, char** next_line, uint32_t* line_length );

wiced_result_t http_get_host( const char* line, uint16_t line_length, char** host, uint16_t* host_length, uint16_t* port );

wiced_result_t http_get_next_string_token( const char* string, uint16_t string_length, char delimiter, char** next_token );

#ifdef __cplusplus
} /* extern "C" */
#endif
