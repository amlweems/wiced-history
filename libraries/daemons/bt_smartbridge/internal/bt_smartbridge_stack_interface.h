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

/** @file
 *  Smartbridge's Interface Header with Bluetooth Stack
 */

#include "wiced_utilities.h"
#include "wiced_bt_smartbridge.h"
#include "wiced_bt_smart_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define ATT_DEFAULT_MTU           (23)
#define ATT_STANDARD_VALUE_LENGTH (ATT_DEFAULT_MTU - 3)
#define ATT_STANDARD_TIMEOUT      (500)

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
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t smartbridge_bt_interface_initialize( void );
wiced_result_t smartbridge_bt_interface_deinitialize( void );

wiced_result_t  smartbridge_bt_interface_stop_scan( void  );

wiced_result_t  smartbridge_bt_interface_start_scan( const wiced_bt_smart_scan_settings_t* setting, wiced_bt_smart_scan_complete_callback_t complete_callback, wiced_bt_smart_advertising_report_callback_t advertising_report_callback );
wiced_bool_t    smartbridge_bt_interface_is_scanning( void );

wiced_result_t  smartbridge_bt_interface_connect( const wiced_bt_smart_device_t* remote_device, const wiced_bt_smart_connection_settings_t* settings, wiced_bt_smartbridge_disconnection_callback_t disconnection_callback, wiced_bt_smartbridge_notification_callback_t notification_callback );

wiced_result_t  smartbridge_bt_interface_cancel_last_connect( wiced_bt_device_address_t address );

wiced_result_t  smartbridge_bt_interface_disconnect( uint16_t connection_handle );

wiced_result_t  smartbridge_bt_interface_add_device_to_whitelist( const wiced_bt_device_address_t* device_address, wiced_bt_smart_address_type_t address_type );

wiced_result_t  smartbridge_bt_interface_remove_device_from_whitelist( const wiced_bt_device_address_t* device_address, wiced_bt_smart_address_type_t address_type );

wiced_result_t  smartbridge_bt_interface_get_whitelist_size( uint32_t *size );

wiced_result_t  smartbridge_bt_interface_clear_whitelist( void );

wiced_result_t  smartbridge_bt_interface_set_attribute_timeout( uint32_t timeout_seconds );

wiced_result_t  smartbridge_bt_interface_set_connection_tx_power( uint16_t connection_handle, int8_t transmit_power_dbm );

wiced_result_t  smartbridge_bt_interface_set_max_concurrent_connections( uint8_t count );


wiced_result_t smartbridge_bt_interface_discover_all_primary_services( uint16_t connection_handle, wiced_bt_smart_attribute_list_t* service_list );
wiced_result_t smartbridge_bt_interface_discover_all_characteristics_in_a_service( uint16_t connection_handle, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* characteristic_list );
wiced_result_t smartbridge_bt_interface_discover_all_characteristic_descriptors( uint16_t connection_handle, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* no_value_descriptor_list );
wiced_result_t smartbridge_bt_interface_discover_primary_services_by_uuid( uint16_t connection_handle, const wiced_bt_uuid_t* uuid, wiced_bt_smart_attribute_list_t* service_list );
wiced_result_t smartbridge_bt_interface_find_included_services( uint16_t connection_handle, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* include_list );
wiced_result_t smartbridge_bt_interface_discover_characteristic_by_uuid( uint16_t connection_handle, const wiced_bt_uuid_t* uuid, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* characteristic_list );

wiced_result_t smartbridge_bt_interface_read_characteristic_value( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* type, wiced_bt_smart_attribute_t** characteristic_value );
wiced_result_t smartbridge_bt_interface_read_long_characteristic_value( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* type, wiced_bt_smart_attribute_t** characteristic_value );
wiced_result_t smartbridge_bt_interface_read_characteristic_descriptor( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* uuid, wiced_bt_smart_attribute_t** descriptor );
wiced_result_t smartbridge_bt_interface_read_long_characteristic_descriptor( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* uuid, wiced_bt_smart_attribute_t** descriptor );
wiced_result_t smartbridge_bt_interface_read_characteristic_values_using_uuid( uint16_t connection_handle, const wiced_bt_uuid_t* uuid, wiced_bt_smart_attribute_list_t* characteristic_value_list );

wiced_result_t smartbridge_bt_interface_write_characteristic_value( uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute );
wiced_result_t smartbridge_bt_interface_write_long_characteristic_value( uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute );
wiced_result_t smartbridge_bt_interface_write_long_characteristic_descriptor( uint16_t connection_handle, const wiced_bt_smart_attribute_t* descriptor );
wiced_result_t smartbridge_bt_interface_write_characteristic_descriptor(  uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute );

wiced_result_t smartbridge_bt_interface_enable_pairing( wiced_bt_device_address_t address, wiced_bt_smart_address_type_t type, const wiced_bt_smart_security_settings_t* settings,  const char* passkey );
wiced_result_t smartbridge_bt_interface_disable_pairing( void );
wiced_result_t smartbridge_bt_interface_start_encryption( wiced_bt_device_address_t* address );

#ifdef __cplusplus
} /* extern "C" */
#endif
