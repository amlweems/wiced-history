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

#include "wiced_rtos.h"
#include "wiced_utilities.h"
#include "wiced_bt_smart_attribute.h"
#include "bt_smart_gatt.h"
#include "gattdefs.h"
#include "wiced_bt_gatt.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define GATT_MAX_PROCEDURE_TIMEOUT (10000)

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

wiced_result_t subprocedure_unlock                        ( void );
wiced_result_t subprocedure_lock                          ( void );
wiced_result_t subprocedure_reset                         ( void );
wiced_result_t subprocedure_wait_for_completion           ( void );
wiced_result_t subprocedure_wait_clear_semaphore          ( void );
#if 0
static wiced_result_t subprocedure_timeout_handler               ( void );
static wiced_result_t bt_smart_gatt_client_response_handler      ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t exchange_mtu_cb                            ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t discover_all_primary_services_cb           ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t discover_primary_service_by_uuid_cb        ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t find_included_services_cb                  ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t discover_all_characteristics_cb            ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t discover_characteristic_by_uuid_cb         ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t discover_all_characteristic_descriptors_cb ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t read_characteristic_value_cb               ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t read_characteristic_value_using_uuid_cb    ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t read_long_characteristic_values_cb         ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
//static wiced_result_t read_multiple_characteristic_values_cb     ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
//static wiced_result_t write_without_response_cb                  ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
//static wiced_result_t signed_write_without_response_cb           ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t write_characteristic_value_cb              ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t write_long_characteristic_value_cb         ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
//static wiced_result_t reliable_writes_cb                         ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t read_characteristic_descriptors_cb         ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t read_long_characteristic_descriptor_cb     ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t write_characteristic_descriptor_cb         ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t write_long_characteristic_descriptor_cb    ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t characteristic_value_notification_cb       ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t characteristic_value_indication_cb         ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu );
static wiced_result_t notification_indication_app_cb             ( void* arg );
#endif

/******************************************************
 *               Variable Definitions
 ******************************************************/

static const wiced_bt_uuid_t const attribute_type_uuid_list[] =
{
    [0] = { .len = UUID_16BIT, .uu.uuid16 = GATT_UUID_PRI_SERVICE },
    [2] = { .len = UUID_16BIT, .uu.uuid16 = GATT_UUID_INCLUDE_SERVICE },
    [3] = { .len = UUID_16BIT, .uu.uuid16 = GATT_UUID_CHAR_DECLARE },
};
#if 0
static const bt_smart_att_response_handler_t const subprocedure_cb_list[] =
{
    [GATT_SUBPROCEDURE_NONE                        ] = NULL,
    [GATT_EXCHANGE_MTU                             ] = exchange_mtu_cb,
    [GATT_DISCOVER_ALL_PRIMARY_SERVICES            ] = discover_all_primary_services_cb,
    [GATT_DISCOVER_PRIMARY_SERVICE_BY_SERVICE_UUID ] = discover_primary_service_by_uuid_cb,
    [GATT_FIND_INCLUDED_SERVICES                   ] = find_included_services_cb,
    [GATT_DISCOVER_ALL_CHARACTERISTICS_OF_A_SERVICE] = discover_all_characteristics_cb,
    [GATT_DISCOVER_CHARACTERISTIC_BY_UUID          ] = discover_characteristic_by_uuid_cb,
    [GATT_DISCOVER_ALL_CHARACTERISTICS_DESCRIPTORS ] = discover_all_characteristic_descriptors_cb,
    [GATT_READ_CHARACTERISTIC_VALUE                ] = read_characteristic_value_cb,
    [GATT_READ_USING_CHARACTERISTIC_UUID           ] = read_characteristic_value_using_uuid_cb,
    [GATT_READ_LONG_CHARACTERISTIC_VALUES          ] = read_long_characteristic_values_cb,
    [GATT_READ_MULTIPLE_CHARACTERISTIC_VALUES      ] = NULL,//read_multiple_characteristic_values_cb,
    [GATT_WRITE_WITHOUT_RESPONSE                   ] = NULL,//write_without_response_cb,
    [GATT_SIGNED_WRITE_WITHOUT_RESPONSE            ] = NULL,//signed_write_without_response_cb,
    [GATT_WRITE_CHARACTERISTIC_VALUE               ] = write_characteristic_value_cb,
    [GATT_WRITE_LONG_CHARACTERISTIC_VALUE          ] = write_long_characteristic_value_cb,
    [GATT_CHARACTERISTIC_VALUE_RELIABLE_WRITES     ] = NULL,//reliable_writes_cb,
    [GATT_NOTIFICATIONS                            ] = characteristic_value_notification_cb,
    [GATT_INDICATIONS                              ] = characteristic_value_indication_cb,
    [GATT_READ_CHARACTERISTIC_DESCRIPTORS          ] = read_characteristic_descriptors_cb,
    [GATT_READ_LONG_CHARACTERISTIC_DESCRIPTORS     ] = read_long_characteristic_descriptor_cb,
    [GATT_WRITE_CHARACTERISTIC_DESCRIPTORS         ] = write_characteristic_descriptor_cb,
    [GATT_WRITE_LONG_CHARACTERISTIC_DESCRIPTORS    ] = write_long_characteristic_descriptor_cb,

};
#endif
gatt_subprocedure_t subprocedure;
volatile uint16_t current_service_handle_end;
//static bt_smart_gatt_notification_indication_handler_t notification_indication_handler = NULL;

/******************************************************
 *               Function Definitions
 ******************************************************/
#if 1
wiced_result_t bt_smart_gatt_init( void )
{
    wiced_result_t result;

    WPRINT_LIB_INFO( ( "[GATT] Initializing\n" ) );

    result = wiced_rtos_init_mutex( &subprocedure.mutex );
    if ( result  != WICED_BT_SUCCESS )
    {
        WPRINT_LIB_INFO( ( "[GATT] Error creating mutex\n" ) );
        return result;
    }

    result = wiced_rtos_init_semaphore( &subprocedure.done_semaphore );
    if ( result != WICED_BT_SUCCESS )
    {
        WPRINT_LIB_INFO( ( "[GATT] Error creating semaphore\n" ) );
        return result;
    }

    subprocedure_reset();
    WPRINT_LIB_INFO(("[GATT] Attribute count is:%u\n", (unsigned int)subprocedure.attr_count ));
    return WICED_BT_SUCCESS;
}

wiced_result_t bt_smart_gatt_deinit( void )
{

    WPRINT_LIB_INFO( ( "[GATT] Deinitializing\n" ) );

    wiced_rtos_deinit_mutex( &subprocedure.mutex );
    wiced_rtos_deinit_semaphore( &subprocedure.done_semaphore );
    subprocedure_reset();

    return WICED_BT_SUCCESS;
}

wiced_result_t bt_smart_gatt_set_timeout( uint32_t timeout_ms )
{

    WPRINT_LIB_INFO( ( "[GATT] Timeout set to %lu ms\n", timeout_ms ) );
    //return bt_smart_att_set_timeout( timeout_ms );
    return WICED_BT_ERROR;
}

wiced_result_t bt_smart_gatt_register_notification_indication_handler( bt_smart_gatt_notification_indication_handler_t handler )
{
    return WICED_BT_SUCCESS;
}

wiced_result_t bt_smart_gatt_exchange_mtu( uint16_t connection_handle, uint16_t client_mtu, uint16_t* server_mtu )
{
    WPRINT_LIB_INFO( ( "[GATT] Exchange MTU request\n" ) );

    subprocedure_lock();
    subprocedure_reset();

    subprocedure.subprocedure      = GATT_EXCHANGE_MTU;
    subprocedure.connection_handle = connection_handle;

    //bt_smart_att_exchange_mtu_request( connection_handle, client_mtu );

    subprocedure_wait_for_completion();

    if ( subprocedure.result == WICED_BT_SUCCESS )
    {
        *server_mtu = subprocedure.server_mtu;
    }

    subprocedure_unlock();
    return subprocedure.result;

}
#endif

wiced_result_t bt_smart_gatt_discover_all_primary_services( uint16_t connection_handle, wiced_bt_smart_attribute_list_t* service_list )
{
    wiced_bt_gatt_discovery_param_t parameter;
    //wiced_result_t result = WICED_ERROR;
    WPRINT_LIB_INFO( ( "[GATT] Discover all Primary Services\n" ) );

    subprocedure_lock();

    WPRINT_LIB_INFO( ("[GATT] before-reset-count:%u\n", (unsigned int)subprocedure.attr_count) );
    subprocedure_reset();

    memset( &parameter.uuid, 0, sizeof( parameter.uuid ) );
    parameter.s_handle = 1;
    parameter.e_handle = 0xffff;

    subprocedure.end_handle = 0xffff;
    subprocedure.start_handle = 1;

    wiced_bt_gatt_send_discover( connection_handle, GATT_DISCOVER_SERVICES_ALL, &parameter );

    WPRINT_LIB_INFO( ("[GATT] after-reset-count:%u\n", (unsigned int)subprocedure.attr_count) );

    subprocedure_wait_for_completion();

    if ( subprocedure.result == WICED_BT_SUCCESS )
    {
        service_list->count = subprocedure.attr_count;
        service_list->list  = subprocedure.attr_head;
        WPRINT_LIB_INFO( ("[GATT] Waiting done..Successful count:%u %u %u %u\n", (unsigned int)service_list->count, (unsigned int)service_list->list, (unsigned int)subprocedure.attr_count, (unsigned int)subprocedure.attr_head ));
    }

    else
    {
        /* Clean up */
        WPRINT_LIB_INFO( ("[GATT] Waiting done..NOT-Successful\n") );
        wiced_bt_smart_attribute_list_t list;

        list.count = subprocedure.attr_count;
        list.list  = subprocedure.attr_head;

        wiced_bt_smart_attribute_delete_list( &list );
    }

    subprocedure_unlock();

    return subprocedure.result;
}

wiced_result_t bt_smart_gatt_discover_primary_services_by_uuid( uint16_t connection_handle, const wiced_bt_uuid_t* uuid, wiced_bt_smart_attribute_list_t* service_list )
{

    wiced_bt_gatt_discovery_param_t parameter;

    WPRINT_LIB_INFO( ( "[GATT] Discover all Primary Services by UUID\n" ) );

    subprocedure_lock();
    subprocedure_reset();

    memcpy( (void*)&parameter.uuid, (void*)uuid, sizeof( wiced_bt_uuid_t ) );
    parameter.s_handle = 1;
    parameter.e_handle = 0xffff;

    subprocedure.end_handle = 0xffff;
    subprocedure.start_handle = 1;

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
}

wiced_result_t bt_smart_gatt_find_included_services( uint16_t connection_handle, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* include_list )
{
    UNUSED_PARAMETER(connection_handle);
    UNUSED_PARAMETER(start_handle);
    UNUSED_PARAMETER(end_handle);
    UNUSED_PARAMETER(include_list);

    return WICED_BT_ERROR;

}

wiced_result_t bt_smart_gatt_discover_all_characteristics_in_a_service( uint16_t connection_handle, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* characteristic_list )
{
    wiced_bt_gatt_discovery_param_t parameter;

    WPRINT_LIB_INFO( ( "[GATT] Discover Characteristics by Service[%x %x]\n",start_handle, end_handle ) );
    subprocedure_lock();

    subprocedure_reset();

    memset( &parameter.uuid, 0, sizeof( parameter.uuid ) );
    parameter.s_handle = start_handle;
    parameter.e_handle = end_handle;

    subprocedure.end_handle = end_handle;
    subprocedure.start_handle = start_handle;

    current_service_handle_end = end_handle;

    wiced_bt_gatt_send_discover( connection_handle, GATT_DISCOVER_CHARACTERISTICS, &parameter );

    subprocedure_wait_for_completion();

    WPRINT_LIB_INFO( ( "[GATT] Waiting completed\n" ) );

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
}

wiced_result_t bt_smart_gatt_discover_characteristics_by_uuid( uint16_t connection_handle, const wiced_bt_uuid_t* uuid, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* characteristic_list )
{
    wiced_bt_gatt_discovery_param_t parameter;

    WPRINT_LIB_INFO( ( "[GATT] Discover Characteristics by UUID\n" ) );
    subprocedure_lock();

    subprocedure_reset();

    memcpy( (void*)&parameter.uuid, (void*)uuid, sizeof( wiced_bt_uuid_t ) );
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

wiced_result_t bt_smart_gatt_discover_all_characteristic_descriptors( uint16_t connection_handle, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* no_value_descriptor_list )
{
    UNUSED_PARAMETER(connection_handle);
    UNUSED_PARAMETER(start_handle);
    UNUSED_PARAMETER(end_handle);
    UNUSED_PARAMETER(no_value_descriptor_list);
    return WICED_BT_ERROR;

}

wiced_result_t bt_smart_gatt_read_characteristic_value( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* type, wiced_bt_smart_attribute_t** characteristic_value )
{

    wiced_bt_gatt_read_param_t parameter;
    WPRINT_LIB_INFO( ( "[GATT] Read Characteristic Value\n" ) );


    subprocedure_lock();

    parameter.by_handle.auth_req = GATT_AUTH_REQ_NONE;
    parameter.by_handle.handle   = handle;
    subprocedure_reset();

    //subprocedure.start_handle      = handle;
    //subprocedure.uuid              = *type;
    //subprocedure.connection_handle = connection_handle;

    //WPRINT_LIB_INFO( ( "[GATT] Read Characteristic Value-2\n" ) );

    wiced_bt_gatt_send_read( connection_handle, GATT_READ_BY_HANDLE, &parameter );

    //WPRINT_LIB_INFO( ( "[GATT] Read..Waiting for Completion result:%d\n", (int)result ) );
    subprocedure_wait_for_completion();

    WPRINT_LIB_INFO( ( "[GATT] Read..Wait is over :%d\n", subprocedure.result ) );

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

wiced_result_t bt_smart_gatt_read_characteristic_values_using_uuid( uint16_t connection_handle, const wiced_bt_uuid_t* uuid, wiced_bt_smart_attribute_list_t* characteristic_value_list )
{
    UNUSED_PARAMETER(connection_handle);
    UNUSED_PARAMETER(uuid);
    UNUSED_PARAMETER(characteristic_value_list);
    return WICED_BT_ERROR;

}

wiced_result_t bt_smart_gatt_read_long_characteristic_value( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* type, wiced_bt_smart_attribute_t** characteristic_value )
{
    UNUSED_PARAMETER(connection_handle);
    UNUSED_PARAMETER(handle);
    UNUSED_PARAMETER(type);
    UNUSED_PARAMETER(characteristic_value);
    return WICED_BT_ERROR;

}

wiced_result_t bt_smart_gatt_write_characteristic_value( uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute )
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

    WPRINT_LIB_INFO( ( "[GATT] Write...Wait is over\n" ) );

    subprocedure_unlock();

    return subprocedure.result;
}

wiced_result_t bt_smart_gatt_write_long_characteristic_value( uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute )
{
    UNUSED_PARAMETER(connection_handle);
    UNUSED_PARAMETER(attribute);
    return WICED_BT_ERROR;

}

wiced_result_t bt_smart_gatt_read_characteristic_descriptor( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* uuid, wiced_bt_smart_attribute_t** descriptor )
{
    UNUSED_PARAMETER(connection_handle);
    UNUSED_PARAMETER(uuid);
    UNUSED_PARAMETER(handle);
    UNUSED_PARAMETER(descriptor);
    return WICED_BT_ERROR;
}

wiced_result_t bt_smart_gatt_read_long_characteristic_descriptor( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* uuid, wiced_bt_smart_attribute_t** descriptor )
{
    UNUSED_PARAMETER(connection_handle);
    UNUSED_PARAMETER(uuid);
    UNUSED_PARAMETER(handle);
    UNUSED_PARAMETER(descriptor);
    return WICED_BT_ERROR;

}

wiced_result_t bt_smart_gatt_write_characteristic_descriptor(  uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute )
{
    UNUSED_PARAMETER(connection_handle);
    UNUSED_PARAMETER(attribute);
    return WICED_BT_ERROR;

}

wiced_result_t bt_smart_gatt_write_long_characteristic_descriptor( uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute )
{
    UNUSED_PARAMETER(connection_handle);
    UNUSED_PARAMETER(attribute);
    return WICED_BT_ERROR;
}

wiced_result_t subprocedure_lock( void )
{
    return wiced_rtos_lock_mutex( &subprocedure.mutex );
}

wiced_result_t subprocedure_unlock( void )
{
    return wiced_rtos_unlock_mutex( &subprocedure.mutex );
}

wiced_result_t subprocedure_reset( void )
{
    subprocedure.subprocedure      = GATT_SUBPROCEDURE_NONE;
    subprocedure.attr_head         = NULL;
    subprocedure.attr_tail         = NULL;
    subprocedure.attr_count        = 0;
    subprocedure.result            = WICED_BT_SUCCESS;
    subprocedure.start_handle      = 0;
    subprocedure.end_handle        = 0;
    //subprocedure.pdu               = 0;
    subprocedure.length            = 0;
    subprocedure.offset            = 0;
    subprocedure.connection_handle = 0;
    memset( &subprocedure.uuid, 0, sizeof( subprocedure.uuid ) );
    subprocedure_wait_clear_semaphore();
    return WICED_BT_SUCCESS;
}

wiced_result_t subprocedure_wait_for_completion( void )
{
    return wiced_rtos_get_semaphore( &subprocedure.done_semaphore, GATT_MAX_PROCEDURE_TIMEOUT );
}

wiced_result_t subprocedure_wait_clear_semaphore( void )
{
    while ( wiced_rtos_get_semaphore( &subprocedure.done_semaphore, WICED_NO_WAIT ) == WICED_BT_SUCCESS )
    {
    }
    return WICED_BT_SUCCESS;
}

wiced_result_t subprocedure_notify_complete( void )
{
    return wiced_rtos_set_semaphore( &subprocedure.done_semaphore );
}

#if 0
static wiced_result_t bt_smart_gatt_client_response_handler( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    wiced_result_t result = WICED_BT_SUCCESS;

    bt_smart_gatt_subprocedure_t current_subprocedure;

    if ( pdu->code == LEATT_OPCODE_HANDLE_VALUE_INDICATION )
    {
        current_subprocedure = GATT_INDICATIONS;
    }
    else if ( pdu->code == LEATT_OPCODE_HANDLE_VALUE_NOTIFICATION )
    {
        current_subprocedure = GATT_NOTIFICATIONS;
    }
    else
    {
        current_subprocedure = subprocedure.subprocedure;
    }

    if ( subprocedure_cb_list[current_subprocedure] != NULL )
    {
        result = subprocedure_cb_list[current_subprocedure]( packet, pdu_length, pdu );
    }
    else
    {
        // TODO: send error
    }

    return result;
}

static wiced_result_t exchange_mtu_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    LEATT_PDU_MTU_EXCHANGE_RSP* response_pdu = (LEATT_PDU_MTU_EXCHANGE_RSP*)pdu;

    subprocedure.server_mtu = response_pdu->mtu;
    subprocedure.result     = WICED_BT_SUCCESS;
    subprocedure_notify_complete( );
    return WICED_BT_SUCCESS;
}

static wiced_result_t discover_all_primary_services_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    wiced_bool_t subprocedure_done = WICED_FALSE;

    if ( pdu->code == LEATT_OPCODE_READ_BY_GROUP_TYPE_RSP )
    {
        LEATT_PDU_READ_BY_GROUP_TYPE_RSP_HDR* response_pdu = (LEATT_PDU_READ_BY_GROUP_TYPE_RSP_HDR*)pdu;
        uint16_t*                             data         = (uint16_t*)( response_pdu + 1 );
        uint16_t*                             data_end     = (uint16_t*)( (uint8_t*)pdu + pdu_length );
        uint16_t                              last_handle  = 0;

        while ( data < data_end )
        {
            /* Create attribute(s) based on information included in the response PDU */
            wiced_bt_smart_attribute_t* attr;

            wiced_bt_smart_attribute_create( &attr, WICED_ATTRIBUTE_TYPE_PRIMARY_SERVICE, 0 );

            if ( attr != NULL )
            {
                attr->next                       = NULL;
                attr->handle                     = *data++;
                attr->type.size                  = UUID_16BIT;
                attr->type.value.value_16_bit    = GATT_UUID_PRI_SERVICE;
                attr->value_length               = response_pdu->length;
                attr->value.service.start_handle = attr->handle;
                attr->value.service.end_handle   = *data++;

                if ( ( response_pdu->length - 4 ) == UUID_16BIT )
                {
                    attr->value.service.uuid.size               = UUID_16BIT;
                    attr->value.service.uuid.value.value_16_bit = *data++;
                }
                else if ( ( response_pdu->length - 4 ) == UUID_128BIT )
                {
                    attr->value.service.uuid.size = UUID_128BIT;
                    memcpy( attr->value.service.uuid.value.value_128_bit, data, UUID_128BIT );
                    data += 8;
                }

                /* Update temporary variables */
                last_handle = attr->value.service.end_handle;

                if ( subprocedure.attr_head == NULL )
                {
                    subprocedure.attr_head = attr;
                }

                if ( subprocedure.attr_tail != NULL )
                {
                    subprocedure.attr_tail->next = attr;
                }

                subprocedure.attr_tail = attr;
                subprocedure.attr_count++;
            }
        }

        if ( last_handle == subprocedure.end_handle )
        {
            /* Last handle equals to maximum handle. Stop procedure and return */
            if ( subprocedure.attr_count != 0 )
            {
                subprocedure.result = WICED_BT_SUCCESS;
            }
            else
            {
                subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
            }

            subprocedure_done = WICED_TRUE;
        }
        else
        {
            /* Keep sending READ_BY_GROUP_TYPE_REQUEST until ERROR_RESPONSE: LEATT_ERR_CODE_ATTRIBUTE_NOT_FOUND is returned. */
            bt_smart_att_read_by_group_type_request( subprocedure.connection_handle, last_handle + 1, subprocedure.end_handle, &attribute_type_uuid_list[0] );
        }
    }
    else if ( pdu->code == LEATT_OPCODE_ERR_RSP )
    {
        LEATT_PDU_ERR_RSP* response_pdu = (LEATT_PDU_ERR_RSP*)pdu;

        if ( response_pdu->errCode == LEATT_ERR_CODE_ATTRIBUTE_NOT_FOUND )
        {
            if ( subprocedure.attr_count != 0 )
            {
                subprocedure.result = WICED_BT_SUCCESS;
            }
            else
            {
                subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
            }
        }
        else
        {
            subprocedure.result = WICED_BT_ERROR;
        }

        subprocedure_done = WICED_TRUE;
    }

    if ( subprocedure_done == WICED_TRUE )
    {
        subprocedure_notify_complete( );
    }

    return WICED_BT_SUCCESS;
}

static wiced_result_t discover_primary_service_by_uuid_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    wiced_bool_t subprocedure_done = WICED_FALSE;

    if ( pdu->code == LEATT_OPCODE_FIND_BY_TYPE_VALUE_RSP )
    {
        LEATT_PDU_FIND_BY_TYPE_VALUE_RSP_HDR* response_pdu = (LEATT_PDU_FIND_BY_TYPE_VALUE_RSP_HDR*)pdu;
        uint16_t*                             data         = (uint16_t*)( response_pdu + 1 );
        uint16_t*                             data_end     = (uint16_t*)( (uint8_t*)pdu + pdu_length );
        uint16_t                              last_handle  = 0;

        while ( data < data_end )
        {
            /* Create attribute(s) based on information included in the response PDU */
            wiced_bt_smart_attribute_t* attr;

            wiced_bt_smart_attribute_create( &attr, WICED_ATTRIBUTE_TYPE_PRIMARY_SERVICE, 0 );

            if ( attr != NULL )
            {
                attr->next                       = NULL;
                attr->handle                     = *data++;
                attr->type.size                  = UUID_16BIT;
                attr->type.value.value_16_bit    = GATT_UUID_PRI_SERVICE;
                attr->value_length               = 2 * sizeof( uint16_t ) + subprocedure.uuid.size;
                attr->value.service.start_handle = attr->handle;
                attr->value.service.end_handle   = *data++;
                attr->value.service.uuid         = subprocedure.uuid;

                /* Update temporary variables */
                last_handle = attr->value.service.end_handle;

                if ( subprocedure.attr_head == NULL )
                {
                    subprocedure.attr_head = attr;
                }

                if ( subprocedure.attr_tail != NULL )
                {
                    subprocedure.attr_tail->next = attr;
                }

                subprocedure.attr_tail = attr;
                subprocedure.attr_count++;
            }
        }

        if ( last_handle == subprocedure.end_handle )
        {
            /* Last handle equals to maximum handle. Stop procedure and return */
            if ( subprocedure.attr_count != 0 )
            {
                subprocedure.result = WICED_BT_SUCCESS;
            }
            else
            {
                subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
            }

            subprocedure_done = WICED_TRUE;
        }
        else
        {
            /* Keep sending FIND_BY_TYPE_VALUE_REQUEST until ERROR_RESPONSE: LEATT_ERR_CODE_ATTRIBUTE_NOT_FOUND is returned  */
            bt_smart_att_find_by_type_value_request( subprocedure.connection_handle, last_handle + 1, subprocedure.end_handle, GATT_UUID_PRI_SERVICE, (uint8_t*)&subprocedure.uuid.value, subprocedure.uuid.size );
        }
    }
    else if ( pdu->code == LEATT_OPCODE_ERR_RSP )
    {
        LEATT_PDU_ERR_RSP* response_pdu = (LEATT_PDU_ERR_RSP*)pdu;

        if ( response_pdu->errCode == LEATT_ERR_CODE_ATTRIBUTE_NOT_FOUND )
        {
            if ( subprocedure.attr_count != 0 )
            {
                subprocedure.result = WICED_BT_SUCCESS;
            }
            else
            {
                subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
            }
        }
        else
        {
            subprocedure.result = WICED_BT_ERROR;
        }

        subprocedure_done = WICED_TRUE;
    }

    if ( subprocedure_done == WICED_TRUE )
    {
        subprocedure_notify_complete( );
    }

    return WICED_BT_SUCCESS;
}

static wiced_result_t find_included_services_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    wiced_bool_t subprocedure_done = WICED_FALSE;

    if ( pdu->code == LEATT_OPCODE_READ_BY_TYPE_RSP )
    {
        LEATT_PDU_READ_BY_TYPE_RSP_HDR* response_pdu = (LEATT_PDU_READ_BY_TYPE_RSP_HDR*)pdu;
        uint16_t*                       data         = (uint16_t*)( response_pdu + 1 );
        wiced_bt_smart_attribute_t*     attr;

        wiced_bt_smart_attribute_create( &attr, WICED_ATTRIBUTE_TYPE_INCLUDE, 0 );

        if ( attr != NULL )
        {
            attr->next                                  = NULL;
            attr->handle                                = *data++;
            attr->type.size                             = UUID_16BIT;
            attr->type.value.value_16_bit               = GATT_UUID_INCLUDE_SERVICE;
            attr->value.include.included_service_handle = *data++;
            attr->value.include.end_group_handle        = *data++;

            /* Update temporary variables */
            if ( subprocedure.attr_head == NULL )
            {
                subprocedure.attr_head = attr;
            }

            if ( subprocedure.attr_tail != NULL )
            {
                subprocedure.attr_tail->next = attr;
            }

            subprocedure.attr_tail = attr;
            subprocedure.attr_count++;

            if ( ( response_pdu->length - 6 ) == UUID_16BIT )
            {
                attr->value.include.uuid.size               = UUID_16BIT;
                attr->value.include.uuid.value.value_16_bit = *data++;
                attr->value_length                          = 2 * sizeof( uint16_t ) + attr->value.include.uuid.size;

                if ( attr->handle == subprocedure.end_handle )
                {
                    /* Last handle equals to maximum handle. Stop procedure and return */
                    if ( subprocedure.attr_count != 0 )
                    {
                        subprocedure.result = WICED_BT_SUCCESS;
                    }
                    else
                    {
                        subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
                    }

                    subprocedure_done = WICED_TRUE;
                }
                else
                {
                    /* Include attribute value retrieved. Increment handle and send another Read_By_Type_Request  */
                    bt_smart_att_read_by_type_request( subprocedure.connection_handle, attr->handle + 1, subprocedure.end_handle, &attribute_type_uuid_list[2] );
                }
            }
            else
            {
                /* UUID is 128-bit. Issue Read_Request to retrieve UUID */
                bt_smart_att_read_request( subprocedure.connection_handle,  attr->value.include.included_service_handle );
            }
        }
        else
        {
            subprocedure.result = WICED_BT_OUT_OF_HEAP_SPACE;
        }

        subprocedure_done = WICED_TRUE;
    }
    else if ( pdu->code == LEATT_OPCODE_READ_RSP )
    {
        LEATT_PDU_READ_RSP_HDR* response_pdu = (LEATT_PDU_READ_RSP_HDR*)pdu;
        uint8_t*                        data = (uint8_t*)( response_pdu + 1 );

        subprocedure.attr_tail->value.include.uuid.size = UUID_128BIT;
        subprocedure.attr_tail->value_length            = 2 * sizeof(uint16_t) + subprocedure.attr_tail->value.include.uuid.size;

        memcpy( subprocedure.attr_tail->value.include.uuid.value.value_128_bit, data, UUID_128BIT );

        if ( subprocedure.attr_tail->handle == subprocedure.end_handle )
        {
            /* Last handle equals to maximum handle. Stop procedure and return */
            if ( subprocedure.attr_count != 0 )
            {
                subprocedure.result = WICED_BT_SUCCESS;
            }
            else
            {
                subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
            }

            subprocedure_done = WICED_TRUE;
        }
        else
        {
            /* Include attribute value retrieved. Increment handle and send another Read_By_Type_Request  */
            bt_smart_att_read_by_type_request( subprocedure.connection_handle, subprocedure.attr_tail->handle + 1, subprocedure.end_handle, &attribute_type_uuid_list[2] );
        }
    }
    else if ( pdu->code == LEATT_OPCODE_ERR_RSP )
    {
        LEATT_PDU_ERR_RSP* response_pdu = (LEATT_PDU_ERR_RSP*)pdu;

        if ( response_pdu->errCode == LEATT_ERR_CODE_ATTRIBUTE_NOT_FOUND )
        {
            if ( subprocedure.attr_count != 0 )
            {
                subprocedure.result = WICED_BT_SUCCESS;
            }
            else
            {
                subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
            }
        }
        else
        {
            subprocedure.result = WICED_BT_ERROR;
        }

        subprocedure_done = WICED_TRUE;
    }

    if ( subprocedure_done == WICED_TRUE )
    {
        subprocedure_notify_complete( );
    }

    return WICED_BT_SUCCESS;
}

static wiced_result_t discover_all_characteristics_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    wiced_bool_t subprocedure_done = WICED_FALSE;

    if ( pdu->code == LEATT_OPCODE_READ_BY_TYPE_RSP )
    {
        LEATT_PDU_READ_BY_TYPE_RSP_HDR* response_pdu = (LEATT_PDU_READ_BY_TYPE_RSP_HDR*)pdu;
        uint8_t*                        data         = (uint8_t*)( response_pdu + 1 );
        uint8_t*                        data_end     = (uint8_t*)( (uint8_t*)pdu + pdu_length );

        while ( data < data_end )
        {
            wiced_bt_smart_attribute_t* attr;

            wiced_bt_smart_attribute_create( &attr, WICED_ATTRIBUTE_TYPE_CHARACTERISTIC, 0 );

            if ( attr != NULL )
            {
                attr->next                    = NULL;
                attr->type.size               = UUID_16BIT;
                attr->type.value.value_16_bit = UUID_ATTRIBUTE_CHARACTERISTIC;
                attr->value_length            = response_pdu->length - 2;
                attr->handle                  = *(uint16_t*)data;
                data += 2;

                memcpy( &attr->value.characteristic, data, response_pdu->length - 2 );
                data += ( response_pdu->length - 2 );

                attr->value.characteristic.uuid.size = ( ( response_pdu->length ) - 5 == UUID_16BIT ) ? UUID_16BIT : UUID_128BIT;

                /* Update temporary variables */
                if ( subprocedure.attr_head == NULL )
                {
                    subprocedure.attr_head = attr;
                }

                if ( subprocedure.attr_tail != NULL )
                {
                    subprocedure.attr_tail->next = attr;
                }

                subprocedure.attr_tail = attr;
                subprocedure.attr_count++;
            }
            else
            {
                subprocedure.result = WICED_BT_OUT_OF_HEAP_SPACE;
                subprocedure_done = WICED_TRUE;
            }
        }

        if ( subprocedure.attr_tail->handle == subprocedure.end_handle )
        {
            /* Last handle equals to maximum handle. Stop procedure and return */
            if ( subprocedure.attr_count > 0 )
            {
                wiced_bt_smart_attribute_t* curr_attr = subprocedure.attr_head;

                while ( curr_attr != NULL )
                {
                    /* Characteristic descriptor starts right after Characteristic Value */
                    curr_attr->value.characteristic.descriptor_start_handle = curr_attr->value.characteristic.value_handle + 1;

                    if ( curr_attr->next != NULL )
                    {
                        /* Characteristic descriptor ends right before the beginning of next Characteristic */
                        curr_attr->value.characteristic.descriptor_end_handle = curr_attr->next->handle - 1;
                    }
                    else
                    {
                        /* Characteristic descriptor ends right at the end of the Service range */
                        curr_attr->value.characteristic.descriptor_end_handle = subprocedure.end_handle;
                    }

                    curr_attr = curr_attr->next;
                }

                /* Update descriptor start and end handles of the characteristics in the list */
                subprocedure.result = WICED_BT_SUCCESS;

            }
            else
            {
                subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
            }

            subprocedure_done = WICED_TRUE;
        }
        else
        {
            /* Include attribute value retrieved. Increment handle and send another Read_By_Type_Request  */
            bt_smart_att_read_by_type_request( subprocedure.connection_handle, subprocedure.attr_tail->handle + 1, subprocedure.end_handle, &attribute_type_uuid_list[3] );
        }
    }
    else if ( pdu->code == LEATT_OPCODE_ERR_RSP )
    {
        LEATT_PDU_ERR_RSP* response_pdu = (LEATT_PDU_ERR_RSP*)pdu;

        if ( response_pdu->errCode == LEATT_ERR_CODE_ATTRIBUTE_NOT_FOUND )
        {
            if ( subprocedure.attr_count > 0 )
            {
                wiced_bt_smart_attribute_t* curr_attr = subprocedure.attr_head;

                while ( curr_attr != NULL )
                {
                    /* Characteristic descriptor starts right after Characteristic Value */
                    curr_attr->value.characteristic.descriptor_start_handle = curr_attr->value.characteristic.value_handle + 1;

                    if ( curr_attr->next != NULL )
                    {
                        /* Characteristic descriptor ends right before the beginning of next Characteristic */
                        curr_attr->value.characteristic.descriptor_end_handle = curr_attr->next->handle - 1;
                    }
                    else
                    {
                        /* Characteristic descriptor ends right at the end of the Service range */
                        curr_attr->value.characteristic.descriptor_end_handle = subprocedure.end_handle;
                    }

                    curr_attr = curr_attr->next;
                }

                /* Update descriptor start and end handles of the characteristics in the list */
                subprocedure.result = WICED_BT_SUCCESS;

            }
            else
            {
                subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
            }
        }
        else
        {
            subprocedure.result = WICED_ERROR;
        }

        subprocedure_done = WICED_TRUE;
    }

    if ( subprocedure_done == WICED_TRUE )
    {
        subprocedure_notify_complete( );
    }

    return WICED_BT_SUCCESS;
}

static wiced_result_t discover_characteristic_by_uuid_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    wiced_bool_t subprocedure_done = WICED_FALSE;

    if ( pdu->code == LEATT_OPCODE_READ_BY_TYPE_RSP )
    {
        LEATT_PDU_READ_BY_TYPE_RSP_HDR* response_pdu = (LEATT_PDU_READ_BY_TYPE_RSP_HDR*)pdu;
        uint8_t*                        data         = (uint8_t*)( response_pdu + 1 );
        uint8_t*                        data_end     = (uint8_t*)( (uint8_t*)pdu + pdu_length );

        while ( data < data_end )
        {
            /* If UUID is found, keep attribute; otherwise, toss. Then, move to the next one */
            if ( memcmp(data + 5, &subprocedure.uuid.value, subprocedure.uuid.size ) == 0 )
            {
                wiced_bt_smart_attribute_t* attr;

                wiced_bt_smart_attribute_create( &attr, WICED_ATTRIBUTE_TYPE_CHARACTERISTIC, 0 );

                if ( attr != NULL )
                {
                    attr->next                    = NULL;
                    attr->type.size               = UUID_16BIT;
                    attr->type.value.value_16_bit = UUID_ATTRIBUTE_CHARACTERISTIC;
                    attr->value_length            = response_pdu->length - 2;
                    attr->handle                  = *(uint16_t*)data;
                    data += 2;

                    memcpy( &attr->value.characteristic, data, response_pdu->length - 2 );
                    data += ( response_pdu->length - 2 );

                    attr->value.characteristic.uuid.size = ( ( response_pdu->length ) - 5 == UUID_16BIT ) ? UUID_16BIT : UUID_128BIT;

                    /* Characteristic descriptor starts right after Characteristic Value */
                    attr->value.characteristic.descriptor_start_handle = attr->value.characteristic.value_handle + 1;

                    if ( data < data_end )
                    {
                        /* Characteristic descriptor ends right before the beginning of next Characteristic */
                        attr->value.characteristic.descriptor_end_handle = *(uint16_t*)data;
                    }
                    else
                    {
                        /* Characteristic descriptor ends right at the end of the Service range */
                        attr->value.characteristic.descriptor_end_handle = subprocedure.end_handle - 1;
                    }

                    /* Update temporary variables */
                    if ( subprocedure.attr_head == NULL )
                    {
                        subprocedure.attr_head = attr;
                    }

                    if ( subprocedure.attr_tail != NULL )
                    {
                        subprocedure.attr_tail->next = attr;
                    }

                    subprocedure.attr_tail = attr;
                    subprocedure.attr_count++;
                }
            }
            else
            {
                data += response_pdu->length;
            }
        }

        if ( subprocedure.attr_tail->handle == subprocedure.end_handle )
        {
            /* Last handle equals to maximum handle. Stop procedure and return */
            if ( subprocedure.attr_count > 0 )
            {
                subprocedure.result = WICED_BT_SUCCESS;
            }
            else
            {
                subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
            }

            subprocedure_done = WICED_TRUE;
        }
        else
        {
            /* Include attribute value retrieved. Increment handle and send another Read_By_Type_Request  */
            bt_smart_att_read_by_type_request( subprocedure.connection_handle, subprocedure.attr_tail->handle + 1, subprocedure.end_handle, &attribute_type_uuid_list[3] );
        }
    }
    else if ( pdu->code == LEATT_OPCODE_ERR_RSP )
    {
        LEATT_PDU_ERR_RSP* response_pdu = (LEATT_PDU_ERR_RSP*)pdu;

        if ( response_pdu->errCode == LEATT_ERR_CODE_ATTRIBUTE_NOT_FOUND )
        {
            if ( subprocedure.attr_count > 0 )
            {
                subprocedure.result = WICED_BT_SUCCESS;
            }
            else
            {
                subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
            }
        }
        else
        {
            subprocedure.result = WICED_ERROR;
        }

        subprocedure_done = WICED_TRUE;
    }

    if ( subprocedure_done == WICED_TRUE )
    {
        subprocedure_notify_complete( );
    }

    return WICED_BT_SUCCESS;
}

static wiced_result_t discover_all_characteristic_descriptors_cb ( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    wiced_bool_t subprocedure_done = WICED_FALSE;

    if ( pdu->code == LEATT_OPCODE_FIND_INFO_RSP )
    {
        LEATT_PDU_FIND_INFO_RSP_HDR* response_pdu = (LEATT_PDU_FIND_INFO_RSP_HDR*)pdu;
        uint8_t*                     data         = (uint8_t*)( response_pdu + 1 );
        uint8_t*                     data_end     = (uint8_t*)( (uint8_t*)pdu + pdu_length );

        while ( data < data_end )
        {
            wiced_bt_smart_attribute_t* attr;

            /* Create a new attribute */
            wiced_bt_smart_attribute_create( &attr, WICED_ATTRIBUTE_TYPE_NO_VALUE, 0 );

            if ( attr != NULL )
            {
                attr->next      = NULL;
                attr->handle    = *(uint16_t*)data;
                data           += sizeof( attr->handle );

                attr->type.size = ( response_pdu->format == 1 ) ? UUID_16BIT : UUID_128BIT ;

                memcpy( &attr->type.value, data, attr->type.size );
                data += attr->type.size;

                /* Update temporary variables */
                if ( subprocedure.attr_head == NULL )
                {
                    subprocedure.attr_head = attr;
                }

                if ( subprocedure.attr_tail != NULL )
                {
                    subprocedure.attr_tail->next = attr;
                }

                subprocedure.attr_tail = attr;
                subprocedure.attr_count++;
            }
        }

        if ( subprocedure.attr_tail->handle == subprocedure.end_handle )
        {
            if ( subprocedure.attr_count > 0 )
            {
                subprocedure.result = WICED_BT_SUCCESS;
            }
            else
            {
                subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
            }

            subprocedure_done = WICED_TRUE;
        }
        else
        {
            bt_smart_att_find_information_request( subprocedure.connection_handle, subprocedure.attr_tail->handle + 1, subprocedure.end_handle );

        }
    }
    else if ( pdu->code == LEATT_OPCODE_ERR_RSP )
    {
        LEATT_PDU_ERR_RSP* response_pdu = (LEATT_PDU_ERR_RSP*)pdu;

        if ( response_pdu->errCode == LEATT_ERR_CODE_ATTRIBUTE_NOT_FOUND )
        {
            if ( subprocedure.attr_count > 0 )
            {
                subprocedure.result = WICED_BT_SUCCESS;
            }
            else
            {
                subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
            }
        }
        else
        {
            subprocedure.result = WICED_ERROR;
        }

        subprocedure_done = WICED_TRUE;
    }

    if ( subprocedure_done == WICED_TRUE )
    {
        subprocedure_notify_complete( );
    }

    return WICED_BT_SUCCESS;
}

static wiced_result_t read_characteristic_value_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    if ( pdu->code == LEATT_OPCODE_READ_RSP )
    {
        LEATT_PDU_READ_RSP_HDR*     response_pdu = (LEATT_PDU_READ_RSP_HDR*)pdu;
        uint8_t*                    data         = (uint8_t*)( response_pdu + 1 );
        wiced_bt_smart_attribute_t* attr;

        /* Create a new attribute */
        wiced_bt_smart_attribute_create( &attr, WICED_ATTRIBUTE_TYPE_CHARACTERISTIC_VALUE, pdu_length - 1 );

        if ( attr != NULL )
        {
            attr->next         = NULL;
            attr->handle       = subprocedure.start_handle;
            attr->type         = subprocedure.uuid;
            attr->value_length = pdu_length - 1;
            memcpy( attr->value.characteristic_value.value, data, pdu_length - 1 );

            /* Update temporary variables */
            if ( subprocedure.attr_head == NULL )
            {
                subprocedure.attr_head = attr;
            }

            subprocedure.result = WICED_BT_SUCCESS;
        }
        else
        {
            subprocedure.result = WICED_BT_OUT_OF_HEAP_SPACE;
        }
    }
    else if ( pdu->code == LEATT_OPCODE_ERR_RSP )
    {
        subprocedure.result = WICED_ERROR;
    }

    subprocedure_notify_complete( );
    return WICED_BT_SUCCESS;
}

static wiced_result_t read_characteristic_value_using_uuid_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    wiced_bool_t subprocedure_done = WICED_FALSE;

    if ( pdu->code == LEATT_OPCODE_READ_BY_TYPE_RSP )
    {
        LEATT_PDU_READ_BY_TYPE_RSP_HDR* response_pdu = (LEATT_PDU_READ_BY_TYPE_RSP_HDR*)pdu;
        uint8_t*                        data         = (uint8_t*)( response_pdu + 1 );
        uint8_t*                        data_end     = (uint8_t*)( (uint8_t*)pdu + pdu_length );

        while ( data < data_end )
        {
            wiced_bt_smart_attribute_t* attr;

            /* Create new attribute */
            wiced_bt_smart_attribute_create( &attr, WICED_ATTRIBUTE_TYPE_CHARACTERISTIC_VALUE, response_pdu->length - 2 );

            if ( attr != NULL )
            {
                attr->next         = NULL;
                attr->type         = subprocedure.uuid;
                attr->value_length = response_pdu->length - 2;
                attr->handle       = *(uint16_t*)data++;
                data += sizeof(uint16_t);

                memcpy( attr->value.characteristic_value.value, data, response_pdu->length - 2 );
                data += response_pdu->length - 2;

                /* Update temporary variables */
                if ( subprocedure.attr_head == NULL )
                {
                    subprocedure.attr_head = attr;
                }

                if ( subprocedure.attr_tail != NULL )
                {
                    subprocedure.attr_tail->next = attr;
                }

                subprocedure.attr_tail = attr;
                subprocedure.attr_count++;
            }
            else
            {
                /* Attribute can't be created. Not enough memory */
                subprocedure.result = WICED_BT_OUT_OF_HEAP_SPACE;
                subprocedure_done   = WICED_TRUE;
            }
        }

        if ( subprocedure.attr_tail->handle == subprocedure.end_handle )
        {
            if ( subprocedure.attr_count > 0 )
            {
                subprocedure.result = WICED_BT_SUCCESS;
            }
            else
            {
                subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
            }

            subprocedure_done = WICED_TRUE;
        }
        else
        {
            /* Include attribute value retrieved. Increment handle and send another Read_By_Type_Request  */
            bt_smart_att_read_by_type_request( subprocedure.connection_handle, subprocedure.attr_tail->handle + 1, subprocedure.end_handle, &subprocedure.uuid );
        }
    }
    else if ( pdu->code == LEATT_OPCODE_ERR_RSP )
    {
        LEATT_PDU_ERR_RSP* response_pdu = (LEATT_PDU_ERR_RSP*)pdu;

        if ( response_pdu->errCode == LEATT_ERR_CODE_ATTRIBUTE_NOT_FOUND )
        {
            if ( subprocedure.attr_count > 0 )
            {
                subprocedure.result = WICED_BT_SUCCESS;
            }
            else
            {
                subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
            }
        }
        else
        {
            subprocedure.result = WICED_ERROR;
        }

        subprocedure_done = WICED_TRUE;
    }

    if ( subprocedure_done == WICED_TRUE )
    {
        subprocedure_notify_complete( );
    }

    return WICED_BT_SUCCESS;
}

static wiced_result_t read_long_characteristic_values_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    wiced_bool_t subprocedure_done = WICED_FALSE;

    if ( pdu->code == LEATT_OPCODE_READ_BLOB_RSP )
    {
        LEATT_PDU_READ_BLOB_RSP_HDR* response_pdu = (LEATT_PDU_READ_BLOB_RSP_HDR*)pdu;
        uint8_t*                     data         = (uint8_t*)( response_pdu + 1 );
        wiced_bt_smart_attribute_t*  attr         = subprocedure.attr_head;

        if ( attr == NULL )
        {
            /* First iteration. Create attribute */
            wiced_bt_smart_attribute_create( &attr, WICED_ATTRIBUTE_TYPE_LONG_VALUE, 0 );

            if ( attr != NULL )
            {
                /* Attribute created. Fill in details */
                attr->next   = NULL;
                attr->handle = subprocedure.start_handle;
                attr->type   = subprocedure.uuid;
            }
            else
            {
                /* Can't create attribute. Running out of memory */
                subprocedure.result = WICED_BT_OUT_OF_HEAP_SPACE;
            }
        }

        if ( attr->value_length + pdu_length - 1 < MAX_CHARACTERISTIC_VALUE_LENGTH )
        {
            memcpy( &attr->value.characteristic_value.value[attr->value_length], data, pdu_length - 1 );
            attr->value_length += pdu_length - 1;

            if ( pdu_length - 1 < subprocedure.server_mtu )
            {
                subprocedure_done = WICED_TRUE;
            }
            else
            {
                /* Not done yet. Read more data from the server */
                bt_smart_att_read_blob_request( subprocedure.connection_handle, subprocedure.start_handle, attr->value_length );
            }
        }
        else
        {
            /* Copy data */
            memcpy( &attr->value.characteristic_value.value[attr->value_length], data, MAX_CHARACTERISTIC_VALUE_LENGTH - attr->value_length );
            attr->value_length = MAX_CHARACTERISTIC_VALUE_LENGTH;
            subprocedure_done = WICED_TRUE;
        }
    }
    else if ( pdu->code == LEATT_OPCODE_ERR_RSP )
    {
        LEATT_PDU_ERR_RSP* response_pdu = (LEATT_PDU_ERR_RSP*)pdu;

        if ( response_pdu->errCode == LEATT_ERR_CODE_INVALID_OFFSET )
        {
            /* Invalid offset indicates the end of successful procedure */
            subprocedure.result = WICED_BT_SUCCESS;
        }
        else
        {
            subprocedure.result = WICED_BT_ERROR;
        }

        subprocedure_done = WICED_TRUE;
    }

    if ( subprocedure_done == WICED_TRUE )
    {
        subprocedure_notify_complete( );
    }
    return WICED_BT_SUCCESS;
}

static wiced_result_t write_characteristic_value_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
     if ( pdu->code == LEATT_OPCODE_WRITE_RSP )
     {
         subprocedure.result = WICED_BT_SUCCESS;
     }
     else if ( pdu->code == LEATT_OPCODE_ERR_RSP )
     {
        subprocedure.result = WICED_BT_ERROR;
     }

     subprocedure_notify_complete( );
     return WICED_BT_SUCCESS;
}

static wiced_result_t write_long_characteristic_value_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    wiced_bool_t subprocedure_done = WICED_FALSE;

    if ( pdu->code == LEATT_OPCODE_PREPARE_WRITE_RSP )
    {
        LEATT_PDU_PREPARE_WRITE_RSP_HDR* response_pdu = (LEATT_PDU_PREPARE_WRITE_RSP_HDR*)pdu;
        uint16_t                         value_length = pdu_length - sizeof( *response_pdu );
        uint16_t                         new_offset   = response_pdu->valOffset + value_length;

        if ( new_offset < subprocedure.length )
        {
            uint16_t new_length = MIN( ( subprocedure.length - new_offset ), ( LEATT_ATT_MTU - sizeof(LEATT_PDU_PREPARE_WRITE_RSP_HDR) ) );

            bt_smart_att_prepare_write_request( subprocedure.connection_handle, response_pdu->attrHandle, new_offset, &subprocedure.attr_head->value.value[new_offset], new_length );
        }
        else
        {
            bt_smart_att_execute_write_request( subprocedure.connection_handle, BT_SMART_ATT_WRITE_ALL_PENDING_PREPARED_VALUES );
        }
    }
    else if ( pdu->code == LEATT_OPCODE_EXECUTE_WRITE_RSP )
    {
        subprocedure.result = WICED_BT_SUCCESS;
        subprocedure_done   = WICED_TRUE;
    }
    else if ( pdu->code == LEATT_OPCODE_ERR_RSP )
    {

        subprocedure.result = WICED_BT_ERROR;
        subprocedure_done   = WICED_TRUE;
    }

    if ( subprocedure_done == WICED_TRUE )
    {
        subprocedure_notify_complete( );
    }

    return WICED_BT_SUCCESS;
}

static wiced_result_t read_characteristic_descriptors_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    if ( pdu->code == LEATT_OPCODE_READ_RSP )
    {
        LEATT_PDU_READ_RSP_HDR* response_pdu = (LEATT_PDU_READ_RSP_HDR*)pdu;
        uint8_t* data = (uint8_t*)( response_pdu + 1 );
        wiced_bt_smart_attribute_t* attr;

        wiced_bt_smart_attribute_create( &attr, WICED_ATTRIBUTE_TYPE_CHARACTERISTIC_VALUE, pdu_length - 1 );


        if ( attr != NULL )
        {
            attr->next         = NULL;
            attr->handle       = subprocedure.start_handle;
            attr->type         = subprocedure.uuid;
            attr->value_length = pdu_length - 1;
            memcpy( &attr->value, data, pdu_length - 1 );

            /* Update temporary variables */
            if ( subprocedure.attr_head == NULL )
            {
                subprocedure.attr_head = attr;
            }

            subprocedure.result = WICED_BT_SUCCESS;
        }
        else
        {
            subprocedure.result = WICED_BT_OUT_OF_HEAP_SPACE;
        }
    }
    else if ( pdu->code == LEATT_OPCODE_ERR_RSP )
    {
        subprocedure.result = WICED_BT_ERROR;
    }

    subprocedure_notify_complete( );
    return WICED_BT_SUCCESS;
}

static wiced_result_t read_long_characteristic_descriptor_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    wiced_bool_t subprocedure_done = WICED_FALSE;

    if ( pdu->code == LEATT_OPCODE_READ_BLOB_RSP )
    {
        LEATT_PDU_READ_BLOB_RSP_HDR* response_pdu = (LEATT_PDU_READ_BLOB_RSP_HDR*)pdu;
        uint8_t*                     data         = (uint8_t*)( response_pdu + 1 );
        wiced_bt_smart_attribute_t*  attr         = subprocedure.attr_head;

        if ( attr == NULL )
        {
            wiced_bt_smart_attribute_create( &attr, WICED_ATTRIBUTE_TYPE_LONG_VALUE, 0 );

            if ( attr != NULL )
            {
                attr->next       = NULL;
                attr->handle     = subprocedure.start_handle;
                attr->type       = subprocedure.uuid;
            }
            else
            {
                subprocedure.result = WICED_BT_OUT_OF_HEAP_SPACE;
            }
        }

        if ( attr->value_length + pdu_length - 1 < MAX_CHARACTERISTIC_VALUE_LENGTH )
        {
            uint8_t* value = (uint8_t*)&attr->value;

            memcpy( value + attr->value_length, data, pdu_length - 1 );
            attr->value_length += pdu_length - 1;

            if ( pdu_length - 1 < subprocedure.server_mtu )
            {
                subprocedure_done = WICED_TRUE;
            }
            else
            {
                bt_smart_att_read_blob_request( subprocedure.connection_handle, subprocedure.start_handle, attr->value_length );
            }
        }
        else
        {
            memcpy( &attr->value.characteristic_value.value[attr->value_length], data, MAX_CHARACTERISTIC_VALUE_LENGTH - attr->value_length );
            attr->value_length = MAX_CHARACTERISTIC_VALUE_LENGTH;
            subprocedure_done = WICED_TRUE;
        }
    }
    else if ( pdu->code == LEATT_OPCODE_ERR_RSP )
    {
        LEATT_PDU_ERR_RSP* response_pdu = (LEATT_PDU_ERR_RSP*)pdu;

        if ( response_pdu->errCode == LEATT_ERR_CODE_INVALID_OFFSET )
        {
            subprocedure.result = WICED_BT_SUCCESS;
        }
        else
        {
            subprocedure.result = WICED_BT_ERROR;
        }

        subprocedure_done = WICED_TRUE;
    }

    if ( subprocedure_done == WICED_TRUE )
    {
        subprocedure_notify_complete( );
    }
    return WICED_BT_SUCCESS;
}

static wiced_result_t write_characteristic_descriptor_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
     if ( pdu->code == LEATT_OPCODE_WRITE_RSP )
     {
         subprocedure.result = WICED_BT_SUCCESS;
     }
     else if ( pdu->code == LEATT_OPCODE_ERR_RSP )
     {
        subprocedure.result = WICED_BT_ERROR;
     }

     subprocedure_notify_complete( );
     return WICED_BT_SUCCESS;
}

static wiced_result_t write_long_characteristic_descriptor_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    wiced_bool_t subprocedure_done = WICED_FALSE;

    if ( pdu->code == LEATT_OPCODE_PREPARE_WRITE_RSP )
    {
        LEATT_PDU_PREPARE_WRITE_RSP_HDR* response_pdu = (LEATT_PDU_PREPARE_WRITE_RSP_HDR*)pdu;
        uint16_t value_length = pdu_length - sizeof( *response_pdu );
        uint16_t new_offset   = response_pdu->valOffset + value_length;

        if ( new_offset < subprocedure.length )
        {
            uint16_t new_length = MIN( ( subprocedure.length - new_offset ), ( LEATT_ATT_MTU - sizeof(LEATT_PDU_PREPARE_WRITE_RSP_HDR) ) );

            bt_smart_att_prepare_write_request( subprocedure.connection_handle, response_pdu->attrHandle, new_offset, &subprocedure.attr_head->value.value[new_offset], new_length );
        }
        else
        {
            bt_smart_att_execute_write_request( subprocedure.connection_handle, BT_SMART_ATT_WRITE_ALL_PENDING_PREPARED_VALUES );
        }
    }
    else if ( pdu->code == LEATT_OPCODE_EXECUTE_WRITE_RSP )
    {
        subprocedure.result = WICED_BT_SUCCESS;
        subprocedure_done   = WICED_TRUE;
    }
    else if ( pdu->code == LEATT_OPCODE_ERR_RSP )
    {

        subprocedure.result = WICED_BT_ERROR;
        subprocedure_done   = WICED_TRUE;
    }

    if ( subprocedure_done == WICED_TRUE )
    {
        subprocedure_notify_complete( );
    }

    return WICED_BT_SUCCESS;
}

static wiced_result_t characteristic_value_notification_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    subprocedure.length = pdu_length;
    subprocedure.pdu    = (bt_smart_att_pdu_t*)pdu;

    return wiced_rtos_send_asynchronous_event( WICED_NETWORKING_WORKER_THREAD, notification_indication_app_cb, (void*)packet );
}

static wiced_result_t characteristic_value_indication_cb( bt_packet_t* packet, uint16_t pdu_length, const bt_smart_att_pdu_t* pdu )
{
    subprocedure.length = pdu_length;
    subprocedure.pdu    = (bt_smart_att_pdu_t*)pdu;

    /* Send Value Confirmation */
    bt_smart_att_handle_value_confirmation( emconinfo_getConnHandle() );
    return wiced_rtos_send_asynchronous_event( WICED_NETWORKING_WORKER_THREAD, notification_indication_app_cb, (void*)packet );
}

static wiced_result_t notification_indication_app_cb( void* arg )
{
    bt_packet_t* packet = (bt_packet_t*)arg;

    if ( notification_indication_handler != NULL )
    {
        LEATT_PDU_NOTIFICATION_HDR* response_pdu = (LEATT_PDU_NOTIFICATION_HDR*)subprocedure.pdu;
        uint8_t*                    data         = (uint8_t*)( response_pdu + 1 );

        notification_indication_handler( emconinfo_getConnHandle(), response_pdu->handle, data, subprocedure.length - 3 );
    }

    return bt_packet_pool_free_packet( packet );
}

static wiced_result_t subprocedure_timeout_handler( void )
{
    if ( subprocedure.subprocedure != GATT_SUBPROCEDURE_NONE )
    {
        subprocedure.result = WICED_BT_GATT_TIMEOUT;
        subprocedure_notify_complete();
    }

    return WICED_BT_SUCCESS;
}
#endif
