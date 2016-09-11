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
 * NuttX TCP/IP library
 */

#include "wwd_constants.h"
#include "wwd_assert.h"

#include "wiced_constants.h"
#include "wiced_result.h"
#include "wiced_tcpip.h"

#include "internal/wiced_internal_api.h"

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

wiced_result_t wiced_ip_get_ipv4_address( wiced_interface_t interface, wiced_ip_address_t* ipv4_address )
{
    UNUSED_PARAMETER( interface );
    UNUSED_PARAMETER( ipv4_address );
    wiced_assert( "not yet implemented", 0 );
    return WICED_TCPIP_SUCCESS;
}

wiced_result_t wiced_ip_get_gateway_address( wiced_interface_t interface, wiced_ip_address_t* ipv4_address )
{
    UNUSED_PARAMETER( interface );
    UNUSED_PARAMETER( ipv4_address );
    wiced_assert( "not yet implemented", 0 );
    return WICED_TCPIP_SUCCESS;
}

wiced_result_t wiced_ip_get_netmask( wiced_interface_t interface, wiced_ip_address_t* ipv4_address )
{
    UNUSED_PARAMETER( interface );
    UNUSED_PARAMETER( ipv4_address );
    wiced_assert( "not yet implemented", 0 );
    return WICED_TCPIP_SUCCESS;
}

wiced_result_t wiced_packet_delete( wiced_packet_t* packet )
{
    UNUSED_PARAMETER( packet );
    wiced_assert( "not yet implemented", 0 );
    return WICED_TCPIP_SUCCESS;
}

wiced_result_t wiced_packet_set_data_end( wiced_packet_t* packet, uint8_t* data_end )
{
    UNUSED_PARAMETER( packet );
    UNUSED_PARAMETER( data_end );
    wiced_assert( "not yet implemented", 0 );
    return WICED_TCPIP_SUCCESS;
}

wiced_result_t wiced_packet_set_data_start( wiced_packet_t* packet, uint8_t* data_start )
{
    UNUSED_PARAMETER( packet );
    UNUSED_PARAMETER( data_start );
    wiced_assert( "not yet implemented", 0 );
    return WICED_TCPIP_SUCCESS;
}

wiced_result_t wiced_packet_get_data( wiced_packet_t* packet, uint16_t offset, uint8_t** data, uint16_t* fragment_available_data_length, uint16_t *total_available_data_length )
{
    UNUSED_PARAMETER( packet );
    UNUSED_PARAMETER( offset );
    UNUSED_PARAMETER( data );
    UNUSED_PARAMETER( fragment_available_data_length );
    UNUSED_PARAMETER( total_available_data_length );
    wiced_assert( "not yet implemented", 0 );
    return WICED_TCPIP_SUCCESS;
}

wiced_result_t wiced_packet_create_tcp( wiced_tcp_socket_t* socket, uint16_t content_length, wiced_packet_t** packet, uint8_t** data, uint16_t* available_space )
{
    UNUSED_PARAMETER( socket );
    UNUSED_PARAMETER( content_length );
    UNUSED_PARAMETER( packet );
    UNUSED_PARAMETER( data );
    UNUSED_PARAMETER( available_space );
    wiced_assert( "not yet implemented", 0 );
    return WICED_TCPIP_SUCCESS;
}

wiced_result_t network_tcp_send_packet( wiced_tcp_socket_t* socket, wiced_packet_t* packet )
{
    UNUSED_PARAMETER( socket );
    UNUSED_PARAMETER( packet );
    wiced_assert( "not yet implemented", 0 );
    return WICED_TCPIP_SUCCESS;
}

wiced_result_t network_tcp_receive( wiced_tcp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout )
{
    UNUSED_PARAMETER( socket );
    UNUSED_PARAMETER( packet );
    UNUSED_PARAMETER( timeout );
    wiced_assert( "not yet implemented", 0 );
    return WICED_TCPIP_SUCCESS;
}
