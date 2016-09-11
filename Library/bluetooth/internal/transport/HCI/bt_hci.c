/**
 * Copyright 2013, Broadcom Corporation
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
#include "bt_bus.h"
#include "bt_hci.h"
#include "bt_hci_interface.h"
#include "bt_transport_driver.h"
#include "bt_transport_thread.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define MAX_ALLOCATE_PACKET_ATTEMPT 50

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

extern wiced_result_t bt_hci_transport_driver_bus_read_handler        ( bt_packet_t** packet );
static wiced_result_t bt_hci_transport_driver_event_handler           ( bt_transport_driver_event_t event );
static wiced_result_t bt_hci_transport_thread_received_packet_handler ( bt_packet_t* packet );

/******************************************************
 *               Variables Definitions
 ******************************************************/

static wiced_bool_t                     hci_initialised        = WICED_FALSE;
static bt_hci_incoming_packet_handler_t hci_event_handler      = NULL;
static bt_hci_incoming_packet_handler_t hci_acl_packet_handler = NULL;
static bt_hci_incoming_packet_handler_t hci_sco_packet_handler = NULL;
static bt_packet_pool_t                 hci_acl_data_packet_pool;
static bt_packet_pool_t                 hci_command_packet_pool;
static bt_packet_pool_t                 hci_event_packet_pool;
static wiced_mutex_t                    hci_mutex;

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t bt_hci_init( void )
{
    if ( hci_initialised == WICED_FALSE )
    {
        wiced_result_t result;

        /* Initialise packet pools */
        result = bt_packet_pool_init( &hci_command_packet_pool, BT_HCI_COMMAND_PACKET_COUNT, BT_HCI_COMMAND_HEADER_SIZE, BT_HCI_COMMAND_DATA_SIZE );

        if ( result != WICED_SUCCESS )
        {
            WPRINT_LIB_ERROR( ( "Error initialising HCI command packet pool\r\n" ) );
            return result;
        }

        result = bt_packet_pool_init( &hci_event_packet_pool, BT_HCI_EVENT_PACKET_COUNT, BT_HCI_EVENT_HEADER_SIZE, BT_HCI_EVENT_DATA_SIZE );

        if ( result != WICED_SUCCESS )
        {
            WPRINT_LIB_ERROR( ( "Error initialising HCI event packet pool\r\n" ) );
            return result;
        }

        result = bt_packet_pool_init( &hci_acl_data_packet_pool, BT_HCI_ACL_PACKET_COUNT, BT_HCI_ACL_HEADER_SIZE, BT_HCI_ACL_DATA_SIZE );

        if ( result != WICED_SUCCESS )
        {
            WPRINT_LIB_ERROR( ( "Error initialising HCI ACL data packet pool\r\n" ) );
            return result;
        }

        /* Switch to MPAF mode. Initialise MPAF transport driver */
        if ( bt_transport_driver_init( bt_hci_transport_driver_event_handler, bt_hci_transport_driver_bus_read_handler ) != WICED_SUCCESS )
        {
            WPRINT_LIB_ERROR( ( "Error initialising BT transport driver\r\n" ) );
            return WICED_ERROR;
        }

        /* Initialise globals */
        hci_event_handler      = NULL;
        hci_acl_packet_handler = NULL;
        hci_sco_packet_handler = NULL;

        /* Create MPAF command mutex */
        if ( wiced_rtos_init_mutex( &hci_mutex ) != WICED_SUCCESS )
        {
            WPRINT_LIB_ERROR( ( "Error creating HCI mutex\r\n" ) );
            return WICED_ERROR;
        }

        /* Initialise BT transport thread */
        if ( bt_transport_thread_init( bt_hci_transport_thread_received_packet_handler ) != WICED_SUCCESS )
        {
            WPRINT_LIB_ERROR( ( "Error initialising BT transport thread\r\n" ) );
            return WICED_ERROR;
        }

        hci_initialised = WICED_TRUE;
    }

    return WICED_SUCCESS;
}

wiced_result_t bt_hci_deinit( void )
{
    if ( hci_initialised == WICED_TRUE )
    {
        wiced_result_t result;

        /* Initialise BT transport thread */
        if ( bt_transport_thread_deinit( ) != WICED_SUCCESS )
        {
            WPRINT_LIB_ERROR( ( "Error deinitialising BT transport thread\r\n" ) );
            return WICED_ERROR;
        }

        /* Switch to MPAF mode. Initialise MPAF transport driver */
        if ( bt_transport_driver_deinit( ) != WICED_SUCCESS )
        {
            WPRINT_LIB_ERROR( ( "Error deinitialising BT transport driver\r\n" ) );
            return WICED_ERROR;
        }

        /* Create MPAF command mutex */
        if ( wiced_rtos_deinit_mutex( &hci_mutex ) != WICED_SUCCESS )
        {
            WPRINT_LIB_ERROR( ( "Error creating HCI mutex\r\n" ) );
            return WICED_ERROR;
        }

        /* Deinitialise packet pools */
        result = bt_packet_pool_deinit( &hci_command_packet_pool );

        if ( result != WICED_SUCCESS )
        {
            WPRINT_LIB_ERROR( ( "Error deinitialising HCI command packet pool\r\n" ) );
            return result;
        }

        result = bt_packet_pool_deinit( &hci_event_packet_pool );

        if ( result != WICED_SUCCESS )
        {
            WPRINT_LIB_ERROR( ( "Error deinitialising HCI event packet pool\r\n" ) );
            return result;
        }

        result = bt_packet_pool_deinit( &hci_acl_data_packet_pool );

        if ( result != WICED_SUCCESS )
        {
            WPRINT_LIB_ERROR( ( "Error deinitialising HCI ACL data packet pool\r\n" ) );
            return result;
        }

        /* Initialise globals */
        hci_event_handler      = NULL;
        hci_acl_packet_handler = NULL;
        hci_sco_packet_handler = NULL;
        hci_initialised        = WICED_FALSE;
    }

    return WICED_SUCCESS;
}

wiced_result_t bt_hci_register_event_handler( bt_hci_incoming_packet_handler_t event_handler )
{
    hci_event_handler = event_handler;
    return WICED_SUCCESS;
}

wiced_result_t bt_hci_register_acl_data_handler( bt_hci_incoming_packet_handler_t acl_data_handler )
{
    hci_acl_packet_handler = acl_data_handler;
    return WICED_SUCCESS;
}

wiced_result_t bt_hci_register_sco_data_handler( bt_hci_incoming_packet_handler_t sco_data_handler )
{
    hci_sco_packet_handler = sco_data_handler;
    return WICED_SUCCESS;
}

wiced_result_t bt_hci_execute_application_callback( event_handler_t application_callback, void* arg )
{
    return bt_transport_thread_execute_callback( application_callback, arg );
}

wiced_result_t bt_hci_create_packet( hci_packet_type_t type, bt_packet_t** packet, uint32_t data_size )
{
    wiced_result_t result = WICED_ERROR;
    uint32_t attempt = 0;

    UNUSED_PARAMETER( data_size );

    while( result != WICED_SUCCESS )
    {
        switch ( type )
        {
            case HCI_COMMAND_PACKET:
            {
                result = bt_packet_pool_allocate_packet( &hci_command_packet_pool, packet );
                break;
            }
            case HCI_EVENT_PACKET:
            {
                result = bt_packet_pool_allocate_packet( &hci_event_packet_pool, packet );
                break;
            }
            case HCI_ACL_DATA_PACKET:
            {
                result = bt_packet_pool_allocate_packet( &hci_acl_data_packet_pool, packet );
                break;
            }
            default:
            {
                return WICED_ERROR;
            }
        }

        if ( result != WICED_SUCCESS )
        {
            /* Sleep for 10 milliseconds to let other threads run and release packets */
            wiced_rtos_delay_milliseconds( 10 );
            attempt++;
            wiced_assert( "Maximum attempt reached! Check for packet leak!", attempt < MAX_ALLOCATE_PACKET_ATTEMPT );
        }
    }

    if ( result == WICED_SUCCESS )
    {
        *( ( *packet )->packet_start ) = (uint8_t)type;
    }

    return result;
}

wiced_result_t bt_hci_create_dynamic_packet( hci_packet_type_t type, bt_packet_t** packet, uint32_t header_size, uint32_t data_size )
{
    wiced_result_t result = bt_packet_pool_dynamic_allocate_packet( packet, header_size, data_size );

    if ( result == WICED_SUCCESS )
    {
        *( ( *packet )->packet_start ) = (uint8_t)type;
    }

    return result;
}

wiced_result_t bt_hci_delete_packet( bt_packet_t* packet )
{
    return bt_packet_pool_free_packet( packet );
}

wiced_result_t bt_hci_send_packet( bt_packet_t* packet )
{
    return bt_transport_thread_send_packet( packet );
}

static wiced_result_t bt_hci_transport_driver_event_handler( bt_transport_driver_event_t event )
{
    if ( event == TRANSPORT_DRIVER_INCOMING_PACKET_READY )
    {
        return bt_transport_thread_notify_packet_received();
    }

    return WICED_ERROR;
}

static wiced_result_t bt_hci_transport_thread_received_packet_handler( bt_packet_t* packet )
{
    hci_packet_type_t packet_type = (hci_packet_type_t)( *( packet->packet_start ) );

     /* Invoke the appropriate callback */
     switch ( packet_type )
     {
         case HCI_EVENT_PACKET:
             if ( hci_event_handler != NULL )
             {
                 hci_event_handler( packet );
             }
             break;

         case HCI_ACL_DATA_PACKET:
             if ( hci_acl_packet_handler != NULL )
             {
                 hci_acl_packet_handler( packet );
             }
             break;

         case HCI_SCO_DATA_PACKET:
             if ( hci_sco_packet_handler != NULL )
             {
                 hci_sco_packet_handler( packet );
             }
             break;

         default:
             bt_hci_delete_packet( packet );
             return WICED_ERROR;
     }

     return WICED_SUCCESS;
}
