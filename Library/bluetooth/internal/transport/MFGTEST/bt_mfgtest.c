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
#include "wiced_rtos.h"
#include "wiced_utilities.h"
#include "wiced_platform.h"
#include "wiced_bt_platform.h"
#include "bt_hci_interface.h"
#include "bt_bus.h"
#include "bt_mfgtest.h"
#include "bt_transport_driver.h"
#include "bt_transport_thread.h"
#include "bt_firmware.h"
#include "bt_firmware_image.h"

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

extern wiced_result_t bt_mfgtest_transport_driver_bus_read_handler        ( bt_packet_t** packet );
static wiced_result_t bt_mfgtest_transport_driver_event_handler           ( bt_transport_driver_event_t event );
static wiced_result_t bt_mfgtest_transport_thread_received_packet_handler ( bt_packet_t* packet );

/******************************************************
 *               Variables Definitions
 ******************************************************/

static wiced_uart_t        pc_uart;
static wiced_ring_buffer_t pc_uart_ring_buffer;
static uint8_t             pc_uart_ring_buffer_data[BT_BUS_RX_FIFO_SIZE];

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t bt_mfgtest_start( const wiced_uart_config_t* config )
{
    /* Download firmware */
    if ( bt_firmware_download( bt_hci_firmware_image, bt_hci_firmware_size, bt_hci_firmware_version ) != WICED_SUCCESS )
    {
        WPRINT_LIB_ERROR( ( "Error downloading HCI firmware\r\n" ) );
        return WICED_ERROR;
    }

    if ( bt_transport_driver_init( bt_mfgtest_transport_driver_event_handler, bt_mfgtest_transport_driver_bus_read_handler ) != WICED_SUCCESS )
    {
        WPRINT_LIB_ERROR( ( "Error initialising BT transport driver\r\n" ) );
        return WICED_ERROR;
    }

    /* Initialise BT transport thread */
    if ( bt_transport_thread_init( bt_mfgtest_transport_thread_received_packet_handler ) != WICED_SUCCESS )
    {
        WPRINT_LIB_ERROR( ( "Error initialising BT transport thread\r\n" ) );
        return WICED_ERROR;
    }

    ring_buffer_init( &pc_uart_ring_buffer, pc_uart_ring_buffer_data, BT_BUS_RX_FIFO_SIZE );

    pc_uart = PC_UART;

    if ( wiced_uart_init( pc_uart, config, &pc_uart_ring_buffer ) != WICED_SUCCESS )
    {
        WPRINT_LIB_ERROR( ( "Error initialising UART connection to PC\r\n" ) );
        return WICED_ERROR;
    }

    /* Grab message from PC and pass it over to the controller */
    while ( 1 )
    {
        hci_command_header_t header;
        bt_packet_t*         packet;

        /* Read HCI header */
        wiced_uart_receive_bytes( pc_uart, (void*)&header, sizeof( header ), WICED_NEVER_TIMEOUT );

        /* Allocate dynamic packet */
        bt_packet_pool_dynamic_allocate_packet( &packet, sizeof( header ), header.content_length );

        /* Copy header to packet */
        memcpy( packet->packet_start, &header, sizeof( header ) );

        /* Read the remaining packet */
        wiced_uart_receive_bytes( pc_uart, packet->data_start, header.content_length, WICED_NEVER_TIMEOUT );

        /* Set the end of the packet */
        packet->data_end = packet->data_start + header.content_length;

        /* Send packet to the controller */
        bt_transport_driver_send_packet( packet );
    }

    return WICED_SUCCESS;
}


static wiced_result_t bt_mfgtest_transport_driver_event_handler( bt_transport_driver_event_t event )
{
    if ( event == TRANSPORT_DRIVER_INCOMING_PACKET_READY )
    {
        return bt_transport_thread_notify_packet_received();
    }

    return WICED_ERROR;
}

static wiced_result_t bt_mfgtest_transport_thread_received_packet_handler( bt_packet_t* packet )
{
    /* Pass HCI event packet to STDIO UART */
    wiced_uart_transmit_bytes( pc_uart, (const void*)packet->packet_start, packet->data_end - packet->packet_start );

    /* Release packet */
    return bt_packet_pool_free_packet( packet );
}
