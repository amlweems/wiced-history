/**
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
#include "wiced.h"
#include "wiced_bt_smartbridge.h"

#include "wiced_bt_gatt.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_cfg.h"

#include "bt_smartbridge_socket_manager.h"
#include "bt_smartbridge_att_cache_manager.h"
#include "bt_smartbridge_helper.h"
#include "bt_smartbridge_stack_interface.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define INTERNAL_SECURITY_LEVEL ( 1 ) /* Encryption enabled, no pairing requested because device is already paired */

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

extern gatt_subprocedure_t                   subprocedure;
extern wiced_bt_cfg_settings_t               wiced_bt_cfg_settings;
wiced_bt_smart_scan_complete_callback_t      app_scan_complete_callback;
wiced_bt_smart_advertising_report_callback_t app_scan_report_callback;

wiced_bt_dev_ble_io_caps_req_t  default_io_caps_ble  =
{
    .bd_addr      = { 0 },
    .local_io_cap = BTM_IO_CAPABILITIES_BLE_DISPLAY_AND_KEYBOARD_INPUT,
    .oob_data     = 0,
    .auth_req     = BTM_LE_AUTH_REQ_BOND|BTM_LE_AUTH_REQ_MITM, /* BTM_LE_AUTH_REQ_SC_MITM_BOND */
    .max_key_size = 16,
    .init_keys    = (BTM_LE_KEY_PENC|BTM_LE_KEY_PID|BTM_LE_KEY_PCSRK|BTM_LE_KEY_PLK), // init_keys - Keys to be distributed, bit mask
    .resp_keys    = (BTM_LE_KEY_PENC|BTM_LE_KEY_PID|BTM_LE_KEY_PCSRK|BTM_LE_KEY_PLK)  // resp_keys - Keys to be distributed, bit mask
};

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t smartbridge_bt_interface_initialize( void )
{
    wiced_result_t result;

    WPRINT_LIB_INFO( ( "[SmartBridge] Initializing Bluetooth Interface...\n" ) );

    result = wiced_rtos_init_mutex( &subprocedure.mutex );
    if ( result  != WICED_BT_SUCCESS )
    {
        WPRINT_LIB_INFO( ( "[SmartBridge] Error creating mutex\n" ) );
        return result;
    }

    result = wiced_rtos_init_semaphore( &subprocedure.done_semaphore );
    if ( result != WICED_BT_SUCCESS )
    {
        WPRINT_LIB_INFO( ( "[SmartBridge] Error creating semaphore\n" ) );
        return result;
    }
    subprocedure_reset();
    return WICED_BT_SUCCESS;
}

wiced_result_t smartbridge_bt_interface_deinitialize( void )
{
    WPRINT_LIB_INFO( ( "[SmartBridge] Deinitializing Bluetooth Interface...\n" ) );

    wiced_rtos_deinit_mutex( &subprocedure.mutex );
    wiced_rtos_deinit_semaphore( &subprocedure.done_semaphore );
    subprocedure_reset();

    return WICED_BT_SUCCESS;
}

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

wiced_result_t smartbridge_bt_interface_discover_primary_services_by_uuid( uint16_t connection_handle, const wiced_bt_uuid_t* uuid, wiced_bt_smart_attribute_list_t* service_list )
{
    wiced_bt_gatt_discovery_param_t parameter;
    WPRINT_LIB_INFO( ( "[SmartBridge] Discover all Primary Services(By-UUID)\n" ) );

    subprocedure_lock();

    subprocedure_reset();

    /* FIXME : reset() above is not taking effect. Need to be debugged */
    subprocedure.attr_count = 0;
    memcpy( &parameter.uuid, uuid, sizeof( parameter.uuid ) );

    parameter.s_handle = subprocedure.end_handle = 0x0001;
    parameter.e_handle = subprocedure.start_handle = 0xffff;

    wiced_bt_gatt_send_discover( connection_handle, GATT_DISCOVER_SERVICES_BY_UUID, &parameter );

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
    return WICED_BT_UNSUPPORTED;

}

wiced_result_t smartbridge_bt_interface_find_included_services( uint16_t connection_handle, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* include_list )
{

    wiced_bt_gatt_discovery_param_t parameter;
    WPRINT_LIB_INFO( ( "[SmartBridge] Discover all Included Services\n" ) );

    subprocedure_lock();

    subprocedure_reset();

    /* FIXME : reset() above is not taking effect. Need to be debugged */
    subprocedure.attr_count = 0;
    memset( &parameter.uuid, 0, sizeof( parameter.uuid ) );

    parameter.s_handle = subprocedure.end_handle = start_handle;
    parameter.e_handle = subprocedure.start_handle = end_handle;

    wiced_bt_gatt_send_discover( connection_handle, GATT_DISCOVER_INCLUDED_SERVICES, &parameter );

    subprocedure_wait_for_completion();

    if ( subprocedure.result == WICED_BT_SUCCESS )
    {
        include_list->count = subprocedure.attr_count;
        include_list->list  = subprocedure.attr_head;
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

wiced_result_t smartbridge_bt_interface_discover_characteristic_by_uuid( uint16_t connection_handle, const wiced_bt_uuid_t* uuid, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* characteristic_list )
{
    UNUSED_PARAMETER(characteristic_list);
    wiced_bt_gatt_discovery_param_t parameter;

    subprocedure_lock();
    WPRINT_LIB_INFO( ( "[SmartBridge] Discover Characteristics by UUID [%x %x]\n",start_handle, end_handle ) );

    subprocedure_reset();
    subprocedure.attr_count = 0;

    memcpy( &parameter.uuid, uuid, sizeof( parameter.uuid ) );

    parameter.s_handle = subprocedure.start_handle  = start_handle;
    parameter.e_handle = subprocedure.end_handle    = end_handle;

    wiced_bt_gatt_send_discover( connection_handle, GATT_DISCOVER_CHARACTERISTICS, &parameter );

    subprocedure_wait_for_completion();


    if ( subprocedure.result == WICED_BT_SUCCESS )
    {
        characteristic_list->count = subprocedure.attr_count;
        characteristic_list->list  = subprocedure.attr_head;
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

    return WICED_BT_UNSUPPORTED;
}

wiced_result_t smartbridge_bt_interface_discover_all_characteristic_descriptors( uint16_t connection_handle, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* no_value_descriptor_list )
{

    wiced_bt_gatt_discovery_param_t parameter;
    WPRINT_LIB_INFO( ( "[SmartBridge] Discover all Characteristic Descriptors\n" ) );
    subprocedure_lock();
    subprocedure_reset();

    subprocedure.attr_count = 0;

    memset( &parameter.uuid, 0, sizeof( parameter.uuid ) );
    parameter.s_handle = start_handle;
    parameter.e_handle = end_handle;

    subprocedure.start_handle      = start_handle;
    subprocedure.end_handle        = end_handle;
    subprocedure.connection_handle = connection_handle;

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

wiced_result_t smartbridge_bt_interface_read_characteristic_descriptor( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* uuid, wiced_bt_smart_attribute_t** descriptor )
{
    wiced_bt_gatt_read_param_t parameter;
    WPRINT_LIB_INFO( ( "[SmartBridge] Read Characteristic Descriptor\n" ) );

    subprocedure_lock();
    subprocedure_reset();


    parameter.by_handle.auth_req = GATT_AUTH_REQ_NONE;
    parameter.by_handle.handle   = handle;

    subprocedure.attr_count     = 0;
    subprocedure.subprocedure   = GATT_READ_CHARACTERISTIC_DESCRIPTORS;
    subprocedure.start_handle   = handle;
    subprocedure.uuid.len       = uuid->len;
    subprocedure.connection_handle = connection_handle;

    if( uuid->len == UUID_16BIT )
    {
        subprocedure.uuid.uu.uuid16 = uuid->uu.uuid16;
    }

    else if( uuid->len == UUID_128BIT )
    {
        memcpy( subprocedure.uuid.uu.uuid128, uuid->uu.uuid128,  UUID_128BIT );
    }

    wiced_bt_gatt_send_read( connection_handle, GATT_READ_BY_HANDLE, &parameter );

    subprocedure_wait_for_completion();

    if ( subprocedure.result == WICED_BT_SUCCESS )
    {
        *descriptor = subprocedure.attr_head;
    }
    else
    {
        /* Clean up */
        wiced_bt_smart_attribute_delete( subprocedure.attr_head );
    }

    subprocedure_unlock();
    return subprocedure.result;
}

wiced_result_t smartbridge_bt_interface_read_characteristic_value( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* type, wiced_bt_smart_attribute_t** characteristic_value )
{

    wiced_bt_gatt_read_param_t parameter;
    WPRINT_LIB_INFO( ( "[SmartBridge] Read Characteristic Value\n" ) );

    subprocedure_lock();
    subprocedure_reset();

    parameter.by_handle.auth_req    = GATT_AUTH_REQ_NONE;
    parameter.by_handle.handle      = handle;

    subprocedure.subprocedure       = GATT_READ_CHARACTERISTIC_VALUE;
    subprocedure.attr_count         = 0;
    subprocedure.start_handle       = handle;
    subprocedure.uuid.len           = type->len;
    subprocedure.connection_handle  = connection_handle;

    if( type->len == UUID_16BIT )
    {
        subprocedure.uuid.uu.uuid16 = type->uu.uuid16;
    }
    else if( type->len == UUID_128BIT )
    {
        memcpy( subprocedure.uuid.uu.uuid128, type->uu.uuid128,  UUID_128BIT );
    }

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

wiced_result_t smartbridge_bt_interface_read_characteristic_values_using_uuid( uint16_t connection_handle, const wiced_bt_uuid_t* uuid, wiced_bt_smart_attribute_list_t* characteristic_value_list )
{
    wiced_bt_gatt_read_param_t parameter;
    WPRINT_LIB_INFO( ( "[SmartBridge] Read Characteristic Value\n" ) );

    subprocedure_lock();
    subprocedure_reset();

    parameter.char_type.auth_req    = GATT_AUTH_REQ_NONE;
    parameter.char_type.s_handle    = subprocedure.start_handle = 0x0001;
    parameter.char_type.e_handle    = subprocedure.end_handle   = 0xffff;

    memcpy( &parameter.char_type.uuid, uuid,  sizeof(wiced_bt_uuid_t) );

    subprocedure.subprocedure       = GATT_READ_USING_CHARACTERISTIC_UUID;
    subprocedure.attr_count         = 0;
    subprocedure.uuid.len           = uuid->len;
    subprocedure.connection_handle  = connection_handle;

    if( uuid->len == UUID_16BIT )
    {
        subprocedure.uuid.uu.uuid16 = uuid->uu.uuid16;
    }
    else if( uuid->len == UUID_128BIT )
    {
        memcpy( subprocedure.uuid.uu.uuid128, uuid->uu.uuid128,  UUID_128BIT );
    }

    wiced_bt_gatt_send_read( connection_handle, GATT_READ_CHAR_VALUE, &parameter );

    subprocedure_wait_for_completion();

    if ( subprocedure.result == WICED_BT_SUCCESS )
    {
        characteristic_value_list->count = subprocedure.attr_count;
        characteristic_value_list->list  = subprocedure.attr_head;
    }
    else
    {
        /* Clean up */
        wiced_bt_smart_attribute_delete( subprocedure.attr_head );
    }

    subprocedure_unlock();

    return subprocedure.result;

    return WICED_BT_UNSUPPORTED;
}

wiced_result_t smartbridge_bt_interface_read_long_characteristic_value( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* type, wiced_bt_smart_attribute_t** characteristic_value )
{
    wiced_bt_gatt_read_param_t parameter;
    WPRINT_LIB_INFO( ( "[SmartBridge] Read Characteristic Value\n" ) );

    subprocedure_lock();
    subprocedure_reset();

    parameter.partial.auth_req    = GATT_AUTH_REQ_NONE;
    parameter.partial.handle      = handle;
    parameter.partial.offset      = 0;

    subprocedure.subprocedure       = GATT_READ_LONG_CHARACTERISTIC_VALUES;
    subprocedure.attr_count         = 0;
    subprocedure.start_handle       = handle;
    subprocedure.uuid.len           = type->len;
    subprocedure.connection_handle  = connection_handle;

    if( type->len == UUID_16BIT )
    {
        subprocedure.uuid.uu.uuid16 = type->uu.uuid16;
    }
    else if( type->len == UUID_128BIT )
    {
        memcpy( subprocedure.uuid.uu.uuid128, type->uu.uuid128,  UUID_128BIT );
    }

    wiced_bt_gatt_send_read( connection_handle, GATT_READ_PARTIAL, &parameter );

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

wiced_result_t smartbridge_bt_interface_read_long_characteristic_descriptor( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* uuid, wiced_bt_smart_attribute_t** descriptor )
{

    UNUSED_PARAMETER(connection_handle);
    UNUSED_PARAMETER(handle);
    UNUSED_PARAMETER(uuid);
    UNUSED_PARAMETER(descriptor);
    return WICED_BT_UNSUPPORTED;
}

wiced_result_t smartbridge_bt_interface_write_characteristic_descriptor(  uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute )
{
    uint8_t                buffer[100] = { 0 };
    wiced_bt_gatt_value_t* write_value = (wiced_bt_gatt_value_t*)buffer;

    WPRINT_LIB_INFO( ( "[SmartBridge] Write Characteristic Descriptor\n" ) );
#if 0
    if ( attribute->value_length > LEATT_ATT_MTU - sizeof(LEATT_PDU_WRITE_HDR) )
    {
        BT_DEBUG_PRINT( ( "[GATT] Write Characteristic Value: value too long\n" ) );
        return WICED_BT_ATTRIBUTE_VALUE_TOO_LONG;
    }
#endif
    subprocedure_lock();
    subprocedure_reset();

    subprocedure.subprocedure       = GATT_WRITE_CHARACTERISTIC_DESCRIPTORS;
    subprocedure.connection_handle  = connection_handle;

    write_value->auth_req           = GATT_AUTH_REQ_NONE;
    write_value->handle             = attribute->handle;
    write_value->len                = attribute->value_length;
    write_value->offset             = 0;

    memcpy( write_value->value, attribute->value.value, attribute->value_length );

    wiced_bt_gatt_send_write( connection_handle, GATT_WRITE, write_value );

    subprocedure_wait_for_completion();

    subprocedure_unlock();
    return subprocedure.result;
}

wiced_result_t smartbridge_bt_interface_write_characteristic_value( uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute )
{
    uint8_t                buffer[100] = { 0 };
    wiced_bt_gatt_value_t* write_value = (wiced_bt_gatt_value_t*)buffer;

    WPRINT_LIB_INFO(("[SmartBridge] Write Characteristic value\n"));

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

wiced_result_t smartbridge_bt_interface_write_long_characteristic_value( uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute )
{
    UNUSED_PARAMETER(connection_handle);
    UNUSED_PARAMETER(attribute);
    return WICED_BT_UNSUPPORTED;
}

wiced_result_t smartbridge_bt_interface_write_long_characteristic_descriptor( uint16_t connection_handle, const wiced_bt_smart_attribute_t* descriptor )
{

    UNUSED_PARAMETER( connection_handle );
    UNUSED_PARAMETER( descriptor );

    return WICED_BT_UNSUPPORTED;
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

        if( app_scan_report_callback != NULL )
        {
            WPRINT_LIB_INFO( ("[SmartBridge] advertising callback reported\n") );
            app_scan_report_callback( &advertising_report );
        }
    }

    else
    {
        WPRINT_LIB_INFO(( "[SmartBridge] LE scan completed.\n" ));
        if( app_scan_complete_callback != NULL )
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
    *size = wiced_bt_ble_get_white_list_size();
    return WICED_BT_SUCCESS;
}

wiced_result_t smartbridge_bt_interface_clear_whitelist( void )
{
    return wiced_bt_ble_clear_white_list();
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
    app_scan_complete_callback = NULL;
    app_scan_report_callback   = NULL;

    return wiced_bt_ble_scan( BTM_BLE_SCAN_TYPE_NONE, WICED_TRUE, smartbridge_scan_result_callback );
}

wiced_result_t smartbridge_bt_interface_start_scan( const wiced_bt_smart_scan_settings_t* settings, wiced_bt_smart_scan_complete_callback_t complete_callback, wiced_bt_smart_advertising_report_callback_t advertising_report_callback )
{
    wiced_bool_t duplicate_filter_enabled = WICED_FALSE;

    /* First delete the previous scan result list */
    smartbridge_helper_delete_scan_result_list();

    /* fill with the settings provided by the smartbridge-application */
    wiced_bt_cfg_settings.ble_scan_cfg.scan_mode               = settings->type;
    wiced_bt_cfg_settings.ble_scan_cfg.high_duty_scan_window   = settings->window;
    wiced_bt_cfg_settings.ble_scan_cfg.high_duty_scan_duration = settings->duration_second;
    wiced_bt_cfg_settings.ble_scan_cfg.high_duty_scan_interval = settings->interval;
    duplicate_filter_enabled                                   = settings->filter_duplicates;

    app_scan_complete_callback = complete_callback;
    app_scan_report_callback   = advertising_report_callback;

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

wiced_result_t smartbridge_bt_interface_enable_pairing( wiced_bt_device_address_t address, wiced_bt_smart_address_type_t type, const wiced_bt_smart_security_settings_t* settings,  const char* passkey )
{
    char      str_passkey    [7]        = { 0 };
    uint8_t   integer_passkey[4]        = { 0 };
    uint32_t* integer_passkey_ptr       = (uint32_t*)integer_passkey;

    /* update the security settings as per passed by the application */
    default_io_caps_ble.local_io_cap      = settings->io_capabilities;
    default_io_caps_ble.auth_req          = settings->authentication_requirements;
    default_io_caps_ble.oob_data          = settings->oob_authentication;
    default_io_caps_ble.max_key_size      = settings->max_encryption_key_size;
    default_io_caps_ble.init_keys         = settings->master_key_distribution;
    default_io_caps_ble.resp_keys         = settings->slave_key_distribution;

    memcpy( str_passkey, passkey, strnlen( passkey, sizeof( str_passkey ) - 1 ) );

    *integer_passkey_ptr = atoi( str_passkey );

    WPRINT_LIB_INFO(("[SmartBridge] Enable Pairing with Pin: %d\n", (int)*integer_passkey_ptr ));

    wiced_bt_dev_sec_bond( address, type, BT_TRANSPORT_LE, 4, integer_passkey );

    return WICED_BT_SUCCESS;
}

wiced_result_t smartbridge_bt_interface_disable_pairing( void )
{
    return WICED_BT_ERROR;
}

wiced_result_t smartbridge_bt_interface_start_encryption( wiced_bt_device_address_t* address )
{
    uint32_t security_level = INTERNAL_SECURITY_LEVEL;

    return wiced_bt_dev_set_encryption( *(BD_ADDR*)address, BT_TRANSPORT_LE, &security_level );
}
