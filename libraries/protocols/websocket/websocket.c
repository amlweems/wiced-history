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
 *
 */
#include "websocket.h"
#include "websocket_handshake.h"
#include "wwd_debug.h"
#include "wiced_crypto.h"
#include "wiced_utilities.h"
#include "wwd_assert.h"
#include "wiced_tls.h"

/*  Websocket packet format
 *
 *   (MSB)
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *   +-+-+-+-+-------+-+-------------+-------------------------------+
 *   |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 *   |I|S|S|S|  (4)  |A|     (7)     |             (16/63)           |
 *   |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 *   | |1|2|3|       |K|             |                               |
 *   +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 *   |     Extended payload length continued, if payload len == 127  |
 *   + - - - - - - - - - - - - - - - +-------------------------------+
 *   |                               |         Masking-key           |
 *   +-------------------------------+ - - - - - - - - - - - - - - - +<---------
 *   :    Masking-key (cont.)        |         Payload Data          :
 *   +---------------------------------------------------------------+ AREA TO
 *   :                          Payload data                         : MASK
 *   +---------------------------------------------------------------+<---------
 */

/******************************************************
 *                      Macros
 ******************************************************/

#define IF_ERROR_RECORD_AND_EXIT( GENERIC_ERROR, WEBSOCKET_ERROR )            \
    {                                                                         \
        if( GENERIC_ERROR != WICED_SUCCESS )                                  \
        {                                                                     \
            websocket->error_type=WEBSOCKET_ERROR;                            \
            if( websocket->callbacks.on_error != NULL )                       \
            {                                                                 \
                websocket->callbacks.on_error(websocket);                     \
            }                                                                 \
            unlock_websocket_mutex();                                         \
            return GENERIC_ERROR;                                             \
        }                                                                     \
    }

/******************************************************
 *                    Constants
 ******************************************************/
#define MAXIMUM_FRAME_OVERHEAD  8
#define MASKING_KEY_BYTES       4
#define MAX_PAYLOAD_FIELD_BYTES 2
#define CONTROL_FIELD_BYTES     2
#define MAX_HEADER_SIZE         (MASKING_KEY_BYTES + MAX_PAYLOAD_FIELD_BYTES + CONTROL_FIELD_BYTES)
#define MAX_CONTROL_FRAME       125

/* Websocket frame control byte mask */
#define FIN_MASK                         0x80
#define RSV1_MASK                        0x40
#define RSV2_MASK                        0x20
#define RSV3_MASK                        0x10
#define OPCODE_MASK                      0x0F

/* Websocket frame payloag byte mask */
#define PAYLOAD_MASK                     0x80
#define PAYLOAD_LENGTH_1_BYTE_MASK       0x7F

#define RSV_ERR_SHUTDOWN_MESSAGE              "rsv bits/extension negotiation is not supported"
#define CLOSE_REQUEST_SHUTDOWN_MESSAGE        "close requested from Server"
#define LENGTH_SHUTDOWN_MESSAGE               "max Frame length supported is 1024"
#define PAYLOAD_TYPE_ERROR_SHUTDOWN_MESSAGE   "unsupported payload type"


/******************************************************
 *                   Enumerations
 ******************************************************/
typedef enum
{
    MUTEX_UNINITIALISED,
    MUTEX_LOCKED,
    MUTEX_UNLOCKED
}websocket_mutex_state_t;
/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef enum
{
    READ_CONTROL_BYTES,
    READ_LENGTH,
    READ_PAYLOAD,
    READ_COMPLETED_SUCCESSFULLY,
    READ_FRAME_ERROR
} wiced_websocket_read_frame_state_t;

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_mutex_t           handshake_lock;
    websocket_mutex_state_t mutex_state;
} wiced_websocket_mutex_t;

/******************************************************
 *               Static Function Declarations
 ******************************************************/

static wiced_result_t websocket_common_connect     ( wiced_websocket_t* websocket, const wiced_websocket_handshake_fields_t* websocket_header_fields, wiced_tls_context_t*  tls_context );
static wiced_result_t websocket_connect            ( wiced_websocket_t* websocket, wiced_ip_address_t* address );
static wiced_result_t websocket_tls_connect        ( wiced_websocket_t* websocket, wiced_ip_address_t* address, wiced_tls_context_t* tls_context );
static wiced_result_t mask_unmask_frame_data       ( uint8_t* data_in, uint8_t* data_out, uint16_t data_length, uint8_t* mask );
static wiced_result_t update_socket_state          ( wiced_websocket_t* websocket, wiced_websocket_state_t state);
static wiced_result_t on_websocket_close_callback  ( wiced_tcp_socket_t* socket, void* websocket );
static wiced_result_t on_websocket_message_callback( wiced_tcp_socket_t* socket, void* websocket );
static void           lock_websocket_mutex         ( void );
static void           unlock_websocket_mutex       ( void );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static wiced_websocket_mutex_t     websocket_mutex =
{
    .mutex_state = MUTEX_UNINITIALISED
};
/******************************************************
 *               Function Definitions
 ******************************************************/
void wiced_websocket_initialise( wiced_websocket_t* websocket )
{

    websocket->formatted_websocket_frame = NULL;
    websocket->formatted_websocket_frame_length = 0;
    websocket->state = WEBSOCKET_INITIALISED;
}

void wiced_websocket_uninitialise( wiced_websocket_t* websocket )
{
    websocket->state = WEBSOCKET_UNINITIALISED;

    if( websocket->formatted_websocket_frame != NULL )
    {
        free( websocket->formatted_websocket_frame );
    }
    websocket->formatted_websocket_frame = NULL;
}

wiced_result_t wiced_websocket_connect( wiced_websocket_t* websocket, const wiced_websocket_handshake_fields_t* websocket_header )
{

    if ( websocket_common_connect( websocket, websocket_header, NULL ) != WICED_SUCCESS )
    {
        wiced_websocket_close( websocket, NULL );
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_websocket_secure_connect( wiced_websocket_t* websocket, const wiced_websocket_handshake_fields_t* websocket_header, wiced_tls_context_t*  tls_context )
{
    if ( websocket_common_connect( websocket, websocket_header, tls_context ) != WICED_SUCCESS )
    {
        wiced_websocket_close( websocket, NULL );
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

static wiced_result_t websocket_common_connect( wiced_websocket_t* websocket, const wiced_websocket_handshake_fields_t* websocket_header_fields, wiced_tls_context_t*  tls_context )
{
    wiced_result_t     result = WICED_ERROR;
    wiced_ip_address_t address;

    if ( websocket != NULL )
    {
        /* as per rfc 6455 specification, there can not be more then one socket in a connecting state*/
        lock_websocket_mutex();

        /* perform DNS lookup*/
        result = wiced_hostname_lookup( websocket_header_fields->host, &address, 10000 );
        IF_ERROR_RECORD_AND_EXIT( result, WEBSOCKET_DNS_RESOLVE_ERROR );

        update_socket_state( websocket, WEBSOCKET_CONNECTING );

        /* Establish secure/non secure tcp connection to server */
        if ( tls_context != NULL )
        {
            result = websocket_tls_connect( websocket, &address, tls_context );
        }
        else
        {
            result = websocket_connect( websocket, &address );
        }
        IF_ERROR_RECORD_AND_EXIT( result, WEBSOCKET_CLIENT_CONNECT_ERROR );

        result = wiced_establish_websocket_handshake( websocket, websocket_header_fields );
        IF_ERROR_RECORD_AND_EXIT( result, WEBSOCKET_SERVER_HANDSHAKE_RESPONSE_INVALID );

        /* If a sub protocol was requested, extract it from the handshake */
        if ( websocket_header_fields->sec_websocket_protocol != NULL )
        {
            result = wiced_get_websocket_subprotocol( websocket->subprotocol );
            IF_ERROR_RECORD_AND_EXIT( result, WEBSOCKET_SUBPROTOCOL_NOT_SUPPORTED );
        }

        update_socket_state( websocket, WEBSOCKET_OPEN );
        if( websocket->callbacks.on_open != NULL )
        {
            websocket->callbacks.on_open( websocket );
        }

        /*Once the communication with server is established, register for the message callback and close callback */
        wiced_tcp_register_callbacks( &websocket->socket, NULL, on_websocket_message_callback, on_websocket_close_callback, websocket );

        unlock_websocket_mutex();
    }

    return result;
}

static wiced_result_t websocket_tls_connect( wiced_websocket_t* websocket, wiced_ip_address_t* address, wiced_tls_context_t*  tls_context )
{
    wiced_result_t result;

    wiced_tls_init_context( tls_context, NULL, NULL );

    WICED_VERIFY( wiced_tcp_create_socket( &websocket->socket, WICED_STA_INTERFACE ) );

    wiced_tcp_enable_tls( &websocket->socket, tls_context );

    result = wiced_tcp_connect( &websocket->socket, address, 443, 10000 );
    if ( result != WICED_SUCCESS )
    {
        websocket->error_type = WEBSOCKET_CLIENT_CONNECT_ERROR;

        wiced_tcp_delete_socket( &websocket->socket );
    }

    return result;
}

static wiced_result_t websocket_connect( wiced_websocket_t* websocket, wiced_ip_address_t* address )
{
    wiced_result_t result;

    WICED_VERIFY( wiced_tcp_create_socket( &websocket->socket, WICED_STA_INTERFACE ) );

    result = wiced_tcp_connect( &websocket->socket, address, 80, 10000 );
    if ( result != WICED_SUCCESS )
    {
        websocket->error_type = WEBSOCKET_CLIENT_CONNECT_ERROR;
        if( websocket->callbacks.on_error != NULL )
        {
            websocket->callbacks.on_error( websocket );
        }
        wiced_tcp_delete_socket( &websocket->socket );
        return result;
    }

    return result;
}

wiced_result_t wiced_websocket_send( wiced_websocket_t* websocket, wiced_websocket_frame_t* tx_frame )
{
    wiced_result_t result = WICED_ERROR;
    uint8_t additional_bytes_to_represent_length;
    uint8_t masking_key_byte_offset;
    uint16_t max_websocket_payload;

    max_websocket_payload = (uint16_t)(tx_frame->payload_buffer_size + MAXIMUM_FRAME_OVERHEAD);
    if( websocket->formatted_websocket_frame_length < max_websocket_payload )
    {
        if( websocket->formatted_websocket_frame != NULL )
    {
            free( websocket->formatted_websocket_frame );
            websocket->formatted_websocket_frame = NULL;
        }
        websocket->formatted_websocket_frame_length = max_websocket_payload;
        websocket->formatted_websocket_frame = calloc( websocket->formatted_websocket_frame_length, sizeof(uint8_t) );
    }

    /* Clear frame */
    memset( websocket->formatted_websocket_frame, 0, (int)websocket->formatted_websocket_frame_length );

    /* Set FIN, RSV and opcode fields of Byte 0 */
    websocket->formatted_websocket_frame[ 0 ] = (uint8_t) ( FIN_MASK | ( (int) tx_frame->payload_type & OPCODE_MASK ) );

    /* Before setting payload, need to find out if extended bits of frame are required */
    if ( tx_frame->payload_length < 126 )
    {
        /* In this case, payload length is represented with 1 byte, and bytes extended payload is not used*/
        websocket->formatted_websocket_frame[ 1 ] = (uint8_t) ( PAYLOAD_MASK | tx_frame->payload_length );

        additional_bytes_to_represent_length = 0;
    }
    else
    {
        /*
         * Check we can buffer the data for sending (needed due to masking). Also ensure its not a control frame,
         * as they have a maximum size limit of 125
         */
        if ( tx_frame->payload_type < WEBSOCKET_CONNECTION_CLOSE )
        {
            websocket->formatted_websocket_frame[ 1 ] = (uint8_t) ( PAYLOAD_MASK | 126 );
            websocket->formatted_websocket_frame[ 2 ] = (uint8_t) ( tx_frame->payload_length >> 8 );
            websocket->formatted_websocket_frame[ 3 ] = (uint8_t) ( tx_frame->payload_length );
            additional_bytes_to_represent_length = 2;
        }
        else /* payload > MAX_PAYLOAD */
        {
            /* unsupported frame size*/
            return WICED_UNSUPPORTED;
        }
    }

    /* Note: masking key starts after the payload. Two bytes of the control info of frame also need to be considered */
    masking_key_byte_offset = (uint8_t) ( additional_bytes_to_represent_length + CONTROL_FIELD_BYTES );

    /* Generate random mask to be used in masking the frame data*/
    wiced_crypto_get_random( &websocket->formatted_websocket_frame[ masking_key_byte_offset ], 4 );

    /* use algorithm of RFC6455, 5.3 to mask the payload only, not the header*/
    mask_unmask_frame_data( tx_frame->payload, &websocket->formatted_websocket_frame[ masking_key_byte_offset + MASKING_KEY_BYTES ], tx_frame->payload_length, &websocket->formatted_websocket_frame[ masking_key_byte_offset ] );

    /* Send frame to server */
    result = wiced_tcp_send_buffer( &websocket->socket, websocket->formatted_websocket_frame, (uint16_t) ( tx_frame->payload_length + masking_key_byte_offset + MASKING_KEY_BYTES ) );
    IF_ERROR_RECORD_AND_EXIT( result, WEBSOCKET_FRAME_SEND_ERROR );

    return result;
}

wiced_result_t wiced_websocket_receive( wiced_websocket_t* websocket, wiced_websocket_frame_t* rx_frame )
{
    wiced_result_t                      result                  = WICED_SUCCESS;
    uint16_t                            payload_length          = 0;
    wiced_websocket_read_frame_state_t  read_state              = READ_CONTROL_BYTES;
    uint32_t                            bytes_read              = 0;
    const char*                         connection_close_string = NULL;
    uint16_t                            total_received_bytes    = 0;
    wiced_packet_t*                     tcp_reply_packet;
    uint16_t                            tcp_data_available;
    uint8_t*                            received_data;
    wiced_websocket_frame_t             local_tx_frame;

    /*Clear rx buffer */
    memset( rx_frame->payload, 0x0, rx_frame->payload_buffer_size );
    rx_frame->payload_length = 0;

    /* Unpack websocket frame */
    while ( read_state != ( READ_COMPLETED_SUCCESSFULLY ) || ( read_state == READ_FRAME_ERROR ) )
    {
        result = wiced_tcp_receive( &websocket->socket, &tcp_reply_packet, WICED_NO_WAIT );
        if ( result != WICED_SUCCESS )
        {
            read_state = READ_FRAME_ERROR;
            break;
        }
        result = wiced_packet_get_data( tcp_reply_packet, 0, (uint8_t**) &received_data, &total_received_bytes, &tcp_data_available );
        if ( result != WICED_SUCCESS )
        {
            read_state = READ_FRAME_ERROR;
            break;
        }

        switch ( read_state )
        {
            case READ_CONTROL_BYTES:
                rx_frame->final_frame = ( received_data[ 0 ] & FIN_MASK ) ? WICED_TRUE : WICED_FALSE;

                /* read RSV bits. Note RSV and extension negotiations are not supported*/
                if ( received_data[ 0 ] & ( RSV1_MASK | RSV2_MASK | RSV3_MASK ) )
                {
                    connection_close_string = RSV_ERR_SHUTDOWN_MESSAGE;
                    read_state = READ_FRAME_ERROR;
                    break;
                }

                /* Read the payload type and pass to the application. Note any ping/pong or close action if required*/
                rx_frame->payload_type = (wiced_websocket_payload_type_t) ( received_data[ 0 ] & OPCODE_MASK );
                if ( ( ( rx_frame->payload_type >= WEBSOCKET_RESERVED_3 ) && ( rx_frame->payload_type <= WEBSOCKET_RESERVED_7 ) ) || ( ( rx_frame->payload_type >= WEBSOCKET_RESERVED_B ) ) )
                {
                    connection_close_string = PAYLOAD_TYPE_ERROR_SHUTDOWN_MESSAGE;
                    read_state = READ_FRAME_ERROR;
                    break;
                }
                read_state++;
                bytes_read++;
                /* Fall through */

            case READ_LENGTH:
                /* Extract the length of the payload*/
                payload_length = received_data[ 1 ] & PAYLOAD_LENGTH_1_BYTE_MASK;

                /* This indicates bytes 2 and 3 of frame are used to store actual payload length*/
                if ( payload_length == 126 )
                {
                    /* Extract Frame bytes 2 and 3*/
                    if ( total_received_bytes < ( CONTROL_FIELD_BYTES + MAX_PAYLOAD_FIELD_BYTES ) )
                    {
                        /* break out of switch and wait for get more data*/
                        break;
                    }
                    payload_length = (uint16_t) ( received_data[ 3 ] | ( received_data[ 2 ] << 8 ) );
                    if ( payload_length > rx_frame->payload_buffer_size )
                    {
                        connection_close_string = LENGTH_SHUTDOWN_MESSAGE;
                        read_state = READ_FRAME_ERROR;
                        break;
                    }
                    bytes_read += 3;
                }
                else if ( payload_length == 127 )
                {
                    /* unsupported payload length*/
                    connection_close_string = LENGTH_SHUTDOWN_MESSAGE;
                    read_state = READ_FRAME_ERROR;
                    break;
                }
                else
                {
                    /* payload length already extracted in second byte*/
                    bytes_read++;
                }
                read_state++;
                /* Fall through */

            case READ_PAYLOAD:
                /* Check if payload needs to be unmasked, and maksing key extracted along with data*/
                if ( ( received_data[ 1 ] & PAYLOAD_MASK ) != 0 )
                {
                    if ( ( total_received_bytes - bytes_read ) < (uint16_t) ( MASKING_KEY_BYTES + payload_length ) )
                    {
                        /* break out of switch and wait for get more data*/
                        break;
                    }
                    mask_unmask_frame_data( &received_data[ bytes_read + MASKING_KEY_BYTES ], rx_frame->payload, payload_length, &received_data[ bytes_read ] );
                }
                else if ( total_received_bytes - bytes_read < (uint16_t) payload_length )
                {
                    /* break out of switch and wait for get more data*/
                    break;
                }
                else
                {
                    if(  rx_frame->payload_buffer_size < payload_length )
                    {
                        memcpy( rx_frame->payload, &received_data[ bytes_read ], rx_frame->payload_buffer_size );
                        wiced_assert("RX Buffer is too small !", rx_frame->payload_buffer_size >= payload_length );
                    }
                    else
                    {
                        memcpy( rx_frame->payload, &received_data[ bytes_read ], payload_length );
                    }
                }

                /*Check if more frames are present in packet and restart state machine */
                read_state++;
                /* Fall through */

            case READ_COMPLETED_SUCCESSFULLY:
                break;

            case READ_FRAME_ERROR:
                /* fall through */

            default:
                read_state = READ_FRAME_ERROR;
                break;
        }

    }

    wiced_packet_delete( tcp_reply_packet );

    /* Check if unpack failed, close connection*/
    if ( read_state == READ_FRAME_ERROR )
    {
        wiced_websocket_close( websocket, connection_close_string );
        result = WICED_ERROR;
    }
    else if ( rx_frame->payload_type == WEBSOCKET_PING )
    {
        /* pong back with the data received*/
        wiced_websocket_initialise_tx_frame( &local_tx_frame, WICED_TRUE, WEBSOCKET_PONG, payload_length, rx_frame->payload, rx_frame->payload_buffer_size );
        wiced_websocket_send( websocket, &local_tx_frame );
    }
    else if ( rx_frame->payload_type == WEBSOCKET_CONNECTION_CLOSE )
    {
        /* Did I send a connection close string*/
        wiced_websocket_close( websocket, NULL );
    }

    rx_frame->payload_length = payload_length;

    return result;
}

wiced_result_t wiced_websocket_close( wiced_websocket_t* websocket, const char* optional_close_message )
{
    wiced_result_t          result = WICED_SUCCESS;
    wiced_websocket_frame_t local_tx_frame;

    if ( websocket->state != WEBSOCKET_CLOSED )
    {
        update_socket_state( websocket, WEBSOCKET_CLOSING );

        if ( optional_close_message != NULL )
        {
            wiced_websocket_initialise_tx_frame( &local_tx_frame, WICED_TRUE, WEBSOCKET_CONNECTION_CLOSE, (uint16_t)strlen( optional_close_message ), (char*) optional_close_message, (uint16_t)strlen( optional_close_message ) );
            wiced_websocket_send( websocket, &local_tx_frame );
        }

        if( websocket->formatted_websocket_frame != NULL )
        {
            free( websocket->formatted_websocket_frame );
            websocket->formatted_websocket_frame = NULL;
        }

        unlock_websocket_mutex();

        wiced_tcp_disconnect( &websocket->socket );

        update_socket_state( websocket, WEBSOCKET_CLOSED );

    }

    return result;
}

static wiced_result_t mask_unmask_frame_data( uint8_t* data_in, uint8_t* data_out, uint16_t data_length, uint8_t* mask )
{
    wiced_result_t result = WICED_SUCCESS;
    uint32_t       i;

    for ( i = 0; i < data_length; i++ )
    {
        data_out[ i ] = data_in[ i ] ^ mask[ i % 4 ];
    }

    return result;
}


static wiced_result_t update_socket_state( wiced_websocket_t* websocket, wiced_websocket_state_t state )
{
    websocket->state = state;
    return WICED_SUCCESS;
}

static wiced_result_t on_websocket_close_callback(  wiced_tcp_socket_t* socket, void* websocket )
{
    wiced_websocket_t* websocket_ptr = websocket;

    UNUSED_PARAMETER(socket);

    if( websocket_ptr->callbacks.on_close != NULL )
    {
        websocket_ptr->callbacks.on_close( websocket_ptr );
    }

    return WICED_SUCCESS;
}

static wiced_result_t on_websocket_message_callback( wiced_tcp_socket_t* socket, void* websocket )
{
    wiced_websocket_t* websocket_ptr = websocket;

    UNUSED_PARAMETER(socket);

    if( websocket_ptr->callbacks.on_message != NULL )
    {
        websocket_ptr->callbacks.on_message( websocket_ptr );
    }

    return WICED_SUCCESS;
}

static void lock_websocket_mutex( void )
{
    if ( websocket_mutex.mutex_state == MUTEX_UNINITIALISED )
    {
        /* create handhsake mutex. There can only be one connection in a connecting state*/
        wiced_rtos_init_mutex( &websocket_mutex.handshake_lock );
        websocket_mutex.mutex_state = MUTEX_UNLOCKED;
    }

    if ( websocket_mutex.mutex_state == MUTEX_UNLOCKED )
    {
        wiced_rtos_lock_mutex( &websocket_mutex.handshake_lock );
        websocket_mutex.mutex_state = MUTEX_LOCKED;
    }

}

static void unlock_websocket_mutex( void )
{
    if ( websocket_mutex.mutex_state == MUTEX_LOCKED )
    {
        wiced_rtos_unlock_mutex( &websocket_mutex.handshake_lock );
        websocket_mutex.mutex_state = MUTEX_UNLOCKED;
    }
}

wiced_result_t wiced_websocket_register_callbacks ( wiced_websocket_t* websocket, wiced_websocket_callback_t on_open_callback, wiced_websocket_callback_t on_close_callback, wiced_websocket_callback_t on_message_callback, wiced_websocket_callback_t on_error_callback )
{
    websocket->callbacks.on_open    = on_open_callback;
    websocket->callbacks.on_close   = on_close_callback;
    websocket->callbacks.on_error   = on_error_callback;
    websocket->callbacks.on_message = on_message_callback;

    return WICED_SUCCESS;
}

void wiced_websocket_unregister_callbacks ( wiced_websocket_t* websocket )
{
    websocket->callbacks.on_open    = NULL;
    websocket->callbacks.on_close   = NULL;
    websocket->callbacks.on_error   = NULL;
    websocket->callbacks.on_message = NULL;
}

wiced_result_t wiced_websocket_initialise_tx_frame( wiced_websocket_frame_t* tx_frame, wiced_bool_t final_frame, wiced_websocket_payload_type_t payload_type, uint16_t payload_length, void* payload_buffer, uint16_t payload_buffer_size )
{
    if( payload_buffer == NULL )
    {
        return WICED_BADARG;
    }

    tx_frame->final_frame         = final_frame;
    tx_frame->payload_type        = payload_type;
    tx_frame->payload_length      = payload_length;
    tx_frame->payload             = payload_buffer;
    tx_frame->payload_buffer_size = payload_buffer_size;

    return WICED_SUCCESS;
}

wiced_result_t wiced_websocket_initialise_rx_frame( wiced_websocket_frame_t* rx_frame, void* payload_buffer, uint16_t payload_buffer_size )
{
    if( payload_buffer == NULL )
    {
        return WICED_BADARG;
    }

    memset( payload_buffer, 0x0, payload_buffer_size );
    rx_frame->payload             = payload_buffer;
    rx_frame->payload_buffer_size = payload_buffer_size;

    return WICED_SUCCESS;
}
