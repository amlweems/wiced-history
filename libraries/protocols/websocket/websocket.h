/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "wiced.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * NOTE:
 *
 * Current Limitations:
 *  - Only client websockets are implemented
 *  - This implementation can only support receiving single frames on packet boundaries
 */

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define SUB_PROTOCOL_STRING_LENGTH 10

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WEBSOCKET_UNINITIALISED = 0, /* The WebSocket in uninitialised */
    WEBSOCKET_INITIALISED,       /* The WebSocket in uninitialised */
    WEBSOCKET_CONNECTING,        /* The connection has not yet been established */
    WEBSOCKET_OPEN,              /* The WebSocket connection is established and communication is possible */
    WEBSOCKET_CLOSING,           /* The connection is going through the closing handshake */
    WEBSOCKET_CLOSED,            /* The connection has been closed or could not be opened */
    WEBSOCKET_NOT_REGISTERED
} wiced_websocket_state_t;

typedef enum
{
    WEBSOCKET_CONTINUATION_FRAME = 0,
    WEBSOCKET_TEXT_FRAME,
    WEBSOCKET_BINARY_FRAME,
    WEBSOCKET_RESERVED_3,
    WEBSOCKET_RESERVED_4,
    WEBSOCKET_RESERVED_5,
    WEBSOCKET_RESERVED_6,
    WEBSOCKET_RESERVED_7,
    WEBSOCKET_CONNECTION_CLOSE,
    WEBSOCKET_PING,
    WEBSOCKET_PONG,
    WEBSOCKET_RESERVED_B,
    WEBSOCKET_RESERVED_C,
    WEBSOCKET_RESERVED_D,
    WEBSOCKET_RESERVED_E,
    WEBSOCKET_RESERVED_F
} wiced_websocket_payload_type_t;

typedef enum
{
    WEBSOCKET_NO_ERROR                              = 0,
    WEBSOCKET_CLIENT_CONNECT_ERROR                  = 1,
    WEBSOCKET_NO_AVAILABLE_SOCKET                   = 2,
    WEBSOCKET_SERVER_HANDSHAKE_RESPONSE_INVALID     = 3,
    WEBSOCKET_CREATE_SOCKET_ERROR                   = 4,
    WEBSOCKET_FRAME_SEND_ERROR                      = 5,
    WEBSOCKET_HANDSHAKE_SEND_ERROR                  = 6,
    WEBSOCKET_PONG_SEND_ERROR                       = 7,
    WEBSOCKET_RECEIVE_ERROR                         = 8,
    WEBSOCKET_DNS_RESOLVE_ERROR                     = 9,
    WEBSOCKET_SUBPROTOCOL_NOT_SUPPORTED             = 10
} wiced_websocket_error_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef wiced_result_t (*wiced_websocket_callback_t)( void* websocket );

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_websocket_callback_t on_open;
    wiced_websocket_callback_t on_error;
    wiced_websocket_callback_t on_close;
    wiced_websocket_callback_t on_message;
} wiced_websocket_callbacks_t;

typedef struct
{
    wiced_bool_t                    final_frame;
    wiced_websocket_payload_type_t  payload_type;
    uint16_t                        payload_length;
    void*                           payload;
    uint16_t                        payload_buffer_size;
} wiced_websocket_frame_t;

typedef struct
{
    wiced_tcp_socket_t           socket;
    wiced_websocket_error_t      error_type;
    wiced_websocket_state_t      state;
    wiced_websocket_callbacks_t  callbacks;
    uint8_t*                     formatted_websocket_frame;
    uint16_t                     formatted_websocket_frame_length;
    char                         subprotocol[SUB_PROTOCOL_STRING_LENGTH];
} wiced_websocket_t;

typedef struct
{
    char* request_uri;
    char* host;
    char* origin;
    char* sec_websocket_protocol;
} wiced_websocket_handshake_fields_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/* @brief                          perform opening handshake on port 80 with server and establish a connection
 *
 * @param websocket                websocket identifier
 * @param websocket_header         http header information to be used in handshake
 *
 * @return                         WICED_SUCCESS if successful, or WICED_ERROR.
 *
 * @note                           For additional error information, check the wiced_websocket_error_t field
 *                                 of the  wiced_websocket_t structure
 */
wiced_result_t wiced_websocket_connect( wiced_websocket_t* websocket, const wiced_websocket_handshake_fields_t* websocket_header );

/* @brief                          perform opening handshake on port 443 with server and establish a connection
 *
 * @param websocket                websocket identifier
 * @param websocket_header         http header information to be used in handshake
 * @param tls_context              memory for storing tls_context
 *
 *
 * @return                         WICED_SUCCESS if successful, or WICED_ERROR.
 *
 * @note                           For additional error information, check the wiced_websocket_error_t field
 *                                 of the  wiced_websocket_t structure
 */
wiced_result_t wiced_websocket_secure_connect( wiced_websocket_t* websocket, const wiced_websocket_handshake_fields_t* websocket_header, wiced_tls_context_t*  tls_context );

/* @brief                           send data to websocket server
 *
 * @param websocket                 websocket to send on
 *
 *
 * @return                          WICED_SUCCESS if successful, or WICED_ERROR.
 */
wiced_result_t wiced_websocket_send ( wiced_websocket_t* websocket, wiced_websocket_frame_t* tx_frame );

/* @brief                           receive data from websocket server. This is a blocking call.
 *
 * @param websocket                 websocket to receive on
 * @param tx_frame                  pointer to the websocket frame to send
 *
 * @return                          WICED_SUCCESS if successful, or WICED_ERROR.
 */
wiced_result_t wiced_websocket_receive ( wiced_websocket_t* websocket, wiced_websocket_frame_t* rx_frame );

/* @brief                           close and clean up websocket, and send close message to websocket server
 *
 * @param websocket                 websocket to close
 * @param rx_frame                  pointer to the websocket frame to receive on
 *
 * @param optional_close_message    optional closing message to send server.
 *
 * @return                          WICED_SUCCESS if successful, or WICED_ERROR.
 */
wiced_result_t wiced_websocket_close ( wiced_websocket_t* websocket, const char* optional_close_message) ;

/* @brief                           Register the on_open, on_close, on_message and on_error callbacks
 *
 * @param websocket                 websocket on which to register the callbacks
 * @param on_open_callback          called on open websocket  connection
 * @param on_close_callback         called on close websocket connection
 * @param on_message_callback       called on websocket receive data
 * @param on_error_callback         called on websocket error
 *
 *
 * @return                          WICED_SUCCESS if successful, or WICED_ERROR.
 */
wiced_result_t wiced_websocket_register_callbacks ( wiced_websocket_t* websocket, wiced_websocket_callback_t on_open_callback, wiced_websocket_callback_t on_close_callback, wiced_websocket_callback_t on_message_callback, wiced_websocket_callback_t on_error ) ;

/* @brief                           Un-Register the on_open, on_close, on_message and on_error callbacks for a given websocket
 *
 * @param websocket                 websocket on which to unregister the callbacks
 *
 */
void wiced_websocket_unregister_callbacks ( wiced_websocket_t* websocket );

/* @brief                           Initialise the websocket transmit frame
 *
 * @param websocket                 tx frame we are initialising
 * @param wiced_bool_t final_frame  Indicates if this is the final frame of the message
 * @param payload_type              Type of payload (ping frame, binary frame, text frame etc).
 * @param payload_length            Length of payload being sent
 *
 *
 * @return                          WICED_SUCCESS if successful, or WICED_ERROR.
 */
wiced_result_t wiced_websocket_initialise_tx_frame( wiced_websocket_frame_t* tx_frame, wiced_bool_t final_frame, wiced_websocket_payload_type_t payload_type, uint16_t payload_length, void* payload_buffer, uint16_t payload_buffer_size );


/* @brief                           Initialise the websocket receive frame
 *
 * @param rx_frame                  rx frame we are initialising
 * @param payload_buffer            Pointer to buffer containing the payload
 * @param payload_buffer_size       Length of buffer containing payload
 *
 * @return                          WICED_SUCCESS if successful, or WICED_ERROR.
 */
wiced_result_t wiced_websocket_initialise_rx_frame( wiced_websocket_frame_t* rx_frame, void* payload_buffer, uint16_t payload_buffer_size );

/* @brief                           Initialise the websocket
 *
 * @param rx_frame                  websocket we are initialising
 *
 * @return                          WICED_SUCCESS if successful, or WICED_ERROR.
 */
void wiced_websocket_initialise( wiced_websocket_t* websocket );

/* @brief                           Un-initialise the websocket and free memory allocated
 *                                  in creating sending buffers
 *
 * @param rx_frame                  websocket we are un-initialising
 *
 */
void wiced_websocket_uninitialise( wiced_websocket_t* websocket );

#ifdef __cplusplus
} /* extern "C" */
#endif






