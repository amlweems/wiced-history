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
 *  I/O functions
 *  Provides functions for sending and receiving to the network for use by
 *  framing layer
 */

#include "mqtt_internal.h"
#include "mqtt_frame.h"
#include "mqtt_network.h"
#include "mqtt_connection.h"
#include "mqtt_manager.h"
#include "wiced_tls.h"

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
typedef struct wiced_mqtt_network_message_s
{
    wiced_packet_t                   *packet;
    void                             *data;
} wiced_mqtt_network_message_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/
static wiced_result_t mqtt_network_recv_thread( wiced_tcp_socket_t *socket, void *args );
static wiced_result_t mqtt_disconnect_callback( wiced_tcp_socket_t *socket, void *args );
static void mqtt_thread_main( uint32_t arg );
static void wiced_process_mqtt_request( void *arg );

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t mqtt_core_init( mqtt_connection_t *conn )
{
    wiced_result_t result = WICED_SUCCESS;
    mqtt_socket_t *mqtt_socket = &conn->socket;

    wiced_rtos_init_semaphore( &conn->semaphore );
    if ( ( result = wiced_rtos_init_queue( &mqtt_socket->queue, NULL, sizeof(mqtt_event_message_t), WICED_MQTT_QUEUE_SIZE ) ) != WICED_SUCCESS )
    {
        WPRINT_LIB_ERROR(("[MQTT LIB] Initializing queue failed\n"));
        goto ERROR_SEMAPHORE_INIT;
    }

    if ( ( result = wiced_rtos_create_thread( &mqtt_socket->net_thread, 5, "Mqttmain", mqtt_thread_main, 6200, mqtt_socket ) ) != WICED_SUCCESS )
    {
        goto ERROR_QUEUE_THREAD;
    }
    conn->session_init = WICED_TRUE;
    return result;

ERROR_QUEUE_THREAD:
    wiced_rtos_deinit_queue( &mqtt_socket->queue );

ERROR_SEMAPHORE_INIT:
    wiced_rtos_deinit_semaphore( &conn->semaphore );

    return result;
}

wiced_result_t mqtt_core_deinit( mqtt_connection_t *conn )
{
    mqtt_socket_t *mqtt_socket = &conn->socket;
    if ( conn->net_init_ok == WICED_TRUE )
    {
        conn->net_init_ok = WICED_FALSE;
        mqtt_network_deinit( mqtt_socket );
    }
    if ( wiced_rtos_is_current_thread( &mqtt_socket->net_thread ) != WICED_SUCCESS )
    {
        wiced_rtos_thread_force_awake( &mqtt_socket->net_thread );
        wiced_rtos_delete_thread( &mqtt_socket->net_thread );
    }

    wiced_rtos_deinit_queue( &mqtt_socket->queue );
    wiced_rtos_deinit_semaphore( &conn->semaphore );
    return WICED_SUCCESS;
}

wiced_result_t mqtt_network_init( const wiced_ip_address_t *server_ip_address, uint16_t portnumber, wiced_interface_t interface, void *p_user, mqtt_socket_t *socket, const wiced_mqtt_security_t *security )
{
    wiced_result_t result = WICED_SUCCESS;
    mqtt_connection_t *conn = (mqtt_connection_t *) p_user;
    /* Create a TCP socket */
    if ( ( result = wiced_tcp_create_socket( &socket->socket, interface ) ) != WICED_SUCCESS )
    {
        goto ERROR_CREATE_SOCKET;
    }
    if ( security != NULL )
    {
        if ( ( result = wiced_tls_init_root_ca_certificates( security->ca_cert ) ) != WICED_SUCCESS )
        {
            goto ERROR_CA_CERT_INIT;
        }
        if ( ( security->cert != NULL ) && ( security->key != NULL ) )
        {
            result = wiced_tls_init_identity( &socket->tls_identity, (char*) security->key, (const uint8_t*) security->cert, strlen( security->cert ) );
            if ( result != WICED_SUCCESS )
            {
                goto ERROR_TLS_INIT;
            }
            wiced_tls_init_context( &socket->tls_context, &socket->tls_identity, NULL );
        }
        else
        {
            wiced_tls_init_context( &socket->tls_context, NULL, NULL );

        }
        wiced_tcp_enable_tls( &socket->socket, &socket->tls_context );
    }

    socket->p_user = p_user;
    socket->server_ip_address = *server_ip_address;
    socket->portnumber = portnumber;
    conn->net_init_ok = WICED_TRUE;

    result = mqtt_network_connect( socket );
    if ( result != WICED_SUCCESS )
    {
        goto ERROR_QUEUE_INIT;;
    }

    /*for receive*/

    result = wiced_tcp_register_callbacks( &socket->socket, NULL, mqtt_network_recv_thread, mqtt_disconnect_callback, socket );
    if ( result != WICED_SUCCESS )
    {
        goto ERROR_NETWORK_INIT;
    }

    return result;

ERROR_NETWORK_INIT:
    mqtt_network_disconnect( socket );

ERROR_QUEUE_INIT:
    if ( security == NULL )
    {
        goto ERROR_CA_CERT_INIT;
    }

    wiced_tls_reset_context( &socket->tls_context );
    wiced_tls_deinit_identity(&socket->tls_identity);

ERROR_TLS_INIT:
    wiced_tls_deinit_root_ca_certificates( );

ERROR_CA_CERT_INIT:
    wiced_tcp_delete_socket( &socket->socket );

ERROR_CREATE_SOCKET:

    return result;
}

wiced_result_t mqtt_network_deinit( mqtt_socket_t *socket )
{
    wiced_tcp_unregister_callbacks( &socket->socket );
    mqtt_network_disconnect( socket );
    if ( socket->socket.tls_context != NULL )
    {
        wiced_tls_reset_context( socket->socket.tls_context );
    }

    if ( socket->socket.tls_context != NULL )
    {
        wiced_tls_deinit_root_ca_certificates( );
    }
    wiced_tcp_delete_socket( &socket->socket );
    return WICED_SUCCESS;
}

static void mqtt_thread_main( uint32_t arg )
{
    mqtt_socket_t *socket = (mqtt_socket_t*) arg;
    mqtt_event_message_t current_event;
    wiced_result_t result = WICED_SUCCESS;
    while ( 1 )
    {
        if ( ( result = wiced_rtos_pop_from_queue( &socket->queue, &current_event, WICED_NEVER_TIMEOUT ) ) != WICED_SUCCESS )
        {
            current_event.event_type = MQTT_ERROR_EVENT;
        }

        switch ( current_event.event_type )
        {
            case MQTT_RECEIVE_EVENT:
                wiced_process_mqtt_request( socket );
                break;

            case MQTT_SEND_EVENT:
                mqtt_manager( current_event.send_context.event_t, &current_event.send_context.args, current_event.send_context.conn );
                break;

            case MQTT_ERROR_EVENT:
                break;

            case MQTT_DISCONNECT_EVENT:
                mqtt_frame_recv( NULL, socket->p_user, NULL );
                break;

            default:
                break;
        }
    }
    WICED_END_OF_CURRENT_THREAD( );
}

static void wiced_process_mqtt_request( void *arg )
{
    wiced_result_t result;
    mqtt_socket_t* socket = (mqtt_socket_t*) arg;

    uint8_t *data;
    wiced_packet_t *packet;

    /*Receive the query from the TCP client*/
    result = wiced_tcp_receive( &socket->socket, &packet, WICED_NO_WAIT );
    if ( result == WICED_SUCCESS )
    {
        uint16_t rx_data_length;
        uint16_t available_data_length;
        uint16_t current_size = 0;
        wiced_mqtt_buffer_t buffer;

        /*Process the client request*/
        wiced_packet_get_data( packet, 0, (uint8_t**) ( &data ), &rx_data_length, &available_data_length );
        while ( current_size < rx_data_length )
        {
            uint32_t size;
            buffer.packet = packet;
            buffer.data = data + current_size;
            if ( mqtt_frame_recv( &buffer, socket->p_user, &size ) != WICED_SUCCESS )
            {
                break;
            }
            current_size = (uint16_t) ( current_size + size );
        }
        /*Delete the packet, we're done with it*/
        wiced_packet_delete( buffer.packet );
    }
}

wiced_result_t mqtt_network_connect( mqtt_socket_t *socket )
{
    wiced_result_t result = WICED_SUCCESS;
    int connection_retries;

    /* Connect to the remote TCP server, try several times */
    connection_retries = 0;
    do
    {
        result = wiced_tcp_connect( &socket->socket, &socket->server_ip_address, socket->portnumber, WICED_MQTT_CONNECTION_TIMEOUT );
        connection_retries++ ;
    } while ( ( result != WICED_SUCCESS ) && ( connection_retries < WICED_MQTT_CONNECTION_NUMBER_OF_RETRIES ) );

    return result;
}

wiced_result_t mqtt_network_disconnect( mqtt_socket_t *socket )
{
    return wiced_tcp_disconnect( &socket->socket );
}

wiced_result_t mqtt_network_create_buffer( wiced_mqtt_buffer_t *buffer, uint16_t size, mqtt_socket_t *socket )
{
    uint16_t available_data_length;

    /* Create the TCP packet. Memory for the tx_data is automatically allocated */
    return wiced_packet_create_tcp( &socket->socket, size, &buffer->packet, &buffer->data, &available_data_length );
}

wiced_result_t mqtt_network_delete_buffer( wiced_mqtt_buffer_t *buffer )
{
    /* Create the TCP packet. Memory for the tx_data is automatically allocated */
    return wiced_packet_delete( buffer->packet );
}

wiced_result_t mqtt_network_send_buffer( const wiced_mqtt_buffer_t *buffer, mqtt_socket_t *socket )
{
    wiced_mqtt_network_message_t message;
    wiced_result_t result = WICED_SUCCESS;
    message.packet = buffer->packet;
    message.data = buffer->data;

    /*Set the end of the data portion*/
    wiced_packet_set_data_end( message.packet, message.data );
    result = wiced_tcp_send_packet( &socket->socket, message.packet );
    if ( result != WICED_SUCCESS )
    {
        /*Delete packet, since the send failed*/
        wiced_packet_delete( message.packet );
    }
    return result;
}

static wiced_result_t mqtt_disconnect_callback( wiced_tcp_socket_t *socket, void *args )
{
    mqtt_socket_t *mqtt_socket = (mqtt_socket_t *) args;
    mqtt_event_message_t current_event;
    UNUSED_PARAMETER(socket);
    current_event.event_type = MQTT_DISCONNECT_EVENT;
    return wiced_rtos_push_to_queue( &mqtt_socket->queue, &current_event, WICED_NO_WAIT );
}

static wiced_result_t mqtt_network_recv_thread( wiced_tcp_socket_t *socket, void *args )
{
    mqtt_event_message_t current_event;
    mqtt_socket_t *mqtt_socket = (mqtt_socket_t *) args;
    UNUSED_PARAMETER(socket);
    current_event.event_type = MQTT_RECEIVE_EVENT;
    return wiced_rtos_push_to_queue( &mqtt_socket->queue, &current_event, WICED_NO_WAIT );
}
