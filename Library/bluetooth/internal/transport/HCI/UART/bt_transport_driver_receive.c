/**
 * Copyright 2014, Broadcom Corporation
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

#include "wiced.h"
#include "bt_bus.h"
#include "bt_hci.h"
#include "bt_hci_interface.h"
#include "bt_packet_internal.h"

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
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t bt_hci_transport_driver_bus_read_handler( bt_packet_t** packet )
{
    hci_packet_type_t packet_type;

    /* Get the packet type */
    if ( bt_bus_receive( (uint8_t*)&packet_type, 1, WICED_NEVER_TIMEOUT ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    /* Read the header and determine the   */
    switch ( packet_type )
    {
        case HCI_ACL_DATA_PACKET:
        {
            hci_acl_packet_header_t header;

            /* Get the packet type */
            if ( bt_bus_receive( (uint8_t*)&header.hci_handle, sizeof( header ) - sizeof( packet_type ), WICED_NEVER_TIMEOUT ) != WICED_SUCCESS )
            {
                return WICED_ERROR;
            }

            header.packet_type = packet_type;

            /* Allocate buffer for the incoming packet */
            if ( bt_hci_create_packet( packet_type, packet, header.content_length ) != WICED_SUCCESS )
            {
                return WICED_ERROR;
            }

            /* Copy header to the packet */
            memcpy( ( *packet )->packet_start, &header, sizeof( header ) );
            ( *packet )->data_end  = ( *packet )->data_start + header.content_length;
            break;
        }

        case HCI_SCO_DATA_PACKET:
        {
            hci_sco_packet_header_t header;

            /* Get the packet type */
            if ( bt_bus_receive( (uint8_t*)&header.hci_handle, sizeof( header ) - sizeof( packet_type ), WICED_NEVER_TIMEOUT ) != WICED_SUCCESS )
            {
                return WICED_ERROR;
            }

            header.packet_type = packet_type;

            /* Allocate buffer for the incoming packet */
            if ( bt_hci_create_packet( packet_type, packet, header.content_length ) != WICED_SUCCESS )
            {
                return WICED_ERROR;
            }

            /* Copy header to the packet */
            memcpy( ( *packet )->packet_start, &header, sizeof( header ) );
            ( *packet )->data_end  = ( *packet )->data_start + header.content_length;
            break;
        }

        case HCI_EVENT_PACKET:
        {
            hci_event_header_t header;

            /* Get the packet type */
            if ( bt_bus_receive( (uint8_t*)&header.event_code, sizeof( header ) - sizeof( packet_type ), WICED_NEVER_TIMEOUT ) != WICED_SUCCESS )
            {
                return WICED_ERROR;
            }

            header.packet_type = packet_type;

            /* Allocate buffer for the incoming packet */
            if ( bt_hci_create_packet( packet_type, packet, header.content_length ) != WICED_SUCCESS )
            {
                return WICED_ERROR;
            }

            /* Copy header to the packet */
            memcpy( ( *packet )->packet_start, &header, sizeof( header ) );
            ( *packet )->data_end  = ( *packet )->data_start + header.content_length;
            break;
        }

        case HCI_COMMAND_PACKET: /* Fall-through */
        default:
            return WICED_ERROR;
    }

    /* Receive the remainder of the packet */
    if ( bt_bus_receive( (uint8_t*)( ( *packet )->data_start ), (uint32_t)( ( *packet )->data_end - ( *packet )->data_start ), WICED_NEVER_TIMEOUT ) != WICED_SUCCESS )
    {
        bt_packet_pool_free_packet( *packet );
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}
