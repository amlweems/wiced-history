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

#define SOCKET_STATE_DISCONNECTED              ( 0 )
#define SOCKET_STATE_LINK_CONNECTED            ( 1 )
#define SOCKET_STATE_LINK_ENCRYPTED            ( 2 )

#define SOCKET_ACTION_HOST_CONNECT             ( 1 << 0 )
#define SOCKET_ACTION_HOST_DISCONNECT          ( 1 << 2 )
#define SOCKET_ACTION_INITIATE_PAIRING         ( 1 << 3 )
#define SOCKET_ACTION_ENCRYPT_USING_BOND_INFO  ( 1 << 4 )

#define SOCKET_INVALID_CONNECTION_HANDLE       ( 0xFFFF )

#define MAX_CONNECTION_TIMEOUT                 ( 10000 )

#define NODE_BUFFER_LENGTH           (14)
#define SERVICE_BUFFER_LENGTH        (9)
#define CHARACTERISTIC_BUFFER_LENGTH (13)
#define DESCRIPTOR_BUFFER_LENGTH     (5)
#define UUID_BUFFER_LENGTH           (33)
#define BD_ADDR_BUFFER_LENGTH        (13)


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


wiced_result_t smartbridge_gatt_indication_notification_handler ( uint16_t connection_handle, uint16_t attribute_handle, uint8_t* data, uint16_t length );
static wiced_result_t smartbridge_app_notification_handler             ( void* arg );
static wiced_result_t smartbridge_app_disconnection_handler            ( void* arg );
//static wiced_result_t smartbridge_app_pairing_handler                  ( void* arg );
//static wiced_result_t smartbridge_gap_connection_handler               ( bt_smart_gap_connection_event_t event, wiced_bt_device_address_t* address, wiced_bt_smart_address_type_t type, uint16_t connection_handle );
//static wiced_result_t smartbridge_gap_bonding_handler                  ( uint16_t connection_handle, const wiced_bt_smart_bond_info_t* bond_info );

/******************************************************
 *               Variable Definitions
 ******************************************************/

extern volatile uint16_t current_service_handle_end;
extern gatt_subprocedure_t subprocedure;
static wiced_bt_smartbridge_socket_t* connecting_socket = NULL;
static wiced_bool_t                   initialised       = WICED_FALSE;
extern wiced_bool_t bt_initialised;


wiced_timer_t scan_complete_timer;

wiced_bt_gatt_char_declaration_t        current_characteristic;
wiced_bt_gatt_char_declaration_t        previous_characteristic;

/******************************************************
 *               Function Definitions
 ******************************************************/

void device_address_to_string( wiced_bt_device_address_t* device_address, char* string )
{
    uint8_t a;

    for ( a = 0; a < 6; a++ )
    {
        unsigned_to_hex_string( (uint32_t)device_address[a], string, 2, 2 );
        string += 2;
    }
}

void uuid_to_string( const wiced_bt_uuid_t* uuid, char* string )
{
    int8_t   i;
    uint8_t  j;
    uint8_t* uuid_iter = (uint8_t*)&uuid->uu;

    for ( i = uuid->len - 1, j = 0; i >= 0; i--, j += 2 )
    {
        unsigned_to_hex_string( (uint32_t)uuid_iter[i], &string[j], 2, 2 );
    }
}

void format_node_string( char* output, wiced_bt_device_address_t* address, wiced_bt_ble_address_type_t type )
{
    device_address_to_string( address, output );
    unsigned_to_hex_string( (uint32_t)type, output + 12, 1, 1 );
}

void format_service_string( char* output, uint16_t start_handle, uint16_t end_handle )
{
    unsigned_to_hex_string( (uint32_t)end_handle,   &output[0], 4 , 4 );
    unsigned_to_hex_string( (uint32_t)start_handle, &output[4], 4 , 4 );
}

void format_characteristic_string( char* output, uint16_t start_handle, uint16_t end_handle, uint16_t value_handle )
{
    unsigned_to_hex_string( (uint32_t)end_handle,   &output[0], 4, 4 );
    unsigned_to_hex_string( (uint32_t)value_handle, &output[4], 4, 4 );
    unsigned_to_hex_string( (uint32_t)start_handle, &output[8], 4, 4 );
}

static volatile uint32_t                current_procedure_count = 0;

static void smartbridge_gatt_connection_handler( uint16_t connection_handle )
{
    WPRINT_LIB_INFO(("[Smartbridge] GATT connection was SUCCESS\n"));
    /* Update connection handle and state of the socket */
    connecting_socket->state = SOCKET_STATE_LINK_CONNECTED;

    connecting_socket->connection_handle = connection_handle;

    /* Add socket to the connected list */
    bt_smartbridge_socket_manager_insert_socket( connecting_socket );

    /* Notify app thread that link is connected */
    wiced_rtos_set_semaphore( &connecting_socket->semaphore );
}

static void smartbridge_gatt_disconnection_handler( uint16_t connection_handle )
{
    WPRINT_LIB_INFO( ( "[Smartbridge] GATT disconnection\n" ) );

    wiced_bt_smartbridge_socket_t* removed_socket = NULL;

    /* Remove socket from the connected list */
    if ( bt_smartbridge_socket_manager_remove_socket( connection_handle, &removed_socket ) == WICED_BT_SUCCESS )
    {
        /* Reset connection handle to invalid value */
        removed_socket->connection_handle = SOCKET_INVALID_CONNECTION_HANDLE;

        /* Reset socket state */
        removed_socket->state = SOCKET_STATE_DISCONNECTED;

        /* Mark att cache as inactive and reset reference to cache */
        bt_smartbridge_att_cache_set_active_state( (bt_smartbridge_att_cache_t*)removed_socket->att_cache, WICED_FALSE );
        removed_socket->att_cache = NULL;

        /* Check if disconnection is from host or remote device */
        if ( smartbridge_helper_socket_check_actions_enabled( removed_socket, SOCKET_ACTION_HOST_DISCONNECT ) == WICED_TRUE )
        {
            /* Disconnection is originated from the host. Notify app thread that disconnection is complete */
            wiced_rtos_set_semaphore( &removed_socket->semaphore );
        }
        else
        {
            /* Notify app that connection is disconnected by the remote device */
            if ( removed_socket->disconnection_callback != NULL )
            {
                wiced_rtos_send_asynchronous_event( WICED_NETWORKING_WORKER_THREAD, smartbridge_app_disconnection_handler, (void*)removed_socket );
            }

            /* If disconnection happens when connection is still being established. Notify app */
            if ( connecting_socket == removed_socket )
            {
                wiced_rtos_set_semaphore( &connecting_socket->semaphore );
            }
        }
    }
    else
    {
        /* If disconnection happens when connection is still being established. Notify app */
        if ( connecting_socket != NULL )
        {
            wiced_rtos_set_semaphore( &connecting_socket->semaphore );
        }
    }
}

static void smartbridge_gatt_read_operation_complete_handler( wiced_bt_gatt_data_t* response_data )
{
    wiced_bt_smart_attribute_t* attr;
    uint8_t *data = NULL;
        uint32_t i;
    /* Create a new attribute */
    wiced_bt_smart_attribute_create( &attr, WICED_ATTRIBUTE_TYPE_CHARACTERISTIC_VALUE, response_data->len );

    if ( attr != NULL )
    {
        attr->next         = NULL;
        attr->handle       = subprocedure.start_handle;
        attr->type.len     = subprocedure.uuid.len;
        attr->type.uu.uuid16 = subprocedure.uuid.uu.uuid16;
        data               = response_data->p_data;
        attr->value_length = response_data->len;

        if( subprocedure.uuid.len == UUID_16BIT )
        {
            attr->type.uu.uuid16 = subprocedure.uuid.uu.uuid16;
        }
        else if( subprocedure.uuid.len == UUID_128BIT )
        {
            memcpy( attr->type.uu.uuid128, subprocedure.uuid.uu.uuid128,  UUID_128BIT );
        }

        memcpy( attr->value.characteristic_value.value, data, attr->value_length );

        //memcpy( &attr->type, &subprocedure.uuid, sizeof(wiced_bt_uuid_t));

        /* Update temporary variables */
        if ( subprocedure.attr_head == NULL )
        {
            subprocedure.attr_head = attr;
        }
        subprocedure.result = WICED_BT_SUCCESS;

        WPRINT_LIB_INFO( ( "[Smartbridge] Read value: UUID:%x length:%d\n", attr->type.uu.uuid16, attr->type.len ) );
        for ( i = 0; i < attr->value_length; i ++ )
        {
            WPRINT_LIB_INFO( ( "%02x ", (int)attr->value.characteristic_value.value[i] ) );
        }
        WPRINT_LIB_INFO( ( "\n" ) );
    }

    else
    {
        subprocedure.result = WICED_BT_OUT_OF_HEAP_SPACE;
    }

}

void smartbridge_gatt_discover_characteristic_descriptor_result(  wiced_bt_gatt_event_data_t *p_event_data )
{
    char  descriptor[DESCRIPTOR_BUFFER_LENGTH];
    char  uuid      [UUID_BUFFER_LENGTH];

    if ( current_procedure_count == 0 )
    {
        //restful_gateway_write_status_code( current_stream, BT_REST_GATEWAY_STATUS_200 );
        //restful_gateway_write_descriptor_array_start( current_stream );
    }

    unsigned_to_hex_string( (uint32_t)p_event_data->discovery_result.discovery_data.char_descr_info.handle, descriptor, 4, 4 );
    WPRINT_LIB_INFO(("Descriptor +++ %s\n", descriptor ));
    uuid_to_string( &p_event_data->discovery_result.discovery_data.char_descr_info.type, uuid );
    WPRINT_LIB_INFO(("UUID +++ %s\n", uuid ));
    //restful_gateway_write_descriptor( current_stream, node, descriptor, data->discovery_result.discovery_data.char_descr_info.handle, uuid );
    current_procedure_count++;
}

void smartbrige_gatt_discover_services_result(  wiced_bt_gatt_event_data_t *p_event_data )
{
    /* Create attribute(s) based on information included in the response PDU */
    wiced_bt_smart_attribute_t* attr;

    wiced_bt_smart_attribute_create( &attr, WICED_ATTRIBUTE_TYPE_PRIMARY_SERVICE, 0 );

    if ( attr != NULL )
    {
        attr->next                          = NULL;
        attr->type.len                      = UUID_16BIT;
        attr->type.uu.uuid16                = GATT_UUID_PRI_SERVICE;
        attr->value_length                  = 2;
        attr->handle                        = p_event_data->discovery_result.discovery_data.group_value.s_handle;

        attr->value.service.start_handle    = attr->handle;
        attr->value.service.end_handle      = p_event_data->discovery_result.discovery_data.group_value.e_handle;

        memcpy( &attr->value.service.uuid, &p_event_data->discovery_result.discovery_data.group_value.service_type, sizeof(wiced_bt_uuid_t) );

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
        WPRINT_LIB_INFO(("[SmartBridge] Service s_handle:%x end_handle:%x uuid:%x list-count:%u\n", attr->handle, attr->value.service.end_handle, attr->value.service.uuid.uu.uuid16, (unsigned int)subprocedure.attr_count));
    }
}

void smartbridge_gatt_discover_characteristic_result(  wiced_bt_gatt_event_data_t *p_event_data )
{
    /* Create attribute(s) based on information included in the response PDU */
    wiced_bt_smart_attribute_t* attr;

    wiced_bt_smart_attribute_create( &attr, WICED_ATTRIBUTE_TYPE_CHARACTERISTIC, 0 );

    memcpy( &current_characteristic, &p_event_data->discovery_result.discovery_data.characteristic_declaration, sizeof( current_characteristic ) );

    if ( attr != NULL )
    {
        attr->next                      = NULL;
        attr->handle                    = current_characteristic.handle;
        attr->type.len                  = UUID_16BIT;
        attr->type.uu.uuid16            = GATT_UUID_CHAR_DECLARE;
        attr->value_length              = 2;
        attr->value.characteristic.properties = current_characteristic.characteristic_properties;

        attr->value.characteristic.value_handle = current_characteristic.val_handle;

        memcpy( &attr->value.characteristic.uuid, &current_characteristic.char_uuid, sizeof(wiced_bt_uuid_t) );

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
        WPRINT_LIB_INFO(("[SmartBridge] Characteristic value_handle:%x handle:%x uuid:%x list-count:%u properties:%u\n",current_characteristic.val_handle, current_characteristic.handle,current_characteristic.char_uuid.uu.uuid16, (unsigned int)subprocedure.attr_count, (int)attr->value.characteristic.properties ));
    }
}

void smartbridge_gatt_discovery_complete_handler( wiced_bt_gatt_event_data_t *p_event_data )
{
    if ( ( p_event_data->discovery_complete.disc_type == GATT_DISCOVER_SERVICES_ALL || p_event_data->discovery_complete.disc_type == GATT_DISCOVER_SERVICES_BY_UUID ) )
    {
        if ( subprocedure.attr_count != 0 )
        {
            subprocedure.result = WICED_BT_SUCCESS;
        }
        else
        {
            subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
        }
        WPRINT_LIB_INFO(("[SmartBridge] Discovery Complete for Services %d type:%d\n", subprocedure.result, p_event_data->discovery_complete.disc_type));
        subprocedure_notify_complete( );
    }
    else if ( ( p_event_data->discovery_complete.disc_type == GATT_DISCOVER_CHARACTERISTICS ) )
    {
        if ( subprocedure.attr_count != 0 )
        {
            subprocedure.result = WICED_BT_SUCCESS;
        }
        else
        {
            subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
        }

        WPRINT_LIB_INFO( ("[SmartBridge] Discovery Complete for Characteristics[result: %d]\n", subprocedure.result ) );
        subprocedure_notify_complete( );
    }
    else if ( ( p_event_data->discovery_complete.disc_type == GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS ) )
    {
        if ( subprocedure.attr_count != 0 )
        {
            subprocedure.result = WICED_BT_SUCCESS;
        }
        else
        {
            subprocedure.result = WICED_BT_ITEM_NOT_IN_LIST;
        }

        WPRINT_LIB_INFO( ("[SmartBridge] Discovery Complete for Characteristic-Descriptors [result: %d]\n", subprocedure.result ) );
        subprocedure_notify_complete( );
    }
}


wiced_bt_gatt_status_t smartbridge_gatt_callback( wiced_bt_gatt_evt_t event, wiced_bt_gatt_event_data_t *p_event_data )
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    switch(event)
    {
        case GATT_CONNECTION_STATUS_EVT:
        {
            /* Connection */
            if ( p_event_data->connection_status.connected == WICED_TRUE )
            {
                smartbridge_gatt_connection_handler( p_event_data->connection_status.conn_id );
            }
            else
            {
                smartbridge_gatt_disconnection_handler( p_event_data->connection_status.conn_id );
            }
            break;
        }

        case GATT_ATTRIBUTE_REQUEST_EVT:
        {
            WPRINT_LIB_INFO(("[SmartBridge] Gatt attribute status event\n"));
            break;
        }

        case GATT_OPERATION_CPLT_EVT:
        {
            if(p_event_data != NULL )
            {
                if ( p_event_data->operation_complete.op == GATTC_OPTYPE_READ )
                {
                    smartbridge_gatt_read_operation_complete_handler( &p_event_data->operation_complete.response_data.att_value );
                }

                else if ( p_event_data->operation_complete.op == GATTC_OPTYPE_WRITE )
                {
                    WPRINT_LIB_INFO(("[SmartBridge] Gatt attribute Write-Callback event\n"));
                }
                else if ( p_event_data->operation_complete.op == GATTC_OPTYPE_NOTIFICATION)
                {
                }
                subprocedure_notify_complete( );
            }
            break;
        }

        case GATT_DISCOVERY_RESULT_EVT:
        {
            if( p_event_data != NULL )
            {
                if ( p_event_data->discovery_result.discovery_type == GATT_DISCOVER_SERVICES_ALL || p_event_data->discovery_result.discovery_type == GATT_DISCOVER_SERVICES_BY_UUID )
                {
                    smartbrige_gatt_discover_services_result(p_event_data);
                }
                else if ( p_event_data->discovery_result.discovery_type == GATT_DISCOVER_CHARACTERISTICS )
                {
                    smartbridge_gatt_discover_characteristic_result( p_event_data);
                }
                else if ( p_event_data->discovery_result.discovery_type == GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS )
                {
                    smartbridge_gatt_discover_characteristic_descriptor_result( p_event_data );
                }
            }
            break;
        }

        case GATT_DISCOVERY_CPLT_EVT:
        {
            if ( ( p_event_data != NULL ) )
            {
                smartbridge_gatt_discovery_complete_handler( p_event_data );
            }
            break;
        }

        default:
        {
            WPRINT_LIB_INFO(("[SmartBridge] Gatt callback event:%d\n", event));
            break;
        }
    }

    return status;
}

wiced_result_t wiced_bt_smartbridge_init( void )
{
    wiced_result_t result;

    if ( initialised == WICED_TRUE )
    {
        return WICED_BT_SUCCESS;
    }

    WPRINT_LIB_INFO( ("[SmartBridge] Initialising WICED SmartBridge ...\n" ) );

    bt_smart_gatt_init();

    /* Initialise SmartBridge Socket Manager */
    result = bt_smartbridge_socket_manager_init();
    if ( result != WICED_BT_SUCCESS )
    {
        WPRINT_LIB_INFO( ( "Error initialising SmartBridge Socket Manager\n" ) );
        return result;
    }


    initialised = WICED_TRUE;
    return WICED_BT_SUCCESS;
}

wiced_result_t wiced_bt_smartbridge_deinit( void )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SUCCESS;
    }

    /* Disable Attribute Cache (The function checks if it's enabled) */
    bt_smartbridge_att_cache_disable();
    /* Deinitialise socket manager */
    bt_smartbridge_socket_manager_deinit();

    /* Deinitialise GATT */
    bt_smart_gatt_deinit();

    bt_smart_gatt_register_notification_indication_handler( NULL );

    initialised = WICED_FALSE;

    return WICED_BT_SUCCESS;
}

wiced_bool_t wiced_bt_smartbridge_is_scanning( void )
{
    return ( initialised == WICED_TRUE ) ? smartbridge_bt_interface_is_scanning( ) : WICED_FALSE;
}

wiced_result_t wiced_bt_smartbridge_set_max_concurrent_connections( uint8_t count )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    /* Set socket manager's connections limit */
    bt_smartbridge_socket_manager_set_max_concurrent_connections( count );

    return smartbridge_bt_interface_set_max_concurrent_connections(count);
}

wiced_bool_t   wiced_bt_smartbridge_is_ready_to_connect( void )
{
    return ( initialised == WICED_FALSE || connecting_socket != NULL ) ? WICED_FALSE : WICED_TRUE;
}

wiced_result_t wiced_bt_smartbridge_start_scan( const wiced_bt_smart_scan_settings_t* settings, wiced_bt_smart_scan_complete_callback_t complete_callback, wiced_bt_smart_advertising_report_callback_t advertising_report_callback )
{

    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    return smartbridge_bt_interface_start_scan( settings, complete_callback, advertising_report_callback );
}

wiced_result_t wiced_bt_smartbridge_stop_scan( void )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }
    return smartbridge_bt_interface_stop_scan();
}

wiced_result_t wiced_bt_smartbridge_get_scan_result_list( wiced_bt_smart_scan_result_t** result_list, uint32_t* count )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }
    return smartbridge_helper_get_scan_results( result_list, count );
}

wiced_result_t wiced_bt_smartbridge_add_device_to_whitelist( const wiced_bt_device_address_t* device_address, wiced_bt_smart_address_type_t address_type )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    return smartbridge_bt_interface_add_device_to_whitelist( device_address, address_type );
}

wiced_result_t wiced_bt_smartbridge_remove_device_from_whitelist( const wiced_bt_device_address_t* device_address, wiced_bt_smart_address_type_t address_type )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    return smartbridge_bt_interface_remove_device_from_whitelist( device_address, address_type );
}

wiced_result_t wiced_bt_smartbridge_get_whitelist_size( uint32_t* size )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    return smartbridge_bt_interface_get_whitelist_size( size );
}

wiced_result_t wiced_bt_smartbridge_clear_whitelist( void )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    return smartbridge_bt_interface_clear_whitelist( );
}

wiced_result_t wiced_bt_smartbridge_create_socket( wiced_bt_smartbridge_socket_t* socket )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    /* Reset socket fields */
    memset( socket, 0, sizeof( *socket ) );
    socket->connection_handle = SOCKET_INVALID_CONNECTION_HANDLE;

    /* Point node data to socket */
    socket->node.data = (void*)socket;

    /* Initialise socket semaphore */
    return wiced_rtos_init_semaphore( &socket->semaphore );
}

wiced_result_t wiced_bt_smartbridge_delete_socket( wiced_bt_smartbridge_socket_t* socket )
{
    wiced_result_t result;
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    result = wiced_rtos_deinit_semaphore( &socket->semaphore );
    if ( result != WICED_BT_SUCCESS )
    {
        return result;
    }

    memset( socket, 0, sizeof( *socket ) );
    socket->connection_handle = SOCKET_INVALID_CONNECTION_HANDLE;
    return WICED_BT_SUCCESS;
}

wiced_result_t wiced_bt_smartbridge_get_socket_status( wiced_bt_smartbridge_socket_t* socket, wiced_bt_smartbridge_socket_status_t* status )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    if ( socket->state == SOCKET_STATE_LINK_ENCRYPTED )
    {
        *status = SMARTBRIDGE_SOCKET_CONNECTED;
    }
    else if ( socket->state == SOCKET_STATE_LINK_CONNECTED )
    {
        /* Status is connected if socket does not have loaded bond info and does not initiate pairing */
        if ( smartbridge_helper_socket_check_actions_disabled( socket, SOCKET_ACTION_ENCRYPT_USING_BOND_INFO | SOCKET_ACTION_INITIATE_PAIRING ) == WICED_TRUE )
        {
            *status = SMARTBRIDGE_SOCKET_CONNECTED;
        }
        else
        {
            *status = SMARTBRIDGE_SOCKET_CONNECTING;
        }
    }
    else
    {
        *status = SMARTBRIDGE_SOCKET_DISCONNECTED;
    }

    return WICED_BT_SUCCESS;
}

wiced_result_t wiced_bt_smartbridge_connect( wiced_bt_smartbridge_socket_t* socket, const wiced_bt_smart_device_t* remote_device, const wiced_bt_smart_connection_settings_t* settings, wiced_bt_smartbridge_disconnection_callback_t disconnection_callback, wiced_bt_smartbridge_notification_callback_t notification_callback )
{
    wiced_bt_smartbridge_socket_t* found_socket;

    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    if ( connecting_socket != NULL )
    {
        /* Only 1 connecting socket is allowed */
        return WICED_BT_CONNECT_IN_PROGRESS;
    }

    if ( bt_smartbridge_socket_manager_is_full() == WICED_TRUE )
    {
        return WICED_BT_MAX_CONNECTIONS_REACHED;
    }

    if ( bt_smartbridge_socket_manager_find_socket_by_address( &remote_device->address, &found_socket ) == WICED_BT_SUCCESS )
    {
        /* device is already connected */
        return WICED_BT_SOCKET_IN_USE;
    }

    WPRINT_LIB_INFO(("[Smartbridge] connect()...socket things are okay\n"));

    /* Clean-up accidentally set semaphores */
    while( wiced_rtos_get_semaphore( &socket->semaphore, WICED_NO_WAIT ) == WICED_BT_SUCCESS )
    {
    }

    /* Store socket pointer in a temporary global variable so it can be referenced in smartbridge_gap_connection_handler */
    connecting_socket = socket;

    /* Store connection settings */
    memcpy( &socket->connection_settings, settings, sizeof( *settings ) );

    /* Store remote device information */
    memcpy( &socket->remote_device, remote_device, sizeof( *remote_device ) );

    /* Set callback functions */
    socket->disconnection_callback = disconnection_callback;
    socket->notification_callback  = notification_callback;

    /* Reset connection handle to invalid value */
    socket->connection_handle = SOCKET_INVALID_CONNECTION_HANDLE;

    /* Reset state */
    socket->state = SOCKET_STATE_DISCONNECTED;

#if 0
    if ( smartbridge_socket_check_actions_enabled( socket, SOCKET_ACTION_INITIATE_PAIRING ) == WICED_TRUE )
    {
        /* Register callback to let GAP notify SmartBridge if bond is successfully created */
        bt_smart_gap_register_bonding_callback( smartbridge_gap_bonding_handler );

        /* Tell GAP to initiate pairing on the next connection attempt */
        bt_smart_gap_enable_pairing( &socket->security_settings, socket->passkey );
    }
    else
    {
        /* Reset GAP bonding callback */
        bt_smart_gap_register_bonding_callback( NULL );

        /* Tell GAP to not send pairing request on the next connection attempt */
        bt_smart_gap_disable_pairing();
    }

    if ( smartbridge_socket_check_actions_enabled( socket, SOCKET_ACTION_ENCRYPT_USING_BOND_INFO ) == WICED_TRUE )
    {
        /* Tell GAP to enable encryption using bond info and security settings provided */
        bt_smart_gap_set_bond_info( &socket->security_settings, &socket->bond_info );
    }
    else
    {
        /* Tell GAP to clear bond info and disable encryption */
        bt_smart_gap_clear_bond_info();
    }
#endif

    /* Set socket action to connecting */
    smartbridge_helper_socket_set_actions( socket, SOCKET_ACTION_HOST_CONNECT );

    /* Set attribute protocol timeout */
    smartbridge_bt_interface_set_attribute_timeout( settings->attribute_protocol_timeout_ms );

    smartbridge_bt_interface_connect( remote_device, settings, disconnection_callback, notification_callback );

    /* Wait for connection */
    wiced_rtos_get_semaphore( &socket->semaphore, WICED_NEVER_TIMEOUT );

#if 0
    /* Check if link is connected. Otherwise, return error */
    if ( socket->state == SOCKET_STATE_LINK_CONNECTED )
    {
        /* Check if encryption is required */
        if ( smartbridge_socket_check_actions_enabled( socket, SOCKET_ACTION_INITIATE_PAIRING ) == WICED_TRUE ||
             smartbridge_socket_check_actions_enabled( socket, SOCKET_ACTION_ENCRYPT_USING_BOND_INFO ) == WICED_TRUE )
        {
            /* Wait until link is encrypted */
            wiced_rtos_get_semaphore( &socket->semaphore, WICED_NEVER_TIMEOUT );

            if ( socket->state != SOCKET_STATE_LINK_ENCRYPTED )
            {
                result = WICED_BT_ENCRYPTION_FAILED;
                goto error;
            }
        }
    }
    else
    {
        result = WICED_BT_SOCKET_NOT_CONNECTED;
        goto error;
    }


#endif

    /* Successful */
    if ( bt_smartbridge_att_cache_is_enabled() == WICED_TRUE )
    {
        wiced_result_t result;
        bt_smartbridge_att_cache_t* cache = NULL;

        result = bt_smartbridge_att_cache_find( remote_device, &cache );
        if ( result == WICED_BT_SUCCESS )
        {
            WPRINT_LIB_INFO(( "[SmartBridge]USING ATT CACHE ...\n" ));
        }
        else
        {
            WPRINT_LIB_INFO(( "[SmartBridge]GENERATING ATT CACHE ...\n" ));

            result = bt_smartbridge_att_cache_generate( remote_device, socket->connection_handle, &cache );

            if ( result != WICED_BT_SUCCESS )
            {
                WPRINT_LIB_INFO(("[SmartBridge] Error Generating Cache result:%d\n", result));
            }
        }

        /* Successful. Mark cache as active and store reference in socket */
        bt_smartbridge_att_cache_set_active_state( cache, WICED_TRUE );
        socket->att_cache = (void*)cache;
    }

    /* Clear connect action as it's no longer needed */
    smartbridge_helper_socket_clear_actions( socket, SOCKET_ACTION_HOST_CONNECT );

    /* Reset connecting socket pointer */
    connecting_socket = NULL;

    /* Link is connected. Return success */
    return WICED_BT_SUCCESS;

#if 0
    error:
    /* Link is not connected nor encrypted. Issue disconnection attempt to clean-up */
    wiced_bt_smartbridge_disconnect( socket );

    /* Clear connect action as it's no longer needed */
    smartbridge_helper_socket_clear_actions( socket, SOCKET_ACTION_HOST_CONNECT );

    /* Reset connecting socket pointer */
    connecting_socket = NULL;

    /* Link is not connected. Return error */
    return result;
#endif
}

wiced_result_t wiced_bt_smartbridge_disconnect( wiced_bt_smartbridge_socket_t* socket )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    /* Mark disconnection flag that it's coming from the host */
    smartbridge_helper_socket_set_actions( socket, SOCKET_ACTION_HOST_DISCONNECT );

    /* Clean-up accidentally set semaphores */
    while( wiced_rtos_get_semaphore( &socket->semaphore, WICED_NO_WAIT ) == WICED_BT_SUCCESS )
    {
    }

    /* Check if either link is encrypted or connected */
    if ( socket->state >= SOCKET_STATE_LINK_CONNECTED )
    {
        smartbridge_bt_interface_disconnect( socket->connection_handle );
        /* Wait for disconnection */
        wiced_rtos_get_semaphore( &socket->semaphore, socket->connection_settings.timeout_second );
    }
    else
    {
        /* Link is not yet connected. Cancel last */
        smartbridge_bt_interface_cancel_last_connect( socket->remote_device.address );
    }

    /* Clear socket disconnect action */
    smartbridge_helper_socket_set_actions( socket, SOCKET_ACTION_HOST_DISCONNECT );

    /* Proper clean-up if socket isn't properly disconnected */
    if ( socket->state != SOCKET_STATE_DISCONNECTED )
    {
        wiced_bt_smartbridge_socket_t* removed_socket;

        bt_smartbridge_socket_manager_remove_socket( socket->connection_handle, &removed_socket );

        /* Reset connection handle to invalid value */
        socket->connection_handle = SOCKET_INVALID_CONNECTION_HANDLE;

        /* Clear socket state */
        socket->state = SOCKET_STATE_DISCONNECTED;

        /* Mark att cache as inactive and reset reference to cache */
        bt_smartbridge_att_cache_set_active_state( (bt_smartbridge_att_cache_t*)socket->att_cache, WICED_FALSE );
        socket->att_cache = NULL;
    }

    return WICED_BT_SUCCESS;
}

wiced_result_t wiced_bt_smartbridge_set_transmit_power( wiced_bt_smartbridge_socket_t* socket, int8_t transmit_power_dbm )
{
    if ( initialised == WICED_FALSE || socket->state == SOCKET_STATE_DISCONNECTED )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    return smartbridge_bt_interface_set_connection_tx_power( socket->connection_handle, transmit_power_dbm );
}

wiced_result_t wiced_bt_smartbridge_set_bond_info( wiced_bt_smartbridge_socket_t* socket, const wiced_bt_smart_security_settings_t* settings, const wiced_bt_smart_bond_info_t* bond_info )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    /* Set local copies of security settings and bond info */
    memcpy( &socket->bond_info, bond_info, sizeof( *bond_info ) );
    memcpy( &socket->security_settings, settings, sizeof( *settings ) );

    /* Clear socket action to initiate pairing request */
    smartbridge_helper_socket_clear_actions( socket, SOCKET_ACTION_INITIATE_PAIRING );

    /* Set socket action to encrypt using loaded bond info */
    smartbridge_helper_socket_set_actions( socket, SOCKET_ACTION_ENCRYPT_USING_BOND_INFO );

    return WICED_BT_SUCCESS;
}

wiced_result_t wiced_bt_smartbridge_clear_bond_info( wiced_bt_smartbridge_socket_t* socket )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    /* Reset bond info */
    memset( &socket->bond_info, 0, sizeof( socket->bond_info ) );

    /* Clear socket action to encrypt using loaded bond info */
    smartbridge_helper_socket_clear_actions( socket, SOCKET_ACTION_ENCRYPT_USING_BOND_INFO );

    return WICED_BT_SUCCESS;
}

wiced_result_t wiced_bt_smartbridge_enable_pairing( wiced_bt_smartbridge_socket_t* socket, const wiced_bt_smart_security_settings_t* settings, const char* numeric_passkey, wiced_bt_smartbridge_bonding_callback_t bonding_callback )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    /* Store security settings in local copy */
    memcpy( &socket->security_settings, settings, sizeof( *settings ) );

    /* Reset bond info */
    memset( &socket->bond_info, 0, sizeof( socket->bond_info ) );

    /* Store passkey */
    memset( socket->passkey, 0, sizeof( socket->passkey ) );
    if ( numeric_passkey != NULL )
    {
        memcpy( socket->passkey, numeric_passkey, strnlen( numeric_passkey, sizeof( socket->passkey ) - 1 ) );
    }

    /* Set pairing callback */
    socket->bonding_callback = bonding_callback;

    /* Clear socket action to encrypt using loaded bond info */
    smartbridge_helper_socket_clear_actions( socket, SOCKET_ACTION_ENCRYPT_USING_BOND_INFO );

    /* Set socket action to iniatiate pairing request */
    smartbridge_helper_socket_set_actions( socket, SOCKET_ACTION_INITIATE_PAIRING );

    return WICED_BT_SUCCESS;
}

wiced_result_t wiced_bt_smartbridge_disable_pairing( wiced_bt_smartbridge_socket_t* socket )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    /* Clear socket action to iniatiate pairing request */
    smartbridge_helper_socket_clear_actions( socket, SOCKET_ACTION_INITIATE_PAIRING );

    return WICED_BT_SUCCESS;
}

wiced_result_t wiced_bt_smartbridge_enable_attribute_cache( uint32_t cache_count )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    /* Call internal function */
    return bt_smartbridge_att_cache_enable( cache_count );
}

wiced_result_t wiced_bt_smartbridge_disable_attribute_cache( void )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    /* Call internal function */
    return bt_smartbridge_att_cache_disable();
}

wiced_result_t wiced_bt_smartbridge_enable_attribute_cache_notification( wiced_bt_smartbridge_socket_t* socket )
{
    wiced_bt_smart_attribute_list_t* list;
    wiced_bt_smart_attribute_t*      iterator;
    bt_smartbridge_att_cache_t*      cache;
    wiced_result_t                   result;

    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    if ( bt_smartbridge_att_cache_is_enabled( ) == WICED_FALSE )
    {
        return WICED_BT_ATT_CACHE_UNINITIALISED;
    }

    if ( socket == NULL || socket->att_cache == NULL )
    {
        return WICED_BT_BADARG;
    }

    cache = (bt_smartbridge_att_cache_t*)socket->att_cache;

    if ( bt_smartbridge_att_cache_is_discovering( cache ) == WICED_TRUE )
    {
        return WICED_BT_DISCOVER_IN_PROGRESS;
    }

    result = bt_smartbridge_att_cache_get_list( cache, &list );
    if ( result != WICED_BT_SUCCESS )
    {
        return result;
    }

    result = wiced_bt_smart_attribute_get_list_head( list, &iterator );
    if ( result != WICED_BT_SUCCESS )
    {
        return result;
    }

    while( iterator != NULL )
    {
        if ( iterator->type.uu.uuid16 == 0x2902 )
        {
            /* Swith on notification from server */
            bt_smartbridge_att_cache_lock( cache );

            iterator->value.client_config.config_bits = 1;

            bt_smartbridge_att_cache_unlock( cache );

            result = bt_smart_gatt_write_characteristic_descriptor( socket->connection_handle, iterator );
        }

        iterator = iterator->next;
    }

    return result;
}

wiced_result_t wiced_bt_smartbridge_disable_attribute_cache_notification( wiced_bt_smartbridge_socket_t* socket )
{
    wiced_bt_smart_attribute_list_t* list;
    wiced_bt_smart_attribute_t*      iterator;
    bt_smartbridge_att_cache_t*      cache;
    wiced_result_t                   result;

    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    if ( bt_smartbridge_att_cache_is_enabled( ) == WICED_FALSE )
    {
        return WICED_BT_ATT_CACHE_UNINITIALISED;
    }

    if ( socket == NULL || socket->att_cache == NULL )
    {
        return WICED_BT_BADARG;
    }

    cache = (bt_smartbridge_att_cache_t*)socket->att_cache;

    if ( bt_smartbridge_att_cache_is_discovering( cache ) == WICED_TRUE )
    {
        return WICED_BT_DISCOVER_IN_PROGRESS;
    }

    result = bt_smartbridge_att_cache_get_list( cache, &list );
    if ( result != WICED_BT_SUCCESS )
    {
        return result;
    }

    result = wiced_bt_smart_attribute_get_list_head( list, &iterator );
    if ( result != WICED_BT_SUCCESS )
    {
        return result;
    }

    while( iterator != NULL )
    {
        if ( iterator->type.uu.uuid16 == 0x2902 )
        {
            /* Swith on notification from server */
            bt_smartbridge_att_cache_lock( cache );

            iterator->value.client_config.config_bits = 0;

            bt_smartbridge_att_cache_unlock( cache );

            result = bt_smart_gatt_write_characteristic_descriptor( socket->connection_handle, iterator );
        }

        iterator = iterator->next;
    }

    return result;
}

wiced_result_t wiced_bt_smartbridge_get_attribute_cache_list( wiced_bt_smartbridge_socket_t* socket, wiced_bt_smart_attribute_list_t** att_cache_list )
{
    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    if ( bt_smartbridge_att_cache_is_enabled( ) == WICED_FALSE )
    {
        return WICED_BT_ATT_CACHE_UNINITIALISED;
    }

    if ( socket == NULL || socket->att_cache == NULL || att_cache_list == NULL )
    {
        return WICED_BT_BADARG;
    }

    if ( bt_smartbridge_att_cache_is_discovering( (bt_smartbridge_att_cache_t*)socket->att_cache ) == WICED_TRUE )
    {
        return WICED_BT_DISCOVER_IN_PROGRESS;
    }

    /* Call internal function */
    return bt_smartbridge_att_cache_get_list( (bt_smartbridge_att_cache_t*)socket->att_cache, att_cache_list );
}

wiced_result_t wiced_bt_smartbridge_get_attribute_cache_by_handle( wiced_bt_smartbridge_socket_t* socket, uint16_t handle, wiced_bt_smart_attribute_t* attribute, uint16_t size )
{
    bt_smartbridge_att_cache_t*      cache          = NULL;
    wiced_bt_smart_attribute_list_t* att_cache_list = NULL;
    wiced_bt_smart_attribute_t*      att            = NULL;
    wiced_result_t                   result;

    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    if ( bt_smartbridge_att_cache_is_enabled( ) == WICED_FALSE )
    {
        return WICED_BT_ATT_CACHE_UNINITIALISED;
    }

    if ( socket == NULL || socket->att_cache == NULL || attribute == NULL )
    {
        return WICED_BT_BADARG;
    }

    cache = (bt_smartbridge_att_cache_t*)socket->att_cache;

    if ( bt_smartbridge_att_cache_is_discovering( cache ) == WICED_TRUE )
    {
        return WICED_BT_DISCOVER_IN_PROGRESS;
    }

    bt_smartbridge_att_cache_get_list( cache, &att_cache_list );

    bt_smartbridge_att_cache_lock( cache );

    result = wiced_bt_smart_attribute_search_list_by_handle( att_cache_list, handle, &att );

    if ( result == WICED_BT_SUCCESS )
    {
        if ( att->value_struct_size + ATTR_COMMON_FIELDS_SIZE > size )
        {
            result = WICED_BT_ATTRIBUTE_VALUE_TOO_LONG;
        }
        else
        {
            memcpy( attribute, att, att->value_struct_size + ATTR_COMMON_FIELDS_SIZE );
        }
    }

    bt_smartbridge_att_cache_unlock( cache );
    return result;
}

wiced_result_t wiced_bt_smartbridge_get_attribute_cache_by_uuid( wiced_bt_smartbridge_socket_t* socket, const wiced_bt_uuid_t* uuid, uint16_t starting_handle, uint16_t ending_handle, wiced_bt_smart_attribute_t* attribute, uint32_t size )
{
    bt_smartbridge_att_cache_t*      cache          = NULL;
    wiced_bt_smart_attribute_list_t* att_cache_list = NULL;
    wiced_bt_smart_attribute_t*      att            = NULL;
    wiced_result_t                   result;

    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    if ( bt_smartbridge_att_cache_is_enabled( ) == WICED_FALSE )
    {
        return WICED_BT_ATT_CACHE_UNINITIALISED;
    }

    if ( socket == NULL || socket->att_cache == NULL || uuid == NULL || attribute == NULL )
    {
        return WICED_BT_BADARG;
    }

    cache = (bt_smartbridge_att_cache_t*)socket->att_cache;

    if ( bt_smartbridge_att_cache_is_discovering( cache ) == WICED_TRUE )
    {
        return WICED_BT_DISCOVER_IN_PROGRESS;
    }

    bt_smartbridge_att_cache_get_list( cache, &att_cache_list );

    bt_smartbridge_att_cache_lock( cache );

    result = wiced_bt_smart_attribute_search_list_by_uuid( att_cache_list, uuid, starting_handle, ending_handle, &att );
    if ( result == WICED_BT_SUCCESS )
    {
        if ( att->value_struct_size + ATTR_COMMON_FIELDS_SIZE > size )
        {
            result = WICED_BT_ATTRIBUTE_VALUE_TOO_LONG;
        }
        else
        {
            memcpy( attribute, att, att->value_struct_size + ATTR_COMMON_FIELDS_SIZE );
        }
    }

    bt_smartbridge_att_cache_unlock( cache );
    return result;
}

wiced_result_t wiced_bt_smartbridge_refresh_attribute_cache_characteristic_value( wiced_bt_smartbridge_socket_t* socket, uint16_t handle )
{
    bt_smartbridge_att_cache_t*      cache          = NULL;
    wiced_bt_smart_attribute_list_t* att_cache_list = NULL;
    wiced_bt_smart_attribute_t*      current_att    = NULL;
    wiced_bt_smart_attribute_t*      refreshed_att  = NULL;
    wiced_result_t                   result;

    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    if ( bt_smartbridge_att_cache_is_enabled( ) == WICED_FALSE )
    {
        return WICED_BT_ATT_CACHE_UNINITIALISED;
    }

    if ( socket == NULL || socket->att_cache == NULL )
    {
        return WICED_BT_BADARG;
    }

    cache = (bt_smartbridge_att_cache_t*)socket->att_cache;

    if ( bt_smartbridge_att_cache_is_discovering( cache ) == WICED_TRUE )
    {
        return WICED_BT_DISCOVER_IN_PROGRESS;
    }

    bt_smartbridge_att_cache_get_list( cache, &att_cache_list );

    bt_smartbridge_att_cache_lock( cache );

    result = wiced_bt_smart_attribute_search_list_by_handle( att_cache_list, handle, &current_att );
    if ( result == WICED_BT_SUCCESS )
    {
        /* Check if length is longer than what read characteristic value can handle
         * If longer, use read long characteristic value
         */
        if ( current_att->value_length <= ATT_STANDARD_VALUE_LENGTH )
        {
            result = smartbridge_bt_interface_read_characteristic_value( socket->connection_handle, current_att->handle, &current_att->type, &refreshed_att );
        }
        else
        {
            result = bt_smart_gatt_read_long_characteristic_value( socket->connection_handle, current_att->handle, &current_att->type, &refreshed_att );
        }

        /* If read is successful, replace attribute with the refreshed one
         */
        if ( result == WICED_BT_SUCCESS )
        {
            /* This function removes and also deletes the attribute with handle specified */
            result = wiced_bt_smart_attribute_remove_from_list( att_cache_list, current_att->handle );
            if ( result == WICED_BT_SUCCESS )
            {
                result = wiced_bt_smart_attribute_add_to_list( att_cache_list, refreshed_att );
            }
        }
    }

    bt_smartbridge_att_cache_unlock( cache );
    return result;
}

wiced_result_t wiced_bt_smartbridge_write_attribute_cache_characteristic_value( wiced_bt_smartbridge_socket_t* socket, const wiced_bt_smart_attribute_t* char_value )
{
    bt_smartbridge_att_cache_t*      cache          = NULL;
    wiced_bt_smart_attribute_list_t* att_cache_list = NULL;
    wiced_bt_smart_attribute_t*      att            = NULL;
    wiced_result_t                   result;

    if ( initialised == WICED_FALSE )
    {
        return WICED_BT_SMARTBRIDGE_UNINITIALISED;
    }

    if ( bt_smartbridge_att_cache_is_enabled( ) == WICED_FALSE )
    {
        return WICED_BT_ATT_CACHE_UNINITIALISED;
    }

    if ( socket == NULL || socket->att_cache == NULL || char_value == NULL )
    {
        return WICED_BT_BADARG;
    }

    cache = (bt_smartbridge_att_cache_t*)socket->att_cache;

    if ( bt_smartbridge_att_cache_is_discovering( cache ) == WICED_TRUE )
    {
        return WICED_BT_DISCOVER_IN_PROGRESS;
    }

    bt_smartbridge_att_cache_get_list( cache, &att_cache_list );


    if ( char_value->value_length <= ATT_STANDARD_VALUE_LENGTH )
    {
        result = smartbridge_bt_interface_write_characteristic_value( socket->connection_handle, (wiced_bt_smart_attribute_t*)char_value );
    }
    else
    {
        result = bt_smart_gatt_write_long_characteristic_value( socket->connection_handle, (wiced_bt_smart_attribute_t*)char_value );
    }

    if ( result != WICED_BT_SUCCESS )
    {
        return result;
    }

    bt_smartbridge_att_cache_lock( cache );

    /* Find characteristic value in local attribute list. Add to the list if not found */

    result = wiced_bt_smart_attribute_search_list_by_handle( att_cache_list, char_value->handle, &att );

    if ( result == WICED_BT_SUCCESS )
    {
        /* Found. Compare lengths first.
         * If new length is not equal old length, replace old attribute with new one.
         * If equal, copy content directly.
         */
        if ( char_value->value_length != att->value_length )
        {
            result = wiced_bt_smart_attribute_remove_from_list( att_cache_list, att->handle );

            if ( result != WICED_BT_SUCCESS )
            {
                goto exit;
            }

            result = wiced_bt_smart_attribute_delete( att );
            att = NULL; /* Reuse attribute pointer */

            if ( result != WICED_BT_SUCCESS )
            {
                goto exit;
            }

            result = wiced_bt_smart_attribute_create( &att, WICED_ATTRIBUTE_TYPE_CHARACTERISTIC_VALUE, char_value->value_length );

            if ( result != WICED_BT_SUCCESS )
            {
                goto exit;
            }

            att->handle            = char_value->handle;
            att->type              = char_value->type;
            att->value_length      = char_value->value_length;
            att->value_struct_size = char_value->value_struct_size;

            memcpy( att->value.value, char_value->value.value, char_value->value_length );

            result = wiced_bt_smart_attribute_add_to_list( att_cache_list, att );
        }
        else
        {
            memcpy( att->value.value, char_value->value.value, char_value->value_length );
        }
    }
    else if ( result == WICED_BT_ITEM_NOT_IN_LIST )
    {
        /* Not found. Create new one and add attribute to the list */
        result = wiced_bt_smart_attribute_create( &att, WICED_ATTRIBUTE_TYPE_CHARACTERISTIC_VALUE, char_value->value_length );

        if ( result != WICED_BT_SUCCESS )
        {
            goto exit;
        }

        att->handle            = char_value->handle;
        att->type              = char_value->type;
        att->value_length      = char_value->value_length;
        att->value_struct_size = char_value->value_struct_size;

        memcpy( att->value.value, char_value->value.value, char_value->value_length );

        result = wiced_bt_smart_attribute_add_to_list( att_cache_list, att );
    }

    exit:
    bt_smartbridge_att_cache_unlock( cache );
    return result;
}

/******************************************************
 *               Callback Definitions
 ******************************************************/
#if 0
static wiced_result_t smartbridge_gap_connection_handler( bt_smart_gap_connection_event_t event, wiced_bt_device_address_t* address, wiced_bt_smart_address_type_t type, uint16_t connection_handle )
{
    switch ( event )
    {
        case GAP_CONNECTED:
        {
            /* Update connection handle and state of the socket */
            connecting_socket->state = SOCKET_STATE_LINK_CONNECTED;
            connecting_socket->connection_handle = connection_handle;

            /* Add socket to the connected list */
            bt_smartbridge_socket_manager_insert_socket( connecting_socket );

            /* Notify app thread that link is connected */
            wiced_rtos_set_semaphore( &connecting_socket->semaphore );
            break;
        }
        case GAP_ENCRYPTION_ENABLED:
        {
            /* Update state of the socket to encrypted */
            connecting_socket->state = SOCKET_STATE_LINK_ENCRYPTED;

            /* Notify app thread that link is already encrypted */
            wiced_rtos_set_semaphore( &connecting_socket->semaphore );
            break;
        }
        case GAP_CONNECTION_TIMEOUT:
        case GAP_ENCRYPTION_FAILED:
        case GAP_ENCRYPTION_TIMEOUT:
        {
            /* Notify app thread that link is already encrypted */
            wiced_rtos_set_semaphore( &connecting_socket->semaphore );
            break;
        }
        case GAP_DISCONNECTED:
        {
            wiced_bt_smartbridge_socket_t* removed_socket = NULL;

            /* Remove socket from the connected list */
            if ( bt_smartbridge_socket_manager_remove_socket( connection_handle, &removed_socket ) == WICED_BT_SUCCESS )
            {
                /* Reset connection handle to invalid value */
                removed_socket->connection_handle = SOCKET_INVALID_CONNECTION_HANDLE;

                /* Reset socket state */
                removed_socket->state = SOCKET_STATE_DISCONNECTED;

                /* Mark att cache as inactive and reset reference to cache */
                bt_smartbridge_att_cache_set_active_state( (bt_smartbridge_att_cache_t*)removed_socket->att_cache, WICED_FALSE );
                removed_socket->att_cache = NULL;

                /* Check if disconnection is from host or remote device */

                if ( smartbridge_helper_socket_check_actions_enabled( removed_socket, SOCKET_ACTION_HOST_DISCONNECT ) == WICED_TRUE )
                {
                    /* Disconnection is originated from the host. Notify app thread that disconnection is complete */
                    wiced_rtos_set_semaphore( &removed_socket->semaphore );
                }
                else
                {
                    /* Notify app that connection is disconnected by the remote device */
                    if ( removed_socket->disconnection_callback != NULL )
                    {
                        wiced_rtos_send_asynchronous_event( WICED_NETWORKING_WORKER_THREAD, smartbridge_app_disconnection_handler, (void*)removed_socket );
                    }

                    /* If disconnection happens when connection is still being established. Notify app */
                    if ( connecting_socket == removed_socket )
                    {
                        wiced_rtos_set_semaphore( &connecting_socket->semaphore );
                    }
                }
            }
            else
            {
                /* If disconnection happens when connection is still being established. Notify app */
                if ( connecting_socket != NULL )
                {
                    wiced_rtos_set_semaphore( &connecting_socket->semaphore );
                }
            }


            break;
        }
        default:
        {
            return WICED_BT_UNKNOWN_EVENT;
        }
    }

    return WICED_BT_SUCCESS;
}

#endif

wiced_result_t smartbridge_gatt_indication_notification_handler( uint16_t connection_handle, uint16_t attribute_handle, uint8_t* data, uint16_t length )
{
    wiced_bt_smartbridge_socket_t* socket;

    /* Search for socket with indicated connection handle in the connected list */
    if ( bt_smartbridge_socket_manager_find_socket_by_handle( connection_handle, &socket ) == WICED_BT_SUCCESS )
    {
        if ( bt_smartbridge_att_cache_is_enabled() == WICED_TRUE && socket->att_cache != NULL )
        {

            bt_smartbridge_att_cache_t*      cache          = (bt_smartbridge_att_cache_t*)socket->att_cache;
            wiced_bt_smart_attribute_list_t* att_cache_list = NULL;
            wiced_bt_smart_attribute_t*      att            = NULL;

            bt_smartbridge_att_cache_get_list( cache, &att_cache_list );

            /* Socket found. lock mutex for protected access */
            bt_smartbridge_att_cache_lock( cache );

            /* Search for att in the socket's att list */
            if ( wiced_bt_smart_attribute_search_list_by_handle( att_cache_list, attribute_handle, &att ) == WICED_BT_SUCCESS )
            {
                wiced_bt_uuid_t uuid       = att->type;
                wiced_bool_t    is_new_att = WICED_FALSE;

                /* Check if existing att memory length is sufficient */
                if ( length > att->value_length )
                {
                    /* length isn't sufficient. Remove existing from the list */
                    wiced_bt_smart_attribute_remove_from_list( att_cache_list, attribute_handle );
                    att = NULL;

                    /* Create a new one and marked as new */
                    wiced_bt_smart_attribute_create( &att, WICED_ATTRIBUTE_TYPE_CHARACTERISTIC_VALUE, length );
                    is_new_att = WICED_TRUE;
                }

                /* Copy new value to the att */
                att->handle       = attribute_handle;
                att->type         = uuid;
                att->value_length = length;
                memcpy( att->value.value, data, length );

                if ( is_new_att == WICED_TRUE )
                {
                    /* Add newly created att to the list */
                    wiced_bt_smart_attribute_add_to_list( att_cache_list, att );
                }
            }

            /* Socket found. lock mutex for protected access */
            bt_smartbridge_att_cache_unlock( cache );
        }

        socket->last_notified_attribute_handle = attribute_handle;

        /* Notification callback is called regardless of att cache is enabled or not */
        if ( socket->notification_callback != NULL )
        {
            wiced_rtos_send_asynchronous_event( WICED_NETWORKING_WORKER_THREAD, smartbridge_app_notification_handler, (void*)socket );
        }

        return WICED_BT_SUCCESS;
    }

    return WICED_BT_ERROR;
}

static wiced_result_t smartbridge_app_notification_handler( void* arg )
{
    wiced_bt_smartbridge_socket_t* socket = (wiced_bt_smartbridge_socket_t*)arg;

    if ( socket != NULL && socket->notification_callback != NULL )
    {
        socket->notification_callback( socket, socket->last_notified_attribute_handle );
        return WICED_BT_SUCCESS;
    }

    return WICED_BT_ERROR;
}

static wiced_result_t smartbridge_app_disconnection_handler( void* arg )
{
    wiced_bt_smartbridge_socket_t* socket = (wiced_bt_smartbridge_socket_t*)arg;

    if ( socket != NULL && socket->disconnection_callback != NULL )
    {
        socket->disconnection_callback( socket );
        return WICED_BT_SUCCESS;
    }

    return WICED_BT_ERROR;
}

#if 0
static wiced_result_t smartbridge_gap_bonding_handler( uint16_t connection_handle, const wiced_bt_smart_bond_info_t* bond_info )
{
    wiced_bt_smartbridge_socket_t* socket;

    if ( bt_smartbridge_socket_manager_find_socket_by_handle( connection_handle, &socket ) == WICED_BT_SUCCESS )
    {
        /* Bonding successful. Update socket's bond info and post callback to WICED_NETWORKING_WORKER_THREAD */
        memcpy( &socket->bond_info, bond_info, sizeof( *bond_info ) );

        if ( socket != NULL && socket->bonding_callback != NULL )
        {
            wiced_rtos_send_asynchronous_event( WICED_NETWORKING_WORKER_THREAD, smartbridge_app_pairing_handler, (void*)socket );
        }

        return WICED_BT_SUCCESS;
    }

    return WICED_BT_ERROR;
}

static wiced_result_t smartbridge_app_pairing_handler( void* arg )
{
    wiced_bt_smartbridge_socket_t* socket = (wiced_bt_smartbridge_socket_t*)arg;

    if ( socket != NULL && socket->bonding_callback != NULL )
    {
        socket->bonding_callback( socket, &socket->bond_info );
        return WICED_BT_SUCCESS;
    }

    return WICED_BT_ERROR;
}

#endif

