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
#include "dns.h"
#include "wiced_bt_dev.h"
#include "big_stack_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define HPS_HEADERS_CHARACTERISTIC_VALUE_LENGTH (512)
#define HPS_BODY_CHARACTERISTIC_VALUE_LENGTH    (512)
#define HPS_URI_CHARACTERISTIC_VALUE_LENGTH     (512)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    HPS_IDLE,
    HPS_BLE_CONNECTION_ESTABLISHED,
    HPS_TCP_CONNECTION_INITIATED,
    HPS_TCP_CONNECTION_ESTABLISHED,
    HPS_HTTP_REQUEST_SENT,
    HPS_HTTP_RESPONSE_RECEIVED,
} hps_connection_state_t;

typedef enum
{
    HOSTNAME_UNRESOLVED,
    HOSTNAME_LOOKUP_PENDING,
    HOSTNAME_LOOKUP_TIMEOUT,
    HOSTNAME_RESOLVED
} hps_connection_dns_lookup_state_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef void (*hps_server_ble_connect_callback_t)      ( uint8_t* bd_address_ptr, wiced_bool_t* is_connection_allowed );
typedef void (*hps_sever_https_certificate_callback_t) ( const uint8_t** ca_certificate_ptr, uint16_t* ca_certificate_length, const uint8_t** client_certificate_ptr, uint16_t* client_certificate_length );

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    hps_connection_state_t            state;
    hps_connection_dns_lookup_state_t dns_state;
    uint8_t                           bd_address[ BD_ADDR_LEN ];
    uint16_t                          connection_handle;
    wiced_ip_address_t                server_ip;
    wiced_tcp_socket_t                tcp_socket;
    wiced_packet_t*                   tcp_tx_packet;
    wiced_packet_t*                   tcp_rx_packet;
    uint16_t                          write_handle_in_progress;
    const char*                       http_method;
    wiced_timed_event_t               receive_timer;
    wiced_bool_t                      receive_timer_expiry;

    /* characteristic values storage */
    uint8_t                           headers_char_value[ HPS_HEADERS_CHARACTERISTIC_VALUE_LENGTH ];
    uint16_t                          headers_length;
    uint8_t                           body_char_value[ HPS_BODY_CHARACTERISTIC_VALUE_LENGTH ];
    uint16_t                          body_length;
    uint8_t                           uri_char_value[ HPS_URI_CHARACTERISTIC_VALUE_LENGTH ];
    uint16_t                          uri_length;
    uint8_t*                          hostname_end;
    uint8_t*                          hostname_start;
    wiced_bool_t                      hostname_found_in_uri;
    wiced_bool_t                      https_enabled;
    uint8_t                           https_security_char_value;
    uint8_t                           status_code_char_value[ 3 ];
    uint8_t                           security_char_value;
    uint8_t                           control_point_char_value;
    uint16_t                          client_char_config;
} hps_connection_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t hps_server_start                               ( hps_connection_t* connections_array, uint8_t connection_count, wiced_interface_t interface, big_peer_device_link_keys_callback_t keys_callback );
wiced_result_t hps_server_stop                                ( void );
wiced_result_t hps_server_register_ble_connection_callback    ( hps_server_ble_connect_callback_t callback );
wiced_result_t hps_server_register_https_certificate_callback ( hps_sever_https_certificate_callback_t callback );

#ifdef __cplusplus
} /* extern "C" */
#endif
