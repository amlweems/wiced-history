/*
 * Copyright 2015, Broadcom Corporation
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
#include "wiced_tcpip.h"
#include "wiced_tls.h"
#include "wwd_debug.h"
#include "wiced_utilities.h"

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
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t wiced_http_get( wiced_ip_address_t* address, const char* query, void* buffer, uint32_t buffer_length )
{
    wiced_tcp_socket_t socket;
    wiced_packet_t*    reply_packet;
    wiced_result_t     result = WICED_ERROR;
    wiced_result_t     rx_result;
    char *             buffer_ptr = (char*) buffer;

    if ( ( result = wiced_tcp_create_socket( &socket, WICED_STA_INTERFACE ) ) != WICED_SUCCESS )
    {
        return result;
    }
    result = wiced_tcp_connect( &socket, address, 80, 10000 );
    if ( result != WICED_SUCCESS )
    {
        wiced_tcp_delete_socket( &socket );
        return ( result );
    }

    if ( ( result = wiced_tcp_send_buffer( &socket, query, (uint16_t) strlen( query ) ) != WICED_SUCCESS ) )
    {
        wiced_tcp_disconnect( &socket );
        wiced_tcp_delete_socket( &socket );
        return ( result );
    }

    WPRINT_LIB_INFO( ("waiting for HTTP reply\n") );

    do
    {
        rx_result = wiced_tcp_receive( &socket, &reply_packet, 5000 );
        if ( rx_result == WICED_SUCCESS )
        {
            uint8_t* data;
            uint16_t data_length;
            uint16_t available;
            uint32_t data_to_copy;

            /* Record the fact we received a reply of some kind */
            result = WICED_SUCCESS;

            /* Copy data into provided buffer */
            wiced_packet_get_data( reply_packet, 0, &data, &data_length, &available );
            data_to_copy = MIN(data_length, buffer_length);
            memcpy( buffer_ptr, data, data_to_copy );
            buffer_ptr    += data_to_copy;
            buffer_length -= data_to_copy;
            wiced_packet_delete( reply_packet );
        }
    } while ( rx_result == WICED_SUCCESS );

    wiced_tcp_disconnect( &socket );
    wiced_tcp_delete_socket( &socket );

    return ( result );
}

wiced_result_t wiced_https_get( wiced_ip_address_t* address, const char* query, void* buffer, uint32_t buffer_length, const char* peer_cn )
{
    wiced_tcp_socket_t         socket;
    wiced_packet_t*            reply_packet;
    wiced_result_t             rx_result;
    wiced_tls_context_t        context;
    wiced_result_t             result;
    char*                      buffer_ptr = (char*) buffer;

    wiced_tls_init_context( &context, NULL, peer_cn );

    if ( ( result = wiced_tcp_create_socket( &socket, WICED_STA_INTERFACE ) ) != WICED_SUCCESS )
    {
        return result;
    }
    wiced_tcp_enable_tls( &socket, &context );

    result = wiced_tcp_connect( &socket, address, 443, 20000 );
    if ( result != WICED_SUCCESS )
    {
        wiced_tcp_delete_socket( &socket );
        return ( result );
    }

    if ( ( result = wiced_tcp_send_buffer( &socket, query, (uint16_t) strlen( query ) ) != WICED_SUCCESS ) )
    {
        wiced_tcp_disconnect( &socket );
        wiced_tcp_delete_socket( &socket );
        return ( result );
    }

    WPRINT_LIB_INFO( ("waiting for HTTP reply\n") );

    do
    {
        rx_result = wiced_tcp_receive( &socket, &reply_packet, 5000 );
        if ( rx_result == WICED_SUCCESS )
        {
            uint8_t* data;
            uint16_t data_length;
            uint16_t available;
            uint32_t data_to_copy;

            /* Record the fact we received a reply of some kind */
            result = WICED_SUCCESS;

            /* Copy data into provided buffer */
            wiced_packet_get_data( reply_packet, 0, &data, &data_length, &available );
            data_to_copy = MIN(data_length, buffer_length);
            memcpy( buffer_ptr, data, data_to_copy );
            buffer_ptr    += data_to_copy;
            buffer_length -= data_to_copy;
            wiced_packet_delete( reply_packet );
        }
    } while ( rx_result == WICED_SUCCESS );

    wiced_tcp_disconnect( &socket );
    wiced_tcp_delete_socket( &socket );

    return ( result );
}

wiced_result_t wiced_https_get_with_specified_port( wiced_ip_address_t* address, const char* query, void* buffer, uint32_t buffer_length, uint16_t port, const char* peer_cn )
{
    wiced_tcp_socket_t         socket;
    wiced_packet_t*            reply_packet;
    wiced_result_t             rx_result;
    wiced_tls_context_t        context;
    wiced_result_t             result     = WICED_ERROR;
    char*                      buffer_ptr = (char*) buffer;
    uint32_t                   byte_count = 0;
    uint32_t                   packet_count = 0;

    wiced_tls_init_context( &context, NULL, peer_cn );

    if ( ( result = wiced_tcp_create_socket( &socket, WICED_STA_INTERFACE ) ) != WICED_SUCCESS )
    {
        return result;
    }
    wiced_tcp_enable_tls( &socket, &context );

    result = wiced_tcp_connect( &socket, address, port, 20000 );
    if ( result != WICED_SUCCESS )
    {
        wiced_tcp_delete_socket( &socket );
        return ( result );
    }

    if ( ( result = wiced_tcp_send_buffer( &socket, query, (uint16_t) strlen( query ) ) != WICED_SUCCESS ) )
    {
        wiced_tcp_disconnect( &socket );
        wiced_tcp_delete_socket( &socket );
        return ( result );
    }

    WPRINT_LIB_INFO( ("waiting for HTTP reply\n") );

    do
    {
        rx_result = wiced_tcp_receive( &socket, &reply_packet, 5000 );
        if ( rx_result == WICED_SUCCESS )
        {
            uint8_t* data;
            uint16_t data_length;
            uint16_t available;
            uint32_t data_to_copy;

            /* Record the fact we received a reply of some kind */
            result = WICED_SUCCESS;

            /* Copy data into provided buffer */
            wiced_packet_get_data( reply_packet, 0, &data, &data_length, &available );
            byte_count += data_length;
            packet_count++;
            data_to_copy = MIN(data_length, buffer_length);
            memcpy( buffer_ptr, data, data_to_copy );
            buffer_ptr    += data_to_copy;
            buffer_length -= data_to_copy;
            wiced_packet_delete( reply_packet );
        }
    } while ( rx_result == WICED_SUCCESS );

    WPRINT_LIB_INFO( ("Packet count: %u\n", (unsigned int)packet_count) );
    WPRINT_LIB_INFO( ("Byte count  : %u\n", (unsigned int)byte_count) );

    wiced_tcp_disconnect( &socket );
    wiced_tcp_delete_socket( &socket );

    return ( result );
}
