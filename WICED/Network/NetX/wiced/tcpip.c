/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * NetX TCP/IP library
 */

#include "wiced.h"
#include "wiced_network.h"
#include "nx_api.h"
#include "RTOS/wwd_rtos_interface.h"
#include "wiced_tcpip.h"
#include "wwd_assert.h"
#include "wwd_crypto.h"
#include "dns.h"
#include "wiced_tls.h"
#include "tls_host_api.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define MIN(x,y)  ((x) < (y) ? (x) : (y))

#define IP_HANDLE(x)    (wiced_ip_handle[(x)&1])

#define NX_TIMEOUT(timeout_ms)   ((timeout_ms != WICED_NEVER_TIMEOUT) ? ((ULONG)(timeout_ms * SYSTICK_FREQUENCY / 1000)) : NX_WAIT_FOREVER )

#define WICED_LINK_CHECK( ip_handle )  { if ( (ip_handle)->nx_ip_driver_link_up == 0 ){ return WICED_NOTUP; }}

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
 *               Function Declarations
 ******************************************************/

/* TLS helper function to do TCP without involving TLS context */
wiced_result_t network_tcp_receive    ( wiced_tcp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout );
wiced_result_t network_tcp_send_packet( wiced_tcp_socket_t* socket, wiced_packet_t*  packet );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t wiced_tcp_create_socket(wiced_tcp_socket_t* socket, wiced_interface_t interface)
{
    UINT result;

    memset(socket, 0, sizeof(wiced_tcp_socket_t));
    result = nx_tcp_socket_create( &IP_HANDLE(interface), &socket->socket, (CHAR*)"", NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, WICED_TCP_WINDOW_SIZE, NX_NULL, NX_NULL );
    if ( result != NX_SUCCESS )
    {
        wiced_assert("Error creating socket", 0 != 0 )
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_tcp_accept(wiced_tcp_socket_t* socket)
{
    UINT result;

    WICED_LINK_CHECK( socket->socket.nx_tcp_socket_ip_ptr );

    if ( socket->socket.nx_tcp_socket_state != NX_TCP_LISTEN_STATE )
    {
        nx_tcp_socket_disconnect( &socket->socket, NX_TIMEOUT(WICED_TCP_DISCONNECT_TIMEOUT) );
    }

    nx_tcp_server_socket_unaccept( &socket->socket );
    nx_tcp_server_socket_relisten( socket->socket.nx_tcp_socket_ip_ptr, socket->socket.nx_tcp_socket_port, &socket->socket );

    if ( socket->tls_context != NULL )
    {
        wiced_tls_reset_context( socket->tls_context);
    }

    result = nx_tcp_server_socket_accept( &socket->socket, NX_TIMEOUT(WICED_TCP_ACCEPT_TIMEOUT) );
    if ( ( result == NX_NOT_CONNECTED ) ||
         ( result == NX_WAIT_ABORTED ) )
    {
        return WICED_TIMEOUT;
    }
    else if ( result != NX_SUCCESS )
    {
        WPRINT_LIB_INFO(( "Error accepting connection\r\n" ));
        return WICED_ERROR;
    }

    if ( socket->tls_context != NULL )
    {
        if ( wiced_tcp_start_tls( socket, WICED_TLS_AS_SERVER, WICED_TLS_DEFAULT_VERIFICATION ) != WICED_SUCCESS )
        {
            WPRINT_LIB_INFO(( "Error starting TLS connection\r\n" ));
            return WICED_ERROR;
        }
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_tcp_connect(wiced_tcp_socket_t* socket, const wiced_ip_address_t* address, uint16_t port, uint32_t timeout_ms )
{
    WICED_LINK_CHECK( socket->socket.nx_tcp_socket_ip_ptr );

    /* Check if socket is not yet bound to a local port */
    if (!socket->socket.nx_tcp_socket_bound_next)
    {
        UINT result;
        result = nx_tcp_client_socket_bind( &socket->socket, NX_ANY_PORT, NX_TIMEOUT(WICED_TCP_BIND_TIMEOUT) );
        if ( result != NX_SUCCESS )
        {
            wiced_assert("Error binding to port", 0 != 0 );
            return WICED_ERROR;
        }
    }

    if ( NX_SUCCESS != nx_tcp_client_socket_connect( &socket->socket, address->ip.v4, port, NX_TIMEOUT(timeout_ms) ) )
    {
        wiced_assert("Error connecting to TCP destination", 0 != 0 );
        return WICED_ERROR;
    }

    if (socket->tls_context != NULL)
    {
        if (wiced_tcp_start_tls(socket, WICED_TLS_AS_CLIENT, WICED_TLS_DEFAULT_VERIFICATION) != WICED_SUCCESS)
        {
            nx_tcp_socket_disconnect( &socket->socket, NX_TIMEOUT(WICED_TCP_DISCONNECT_TIMEOUT) );
            return WICED_ERROR;
        }
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_tcp_bind( wiced_tcp_socket_t* socket, uint16_t port )
{
    WICED_LINK_CHECK( socket->socket.nx_tcp_socket_ip_ptr );

    if ( NX_SUCCESS != nx_tcp_client_socket_bind( &socket->socket, port, NX_TIMEOUT(WICED_TCP_BIND_TIMEOUT) ) )
    {
        return WICED_ERROR;
    }
    else
    {
        return WICED_SUCCESS;
    }
}


wiced_result_t wiced_tcp_delete_socket(wiced_tcp_socket_t* socket)
{
    if ( socket->tls_context != NULL )
    {
        wiced_tls_deinit_context( socket->tls_context );

        if (socket->context_malloced == WICED_TRUE)
        {
            free(socket->tls_context);
            socket->tls_context = NULL;
            socket->context_malloced = WICED_FALSE;
        }
    }
    nx_tcp_socket_disconnect( &socket->socket, NX_TIMEOUT(WICED_TCP_DISCONNECT_TIMEOUT));
    nx_tcp_server_socket_unaccept( &socket->socket );
    nx_tcp_server_socket_unlisten(socket->socket.nx_tcp_socket_ip_ptr, socket->socket.nx_tcp_socket_port);
    nx_tcp_client_socket_unbind(&socket->socket);
    nx_tcp_socket_delete( &socket->socket );
    return WICED_SUCCESS;
}

wiced_result_t wiced_tcp_listen(wiced_tcp_socket_t* socket, uint16_t port)
{
    WICED_LINK_CHECK( socket->socket.nx_tcp_socket_ip_ptr );

    if ( nx_tcp_server_socket_listen( socket->socket.nx_tcp_socket_ip_ptr, port, &socket->socket, 5, NULL ) == NX_SUCCESS )
    {
        return WICED_SUCCESS;
    }
    else
    {
        return WICED_ERROR;
    }
}

wiced_result_t wiced_tcp_send_packet( wiced_tcp_socket_t* socket, wiced_packet_t* packet )
{
    if ( socket->tls_context != NULL )
    {
        wiced_tls_encrypt_packet( &socket->tls_context->context, packet );
    }

    return network_tcp_send_packet( socket, packet );
}

wiced_result_t network_tcp_send_packet( wiced_tcp_socket_t* socket, wiced_packet_t* packet )
{
    UINT result;

    WICED_LINK_CHECK( socket->socket.nx_tcp_socket_ip_ptr );

    result = nx_tcp_socket_send(&socket->socket, packet, NX_TIMEOUT(WICED_TCP_SEND_TIMEOUT) );
    if ( result == NX_SUCCESS )
    {
        return WICED_SUCCESS;
    }
    else
    {
        return WICED_ERROR;
    }
}

wiced_result_t wiced_tcp_send_buffer(wiced_tcp_socket_t* socket, const void* buffer, uint16_t buffer_length)
{
    NX_PACKET* packet = NULL;
    uint8_t* data_ptr = (uint8_t*)buffer;
    uint8_t* packet_data_ptr;
    uint16_t available_space;

    WICED_LINK_CHECK( socket->socket.nx_tcp_socket_ip_ptr );

    while (buffer_length != 0)
    {
    	uint16_t data_to_write;

        if (wiced_packet_create_tcp(socket, buffer_length, &packet, &packet_data_ptr, &available_space ) != WICED_SUCCESS)
        {
            return WICED_ERROR;
        }

        /* Write data */
        data_to_write = MIN(buffer_length, available_space);
        packet_data_ptr = MEMCAT(packet_data_ptr, data_ptr, data_to_write);

        wiced_packet_set_data_end(packet, packet_data_ptr);

        /* Update variables */
        data_ptr       += data_to_write;
        buffer_length   = (uint16_t)(buffer_length - data_to_write);
        available_space = (uint16_t)(available_space - data_to_write);

        if (wiced_tcp_send_packet(socket, packet) != WICED_SUCCESS)
        {
            wiced_packet_delete(packet);
            return WICED_ERROR;
        }
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_tcp_receive(wiced_tcp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout)
{
    WICED_LINK_CHECK( socket->socket.nx_tcp_socket_ip_ptr );

    if ( socket->tls_context != NULL )
    {
        return wiced_tls_receive_packet( socket, packet, timeout );
    }
    else
    {
        return network_tcp_receive( socket, packet, timeout );
    }
}

wiced_result_t network_tcp_receive(wiced_tcp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout)
{
    UINT result;

    result = nx_tcp_socket_receive( &socket->socket, packet, NX_TIMEOUT(timeout) );

    if ( ( result == NX_NO_PACKET ) || ( result == NX_WAIT_ABORTED ) )
    {
        return WICED_TIMEOUT;
    }
    else if ( result != NX_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_tcp_disconnect( wiced_tcp_socket_t* socket )
{
    WICED_LINK_CHECK( socket->socket.nx_tcp_socket_ip_ptr );

    if ( nx_tcp_socket_disconnect( &socket->socket, NX_TIMEOUT(WICED_TCP_DISCONNECT_TIMEOUT) ) == NX_SUCCESS )
    {
        /* Un-bind the socket, so the TCP port becomes available for other sockets which are suspended on bind requests. This will also flush the receive queue of the socket */
        /* We ignore the return of the unbind as there isn't much we can do */
        nx_tcp_client_socket_unbind( &socket->socket );
        return WICED_SUCCESS;
    }
    else
    {
        nx_tcp_client_socket_unbind( &socket->socket );
        return WICED_ERROR;
    }
}

wiced_result_t wiced_packet_create_tcp(wiced_tcp_socket_t* socket, uint16_t content_length, wiced_packet_t** packet, uint8_t** data, uint16_t* available_space)
{
    uint16_t maximum_segment_size = (uint16_t)MIN(socket->socket.nx_tcp_socket_mss, socket->socket.nx_tcp_socket_connect_mss);
    UINT result;

    UNUSED_PARAMETER(content_length);
    result = nx_packet_allocate( &wiced_packet_pools[0], packet, NX_TCP_PACKET, NX_TIMEOUT(WICED_ALLOCATE_PACKET_TIMEOUT) );
    if ( result == NX_NO_PACKET )
    {
        *packet = NULL;
        *data   = NULL;
        *available_space = 0;
        return WICED_TIMEOUT;
    }
    else if ( result != NX_SUCCESS )
    {
        *packet = NULL;
        *data   = NULL;
        *available_space = 0;
        wiced_assert( "Packet allocation error", 0 != 0 );
        return WICED_ERROR;
    }

    if ( socket->tls_context != NULL && socket->tls_context->context.state == SSL_HANDSHAKE_OVER)
    {
//            uint32_t additional_tls_info_size = sizeof(tls_record_header_t) + (uint32_t) socket->tls_context->context.maclen;
        uint32_t tls_available_space;

        /* Reserve space for the TLS record header */
        ( *packet )->nx_packet_prepend_ptr += sizeof(tls_record_header_t);

        /* Reserve space at the end for the TLS MAC */
        tls_available_space  = (uint32_t)MIN((*packet)->nx_packet_data_end - (*packet)->nx_packet_prepend_ptr, maximum_segment_size);
        tls_available_space -= sizeof(tls_record_header_t);
        if (socket->tls_context->context.ivlen != 0)
        {
            tls_available_space -= tls_available_space%(uint32_t)socket->tls_context->context.ivlen;
        }
        tls_available_space -= (uint32_t)socket->tls_context->context.maclen + 40;
        *available_space = (uint16_t) tls_available_space;
    }
    else
    {
        *available_space = (uint16_t) MIN((*packet)->nx_packet_data_end - (*packet)->nx_packet_prepend_ptr, maximum_segment_size);
    }

    ( *packet )->nx_packet_append_ptr = ( *packet )->nx_packet_prepend_ptr;
    *data = ( *packet )->nx_packet_prepend_ptr;
    return WICED_SUCCESS;
}

wiced_result_t wiced_tcp_stream_init ( wiced_tcp_stream_t* tcp_stream, wiced_tcp_socket_t* socket )
{
    tcp_stream->packet                 = NULL;
    tcp_stream->packet_data            = NULL;
    tcp_stream->packet_space_available = 0;
    tcp_stream->socket                 = socket;
    return WICED_SUCCESS;
}

wiced_result_t wiced_tcp_stream_deinit( wiced_tcp_stream_t* tcp_stream )
{
    wiced_tcp_stream_flush( tcp_stream );
    tcp_stream->packet                 = NULL;
    tcp_stream->packet_data            = NULL;
    tcp_stream->packet_space_available = 0;
    return WICED_SUCCESS;
}

wiced_result_t wiced_tcp_stream_write( wiced_tcp_stream_t* tcp_stream, const void* data, uint16_t data_length )
{
    WICED_LINK_CHECK( tcp_stream->socket->socket.nx_tcp_socket_ip_ptr );

    while (data_length != 0)
    {
    	uint16_t data_to_write;

        /* Check if we don't have a packet */
        if (tcp_stream->packet == NULL)
        {
            wiced_result_t result;
            result = wiced_packet_create_tcp(tcp_stream->socket, data_length, &tcp_stream->packet, &tcp_stream->packet_data, &tcp_stream->packet_space_available );
            if ( result != WICED_SUCCESS)
            {
                return result;
            }
        }

        /* Write data */
        data_to_write = MIN(data_length, tcp_stream->packet_space_available);
        tcp_stream->packet_data = MEMCAT(tcp_stream->packet_data, data, data_to_write);

        /* Update variables */
        data_length                        = (uint16_t)(data_length - data_to_write);
        tcp_stream->packet_space_available = (uint16_t)(tcp_stream->packet_space_available - data_to_write);
        data                               = (void*)((uint32_t)data + data_to_write);

        /* Check if the packet is full */
        if (tcp_stream->packet_space_available == 0)
        {
            wiced_result_t result;
            wiced_packet_set_data_end(tcp_stream->packet, tcp_stream->packet_data);
            result = wiced_tcp_send_packet( tcp_stream->socket, tcp_stream->packet );

            tcp_stream->packet_data            = NULL;
            tcp_stream->packet_space_available = 0;

            if ( result != WICED_SUCCESS )
            {
                wiced_packet_delete(tcp_stream->packet);
                tcp_stream->packet = NULL;
                return result;
            }

            tcp_stream->packet = NULL;
        }
    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_tcp_stream_flush( wiced_tcp_stream_t* tcp_stream )
{
    WICED_LINK_CHECK( tcp_stream->socket->socket.nx_tcp_socket_ip_ptr );

    /* Check if there is a packet to send */
    if (tcp_stream->packet != NULL)
    {
        wiced_result_t result;
        wiced_packet_set_data_end(tcp_stream->packet, tcp_stream->packet_data);
        result = wiced_tcp_send_packet(tcp_stream->socket, tcp_stream->packet);

        tcp_stream->packet_data            = NULL;
        tcp_stream->packet_space_available = 0;

        if ( result != WICED_SUCCESS )
        {
            wiced_packet_delete(tcp_stream->packet);
            tcp_stream->packet                 = NULL;
            return result;
        }

        tcp_stream->packet                 = NULL;
    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_tcp_enable_keepalive(wiced_tcp_socket_t* socket, uint16_t interval, uint16_t probes, uint16_t _time )
{
    UNUSED_PARAMETER(socket);
    UNUSED_PARAMETER(interval);
    UNUSED_PARAMETER(probes);
    UNUSED_PARAMETER(_time);

    /* Not supported in current version of SDK . TO DO */
    return WICED_SUCCESS;
}


wiced_result_t wiced_packet_create_udp( wiced_udp_socket_t* socket, uint16_t content_length, wiced_packet_t** packet, uint8_t** data, uint16_t* available_space )
{
    UINT result;
    UNUSED_PARAMETER(content_length);
    UNUSED_PARAMETER(socket);

    result = nx_packet_allocate( &wiced_packet_pools[0], packet, NX_UDP_PACKET, NX_TIMEOUT(WICED_ALLOCATE_PACKET_TIMEOUT) );
    if ( result == NX_NO_PACKET )
    {
        *packet = NULL;
        *data   = NULL;
        *available_space = 0;
        return WICED_TIMEOUT;
    }
    else if ( result != NX_SUCCESS )
    {
        *packet = NULL;
        *data   = NULL;
        *available_space = 0;
        wiced_assert( "Packet allocation error", 0 != 0 );
        return WICED_ERROR;
    }

    *data = ( *packet )->nx_packet_prepend_ptr;
    *available_space = (uint16_t)((*packet)->nx_packet_data_end - (*packet)->nx_packet_prepend_ptr);
    return WICED_SUCCESS;
}

wiced_result_t wiced_packet_delete(wiced_packet_t* packet)
{
    nx_packet_release(packet);
    return WICED_SUCCESS;
}

wiced_result_t wiced_packet_set_data_end(wiced_packet_t* packet, uint8_t* data_end)
{
    wiced_assert("Bad packet end\r\n", (data_end >= packet->nx_packet_prepend_ptr) && (data_end <= packet->nx_packet_data_end));
    packet->nx_packet_append_ptr = data_end;
    packet->nx_packet_length     = (ULONG)(packet->nx_packet_append_ptr - packet->nx_packet_prepend_ptr);
    return WICED_SUCCESS;
}

wiced_result_t wiced_packet_set_data_start( wiced_packet_t* packet, uint8_t* data_start )
{
    wiced_assert("Bad packet end\r\n", (data_start >= packet->nx_packet_data_start) && (data_start <= packet->nx_packet_data_end));
    packet->nx_packet_prepend_ptr = data_start;
    packet->nx_packet_length      = (ULONG)(packet->nx_packet_append_ptr - packet->nx_packet_prepend_ptr);
    return WICED_SUCCESS;
}

wiced_result_t wiced_udp_packet_get_info(wiced_packet_t* packet, wiced_ip_address_t* address, uint16_t* port)
{
    UINT p;
    if (nx_udp_source_extract( packet, (ULONG*)&address->ip.v4, &p ) == NX_SUCCESS)
    {
        address->version = WICED_IPV4;
        *port = (uint16_t)p;
        return WICED_SUCCESS;
    }
    else
    {
        return WICED_ERROR;
    }
}

wiced_result_t wiced_packet_get_data( wiced_packet_t* packet, uint16_t offset, uint8_t** data, uint16_t* data_length, uint16_t* available_data_length )
{
    NX_PACKET* iter = packet;

    while ( iter != NULL )
    {
        if ( iter->nx_packet_length == 0 )
        {
            *data                  = NULL;
            *data_length           = 0;
            *available_data_length = 0;
            return WICED_SUCCESS;
        }
        else if ( offset < iter->nx_packet_length )
        {
            *data = iter->nx_packet_prepend_ptr + offset;
            *data_length = (uint16_t) ( iter->nx_packet_append_ptr - *data );
            *available_data_length = (uint16_t) ( iter->nx_packet_length );
            return WICED_SUCCESS;
        }
        else
        {
            offset = (uint16_t)(offset - iter->nx_packet_length);
        }
        iter = iter->nx_packet_next;
    }

    return WICED_ERROR;
}

wiced_result_t wiced_ip_get_ipv4_address( wiced_interface_t interface, wiced_ip_address_t* ipv4_address )
{
    ULONG temp;
    nx_ip_address_get(&IP_HANDLE(interface), (ULONG*)&ipv4_address->ip.v4, &temp);
    return WICED_SUCCESS;
}

wiced_result_t wiced_ip_get_ipv6_address( wiced_interface_t interface, wiced_ip_address_t* ipv6_address, wiced_ipv6_address_type_t address_type )
{
    UNUSED_PARAMETER( interface );
    UNUSED_PARAMETER( ipv6_address );
    UNUSED_PARAMETER( address_type );
//    wiced_assert("IPv6 unsupported in NetX", 0!=0);
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_udp_create_socket( wiced_udp_socket_t* socket, uint16_t port, wiced_interface_t interface )
{
    UINT result;

    WICED_LINK_CHECK( &IP_HANDLE(interface) );

    result = nx_udp_socket_create(&IP_HANDLE(interface), socket, (char*)"WICEDsock", 0, NX_DONT_FRAGMENT, 255, 5);
    if ( result != NX_SUCCESS )
    {
        wiced_assert( "Socket creation error", 0 != 0 );
        return WICED_ERROR;
    }

    result = nx_udp_socket_bind( socket, ( port == WICED_ANY_PORT ) ? NX_ANY_PORT : (UINT) port, NX_TIMEOUT(WICED_UDP_BIND_TIMEOUT) );
    if ( result != NX_SUCCESS )
    {
        wiced_assert( "Socket bind error", 0 != 0 );
        nx_udp_socket_delete( socket );
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_udp_send( wiced_udp_socket_t* socket, const wiced_ip_address_t* address, uint16_t port, wiced_packet_t* packet )
{
    UINT result;

    WICED_LINK_CHECK( socket->nx_udp_socket_ip_ptr );

    result = nx_udp_socket_send(socket, packet, address->ip.v4, port);
    if ( result != NX_SUCCESS )
    {
        wiced_assert( "UDP send error", 0 != 0 );
        return WICED_ERROR;
    }
    else
    {
        return WICED_SUCCESS;
    }
}

wiced_result_t wiced_udp_reply( wiced_udp_socket_t* socket, wiced_packet_t* in_packet, wiced_packet_t* out_packet )
{
    wiced_ip_address_t source_ip;
    UINT               source_port;

    WICED_LINK_CHECK( socket->nx_udp_socket_ip_ptr );

    nx_udp_source_extract( (NX_PACKET*)in_packet, (ULONG*)&source_ip.ip.v4, &source_port );
    return wiced_udp_send(socket, &source_ip, (uint16_t)source_port, out_packet);
}

wiced_result_t wiced_udp_receive( wiced_udp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout )
{
    UINT result;

    WICED_LINK_CHECK( socket->nx_udp_socket_ip_ptr );

    result = nx_udp_socket_receive( socket, packet, NX_TIMEOUT(timeout) );
    if ( ( result == NX_NO_PACKET ) ||
         ( result == NX_WAIT_ABORTED ) )
    {
        return WICED_TIMEOUT;
    }
    else if ( result != NX_SUCCESS )
    {
        wiced_assert("UDP receive error", 0 != 0 );
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_udp_delete_socket( wiced_udp_socket_t* socket )
{
    /* Check if socket is still bound */
    if (socket->nx_udp_socket_bound_next)
    {
        nx_udp_socket_unbind(socket);
    }

    if ( nx_udp_socket_delete( socket ) != NX_SUCCESS )
    {
        return WICED_ERROR;
    }
    else
    {
        return WICED_SUCCESS;
    }
}

wiced_result_t wiced_multicast_join( wiced_interface_t interface, const wiced_ip_address_t* address )
{
    WICED_LINK_CHECK( &IP_HANDLE(interface) );

    if (nx_igmp_multicast_join( &wiced_ip_handle[interface], address->ip.v4 ) == NX_SUCCESS)
    {
        return WICED_SUCCESS;
    }
    else
    {
        return WICED_ERROR;
    }
}

wiced_result_t wiced_multicast_leave( wiced_interface_t interface, const wiced_ip_address_t* address )
{
    WICED_LINK_CHECK( &IP_HANDLE(interface) );

    nx_igmp_multicast_leave( &wiced_ip_handle[interface], address->ip.v4 );
    return WICED_SUCCESS;
}

/*
 ******************************************************************************
 * Convert an ipv4 string to a uint32_t.
 *
 * @param     arg  The string containing the value.
 *
 * @return    The value represented by the string.
 */
static uint32_t str_to_ipv4( const char* arg )
{
    uint32_t addr = 0;
    uint8_t num = 0;

    arg--;

    do
    {
        addr = addr << 8;
        addr += (uint32_t) atoi( ++arg );
        while( ( *arg != '\x00' ) && ( *arg != '.' ) )
        {
            arg++;
        }
        num++;
    } while ( ( num < 4 ) && ( *arg != '\x00' ) );
    if ( num < 4 )
    {
        return 0;
    }
    return addr;
}

wiced_result_t wiced_hostname_lookup( const char* hostname, wiced_ip_address_t* address, uint32_t timeout_ms )
{
    WICED_LINK_CHECK( &IP_HANDLE( WICED_STA_INTERFACE ) );

    /* Check if address is a string representation of a IPv4 address i.e. xxx.xxx.xxx.xxx */
    address->ip.v4 = str_to_ipv4( hostname );
    if ( address->ip.v4 != 0 )
    {
        address->version = WICED_IPV4;
        /* yes this is a string representation of a IPv4 address */
        return WICED_SUCCESS;
    }

    return dns_client_hostname_lookup( hostname, address, timeout_ms );
}

wiced_result_t wiced_ping(wiced_interface_t interface, const wiced_ip_address_t* address, uint32_t timeout_ms, uint32_t* elapsed_ms)
{

    wiced_time_t send_time;
    wiced_time_t reply_time;
    UINT status;
    NX_PACKET* response_ptr;

    WICED_LINK_CHECK( &IP_HANDLE(interface) );

    send_time = host_rtos_get_time( );
    status = nx_icmp_ping( &IP_HANDLE(interface), address->ip.v4, NULL, 0, &response_ptr, NX_TIMEOUT(timeout_ms) );
    reply_time = host_rtos_get_time( );
    if ( status == NX_SUCCESS )
    {
        *elapsed_ms = (reply_time - send_time)*1000/SYSTICK_FREQUENCY;
        nx_packet_release( response_ptr );

        return WICED_SUCCESS;
    }
    else if ( status == NX_NO_RESPONSE )
    {
        return WICED_TIMEOUT;
    }
    else
    {
        return WICED_ERROR;
    }
}


wiced_result_t wiced_ip_get_gateway_address( wiced_interface_t interface, wiced_ip_address_t* ipv4_address )
{
    SET_IPV4_ADDRESS( *ipv4_address, IP_HANDLE(interface).nx_ip_gateway_address );
    return WICED_SUCCESS;
}

wiced_result_t wiced_ip_get_netmask( wiced_interface_t interface, wiced_ip_address_t* ipv4_address )
{
    SET_IPV4_ADDRESS( *ipv4_address, IP_HANDLE(interface).nx_ip_network_mask);
    return WICED_SUCCESS;
}

wiced_result_t wiced_tcp_server_peer( wiced_tcp_socket_t* socket, wiced_ip_address_t* address, uint16_t* port )
{
    ULONG peer_port;
    ULONG peer_address;

    if ( nx_tcp_socket_peer_info_get( &socket->socket, &peer_address, &peer_port ) == NX_SUCCESS )
    {
        *port = (uint16_t)peer_port;
        SET_IPV4_ADDRESS( *address, peer_address );
        return WICED_SUCCESS;
    }

    return WICED_ERROR;
}
