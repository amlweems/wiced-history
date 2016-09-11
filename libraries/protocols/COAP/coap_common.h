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

#ifdef __cplusplus
extern "C"
{
#endif

#define WICED_COAP_MAX_OPTIONS         (16)
#define WICED_COAP_MAX_SERVICE_LENGTH  (255)
#define WICED_COAP_MAX_TOKEN           (8)

typedef enum
{
    WICED_COAP_CONTENTTYPE_NONE                         = -1,
    WICED_COAP_CONTENTTYPE_TEXT_PLAIN                   = 0,
} wiced_coap_content_type_t;

typedef enum
{
    WICED_COAP_METHOD_GET    = 1,
    WICED_COAP_METHOD_POST   = 2,
    WICED_COAP_METHOD_PUT    = 3,
    WICED_COAP_METHOD_DELETE = 4
} wiced_coap_method_t;

typedef enum
{
    WICED_COAP_MSGTYPE_CON    = 0,
    WICED_COAP_MSGTYPE_NONCON = 1,
    WICED_COAP_MSGTYPE_ACK    = 2,
    WICED_COAP_MSGTYPE_RESET  = 3
} wiced_coap_msgtype_t;

/**
 *  Defines observer notification type supported by CoAP server.
 */
typedef enum
{
    WICED_COAP_NOTIFICATION_TYPE_NONE           = -1, /* Don't send notification to observer */
    WICED_COAP_NOTIFICATION_TYPE_CONFIRMABLE    = 0, /* Notify observer with confirmation */
    WICED_COAP_NOTIFICATION_TYPE_NONCONFIRMABLE = 1
/* Notify observer without confirmation */
} wiced_coap_notification_type;

typedef struct
{
        uint8_t *data;
        size_t  len;
} wiced_coap_buffer_t;

/**
 *  Defines information related to security.
 */
typedef struct wiced_coap_security_s
{
        const char* ca_cert; /* CA certificate */
        const char* cert; /* Client certificate in PEM format */
        const char* key; /* Client private key */
} wiced_coap_security_t;

/**
 *  Defines information related to Options.
 */
typedef struct wiced_coap_option_s
{
        uint8_t             num; /* Option number as per CoAP specification */
        wiced_coap_buffer_t buf; /* Option information */
        char                type[ 4 ];
} wiced_coap_option_t;

/**
 *  Defines information related to Token.
 */
typedef struct wiced_coap_token_info_s
{
        size_t  token_len; /* Token length */
        uint8_t data[ WICED_COAP_MAX_TOKEN ]; /* Token value */
} wiced_coap_token_info_t;

/**
 *  Defines information related to total no. of options and option details.
 */
typedef struct wiced_coap_option_info
{
        uint8_t             num_opts; /* Number of options */
        wiced_coap_option_t option[ WICED_COAP_MAX_OPTIONS ]; /* Option information */
} wiced_coap_option_info_t;

/** API is used to set URI path as option in request.
 *
 * @param Option [in]   : Pass option structure.
 * @param value [in]    : value needs to be set for URI path.
 *
 */
void wiced_coap_set_uri_path( wiced_coap_option_info_t* Option, char* value );

/** API is used to set URI query as option in request.
 *
 * @param Option [in]   : Pass option structure.
 * @param value [in]    : value needs to be set for URI Query.
 *
 */
void wiced_coap_set_uri_query( wiced_coap_option_info_t* Option, char* value );

#ifdef __cplusplus
} /* extern "C" */
#endif
