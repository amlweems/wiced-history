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
 *  WICED MQTT constants, data types and APIs.
 */
#pragma once

#include "wiced.h"
#include "mqtt_api.h"
#include "mqtt_frame.h"
#include "mqtt_network.h"
#include "mqtt_session.h"


#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                    Macros
 ******************************************************/
#define MQTT_CONNECTION_FRAME_MAX                     (4 * 1024)        /* Maximum frame size for a connection          */
#define MQTT_CONNECTION_DATA_SIZE_MAX                 (MQTT_CONNECTION_FRAME_MAX - 8) /* Maximum size to put in a frame */

/******************************************************
 *                   Enumerations
 ******************************************************/
typedef enum mqtt_event_e
{
    MQTT_EVENT_SEND_CONNECT              = 1,
    MQTT_EVENT_RECV_CONNACK              = 2,
    MQTT_EVENT_SEND_DISCONNECT           = 3,
    MQTT_EVENT_CONNECTION_CLOSE          = 4,
    MQTT_EVENT_SEND_SUBSCRIBE            = 5,
    MQTT_EVENT_RECV_SUBACK               = 6,
    MQTT_EVENT_SEND_UNSUBSCRIBE          = 7,
    MQTT_EVENT_RECV_UNSUBACK             = 8,
    MQTT_EVENT_SEND_PUBLISH              = 9,
    MQTT_EVENT_RECV_PUBACK               = 10,
    MQTT_EVENT_RECV_PUBLISH              = 11,
    MQTT_EVENT_SEND_PUBACK               = 12,
    MQTT_EVENT_RECV_PUBREC               = 13,
    MQTT_EVENT_SEND_PUBREC               = 14,
    MQTT_EVENT_RECV_PUBREL               = 15,
    MQTT_EVENT_SEND_PUBREL               = 16,
    MQTT_EVENT_RECV_PUBCOMP              = 17,
    MQTT_EVENT_SEND_PUBCOMP              = 18,
    MQTT_EVENT_RECV_PINGRES              = 20,
    MQTT_EVENT_SEND_PINGREQ              = 21,
    MQTT_EVENT_TICK                      = 22,
} mqtt_event_t;

typedef enum
{
    MQTT_RECEIVE_EVENT,
    MQTT_SEND_EVENT,
    MQTT_ERROR_EVENT,
    MQTT_DISCONNECT_EVENT
} mqtt_main_event_t;

/******************************************************
 *                   Typedefs
 ******************************************************/
typedef struct mqtt_heartbeat_s
{
    wiced_timed_event_t               timer;
    uint32_t                          reset_value;
    uint32_t                          step_value;
    uint32_t                          send_counter;
    uint32_t                          recv_counter;
} mqtt_heartbeat_t;


/**
 *  IMPORTANT:
 *       Any change made to the size of this structure should be reflected
 *       in the macro WICED_MQTT_OBJECT_MEMORY_SIZE_REQUIREMENT defined in MQTT API header file
 */
typedef struct mqtt_connection_s
{
    uint8_t                         session_init;
    uint8_t                         net_init_ok;
    uint16_t                        packet_id;
    wiced_semaphore_t               semaphore;
    mqtt_socket_t                   socket;
    wiced_mqtt_callback_t           callbacks;
    mqtt_heartbeat_t                heartbeat;
    mqtt_session_t*                 session;
} mqtt_connection_t;

typedef struct mqtt_send_context_t
{
        mqtt_event_t event_t;
        mqtt_connection_t *conn;
        union
        {
                mqtt_connect_arg_t conn_args;
                mqtt_publish_arg_t pub_args;
                mqtt_subscribe_arg_t sub_args;
                mqtt_unsubscribe_arg_t unsub_args;
        } args;

}mqtt_send_context;

typedef struct
{
    mqtt_main_event_t  event_type;
    mqtt_send_context  send_context;
} mqtt_event_message_t;

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
 *                API Definitions
 ******************************************************/
wiced_result_t mqtt_connection_init( const wiced_ip_address_t *address, uint16_t port_number, wiced_interface_t interface, const wiced_mqtt_callback_t callbacks, mqtt_connection_t *conn, const wiced_mqtt_security_t *security );
wiced_result_t mqtt_connection_deinit( mqtt_connection_t *conn );
wiced_result_t mqtt_connect( mqtt_connection_t *conn, mqtt_connect_arg_t *args, mqtt_session_t *session );
wiced_result_t mqtt_disconnect( mqtt_connection_t *conn );
wiced_result_t mqtt_publish( mqtt_connection_t *conn, mqtt_publish_arg_t *args );
wiced_result_t mqtt_subscribe( mqtt_connection_t *conn, mqtt_subscribe_arg_t *args );
wiced_result_t mqtt_unsubscribe( mqtt_connection_t *conn, mqtt_unsubscribe_arg_t *args );
wiced_result_t mqtt_core_init( mqtt_connection_t *conn );
wiced_result_t mqtt_core_deinit( mqtt_connection_t *conn );



#ifdef __cplusplus
} /* extern "C" */
#endif
