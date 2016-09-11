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
 *  Connection functions
 *
 *  Provides connection methods for use by applications
 */

#include "wiced.h"
#include "mqtt_internal.h"
#include "mqtt_connection.h"
#include "mqtt_frame.h"
#include "mqtt_manager.h"

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
wiced_result_t mqtt_connection_init( const wiced_ip_address_t *address, uint16_t port_number, wiced_interface_t interface, const wiced_mqtt_callback_t callbacks, mqtt_connection_t *conn, const wiced_mqtt_security_t *security )
{
    if ( port_number == 0 )
    {
        port_number = ( security == NULL ) ? WICED_MQTT_CONNECTION_DEFAULT_PORT : WICED_MQTT_CONNECTION_SECURE_PORT;
    }

    wiced_rtos_init_semaphore( &conn->semaphore );
    wiced_rtos_set_semaphore( &conn->semaphore );
    conn->callbacks = callbacks;
    return mqtt_network_init( address, port_number, interface, conn, &conn->socket, security );
}

wiced_result_t mqtt_connect( mqtt_connection_t *conn, mqtt_connect_arg_t *args, mqtt_session_t *session )
{
    mqtt_socket_t socket = conn->socket;
    mqtt_event_message_t current_event;

    if ( args->clean_session == 1 )
    {
        mqtt_session_init( session );
    }
    else
    {
        if ( conn->session_init == WICED_TRUE )
        {
            mqtt_session_init( session );
        }
        conn->session_init = WICED_FALSE;
    }
    conn->session = session;

    current_event.send_context.event_t = MQTT_EVENT_SEND_CONNECT;
    current_event.send_context.conn = conn;
    current_event.send_context.args.conn_args = *args;
    current_event.event_type = MQTT_SEND_EVENT;

    /*push into the main queue*/
    return wiced_rtos_push_to_queue( &socket.queue, &current_event, WICED_NO_WAIT );
}

wiced_result_t mqtt_disconnect( mqtt_connection_t *conn )
{
    mqtt_socket_t socket = conn->socket;
    mqtt_event_message_t current_event;

    current_event.send_context.event_t = MQTT_EVENT_SEND_DISCONNECT;
    current_event.send_context.conn = conn;
    current_event.event_type = MQTT_SEND_EVENT;

    return wiced_rtos_push_to_queue( &socket.queue, &current_event, WICED_NO_WAIT );
}

wiced_result_t mqtt_subscribe( mqtt_connection_t *conn, mqtt_subscribe_arg_t *args )
{
    mqtt_socket_t socket = conn->socket;
    mqtt_event_message_t current_event;

    current_event.send_context.event_t = MQTT_EVENT_SEND_SUBSCRIBE;
    current_event.send_context.conn = conn;
    current_event.send_context.args.sub_args = *args;
    current_event.event_type = MQTT_SEND_EVENT;

    return wiced_rtos_push_to_queue( &socket.queue, &current_event, WICED_NO_WAIT );
}

wiced_result_t mqtt_unsubscribe( mqtt_connection_t *conn, mqtt_unsubscribe_arg_t *args )
{
    mqtt_socket_t socket = conn->socket;
    mqtt_event_message_t current_event;
    wiced_result_t result = WICED_SUCCESS;

    current_event.send_context.event_t = MQTT_EVENT_SEND_UNSUBSCRIBE;
    current_event.send_context.conn = conn;
    current_event.send_context.args.unsub_args = *args;
    current_event.event_type = MQTT_SEND_EVENT;

    result = wiced_rtos_push_to_queue( &socket.queue, &current_event, WICED_NO_WAIT );
    return result;
}

wiced_result_t mqtt_publish( mqtt_connection_t *conn, mqtt_publish_arg_t *args )
{
    mqtt_socket_t socket = conn->socket;
    mqtt_event_message_t current_event;

    current_event.send_context.event_t = MQTT_EVENT_SEND_PUBLISH;
    current_event.send_context.conn = conn;
    current_event.send_context.args.pub_args = *args;
    current_event.event_type = MQTT_SEND_EVENT;

    return wiced_rtos_push_to_queue( &socket.queue, &current_event, WICED_NO_WAIT );
}

wiced_result_t mqtt_connection_deinit( mqtt_connection_t *conn )
{
    WPRINT_LIB_ERROR(( "[MQTT LIB] connection deinit\n " ));
    mqtt_network_deinit( &conn->socket );
    wiced_rtos_deinit_semaphore( &conn->semaphore );
    return WICED_SUCCESS;
}

/******************************************************
 *      Backend functions called from mqtt_queue
 ******************************************************/
wiced_result_t mqtt_backend_put_connect( const mqtt_connect_arg_t *args, mqtt_connection_t *conn )
{
    wiced_result_t ret;
    mqtt_frame_t frame;
    mqtt_connect_arg_t final_args = *args;

    /* Fixing connect args based on specification */
    if ( final_args.username_flag == WICED_FALSE )
    {
        /* make sure password flag is 0 as well */
        final_args.password_flag = WICED_FALSE;
    }

    if ( final_args.will_flag == WICED_FALSE )
    {
        /* make sure will_retain and will_qos are 0 */
        final_args.will_retain = WICED_FALSE;
        final_args.will_qos = MQTT_QOS_DELIVER_AT_MOST_ONCE;
    }

    /* Send Protocol Header */
    ret = mqtt_frame_create( MQTT_CONNECTION_FRAME_MAX, &frame, &conn->socket );
    if ( ret != WICED_SUCCESS )
    {
        return ret;
    }
    mqtt_frame_put_connect( &frame, &final_args );
    return mqtt_frame_send( &frame, &conn->socket );
}

wiced_result_t mqtt_backend_get_connack( mqtt_connack_arg_t *args, mqtt_connection_t *conn )
{
    wiced_result_t ret = WICED_SUCCESS;
    UNUSED_PARAMETER(args);
    if ( ret == WICED_SUCCESS )
    {
        /* Publish is an async method (we don't get an OK), so we simulate the OK after sending it */

        if ( conn->callbacks != NULL )
        {

            wiced_mqtt_event_info_t event;
            event.type = WICED_MQTT_EVENT_TYPE_CONNECT_REQ_STATUS;
            event.data.err_code = args->return_code;
            conn->callbacks( (void*) conn, &event );
        }
    }
    return ret;
}

wiced_result_t mqtt_backend_put_publish( const mqtt_publish_arg_t *args, mqtt_connection_t *conn )
{
    wiced_result_t ret;
    mqtt_frame_t frame;
    mqtt_publish_arg_t final_args = *args;

    /* Send Protocol Header */
    ret = mqtt_frame_create( MQTT_CONNECTION_FRAME_MAX, &frame, &conn->socket );
    if ( ret != WICED_SUCCESS )
    {
        return ret;
    }
    mqtt_frame_put_publish( &frame, &final_args );
    return mqtt_frame_send( &frame, &conn->socket );
}

wiced_result_t mqtt_backend_get_publish( mqtt_publish_arg_t *args, mqtt_connection_t *conn )
{
    wiced_result_t ret;
    wiced_mqtt_event_info_t event;
    ret = mqtt_manager( MQTT_EVENT_RECV_PUBLISH, args, conn );
    if ( ( ret == WICED_SUCCESS ) && ( args->data != NULL ) )
    {
        /* Publish is an async method (we don't get an OK), so we simulate the OK after sending it */
        if ( conn->callbacks != NULL )
        {
            event.type = WICED_MQTT_EVENT_TYPE_PUBLISH_MSG_RECEIVED;
            event.data.pub_recvd.topic = args->topic.str;
            event.data.pub_recvd.topic_len = args->topic.len;
            event.data.pub_recvd.data = args->data;
            event.data.pub_recvd.data_len = args->data_len;
            conn->callbacks( (void*) conn, &event );
        }
    }
    return ret;
}

wiced_result_t mqtt_backend_put_puback( const mqtt_puback_arg_t *args, mqtt_connection_t *conn )
{
    wiced_result_t ret;
    mqtt_frame_t frame;

    /* Send Protocol Header */
    ret = mqtt_frame_create( MQTT_CONNECTION_FRAME_MAX, &frame, &conn->socket );
    if ( ret != WICED_SUCCESS )
    {
        return ret;
    }
    mqtt_frame_put_puback( &frame, args );
    return mqtt_frame_send( &frame, &conn->socket );
}

wiced_result_t mqtt_backend_get_puback( mqtt_puback_arg_t *args, mqtt_connection_t *conn )
{
    wiced_result_t ret;
    ret = mqtt_manager( MQTT_EVENT_RECV_PUBACK, args, conn );
    if ( ret == WICED_SUCCESS )
    {
        if ( conn->callbacks != NULL )
        {
            wiced_mqtt_event_info_t event;
            event.type = WICED_MQTT_EVENT_TYPE_PUBLISHED;
            event.data.msgid = args->packet_id;
            conn->callbacks( (void*) conn, &event );
        }
    }
    return ret;
}

wiced_result_t mqtt_backend_put_pubrec( const mqtt_pubrec_arg_t *args, mqtt_connection_t *conn )
{
    wiced_result_t ret;
    mqtt_frame_t frame;

    /* Send Protocol Header */
    ret = mqtt_frame_create( MQTT_CONNECTION_FRAME_MAX, &frame, &conn->socket );
    if ( ret != WICED_SUCCESS )
    {
        return ret;
    }
    mqtt_frame_put_pubrec( &frame, args );
    return mqtt_frame_send( &frame, &conn->socket );
}

wiced_result_t mqtt_backend_get_pubrec( mqtt_pubrec_arg_t *args, mqtt_connection_t *conn )
{
    wiced_result_t ret;
    ret = mqtt_manager( MQTT_EVENT_RECV_PUBREC, args, conn );
    if ( ret == WICED_SUCCESS )
    {
        /* Publish is an async method (we don't get an OK), so we simulate the OK after sending it */
        if ( conn->callbacks != NULL )
        {
            wiced_mqtt_event_info_t event;
            event.type = WICED_MQTT_EVENT_TYPE_UNKNOWN;
            event.data.msgid = args->packet_id;
            conn->callbacks( (void*) conn, &event );
        }

    }
    return ret;
}

wiced_result_t mqtt_backend_put_pubrel( const mqtt_pubrel_arg_t *args, mqtt_connection_t *conn )
{
    wiced_result_t ret;
    mqtt_frame_t frame;

    /* Send Protocol Header */
    ret = mqtt_frame_create( MQTT_CONNECTION_FRAME_MAX, &frame, &conn->socket );
    if ( ret != WICED_SUCCESS )
    {
        return ret;
    }
    mqtt_frame_put_pubrel( &frame, args );
    return mqtt_frame_send( &frame, &conn->socket );
}

wiced_result_t mqtt_backend_get_pubrel( mqtt_pubrel_arg_t *args, mqtt_connection_t *conn )
{
    wiced_result_t ret;
    ret = mqtt_manager( MQTT_EVENT_RECV_PUBREL, args, conn );
    if ( ret == WICED_SUCCESS )
    {
        if ( conn->callbacks != NULL )
        {
            wiced_mqtt_event_info_t event;
            event.type = WICED_MQTT_EVENT_TYPE_UNKNOWN;
            event.data.msgid = args->packet_id;
            conn->callbacks( (void*) conn, &event );
        }

    }
    return ret;
}

wiced_result_t mqtt_backend_put_pubcomp( const mqtt_pubcomp_arg_t *args, mqtt_connection_t *conn )
{
    wiced_result_t ret;
    mqtt_frame_t frame;

    /* Send Protocol Header */
    ret = mqtt_frame_create( MQTT_CONNECTION_FRAME_MAX, &frame, &conn->socket );
    if ( ret != WICED_SUCCESS )
    {
        return ret;
    }
    mqtt_frame_put_pubcomp( &frame, args );
    return mqtt_frame_send( &frame, &conn->socket );
}

wiced_result_t mqtt_backend_get_pubcomp( mqtt_pubcomp_arg_t *args, mqtt_connection_t *conn )
{
    wiced_result_t ret;
    ret = mqtt_manager( MQTT_EVENT_RECV_PUBCOMP, args, conn );
    if ( ret == WICED_SUCCESS )
    {
        /* Publish is an async method (we don't get an OK), so we simulate the OK after sending it */
        if ( conn->callbacks != NULL )
        {
            wiced_mqtt_event_info_t event;
            event.type = WICED_MQTT_EVENT_TYPE_PUBLISHED;
            event.data.msgid = args->packet_id;
            conn->callbacks( (void*) conn, &event );
        }

    }
    return ret;
}

wiced_result_t mqtt_backend_put_subscribe( const mqtt_subscribe_arg_t *args, mqtt_connection_t *conn )
{
    wiced_result_t ret;
    mqtt_frame_t frame;
    mqtt_subscribe_arg_t final_args = *args;

    /* Send Protocol Header */
    ret = mqtt_frame_create( MQTT_CONNECTION_FRAME_MAX, &frame, &conn->socket );
    if ( ret != WICED_SUCCESS )
    {
        return ret;
    }
    mqtt_frame_put_subscribe( &frame, &final_args );
    return mqtt_frame_send( &frame, &conn->socket );
}

wiced_result_t mqtt_backend_get_suback( wiced_mqtt_suback_arg_t *args, mqtt_connection_t *conn )
{
    wiced_result_t ret;
    ret = mqtt_manager( MQTT_EVENT_RECV_SUBACK, args, conn );
    if ( ret == WICED_SUCCESS )
    {
        /* Publish is an async method (we don't get an OK), so we simulate the OK after sending it */

        if ( conn->callbacks != NULL )
        {

            wiced_mqtt_event_info_t event;
            event.type = WICED_MQTT_EVENT_TYPE_SUBCRIBED;
            event.data.msgid = args->packet_id;
            conn->callbacks( (void*) conn, &event );
        }

    }
    return ret;
}

wiced_result_t mqtt_backend_put_unsubscribe( const mqtt_unsubscribe_arg_t *args, mqtt_connection_t *conn )
{
    wiced_result_t ret;
    mqtt_frame_t frame;
    mqtt_unsubscribe_arg_t final_args = *args;

    /* Generate packet ID */
    // final_args.packet_id = conn->packet_id++;

    /* Send Protocol Header */
    ret = mqtt_frame_create( MQTT_CONNECTION_FRAME_MAX, &frame, &conn->socket );
    if ( ret != WICED_SUCCESS )
    {
        return ret;
    }
    mqtt_frame_put_unsubscribe( &frame, &final_args );
    return mqtt_frame_send( &frame, &conn->socket );
}

wiced_result_t mqtt_backend_get_unsuback( mqtt_unsuback_arg_t *args, mqtt_connection_t *conn )
{
    wiced_result_t ret;
    ret = mqtt_manager( MQTT_EVENT_RECV_UNSUBACK, args, conn );
    if ( ret == WICED_SUCCESS )
    {

        if ( conn->callbacks != NULL )
        {

            wiced_mqtt_event_info_t event;
            event.type = WICED_MQTT_EVENT_TYPE_UNSUBSCRIBED;
            event.data.msgid = args->packet_id;

            conn->callbacks( (void*) conn, &event );
        }
    }
    return ret;
}

wiced_result_t mqtt_backend_put_disconnect( mqtt_connection_t *conn )
{
    wiced_result_t ret;
    mqtt_frame_t frame;

    ret = mqtt_frame_create( MQTT_CONNECTION_FRAME_MAX, &frame, &conn->socket );
    if ( ret != WICED_SUCCESS )
    {
        return ret;
    }
    mqtt_frame_put_disconnect( &frame );
    return mqtt_frame_send( &frame, &conn->socket );
}

wiced_result_t mqtt_backend_connection_close( mqtt_connection_t *conn )
{
    wiced_result_t ret;
    ret = mqtt_manager( MQTT_EVENT_CONNECTION_CLOSE, NULL, conn );
    if ( ret == WICED_SUCCESS )
    {
        /* Publish is an async method (we don't get an OK), so we simulate the OK after sending it */

        if ( conn->callbacks != NULL )
        {

            wiced_mqtt_event_info_t event;
            event.type = WICED_MQTT_EVENT_TYPE_DISCONNECTED;
            conn->callbacks( (void*) conn, &event );
        }
    }
    return ret;
}

wiced_result_t mqtt_backend_put_pingreq( mqtt_connection_t *conn )
{
    wiced_result_t ret;
    mqtt_frame_t frame;

    ret = mqtt_frame_create( MQTT_CONNECTION_FRAME_MAX, &frame, &conn->socket );
    if ( ret != WICED_SUCCESS )
    {
        return ret;
    }
    mqtt_frame_put_pingreq( &frame );
    return mqtt_frame_send( &frame, &conn->socket );
}

wiced_result_t mqtt_backend_get_pingres( mqtt_connection_t *conn )
{
    return mqtt_manager( MQTT_EVENT_RECV_PINGRES, NULL, conn );
}
