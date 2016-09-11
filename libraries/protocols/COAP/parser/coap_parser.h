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

#define COAP_PROTOCOL_VER                    0x01
#define COAP_HEADER_LENGTH                      4
#define COAP_HEADER_VERSION_MASK             0xC0
#define COAP_HEADER_VERSION_POSITION            6
#define COAP_HEADER_TYPE_MASK                0x30
#define COAP_HEADER_TYPE_POSITION               4
#define COAP_HEADER_TOKEN_LEN_MASK           0x0F
#define COAP_HEADER_POSITION                 0x03
#define COAP_HEADER_TOKEN_LEN_POSITION          0
#define COAP_DELTA_LENGTH                       4
#define COAP_TOKEN_LENGTH                       8

#define PAYLOAD_MARKER                 0xFF
#define PAYLOAD_MARKER_LENGTH             1
#define OPTION_DELTA_POSITION             4

#define MAKE_RSPCODE(clas, det) ((clas << 5) | (det))

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef enum
{
    COAP_OPTION_IF_MATCH        = 1,
    COAP_OPTION_URI_HOST        = 3,
    COAP_OPTION_ETAG            = 4,
    COAP_OPTION_IF_NONE_MATCH   = 5,
    COAP_OPTION_OBSERVE         = 6,
    COAP_OPTION_URI_PORT        = 7,
    COAP_OPTION_LOCATION_PATH   = 8,
    COAP_OPTION_URI_PATH        = 11,
    COAP_OPTION_CONTENT_FORMAT  = 12,
    COAP_OPTION_MAX_AGE         = 14,
    COAP_OPTION_URI_QUERY       = 15,
    COAP_OPTION_ACCEPT          = 17,
    COAP_OPTION_LOCATION_QUERY  = 20,
    COAP_OPTION_BLOCK2          = 23,
    COAP_OPTION_BLOCK1          = 27,
    COAP_OPTION_PROXY_URI       = 35,
    COAP_OPTION_PROXY_SCHEME    = 39
} coap_option_num_t;

typedef enum
{
    COAP_RSPCODE_NONE           = -1,
    COAP_METHOD_NOT_ALLOWED     = MAKE_RSPCODE( 4, 5 ),
    COAP_NOT_FOUND              = MAKE_RSPCODE( 4, 4 ),
    COAP_RSPCODE_EMPTY          = 0,
    COAP_RSPCODE_CONTENT        = MAKE_RSPCODE( 2, 5 ),
    COAP_RSPCODE_NOT_FOUND      = MAKE_RSPCODE( 4, 4 ),
    COAP_RSPCODE_BAD_REQUEST    = MAKE_RSPCODE( 4, 0 ),
    COAP_RSPCODE_CHANGED        = MAKE_RSPCODE( 2, 4 ),
    COAP_RSPCODE_DELETED        = MAKE_RSPCODE( 2, 2 ),
    COAP_BAD_OPTION             = MAKE_RSPCODE( 4, 2 ),
    COAP_SERVICE_UNAVAILABLE    = MAKE_RSPCODE( 5, 3 ),
    COAP_GATEWAY_TIMEOUT        = MAKE_RSPCODE( 5, 4 ),
    COAP_PROXYING_NOT_SUPPORTED = MAKE_RSPCODE( 5, 5 )
} coap_responsecode_t;

typedef enum
{
    COAP_INTERNAL_METHOD_EMPTY  = 0,
    COAP_INTERNAL_METHOD_GET    = 1,
    COAP_INTERNAL_METHOD_POST   = 2,
    COAP_INTERNAL_METHOD_PUT    = 3,
    COAP_INTERNAL_METHOD_DELETE = 4
} coap_method_t;

typedef enum
{
    COAP_INTERNAL_CONTENTTYPE_NONE                          = -1,
    COAP_INTERNAL_CONTENTTYPE_TEXT_PLAIN                    = 0,
    COAP_INTERNAL_CONTENTTYPE_TEXT_XML                      = 1,
    COAP_INTERNAL_CONTENTTYPE_TEXT_CSV                      = 2,
    COAP_INTERNAL_CONTENTTYPE_TEXT_HTML                     = 3,
    COAP_INTERNAL_CONTENTTYPE_IMAGE_GIF                     = 21,
    COAP_INTERNAL_CONTENTTYPE_IMAGE_JPEG                    = 22,
    COAP_INTERNAL_CONTENTTYPE_IMAGE_PNG                     = 23,
    COAP_INTERNAL_CONTENTTYPE_IMAGE_TIFF                    = 24,
    COAP_INTERNAL_CONTENTTYPE_AUDIO_RAW                     = 25,
    COAP_INTERNAL_CONTENTTYPE_VIDEO_RAW                     = 26,
    COAP_INTERNAL_CONTENTTYPE_APPLICATION_LINKFORMAT        = 40,
    COAP_INTERNAL_CONTENTTYPE_APPLICATION_XML               = 41,
    COAP_INTERNAL_CONTENTTYPE_APPLICATION_OCTET_STREAM      = 42,
    COAP_INTERNAL_CONTENTTYPE_APPLICATION_RDF_XML           = 43,
    COAP_INTERNAL_CONTENTTYPE_APPLICATION_SOAP_XML          = 44,
    COAP_INTERNAL_CONTENTTYPE_APPLICATION_ATOM_XML          = 45,
    COAP_INTERNAL_CONTENTTYPE_APPLICATION_XMPP_XML          = 46,
    COAP_INTERNAL_CONTENTTYPE_APPLICATION_EXI               = 47,
    COAP_INTERNAL_CONTENTTYPE_APPLICATION_X_BXML            = 48,
    COAP_INTERNAL_CONTENTTYPE_APPLICATION_FASTINFOSET       = 49,
    COAP_INTERNAL_CONTENTTYPE_APPLICATION_SOAP_FASTINFOSET  = 50,
    COAP_INTERNAL_CONTENTTYPE_APPLICATION_JSON              = 51
} coap_content_type_t;

typedef struct
{
        uint8_t ver;
        uint8_t t;
        uint8_t tkl;
        uint8_t code;
        uint8_t id[ 2 ];
} coap_header_t;

typedef struct coap_packet
{
        coap_header_t            hdr;
        wiced_coap_token_info_t  token;
        wiced_coap_option_info_t options;
        wiced_coap_buffer_t      payload;
} coap_packet_t;

/******************************************************
 *                 Function Definitions
 ******************************************************/

wiced_result_t coap_parse_header( coap_header_t* hdr, const uint8_t* buf, size_t buflen );
wiced_result_t coap_parse_token( wiced_coap_token_info_t* token, size_t token_len, uint8_t* buf, size_t buflen );
wiced_result_t coap_parse_options_and_payload( coap_packet_t** pkt, uint8_t * buf, size_t buflen );
wiced_result_t coap_release_buffer( void* buffer );
wiced_result_t coap_get_buffer( void** buffer, uint32_t size );
wiced_coap_option_t *coap_find_options( coap_packet_t* request, uint8_t num );

void coap_set_observer( wiced_coap_option_info_t* pkt, uint32_t value );
void coap_set_content_type( wiced_coap_option_info_t* coap_option, wiced_coap_content_type_t content_type );
void coap_serialize_int_option( wiced_coap_option_info_t* coap_option, uint32_t value );
int coap_frame_create( coap_packet_t* pkt, uint8_t* udp_data );

#ifdef __cplusplus
} /* extern "C" */
#endif
