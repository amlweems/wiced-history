/**
 * Copyright 2015, Broadcom Corporation
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
#include "wiced_bt_smartbridge.h"

#include "wiced_bt_gatt.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_cfg.h"

#include "bt_smart_gatt.h"
#include "bt_transport_thread.h"
#include "bt_smartbridge_socket_manager.h"
#include "bt_smartbridge_att_cache_manager.h"
#include "smartbridge_helper.h"
#include "smartbridge_stack_if.h"

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
extern gatt_subprocedure_t subprocedure;
extern wiced_bt_cfg_settings_t        wiced_bt_cfg_settings;
wiced_bt_smart_scan_complete_callback_t         app_scan_complete_callback;
wiced_bt_smart_advertising_report_callback_t    app_scan_report_callback;
extern volatile uint16_t current_service_handle_end;

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t smartbridge_bt_interface_discover_all_primary_services( uint16_t connection_handle, wiced_bt_smart_attribute_list_t* service_list )
{
    wiced_bt_gatt_discovery_param_t parameter;
    WPRINT_LIB_INFO( ( "[SmartBridge] Discover all Primary Services\n" ) );

    subprocedure_lock();

    subprocedure_reset();

    /* FIXME : reset() above is not taking effect. Need to be debugged */
    subprocedure.attr_count = 0;
    memset( &parameter.uuid, 0, sizeof( parameter.uuid ) );
    parameter.s_handle = 1;
    parameter.e_handle = 0xffff;

    subprocedure.end_handle = 0xffff;
    subprocedure.start_handle = 1;

    wiced_bt_gatt_send_discover( connection_handle, GATT_DISCOVER_SERVICES_ALL, &parameter );

    subprocedure_wait_for_completion();

    if ( subprocedure.result == WICED_BT_SUCCESS )
    {
        service_list->count = subprocedure.attr_count;
        service_list->list  = subprocedure.attr_head;
    }
    else
    {
        /* Clean up */
        wiced_bt_smart_attribute_list_t list;

        list.count = subprocedure.attr_count;
        list.list  = subprocedure.attr_head;

        wiced_bt_smart_attribute_delete_list( &list );
    }

    subprocedure_unlock();

    return subprocedure.result;
}

wiced_result_t smartbridge_bt_interface_discover_all_characteristics_in_a_service( uint16_t connection_handle, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* characteristic_list )
{
    wiced_bt_gatt_discovery_param_t parameter;

    subprocedure_lock();
    WPRINT_LIB_INFO( ( "[SmartBridge] Discover Characteristics by Service[%x %x]\n",start_handle, end_handle ) );

    subprocedure_reset();
    subprocedure.attr_count = 0;
    WPRINT_LIB_INFO( ( "[SmartBridge] New count:%d\n", (int)subprocedure.attr_count ) );

    memset( &parameter.uuid, 0, sizeof( parameter.uuid ) );
    parameter.s_handle = start_handle;
    parameter.e_handle = end_handle;

    subprocedure.end_handle = end_handle;
    subprocedure.start_handle = start_handle;

    current_service_handle_end = end_handle;

    wiced_bt_gatt_send_discover( connection_handle, GATT_DISCOVER_CHARACTERISTICS, &parameter );

    subprocedure_wait_for_completion();


    if ( subprocedure.result == WICED_BT_SUCCESS )
    {
        characteristic_list->count = subprocedure.attr_count;
        characteristic_list->list  = subprocedure.attr_head;
        WPRINT_LIB_INFO( ( "[SmartBridge] Waiting completed count:%d\n", (int)subprocedure.attr_count ) );
    }
    else
    {
        /* Clean up */
        wiced_bt_smart_attribute_list_t list;

        list.count = subprocedure.attr_count;
        list.list  = subprocedure.attr_head;

        wiced_bt_smart_attribute_delete_list( &list );
    }

    subprocedure_unlock();

    return subprocedure.result;
}

wiced_result_t smartbridge_bt_interface_discover_characteristic_descriptors(uint16_t connection_handle, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* no_value_descriptor_list )
{
    wiced_bt_gatt_discovery_param_t parameter;

    subprocedure_lock();
    subprocedure_reset();

    memset( &parameter.uuid, 0, sizeof( parameter.uuid ) );
    parameter.s_handle = start_handle;
    parameter.e_handle = end_handle;

    subprocedure.start_handle      = start_handle;
    subprocedure.end_handle        = end_handle;
    subprocedure.connection_handle = connection_handle;
    subprocedure.attr_count = 0;

    WPRINT_LIB_INFO( ( "[GATT] Discover all Characteristic Descriptors\n" ) );

    wiced_bt_gatt_send_discover( connection_handle, GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS, &parameter );

    subprocedure_wait_for_completion();

    if ( subprocedure.result == WICED_BT_SUCCESS )
    {
        no_value_descriptor_list->count = subprocedure.attr_count;
        no_value_descriptor_list->list  = subprocedure.attr_head;
    }
    else
    {
        /* Clean up */
        wiced_bt_smart_attribute_list_t list;

        list.count = subprocedure.attr_count;
        list.list  = subprocedure.attr_head;

        wiced_bt_smart_attribute_delete_list( &list );
    }

    subprocedure_unlock();

    return subprocedure.result;
}

wiced_result_t smartbridge_bt_interface_read_characteristic_value( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* type, wiced_bt_smart_attribute_t** characteristic_value )
{

    wiced_bt_gatt_read_param_t parameter;
    WPRINT_LIB_INFO( ( "[GATT] Read Characteristic Value\n" ) );


    subprocedure_lock();

    parameter.by_handle.auth_req = GATT_AUTH_REQ_NONE;
    parameter.by_handle.handle   = handle;
    subprocedure_reset();

    subprocedure.attr_count     = 0;
    subprocedure.start_handle   = handle;
    subprocedure.uuid.len       = type->len;
    //memcpy(&(subprocedure.uuid), type, sizeof(wiced_bt_uuid_t));
    if( type->len == UUID_16BIT )
    {
        subprocedure.uuid.uu.uuid16 = type->uu.uuid16;
    }
    else if( type->len == UUID_128BIT )
    {
        memcpy( subprocedure.uuid.uu.uuid128, type->uu.uuid128,  UUID_128BIT );
    }

    subprocedure.connection_handle = connection_handle;

    wiced_bt_gatt_send_read( connection_handle, GATT_READ_BY_HANDLE, &parameter );

    subprocedure_wait_for_completion();

    if ( subprocedure.result == WICED_BT_SUCCESS )
    {
        *characteristic_value = subprocedure.attr_head;
    }
    else
    {
        /* Clean up */
        wiced_bt_smart_attribute_delete( subprocedure.attr_head );
    }

    subprocedure_unlock();

    return subprocedure.result;
}

wiced_result_t smartbridge_bt_interface_write_characteristic_value( uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute )
{
    uint8_t                buffer[100] = { 0 };
    wiced_bt_gatt_value_t* write_value = (wiced_bt_gatt_value_t*)buffer;

    WPRINT_LIB_INFO(("[GATT] Write Characteristic value\n"));

    subprocedure_lock();

    subprocedure_reset();

    write_value->auth_req = GATT_AUTH_REQ_NONE;
    write_value->handle   = attribute->handle;
    write_value->len      = attribute->value_length;
    write_value->offset   = 0;

    memcpy( write_value->value, attribute->value.value, attribute->value_length );

    wiced_bt_gatt_send_write( connection_handle, GATT_WRITE, write_value );

    subprocedure_wait_for_completion();

    subprocedure_unlock();

    return subprocedure.result;
}

static void smartbridge_scan_result_callback( wiced_bt_ble_scan_results_t *p_scan_result, uint8_t *p_adv_data )
{
    if( p_scan_result )
    {
        wiced_bt_smart_advertising_report_t advertising_report;
        wiced_bt_smart_scan_result_t*       current_scan_result = NULL;

        uint8_t         adv_data_length;
        char            adv_data[31*2] = {0};
        uint8_t         *p = p_adv_data;
        char            adv_type_string[2] = { 0 };
        uint8_t         adv_type;
        uint8_t         i;

        advertising_report.remote_device.address[0] = p_scan_result->remote_bd_addr[0];
        advertising_report.remote_device.address[1] = p_scan_result->remote_bd_addr[1];
        advertising_report.remote_device.address[2] = p_scan_result->remote_bd_addr[2];
        advertising_report.remote_device.address[3] = p_scan_result->remote_bd_addr[3];
        advertising_report.remote_device.address[4] = p_scan_result->remote_bd_addr[4];
        advertising_report.remote_device.address[5] = p_scan_result->remote_bd_addr[5];


        advertising_report.remote_device.address_type = p_scan_result->ble_addr_type;
        advertising_report.signal_strength            = p_scan_result->rssi;
        advertising_report.event                      = p_scan_result->ble_evt_type;

        WPRINT_LIB_INFO(("address:%x %x %x %x %x %x rssi:%d bdaddrtype:%d event_type:%x\n",
                    p_scan_result->remote_bd_addr[0], p_scan_result->remote_bd_addr[1], p_scan_result->remote_bd_addr[2],
                    p_scan_result->remote_bd_addr[3], p_scan_result->remote_bd_addr[4], p_scan_result->remote_bd_addr[5],
                    p_scan_result->rssi, p_scan_result->ble_addr_type, p_scan_result->ble_evt_type));

        if(p_scan_result->ble_evt_type > 4)
            return;

        if ( smartbridge_helper_find_device_in_scan_result_list( &advertising_report.remote_device.address, advertising_report.remote_device.address_type, &current_scan_result ) == WICED_BT_ITEM_NOT_IN_LIST )
        {
            /* This is a new result. Create new result object and add to the list */
            current_scan_result = (wiced_bt_smart_scan_result_t*)malloc_named( "scanres", sizeof( *current_scan_result ) );
            if( !current_scan_result )
            {
                WPRINT_LIB_ERROR(("[SmartBridge] Failed to alloc memory for scan-list\n"));
                return;
            }

            memset( current_scan_result, 0, sizeof( *current_scan_result ) );

            current_scan_result->remote_device.address[0] = p_scan_result->remote_bd_addr[0];
            current_scan_result->remote_device.address[1] = p_scan_result->remote_bd_addr[1];
            current_scan_result->remote_device.address[2] = p_scan_result->remote_bd_addr[2];
            current_scan_result->remote_device.address[3] = p_scan_result->remote_bd_addr[3];
            current_scan_result->remote_device.address[4] = p_scan_result->remote_bd_addr[4];
            current_scan_result->remote_device.address[5] = p_scan_result->remote_bd_addr[5];

            current_scan_result->remote_device.address_type = p_scan_result->ble_addr_type;
            current_scan_result->signal_strength            = p_scan_result->rssi;

            smartbridge_helper_add_scan_result_to_list( current_scan_result );
        }

        STREAM_TO_UINT8( adv_data_length, p );
        while( adv_data_length && (p - p_adv_data <= 31 ) )
        {
            memset( adv_data, 0, sizeof( adv_data ) );

            STREAM_TO_UINT8( adv_type, p );
            unsigned_to_hex_string( (uint32_t)adv_type, adv_type_string, 1, 2 );

            if( adv_type == 0x09 || adv_type == 0x08 )
            {
                uint8_t j;
                for( i = 0, j=0; i < ( adv_data_length-1 ) && j <= 31; i++, j++ )
                {
                    advertising_report.remote_device.name[j] = p[i];
                    current_scan_result->remote_device.name[j] = p[i];
                }
                WPRINT_LIB_INFO(("[SmartBridge] Name of Device: %s\n", advertising_report.remote_device.name));
            }

            for( i = 0; i < ( adv_data_length-1 ); i++ )
            {
                unsigned_to_hex_string( (uint32_t)p[i], &adv_data[i*2], 2, 2 );
            }

            p += adv_data_length - 1; /* skip the length of data */
            STREAM_TO_UINT8( adv_data_length, p );
        }

        if( p_scan_result->ble_evt_type == BT_SMART_SCAN_RESPONSE_EVENT )
        {
            WPRINT_LIB_INFO( ( "[SmartBridge] Received SCAN_RSP\n" ) );
            memcpy( &current_scan_result->last_scan_response_received, &advertising_report, sizeof(wiced_bt_smart_advertising_report_t) );
        }
        else
        {
            WPRINT_LIB_INFO( ( "[SMartBridge] Received ADV[%lu]\n", (uint32_t)p_scan_result->ble_evt_type ) );
            memcpy( &current_scan_result->last_advertising_event_received, &advertising_report, sizeof(wiced_bt_smart_advertising_report_t) );
        }

        if( app_scan_report_callback && advertising_report.event == BT_SMART_CONNECTABLE_DIRECTED_ADVERTISING_EVENT )
        {
            WPRINT_LIB_INFO( ("[SmartBridge] advertising callback reported\n") );
            app_scan_report_callback(&advertising_report);
        }
    }

    else
    {
        WPRINT_LIB_INFO(( "[SmartBridge] LE scan completed.\n" ));
        if( app_scan_complete_callback )
        {
            app_scan_complete_callback();
        }
    }
}

wiced_result_t smartbridge_bt_interface_set_attribute_timeout( uint32_t timeout_seconds )
{
    UNUSED_PARAMETER(timeout_seconds);
    return WICED_BT_UNSUPPORTED;
}

wiced_result_t smartbridge_bt_interface_add_device_to_whitelist( const wiced_bt_device_address_t* device_address, wiced_bt_smart_address_type_t address_type )
{
    UNUSED_PARAMETER(address_type);
    return wiced_bt_ble_update_advertising_white_list( WICED_TRUE, *(wiced_bt_device_address_t* )device_address );
}

wiced_result_t smartbridge_bt_interface_remove_device_from_whitelist( const wiced_bt_device_address_t* device_address, wiced_bt_smart_address_type_t address_type )
{
    UNUSED_PARAMETER(address_type);
    return wiced_bt_ble_update_advertising_white_list( WICED_FALSE, *(wiced_bt_device_address_t* )device_address );
}

wiced_result_t smartbridge_bt_interface_get_whitelist_size( uint32_t *size )
{
    UNUSED_PARAMETER(size);
    return WICED_BT_UNSUPPORTED;
}

wiced_result_t smartbridge_bt_interface_clear_whitelist( void )
{
    return WICED_BT_UNSUPPORTED;
}

wiced_result_t smartbridge_bt_interface_cancel_last_connect( wiced_bt_device_address_t address )
{
    return wiced_bt_gatt_cancel_connect( address, WICED_TRUE );
}

wiced_result_t smartbridge_bt_interface_set_connection_tx_power( uint16_t connection_handle, int8_t transmit_power_dbm )
{
    return WICED_BT_UNSUPPORTED;
}

wiced_bool_t smartbridge_bt_interface_is_scanning( void )
{
    wiced_bt_ble_scan_type_t  scan_type;

    scan_type = wiced_bt_ble_get_current_scan_state();

    if ( scan_type != BTM_BLE_SCAN_TYPE_NONE )
    {
        return WICED_TRUE;
    }

    return WICED_FALSE;
}

wiced_result_t smartbridge_bt_interface_set_max_concurrent_connections( uint8_t count )
{
    /* Just update the Stack's configuration settings */
    wiced_bt_cfg_settings.max_simultaneous_links = count;
    return WICED_BT_SUCCESS;
}


wiced_result_t smartbridge_bt_interface_stop_scan( )
{
    return wiced_bt_ble_scan( BTM_BLE_SCAN_TYPE_NONE, WICED_TRUE, smartbridge_scan_result_callback );
}

wiced_result_t smartbridge_bt_interface_start_scan( const wiced_bt_smart_scan_settings_t* settings, wiced_bt_smart_scan_complete_callback_t complete_callback, wiced_bt_smart_advertising_report_callback_t advertising_report_callback )
{
    wiced_bool_t duplicate_filter_enabled = WICED_FALSE;

    /* First delete the previous scan result list */
    smartbridge_helper_delete_scan_result_list();

    /* fill with the settings provided by the smartbridge-application */
    wiced_bt_cfg_settings.ble_scan_cfg.scan_mode = settings->type;
    wiced_bt_cfg_settings.ble_scan_cfg.high_duty_scan_window = settings->window;
    wiced_bt_cfg_settings.ble_scan_cfg.high_duty_scan_duration = settings->duration_second;
    wiced_bt_cfg_settings.ble_scan_cfg.high_duty_scan_interval = settings->interval;
    duplicate_filter_enabled = settings->filter_duplicates;

    return wiced_bt_ble_scan( BTM_BLE_SCAN_TYPE_HIGH_DUTY, duplicate_filter_enabled, smartbridge_scan_result_callback );
}

wiced_result_t smartbridge_bt_interface_connect( const wiced_bt_smart_device_t* remote_device, const wiced_bt_smart_connection_settings_t* settings, wiced_bt_smartbridge_disconnection_callback_t disconnection_callback, wiced_bt_smartbridge_notification_callback_t notification_callback )
{
    wiced_bool_t gatt_connect_result;

    UNUSED_PARAMETER(disconnection_callback);
    UNUSED_PARAMETER(notification_callback);

    /* Update the Stack's configuration */
    wiced_bt_cfg_settings.ble_scan_cfg.conn_min_interval = settings->interval_min;
    wiced_bt_cfg_settings.ble_scan_cfg.conn_max_interval = settings->interval_max;
    wiced_bt_cfg_settings.ble_scan_cfg.conn_latency = settings->latency;
    wiced_bt_cfg_settings.ble_scan_cfg.conn_supervision_timeout = settings->supervision_timeout;

    /* Send connection request */
    gatt_connect_result = wiced_bt_gatt_le_connect( (uint8_t *)remote_device->address, remote_device->address_type, BLE_CONN_MODE_HIGH_DUTY, WICED_TRUE);

    WPRINT_LIB_INFO(("[Smartbridge] LE-connect, result:%d\n", gatt_connect_result));

    return gatt_connect_result;
}

wiced_result_t smartbridge_bt_interface_disconnect( uint16_t connection_handle )
{
    /* First delete the previous scan result list */
    smartbridge_helper_delete_scan_result_list();

    return wiced_bt_gatt_disconnect( connection_handle );
}
