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

#include "wiced_bt_dev.h"
#include "http_server.h"
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

#define INVALID_PIN 0xFFFF

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    REST_SMART_PAIRING_DISPLAY_ONLY     = BTM_IO_CAPABILITIES_DISPLAY_ONLY,                     /**< Display Only           */
    REST_SMART_PAIRING_DISPLAY_YES_NO   = BTM_IO_CAPABILITIES_DISPLAY_AND_YES_NO_INPUT,         /**< Display Yes/No         */
    REST_SMART_PAIRING_KEYBOARD_ONLY    = BTM_IO_CAPABILITIES_KEYBOARD_ONLY,                    /**< Keyboard Only          */
    REST_SMART_PAIRING_NO_IO            = BTM_IO_CAPABILITIES_NONE,                             /**< No Input, No Output    */
    REST_SMART_PAIRING_KEYBOARD_DISPLAY = BTM_IO_CAPABILITIES_BLE_DISPLAY_AND_KEYBOARD_INPUT,   /**< Keyboard display       */
    REST_SMART_PAIRING_LEGACY_OOB,                                                              /**< Legacy OOB             */
    REST_SMART_PAIRING_SECURE_CONNECTIONS_OOB                                                   /**< Secure Connections OOB */
} rest_smart_pairing_type_t;

/**
 * Pairing status
 */
typedef enum
{
    PAIRING_FAILED,
    PAIRING_SUCCESSFUL,
    PAIRING_ABORTED,
    LE_LEGACY_OOB_EXPECTED,
    LE_SECURE_OOB_EXPECTED,
    PASSKEY_INPUT_EXPECTED,
    PASSKEY_DISPLAY_EXPECTED,
    NUMERIC_COMPARISON_EXPECTED,
} rest_smart_pairing_status_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef wiced_bt_ble_address_t smart_node_handle_t;

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    uint16_t start_handle;
    uint16_t end_handle;
} smart_service_handle_t;

typedef struct
{
    uint16_t start_handle;
    uint16_t end_handle;
    uint16_t value_handle;
} smart_characteristic_handle_t;

typedef struct
{
    uint32_t length;
    uint8_t  value[1];
} smart_value_handle_t;

typedef struct
{
    wiced_bool_t              pairing_id_available;
    uint8_t                   pairing_id[16];
    wiced_bool_t              tk_available;
    uint8_t                   tk[16];
    wiced_bool_t              passkey_available;
    uint32_t                  passkey;
    wiced_bool_t              bdaddrb_available;
    wiced_bt_device_address_t bdaddrb;
    wiced_bool_t              rb_available;
    uint8_t                   rb[16];
    wiced_bool_t              cb_available;
    uint8_t                   cb[16];
    wiced_bool_t              confirmed_available;
    wiced_bool_t              confirmed;
} rest_smart_client_pairing_response_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/* GATT Discover API */
wiced_result_t restful_smart_discover_all_primary_services              ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node );
wiced_result_t restful_smart_discover_primary_services_by_uuid          ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const wiced_bt_uuid_t* uuid );
wiced_result_t restful_smart_discover_characteristics_of_a_service      ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const smart_service_handle_t* service );
wiced_result_t restful_smart_discover_characteristics_by_uuid           ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const wiced_bt_uuid_t* uuid );
wiced_result_t restful_smart_discover_characteristic_descriptors        ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic );

/* GATT Read API */
wiced_result_t restful_smart_read_characteristic_value                  ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic );
wiced_result_t restful_smart_read_characteristic_values_by_uuid         ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic, const wiced_bt_uuid_t* uuid );
wiced_result_t restful_smart_read_descriptor_value                      ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, uint16_t descriptor_handle );

/* GATT Write API */
wiced_result_t restful_smart_write_characteristic_value                 ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic, const smart_value_handle_t* value );
wiced_result_t restful_smart_write_characteristic_value_without_response( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic, const smart_value_handle_t* value );
wiced_result_t restful_smart_write_descriptor_value                     ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, uint16_t descriptor_handle, const smart_value_handle_t* value );

/* Notification API */
wiced_result_t restful_smart_read_cached_value                          ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic );
wiced_result_t restful_smart_enable_notification                        ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic );
wiced_result_t restful_smart_disable_notification                       ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic );
wiced_result_t restful_smart_enable_indication                          ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic );
wiced_result_t restful_smart_disable_indication                         ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic );
wiced_result_t restful_smart_subscribe_notification                     ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic );
wiced_result_t restful_smart_unsubscribe_notification                   ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic );
wiced_result_t restful_smart_subscribe_indication                       ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic );
wiced_result_t restful_smart_unsubscribe_indication                     ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic );

/* GAP API */
wiced_result_t restful_smart_connect                                    ( wiced_http_response_stream_t* stream, const smart_node_handle_t* node );
wiced_result_t restful_smart_passive_scan                               ( wiced_http_response_stream_t* stream );
wiced_result_t restful_smart_active_scan                                ( wiced_http_response_stream_t* stream );

/* Pairing API */
wiced_result_t restful_smart_start_pairing                              ( wiced_http_response_stream_t* stream, smart_node_handle_t* node_handle, rest_smart_pairing_type_t pairing_type );
wiced_result_t restful_smart_process_client_pairing_response            ( wiced_http_response_stream_t* stream, smart_node_handle_t* node_handle, rest_smart_client_pairing_response_t* client_response );
wiced_result_t restful_smart_cancel_pairing                             ( wiced_http_response_stream_t* stream, smart_node_handle_t* node_handle );

#ifdef __cplusplus
} /* extern "C" */
#endif
