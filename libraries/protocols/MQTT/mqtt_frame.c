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
 *  Frame packing and unpacking functions
 */

#include "wiced.h"
#include "mqtt_internal.h"
#include "mqtt_connection.h"
#include "mqtt_frame.h"

/******************************************************
 *                      Macros
 ******************************************************/
#define MQTT_CONNECT_PKT_VARIABLE_HEADER_LEN_VER3     (12)
#define MQTT_CONNECT_PKT_VARIABLE_HEADER_LEN_VER4     (10)
/* Note: pointers are not guaranteed to be aligned, so we can't just assign
 * them to htonx values.
 */

/*                      *
 *  BUFFER PUT MACROS   *
 *                      */
#define MQTT_BUFFER_PUT_OCTET( BUFFER, VALUE )   \
    do                                               \
    {                                                \
        uint8_t *data_po = (BUFFER)->data;           \
        *(data_po)   = (VALUE);                      \
        ((BUFFER)->data)++;                          \
    }while(0)

#define MQTT_BUFFER_PUT_SHORT( BUFFER, VALUE )                 \
    do                                                             \
    {                                                              \
        uint8_t *data_ps = (BUFFER)->data;                         \
        *(data_ps)     = (uint8_t)(((VALUE) & 0x0000FF00) >> 8);   \
        *(data_ps + 1) = (uint8_t) ((VALUE) & 0x000000FF);         \
        ((BUFFER)->data) += 2;                                     \
    }while(0)

#define MQTT_BUFFER_PUT_LONG( BUFFER, VALUE )                  \
    do                                                             \
    {                                                              \
        uint8_t *data_pl = (BUFFER)->data;                         \
        *(data_pl)     = (uint8_t)(((VALUE) & 0xFF000000) >> 24);  \
        *(data_pl + 1) = (uint8_t)(((VALUE) & 0x00FF0000) >> 16);  \
        *(data_pl + 2) = (uint8_t)(((VALUE) & 0x0000FF00) >> 8 );  \
        *(data_pl + 3) = (uint8_t) ((VALUE) & 0x000000FF);         \
        ((BUFFER)->data) += 4;                                     \
    }while(0)


#define MQTT_BUFFER_PUT_STRING( BUFFER, STRING, LEN)       \
    do                                                         \
    {                                                          \
        uint8_t *data = (BUFFER)->data;                        \
        MQTT_BUFFER_PUT_SHORT( BUFFER, LEN );                  \
        memcpy( (data) + 2, STRING, LEN);                      \
        ((BUFFER)->data) += LEN;                               \
    }while(0)

#define MQTT_BUFFER_PUT_VARIABLE_LENGTH( BUFFER, VAL, LEN) \
    do                                                         \
    {                                                          \
        uint32_t X = VAL;                                      \
        do                                                     \
        {                                                      \
            uint8_t *data = (BUFFER)->data;                    \
            (LEN)++;                                           \
            *data = (uint8_t)( X % 128 );                      \
            X     = X / 128;                                   \
            if ( X > 0 ){ *data |= 0x80;}                        \
            ((BUFFER)->data) += 1;                             \
        }  while ( X > 0 );                                    \
    }while( 0 )


#define MQTT_BUFFER_PUT_BIT( BUFFER, VALUE, BIT, INC)                                   \
    do                                                                                      \
    {                                                                                       \
        uint8_t  mask_pb = ( 1 << (BIT));                                                   \
        uint8_t *data_pb = (BUFFER)->data;                                                  \
        (*data_pb) = (uint8_t)( (VALUE) ? (*data_pb) | mask_pb : (*data_pb) & (~mask_pb) ); \
        if (INC) { ((BUFFER)->data) += 1; }                                                 \
    }while(0)

#define MQTT_BUFFER_PUT_2BIT( BUFFER, VALUE, BIT, INC)                                  \
    do                                                                                      \
    {                                                                                       \
        uint8_t  mask_pb = ( 3 << (BIT));                                                   \
        uint8_t *data_pb = (BUFFER)->data;                                                  \
        (*data_pb) = (uint8_t)( (*data_pb) & (~mask_pb) );                                  \
        (*data_pb) = (uint8_t)( (*data_pb) | ( (VALUE & 3) << (BIT) ) );                    \
        if (INC) { ((BUFFER)->data) += 1; }                                                 \
    }while(0)

#define MQTT_BUFFER_PUT_4BIT( BUFFER, VALUE, BIT, INC)                                  \
    do                                                                                      \
    {                                                                                       \
        uint8_t  mask_pb = ( 0xF << (BIT));                                                 \
        uint8_t *data_pb = (BUFFER)->data;                                                  \
        (*data_pb) = (uint8_t)( (*data_pb) & (~mask_pb) );                                  \
        (*data_pb) = (uint8_t)( (*data_pb) | ( (VALUE & 0xF) << (BIT) ) );                  \
        if (INC) { ((BUFFER)->data) += 1; }                                                 \
    }while(0)

/*                      *
 *  BUFFER GET MACROS   *
 *                      */
#define MQTT_BUFFER_GET_OCTET( BUFFER, VALUE )   \
    do                                               \
    {                                                \
        uint8_t *data_go = (BUFFER)->data;           \
        (VALUE) = *(data_go);                        \
        ((BUFFER)->data)++;                          \
    }while(0)

#define MQTT_BUFFER_GET_SHORT( BUFFER, VALUE )        \
    do                                                    \
    {                                                     \
        uint8_t *data_gs = (BUFFER)->data;                \
        VALUE = (uint16_t)(         (*(data_gs    ) << 8)); \
        VALUE = (uint16_t)( VALUE + (*(data_gs + 1)     )); \
        ((BUFFER)->data) += 2;                            \
    }while(0)

#define MQTT_BUFFER_GET_LONG( BUFFER, VALUE )               \
    do                                                          \
    {                                                           \
        uint8_t *data_gl = (BUFFER)->data;                      \
        (VALUE) = (uint32_t) (                    (((uint32_t)*(data_gl    )) << 24));    \
        (VALUE) = (uint32_t) ((uint32_t)(VALUE) + (((uint32_t)*(data_gl + 1)) << 16));  \
        (VALUE) = (uint32_t) ((uint32_t)(VALUE) + (((uint32_t)*(data_gl + 2)) << 8 ));  \
        (VALUE) = (uint32_t) ((uint32_t)(VALUE) + (((uint32_t)*(data_gl + 3))      ));         \
        ((BUFFER)->data) += 4;                                  \
    }while(0)

#define MQTT_BUFFER_GET_STRING( BUFFER, STRING, LEN)       \
    do                                                         \
    {                                                          \
        MQTT_BUFFER_GET_SHORT( (BUFFER), (LEN) );              \
        (STRING) = (BUFFER)->data;                             \
        (BUFFER)->data += (LEN);                               \
    }while(0)


#define MQTT_BUFFER_GET_VARIABLE_LENGTH( BUFFER, VALUE )   \
    do                                                         \
    {                                                          \
        uint8_t *data_gb;                                      \
        uint32_t multiplier = 1;                               \
        VALUE = 0;                                             \
        do                                                     \
        {                                                      \
            data_gb = (BUFFER)->data++;                        \
            VALUE += ((*data_gb) & 127 ) * multiplier;         \
            multiplier *= 128;                                 \
        } while (((*data_gb) & 128) != 0);                     \
    }while(0)


#define MQTT_BUFFER_GET_BIT( BUFFER, VALUE, BIT, INC)      \
    do                                                         \
    {                                                          \
        uint8_t  mask_gb = ( 1 << (BIT));                      \
        uint8_t *data_gb = (BUFFER)->data;                     \
        (VALUE) = *data_gb & mask_gb;                          \
        (VALUE) = (VALUE) ? 1 : 0;                             \
        if (INC) { ((BUFFER)->data) += 1; }                    \
    }while(0)

#define MQTT_BUFFER_GET_2BIT( BUFFER, VALUE, BIT, INC)     \
    do                                                         \
    {                                                          \
        uint8_t  mask_gb = ( 3 << (BIT));                      \
        uint8_t *data_gb = (BUFFER)->data;                     \
        (VALUE) = *data_gb & mask_gb;                          \
        (VALUE) = (VALUE) >> (BIT);                            \
        if (INC) { ((BUFFER)->data) += 1; }                    \
    }while(0)

#define MQTT_BUFFER_GET_4BIT( BUFFER, VALUE, BIT, INC)     \
    do                                                         \
    {                                                          \
        uint8_t  mask_gb = ( 0xF << (BIT));                    \
        uint8_t *data_gb = (BUFFER)->data;                     \
        (VALUE) = *data_gb & mask_gb;                          \
        (VALUE) = (VALUE) >> (BIT);                            \
        if (INC) { ((BUFFER)->data) += 1; }                    \
    }while(0)


/******************************************************
 *                    Constants
 ******************************************************/
#define MQTT_PACKET_CONNECT_STR_VER3         "MQIsdp"
#define MQTT_PACKET_CONNECT_STR_VER4         "MQTT"

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/
#define LENGTH_OF_STRING(X) ((X).len + sizeof((X).len))
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

/******************************************************
 *               Interface functions
 ******************************************************/
wiced_result_t  mqtt_frame_recv( wiced_mqtt_buffer_t *buffer, void *p_user, uint32_t *size )
{
    mqtt_frame_type_t  type;
    mqtt_frame_t       frame;
    wiced_result_t           result = WICED_SUCCESS;
    mqtt_connection_t *conn = (mqtt_connection_t *)p_user;

    if ( NULL == buffer )
    {
        /* Receive error, probably connection closed */
        return mqtt_backend_connection_close( p_user );
    }

    /* Check valid type type */
    frame.start  = buffer->data;
    frame.buffer = *buffer;
    MQTT_BUFFER_GET_OCTET( buffer, type );
    buffer->data--;
    type = ( ( type & 0xF0 ) >> 4 );
    switch (type)
    {
        case MQTT_PACKET_TYPE_UNSUBACK              :
        {
            mqtt_unsuback_arg_t   args;
            (*size) = 4;
            mqtt_frame_get_unsuback( &frame, &args );
            result = mqtt_backend_get_unsuback( &args, conn );
            break;
        }
        case MQTT_PACKET_TYPE_SUBACK              :
        {
            wiced_mqtt_suback_arg_t   args;
            (*size) = 5;
            mqtt_frame_get_suback( &frame, &args );
            result = mqtt_backend_get_suback( &args, conn );
            break;
        }
        case MQTT_PACKET_TYPE_CONNACK:
        {
            mqtt_connack_arg_t   args;
            (*size) = 4;
            mqtt_frame_get_connack( &frame, &args );
            result = mqtt_backend_get_connack( &args, conn );
            break;
        }
        case MQTT_PACKET_TYPE_PUBLISH:
        {
            mqtt_publish_arg_t   args;
            uint32_t total_size;
            mqtt_frame_get_publish( &frame, &args );
            total_size = args.data_len + args.topic.len + sizeof(args.topic.len ) + (args.qos == MQTT_QOS_DELIVER_AT_MOST_ONCE ? 0 : 2);
            /* accounting for variable length size */
            if ( total_size <= 127 )
            {
                total_size += 1;
            }
            else if ( total_size <= 16383 )
            {
                total_size += 2;
            }
            else if ( total_size <= 2097151 )
            {
                total_size += 3;
            }
            else
            {
                total_size += 4;
            }
            total_size += 1; /* To account for the type and flags */
            (*size) = total_size;
            result = mqtt_backend_get_publish( &args, conn );
            break;
        }
        case MQTT_PACKET_TYPE_PUBACK              :
        {
            mqtt_puback_arg_t   args;
            (*size) = 4;
            mqtt_frame_get_puback( &frame, &args );
            result = mqtt_backend_get_puback( &args, conn );
            break;
        }
        case MQTT_PACKET_TYPE_PUBREC              :
        {
            mqtt_pubrec_arg_t   args;
            (*size) = 4;
            mqtt_frame_get_pubrec( &frame, &args );
            result = mqtt_backend_get_pubrec( &args, conn );
            break;
        }
        case MQTT_PACKET_TYPE_PUBREL              :
        {
            mqtt_pubrel_arg_t   args;
            (*size) = 4;
            mqtt_frame_get_pubrel( &frame, &args );
            result = mqtt_backend_get_pubrel( &args, conn );
            break;
        }
        case MQTT_PACKET_TYPE_PUBCOMP             :
        {
            mqtt_pubcomp_arg_t   args;
            (*size) = 4;
            mqtt_frame_get_pubcomp( &frame, &args );
            result = mqtt_backend_get_pubcomp( &args, conn );
            break;
        }
        case MQTT_PACKET_TYPE_PINGRESP            :
        {
            (*size) = 2;
            result = mqtt_backend_get_pingres( conn );
            break;
        }
        case MQTT_PACKET_TYPE_PINGREQ             :
        case MQTT_PACKET_TYPE_SUBSCRIBE           :
        case MQTT_PACKET_TYPE_UNSUBSCRIBE         :
        case MQTT_PACKET_TYPE_CONNECT             :
        case MQTT_PACKET_TYPE_DISCONNECT          :
        default:
            return WICED_ERROR;
    }
    return result;
}

wiced_result_t  mqtt_frame_create( uint16_t max_size, mqtt_frame_t *frame, mqtt_socket_t *socket )
{
    wiced_result_t ret;
    ret = mqtt_network_create_buffer( &frame->buffer, max_size, socket );
    if ( ret == WICED_SUCCESS )
    {
        frame->size     = 0;
        frame->start    = frame->buffer.data;
    }
    return ret;
}

wiced_result_t  mqtt_frame_delete( mqtt_frame_t *frame )
{
    return mqtt_network_delete_buffer( &frame->buffer );
}

wiced_result_t  mqtt_frame_send( mqtt_frame_t *frame, mqtt_socket_t *socket )
{
    return mqtt_network_send_buffer( &frame->buffer, socket );
}

/******************************************************
 *               Connect frame
 ******************************************************/
wiced_result_t mqtt_frame_put_connect( mqtt_frame_t *frame, const mqtt_connect_arg_t *args )
{
    uint32_t size = (uint32_t) (                       args->client_id.len    + sizeof( args->client_id.len    )     ) + /* Client ID */
                    (uint32_t) ( args->will_flag     ? args->will_message.len + sizeof( args->will_message.len ) : 0 ) + /* will message if sent */
                    (uint32_t) ( args->will_flag     ? args->will_topic.len   + sizeof( args->will_topic.len   ) : 0 ) + /* will topic if sent */
                    (uint32_t) ( args->username_flag ? args->username.len     + sizeof( args->username.len     ) : 0 ) + /* username if sent */
                    (uint32_t) ( args->password_flag ? args->password.len     + sizeof( args->password.len     ) : 0 );  /* password if sent */

    /* MQTT string + protocol level + connect flags + Keep alive */
    /* variable depending on MQTT or MQIsdp */
    if ( args->mqtt_version == WICED_MQTT_PROTOCOL_VER4 )
    {
        size = size + (uint32_t) MQTT_CONNECT_PKT_VARIABLE_HEADER_LEN_VER4;
    }
    else
    {
        size = size + (uint32_t) MQTT_CONNECT_PKT_VARIABLE_HEADER_LEN_VER3;
    }

    /* A long value carrying string MQTT */MQTT_BUFFER_PUT_4BIT( &frame->buffer, 0, 0, 0 ); /* Reserved */
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, MQTT_PACKET_TYPE_CONNECT, 4, 1 );
    MQTT_BUFFER_PUT_VARIABLE_LENGTH( &frame->buffer, size, frame->size );
    if ( args->mqtt_version == 4 )
    {
        MQTT_BUFFER_PUT_STRING( &frame->buffer, MQTT_PACKET_CONNECT_STR_VER4, strlen(MQTT_PACKET_CONNECT_STR_VER4) );
        MQTT_BUFFER_PUT_OCTET( &frame->buffer, WICED_MQTT_PROTOCOL_VER4 );
    }
    else
    {
        MQTT_BUFFER_PUT_STRING( &frame->buffer, MQTT_PACKET_CONNECT_STR_VER3, strlen(MQTT_PACKET_CONNECT_STR_VER3) );
        MQTT_BUFFER_PUT_OCTET( &frame->buffer, WICED_MQTT_PROTOCOL_VER3 );
    }
    MQTT_BUFFER_PUT_BIT( &frame->buffer, 0, 0, 0 );  /* Broker will validate that the reserved flag in the CONNECT Control Packet is set to zero and disconnect the Client if it is not zero */
    MQTT_BUFFER_PUT_BIT( &frame->buffer, args->clean_session, 1, 0 );
    MQTT_BUFFER_PUT_BIT( &frame->buffer, args->will_flag, 2, 0 );
    MQTT_BUFFER_PUT_2BIT( &frame->buffer, args->will_qos, 3, 0 );
    MQTT_BUFFER_PUT_BIT( &frame->buffer, args->will_retain, 5, 0 );
    MQTT_BUFFER_PUT_BIT( &frame->buffer, args->username_flag, 6, 0 );
    MQTT_BUFFER_PUT_BIT( &frame->buffer, args->password_flag, 7, 1 );
    MQTT_BUFFER_PUT_SHORT( &frame->buffer, 0 ); /* Keep alive for now */
    MQTT_BUFFER_PUT_STRING( &frame->buffer, args->client_id.str, args->client_id.len );
    if ( args->will_flag )
    {
        MQTT_BUFFER_PUT_STRING( &frame->buffer, args->will_topic.str, args->will_topic.len );
    }
    if ( args->will_flag )
    {
        MQTT_BUFFER_PUT_STRING( &frame->buffer, args->will_message.str, args->will_message.len );
    }
    if ( args->username_flag )
    {
        MQTT_BUFFER_PUT_STRING( &frame->buffer, args->username.str, args->username.len );
    }
    if ( args->password_flag )
    {
        MQTT_BUFFER_PUT_STRING( &frame->buffer, args->password.str, args->password.len );
    }
    return WICED_SUCCESS;
}

wiced_result_t mqtt_frame_get_connack( mqtt_frame_t *frame, mqtt_connack_arg_t *args )
{
    uint8_t type;
    uint8_t size;

    (void)size;
    (void)type;
    /* A long value carrying string MQTT */
    MQTT_BUFFER_GET_OCTET( &frame->buffer, type );    /* Type-Reserved */
    type = ( type & 0xF0 ) >> 4;
    wiced_assert("[MQTT] CONNACT type mismatch.", type == MQTT_PACKET_TYPE_CONNACK );
    MQTT_BUFFER_GET_OCTET( &frame->buffer, size );
    wiced_assert("[MQTT] CONNACT size mismatch.", size == 2 );
    MQTT_BUFFER_GET_BIT( &frame->buffer, args->session_present, 1, 1 ); /* Connect acknowledge flag */
    MQTT_BUFFER_GET_OCTET( &frame->buffer, args->return_code );         /* Connect return code */
    return WICED_SUCCESS;
}

wiced_result_t mqtt_frame_put_publish( mqtt_frame_t *frame, const mqtt_publish_arg_t *args )
{
    uint32_t    size = ( uint32_t ) ( args->qos == MQTT_QOS_DELIVER_AT_MOST_ONCE ? 0 : 2)   /* Packet identifier  */
                       + ( uint32_t ) ( args->topic.len + sizeof(args->topic.len))          /* will topic if sent */
                       + ( uint32_t ) args->data_len;                                       /* size of message    */

    /* A long value carrying string MQTT */
    MQTT_BUFFER_PUT_BIT( &frame->buffer, args->retain, 0, 0 );
    MQTT_BUFFER_PUT_2BIT( &frame->buffer, args->qos, 1, 0 );
    MQTT_BUFFER_PUT_BIT( &frame->buffer, args->dup, 3, 0 );
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, MQTT_PACKET_TYPE_PUBLISH, 4, 1 );
    MQTT_BUFFER_PUT_VARIABLE_LENGTH( &frame->buffer, size, frame->size );
    MQTT_BUFFER_PUT_STRING( &frame->buffer, args->topic.str, args->topic.len );
    if ( args->qos != MQTT_QOS_DELIVER_AT_MOST_ONCE )
    {
        MQTT_BUFFER_PUT_SHORT( &frame->buffer, args->packet_id );
    }
    memcpy( frame->buffer.data, args->data, args->data_len );
    frame->buffer.data += args->data_len;
    return WICED_SUCCESS;
}

wiced_result_t mqtt_frame_get_publish( mqtt_frame_t *frame, mqtt_publish_arg_t *args )
{
    uint8_t type;
    uint32_t size;

    (void)size;
    (void)type;
    /* A long value carrying string MQTT */
    /* A long value carrying string MQTT */
    MQTT_BUFFER_GET_BIT( &frame->buffer, args->retain, 0, 0 );
    MQTT_BUFFER_GET_2BIT( &frame->buffer, args->qos, 1, 0 );
    MQTT_BUFFER_GET_BIT( &frame->buffer, args->dup, 3, 0 );
    MQTT_BUFFER_GET_4BIT( &frame->buffer, type, 4, 1 );
    wiced_assert("[MQTT] CONNACT type mismatch.", type == MQTT_PACKET_TYPE_PUBLISH );
    MQTT_BUFFER_GET_VARIABLE_LENGTH( &frame->buffer, size );
    MQTT_BUFFER_GET_STRING( &frame->buffer, args->topic.str, args->topic.len );
    if ( args->qos != MQTT_QOS_DELIVER_AT_MOST_ONCE )
    {
        MQTT_BUFFER_GET_SHORT( &frame->buffer, args->packet_id );
    }
    args->data = frame->buffer.data;
    args->data_len = size - ( args->qos != MQTT_QOS_DELIVER_AT_MOST_ONCE ? 2 : 0) - args->topic.len - 2;
    return WICED_SUCCESS;
}

wiced_result_t mqtt_frame_put_puback( mqtt_frame_t *frame, const mqtt_puback_arg_t *args )
{
    /* A long value carrying string MQTT */
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, 0x2, 0, 0 );    /* Reserved */
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, MQTT_PACKET_TYPE_PUBACK, 4, 1 );
    MQTT_BUFFER_PUT_OCTET( &frame->buffer, 2 ); /* Size */
    MQTT_BUFFER_PUT_SHORT( &frame->buffer, args->packet_id );
    return WICED_SUCCESS;
}

wiced_result_t mqtt_frame_get_puback( mqtt_frame_t *frame, mqtt_puback_arg_t *args )
{
    uint8_t type;
    uint8_t size;

    (void)size;
    (void)type;
    /* A long value carrying string MQTT */
    MQTT_BUFFER_GET_OCTET( &frame->buffer, type );    /* Type-Reserved */
    type = ( type & 0xF0 ) >> 4;
    wiced_assert("[MQTT] CONNACT type mismatch.", type == MQTT_PACKET_TYPE_PUBACK );
    MQTT_BUFFER_GET_OCTET( &frame->buffer, size );
    wiced_assert("[MQTT] CONNACT size mismatch.", size == 2 );
    MQTT_BUFFER_GET_SHORT( &frame->buffer, args->packet_id );
    return WICED_SUCCESS;
}

wiced_result_t mqtt_frame_put_pubrec( mqtt_frame_t *frame, const mqtt_pubrec_arg_t *args )
{
    /* A long value carrying string MQTT */
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, 0x2, 0, 0 );    /* Reserved */
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, MQTT_PACKET_TYPE_PUBREC, 4, 1 );
    MQTT_BUFFER_PUT_OCTET( &frame->buffer, 2 ); /* Size */
    MQTT_BUFFER_PUT_SHORT( &frame->buffer, args->packet_id );
    return WICED_SUCCESS;
}

wiced_result_t mqtt_frame_get_pubrec( mqtt_frame_t *frame, mqtt_pubrec_arg_t *args )
{
    uint8_t type;
    uint8_t size;

    (void)size;
    (void)type;
    /* A long value carrying string MQTT */
    MQTT_BUFFER_GET_OCTET( &frame->buffer, type );    /* Type-Reserved */
    type = ( type & 0xF0 ) >> 4;
    wiced_assert("[MQTT] CONNACT type mismatch.", type == MQTT_PACKET_TYPE_PUBREC );
    MQTT_BUFFER_GET_OCTET( &frame->buffer, size );
    wiced_assert("[MQTT] CONNACT size mismatch.", size == 2 );
    MQTT_BUFFER_GET_SHORT( &frame->buffer, args->packet_id );
    return WICED_SUCCESS;
}

wiced_result_t mqtt_frame_put_pubrel( mqtt_frame_t *frame, const mqtt_pubrel_arg_t *args )
{
    /* A long value carrying string MQTT */
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, 0x2, 0, 0 );    /* Reserved */
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, MQTT_PACKET_TYPE_PUBREL, 4, 1 );
    MQTT_BUFFER_PUT_OCTET( &frame->buffer, 2 ); /* Size */
    MQTT_BUFFER_PUT_SHORT( &frame->buffer, args->packet_id );
    return WICED_SUCCESS;
}

wiced_result_t mqtt_frame_get_pubrel( mqtt_frame_t *frame, mqtt_pubrel_arg_t *args )
{
    uint8_t type;
    uint8_t size;

    (void)size;
    (void)type;
    /* A long value carrying string MQTT */
    MQTT_BUFFER_GET_OCTET( &frame->buffer, type );    /* Type-Reserved */
    type = ( type & 0xF0 ) >> 4;
    wiced_assert("[MQTT] CONNACT type mismatch.", type == MQTT_PACKET_TYPE_PUBREL );
    MQTT_BUFFER_GET_OCTET( &frame->buffer, size );
    wiced_assert("[MQTT] CONNACT size mismatch.", size == 2 );
    MQTT_BUFFER_GET_SHORT( &frame->buffer, args->packet_id );
    return WICED_SUCCESS;
}

wiced_result_t mqtt_frame_put_pubcomp( mqtt_frame_t *frame, const mqtt_pubcomp_arg_t *args )
{
    /* A long value carrying string MQTT */
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, 0x2, 0, 0 );    /* Reserved */
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, MQTT_PACKET_TYPE_PUBCOMP, 4, 1 );
    MQTT_BUFFER_PUT_OCTET( &frame->buffer, 2 ); /* Size */
    MQTT_BUFFER_PUT_SHORT( &frame->buffer, args->packet_id );
    return WICED_SUCCESS;
}

wiced_result_t mqtt_frame_get_pubcomp( mqtt_frame_t *frame, mqtt_pubcomp_arg_t *args )
{
    uint8_t type;
    uint8_t size;

    (void)size;
    (void)type;
    /* A long value carrying string MQTT */
    MQTT_BUFFER_GET_OCTET( &frame->buffer, type );    /* Type-Reserved */
    type = ( type & 0xF0 ) >> 4;
    wiced_assert("[MQTT] CONNACT type mismatch.", type == MQTT_PACKET_TYPE_PUBCOMP );
    MQTT_BUFFER_GET_OCTET( &frame->buffer, size );
    wiced_assert("[MQTT] CONNACT size mismatch.", size == 2 );
    MQTT_BUFFER_GET_SHORT( &frame->buffer, args->packet_id );
    return WICED_SUCCESS;
}

wiced_result_t mqtt_frame_put_subscribe( mqtt_frame_t *frame, const mqtt_subscribe_arg_t *args )
{
    uint32_t    size = ( uint32_t ) 2                                                            /* Packet identifier  */
                       + ( uint32_t ) ( args->topic_filter.len + sizeof(args->topic_filter.len)) /* will topic if sent */
                       + ( uint32_t ) 1;                                                         /* QOS                */

    /* A long value carrying string MQTT */
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, 0x2, 0, 0 );    /* Reserved */
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, MQTT_PACKET_TYPE_SUBSCRIBE, 4, 1 );
    MQTT_BUFFER_PUT_VARIABLE_LENGTH( &frame->buffer, size, frame->size );
    MQTT_BUFFER_PUT_SHORT( &frame->buffer, args->packet_id );
    MQTT_BUFFER_PUT_STRING( &frame->buffer, args->topic_filter.str, args->topic_filter.len );
    MQTT_BUFFER_PUT_OCTET( &frame->buffer, args->qos );
    return WICED_SUCCESS;
}


wiced_result_t mqtt_frame_get_suback( mqtt_frame_t *frame, wiced_mqtt_suback_arg_t *args )
{
    uint8_t type;
    uint8_t size;

    (void)size;
    (void)type;
    /* A long value carrying string MQTT */
    MQTT_BUFFER_GET_OCTET( &frame->buffer, type );    /* Type-Reserved */
    type = ( type & 0xF0 ) >> 4;
    wiced_assert("[MQTT] CONNACT type mismatch.", type == MQTT_PACKET_TYPE_SUBACK );
    MQTT_BUFFER_GET_OCTET( &frame->buffer, size );
    wiced_assert("[MQTT] CONNACT size mismatch.", size == 3 );
    MQTT_BUFFER_GET_SHORT( &frame->buffer, args->packet_id );
    MQTT_BUFFER_GET_OCTET( &frame->buffer, args->return_code );
    return WICED_SUCCESS;
}

wiced_result_t mqtt_frame_put_unsubscribe( mqtt_frame_t *frame, const mqtt_unsubscribe_arg_t *args )
{
    uint32_t    size = ( uint32_t ) 2                                                            /* Packet identifier  */
                       + ( uint32_t ) ( args->topic_filter.len + sizeof(args->topic_filter.len)); /* will topic if sent */

    /* A long value carrying string MQTT */
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, 0x2, 0, 0 );    /* Reserved */
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, MQTT_PACKET_TYPE_UNSUBSCRIBE, 4, 1 );
    MQTT_BUFFER_PUT_VARIABLE_LENGTH( &frame->buffer, size, frame->size );
    MQTT_BUFFER_PUT_SHORT( &frame->buffer, args->packet_id );
    MQTT_BUFFER_PUT_STRING( &frame->buffer, args->topic_filter.str, args->topic_filter.len );
    return WICED_SUCCESS;
}

wiced_result_t mqtt_frame_get_unsuback( mqtt_frame_t *frame, mqtt_unsuback_arg_t *args )
{
    uint8_t type;
    uint8_t size;

    (void)size;
    (void)type;
    /* A long value carrying string MQTT */
    MQTT_BUFFER_GET_OCTET( &frame->buffer, type );    /* Type-Reserved */
    type = ( type & 0xF0 ) >> 4;
    wiced_assert("[MQTT] CONNACT type mismatch.", type == MQTT_PACKET_TYPE_UNSUBACK );
    MQTT_BUFFER_GET_OCTET( &frame->buffer, size );
    wiced_assert("[MQTT] CONNACT size mismatch.", size == 2 );
    MQTT_BUFFER_GET_SHORT( &frame->buffer, args->packet_id );
    return WICED_SUCCESS;
}

wiced_result_t mqtt_frame_put_disconnect( mqtt_frame_t *frame )
{
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, 0, 0, 0 );    /* Reserved */
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, MQTT_PACKET_TYPE_DISCONNECT, 4, 1 );
    MQTT_BUFFER_PUT_OCTET( &frame->buffer, 0 );         /* Size is 0 */
    return WICED_SUCCESS;
}

wiced_result_t mqtt_frame_put_pingreq( mqtt_frame_t *frame )
{
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, 0, 0, 0 );    /* Reserved */
    MQTT_BUFFER_PUT_4BIT( &frame->buffer, MQTT_PACKET_TYPE_PINGREQ, 4, 1 );
    MQTT_BUFFER_PUT_OCTET( &frame->buffer, 0 );         /* Size is 0 */
    return WICED_SUCCESS;
}
