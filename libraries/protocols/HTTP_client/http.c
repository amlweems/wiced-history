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
 *
 */
#include "http.h"
#include "wwd_assert.h"
#include "wiced_tls.h"
#include "wiced_utilities.h"
#include "linked_list.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define SIZEOF_STRING_CONST( string ) ( sizeof( string ) - 1 )

/******************************************************
 *                    Constants
 ******************************************************/

#define STATUS_LINE_PATTERN    "HTTP/* * *\r\n"
#define HEADER_END_PATTERN     "*\r\n\r\n*"

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
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t http_get_line_length( const char* line, uint32_t max_line_length, uint32_t* actual_length )
{
    char* next_line;

    if ( http_get_next_line( line, max_line_length, &next_line ) != WICED_SUCCESS )
    {
        return WICED_NOT_FOUND;
    }

    *actual_length = (uint32_t)( next_line - line );
    return WICED_SUCCESS;
}

wiced_result_t http_parse_header( const uint8_t* data, uint16_t length, http_header_field_t* header, uint32_t number_of_header_fields )
{
    char*          line             = (char*)data;
    char*          next_line        = NULL;
    uint32_t       remaining_length = length;
    wiced_result_t result           = WICED_NOT_FOUND;

    while ( http_get_next_line( (const char*)line, remaining_length, &next_line ) == WICED_SUCCESS )
    {
        uint32_t a;

        for ( a = 0; a < number_of_header_fields; a++ )
        {
            if ( memcmp( header[a].field, line, header[a].field_length ) == 0 )
            {
                if ( http_get_next_string_token( (const char*) line, (uint16_t) ( next_line - line ), ':', &( header[ a ].value ) ) == WICED_SUCCESS )
                {
                    header[a].value_length = (uint16_t)( next_line - header[ a ].value ) - ( sizeof( HTTP_CLRF ) - 1 );
                }
                result = WICED_SUCCESS;
            }
        }

        remaining_length -= (uint32_t)( next_line - line );
        line              = next_line;
    }

    return result;
}

wiced_result_t http_get_status_line( const uint8_t* data, uint16_t length, http_status_line_t* status_line )
{
    uint32_t temp;
    char*    code_start;
    char*    code_end;

    /* Version SP Code SP Reason CRLF */
    if ( memcmp( HTTP_VERSION_1_0, data, SIZEOF_STRING_CONST( HTTP_VERSION_1_0 ) ) == 0 )
    {
        status_line->version = HTTP_1_0;
    }
    else if ( memcmp( HTTP_VERSION_1_1, data, SIZEOF_STRING_CONST( HTTP_VERSION_1_1 ) ) == 0 )
    {
        status_line->version = HTTP_1_1;
    }
    else
    {
        /* Bad HTTP version */
        return WICED_BADARG;
    }

    http_get_next_string_token( (char*)data, length, ' ', &code_start );
    http_get_next_string_token( code_start,  length, ' ', &code_end   );

    string_to_unsigned( code_start, (uint8_t)( code_end - code_start ), &temp, 0 );
    status_line->code = (uint16_t)( temp & 0xffff );
    return WICED_SUCCESS;
}

wiced_result_t http_split_line( const char* line, uint16_t max_length, char** next_line )
{
    if ( http_get_next_line( line, max_length, next_line ) == WICED_SUCCESS )
    {
        char* crlf = *next_line - 2;
        crlf[0] = '\0';
        crlf[1] = '\0';
        return WICED_SUCCESS;
    }

    return WICED_NOT_FOUND;
}

wiced_result_t http_get_next_line( const char* line, uint16_t max_length, char** next_line )
{
    uint32_t a;

    for ( a = 0; a < max_length - 1; a++ )
    {
        if ( ( *( line + a ) == '\r' ) && ( *( line + a + 1 ) == '\n' ) )
        {
            *next_line = (char*)line + a + 2;
            return WICED_SUCCESS;
        }
    }

    return WICED_NOT_FOUND;
}

wiced_result_t http_get_next_line_with_length( const char* data, uint16_t data_length, char** next_line, uint32_t* line_length )
{
    if ( http_get_next_line( data, data_length, next_line ) != WICED_SUCCESS )
    {
        return WICED_NOT_FOUND;
    }

    if ( http_get_line_length( *next_line, data_length - (uint16_t)( *next_line - data ), line_length ) != WICED_SUCCESS )
    {
        return WICED_NOT_FOUND;
    }

    return WICED_SUCCESS;
}

wiced_result_t http_get_host( const char* line, uint16_t line_length, char** host, uint16_t* host_length, uint16_t* port )
{
    uint16_t header_host_length = sizeof( HTTP_HEADER_HOST ) - 1;
    char*    port_string;

    if ( memcmp( line, HTTP_HEADER_HOST, header_host_length ) != 0 )
    {
        return WICED_NOT_FOUND;
    }

    *host        = (char*)line + header_host_length;
    *host_length = line_length - header_host_length;

    /* Check if host name contains port number */
    if ( http_get_next_string_token( *host, *host_length, ':', &port_string ) == WICED_SUCCESS )
    {
        uint32_t temp;

        string_to_unsigned( port_string, line_length - (uint16_t)( port_string - line ), &temp, 0 );
        *port = (uint16_t)( temp & 0xffff );
    }
    else
    {
        *port = 0;
    }

    return WICED_SUCCESS;
}

wiced_result_t http_get_next_string_token( const char* string, uint16_t string_length, char delimiter, char** next_token )
{
    uint16_t a;

    for ( a = 0; a < string_length - 1; a++ )
    {
        if ( *( string + a ) == delimiter )
        {
            *next_token = (char*)string + a + 1;
            return WICED_SUCCESS;
        }
    }

    return WICED_NOT_FOUND;
}
