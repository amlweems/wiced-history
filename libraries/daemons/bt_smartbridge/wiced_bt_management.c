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
#include "wiced_bt.h"
#include "wiced_bt_smartbridge_constants.h"
#include "wiced_bt_smartbridge.h"
#include "wiced_bt_types.h"
#include "wiced_bt_stack.h"
#ifdef BT_MFGTEST_MODE
#include "bt_mfgtest.h"
#endif
#include "bt_smartbridge_helper.h"
#include "bt_smartbridge_stack_interface.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define BT_DEVICE_NAME_MAX_LENGTH 21

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

static wiced_bt_dev_status_t    smartbridge_bt_stack_management_callback    ( wiced_bt_management_evt_t event,  wiced_bt_management_evt_data_t *p_event_data );
extern wiced_bt_gatt_status_t   smartbridge_gatt_callback                   ( wiced_bt_gatt_evt_t event,        wiced_bt_gatt_event_data_t *p_event_data );

/******************************************************
 *               Variable Definitions
 ******************************************************/

wiced_bt_local_identity_keys_t          local_identity_keys;
wiced_bt_device_sec_keys_t              device_link_keys;
wiced_bool_t                            device_link_key_updated = WICED_FALSE;
extern wiced_bt_cfg_settings_t          wiced_bt_cfg_settings;
extern wiced_bt_smartbridge_socket_t*   connecting_socket;
extern const wiced_bt_cfg_buf_pool_t    wiced_bt_cfg_buf_pools[];
static char                             bt_device_name[BT_DEVICE_NAME_MAX_LENGTH + 1] = { 0 };
wiced_bool_t                            bt_initialised           = WICED_FALSE;
static wiced_bt_device_address_t        bt_address               = { 0 };
static wiced_bt_device_address_t        bt_override_address      = { 0 };
static wiced_bt_device_address_t        bt_override_address_mask = { 0 };
static wiced_bool_t                     bt_address_is_overriden  = WICED_FALSE;

extern wiced_result_t wiced_bt_smartbridge_bond_info_update( wiced_bt_device_link_keys_t paired_device_keys );
static wiced_result_t bt_management_override_device_address(void);

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t wiced_bt_init( wiced_bt_mode_t mode, const char* device_name )
{
    wiced_result_t result;

    if ( bt_initialised == WICED_TRUE )
    {
        return WICED_BT_SUCCESS;
    }

    WPRINT_LIB_INFO( ("Initializing WICED Bluetooth\n") );

    if ( mode == WICED_BT_MPAF_MODE )
    {
        WPRINT_LIB_ERROR( ("Error. MPAF-mode is not supported\n") );
        return WICED_BT_UNSUPPORTED;
    }

    /* Initialise Bluetooth Stack */
    result = wiced_bt_stack_init( smartbridge_bt_stack_management_callback, &wiced_bt_cfg_settings, wiced_bt_cfg_buf_pools );
    if ( result != WICED_BT_SUCCESS )
    {
        WPRINT_LIB_ERROR( ("Error initialising Bluetooth stack\n") );
        return result;
    }

    memset( bt_device_name, 0, sizeof( bt_device_name ) );
    memcpy( bt_device_name, device_name, strnlen( device_name, BT_DEVICE_NAME_MAX_LENGTH ) );

    return WICED_BT_SUCCESS;
}

wiced_result_t wiced_bt_deinit( void )
{
    wiced_result_t result;

    if ( bt_initialised == WICED_FALSE )
    {
        return WICED_BT_SUCCESS;
    }

    result = wiced_bt_stack_deinit( );
    if ( result != WICED_BT_SUCCESS )
    {
        WPRINT_LIB_ERROR( ("Error de-initialising Bluetooth stack\n") );
        return result;
    }

    memset( bt_device_name, 0, sizeof( bt_device_name ) );
    bt_initialised = WICED_FALSE;
    return WICED_BT_SUCCESS;
}

wiced_result_t wiced_bt_init_address( const wiced_bt_device_address_t* address, const wiced_bt_device_address_t* mask )
{
    memcpy(&bt_override_address, address, sizeof(wiced_bt_device_address_t));
    memcpy(&bt_override_address_mask, mask, sizeof(wiced_bt_device_address_t));
    bt_address_is_overriden     = WICED_TRUE;

    return WICED_BT_SUCCESS;
}

wiced_result_t wiced_bt_start_mfgtest_mode( const wiced_uart_config_t* config )
{
#ifdef BT_MFGTEST_MODE
    wiced_result_t result = bt_bus_init();
    if ( result != WICED_BT_SUCCESS )
    {
        WPRINT_LIB_ERROR( ("Error initialising Bluetooth transport bus\n") );
        return result;
    }

    return bt_mfgtest_start( config );
#else
    return WICED_BT_UNSUPPORTED;
#endif
}

wiced_result_t wiced_bt_device_get_address( wiced_bt_device_address_t* address )
{
    if ( bt_initialised == WICED_FALSE )
    {
        return WICED_BT_ERROR;
    }

    memcpy( address, &bt_address, sizeof( *address ) );
    return WICED_BT_SUCCESS;
}

const char* wiced_bt_device_get_name( void )
{
    /* TODO: bt_device_name is not provided to Stack/Controller. Once it is provided to the Stack
     * correctly, we should rather read the name of the device from the Stack */
    return (const char*)bt_device_name;
}

wiced_bool_t wiced_bt_device_is_on( void )
{
    /* TODO: Need to be implemented again as per the new Stack */
    return WICED_FALSE;
}

wiced_bool_t wiced_bt_device_is_connectable( void )
{
    /* TODO :  Need to be implemented for the new-stack */
    return WICED_FALSE;
}

wiced_bool_t wiced_bt_device_is_discoverable( void )
{
    /* TODO :  Need to be implemented for the new-stack */
    return WICED_FALSE;
}


static wiced_bt_dev_status_t smartbridge_bt_stack_management_callback( wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data )
{
    wiced_bt_dev_status_t status = WICED_BT_SUCCESS;

    switch(event)
    {
        case BTM_ENABLED_EVT:
        {
            /* Initialize GATT REST API Server once Bluetooth controller and host stack is enabled */
            if ( ( status = p_event_data->enabled.status ) == WICED_BT_SUCCESS )
            {
                bt_management_override_device_address();

                /* Register for GATT event notifications */
                wiced_bt_gatt_register( smartbridge_gatt_callback );
                bt_initialised = WICED_TRUE;
            }
            break;
        }

        case BTM_SECURITY_REQUEST_EVT:
        {
            WPRINT_LIB_INFO( ( "[SmartBridge] Security grant request\n" ) );
            wiced_bt_ble_security_grant( p_event_data->security_request.bd_addr, WICED_BT_SUCCESS );
            break;
        }

        case BTM_LOCAL_IDENTITY_KEYS_UPDATE_EVT:
        {
            memcpy(&local_identity_keys, &p_event_data->local_identity_keys_update, sizeof( wiced_bt_local_identity_keys_t ) );
            WPRINT_LIB_INFO( ("[SmartBridge] Local Identity Keys Update type:%u\n", local_identity_keys.local_key_data[0] ) );
            break;
        }

        case BTM_LOCAL_IDENTITY_KEYS_REQUEST_EVT:
        {
            memcpy( &p_event_data->local_identity_keys_request ,&local_identity_keys, sizeof(wiced_bt_local_identity_keys_t) );
            WPRINT_LIB_INFO(("[SmartBridge] Local Identity Keys Request Event\n"));
            break;
        }

        case BTM_PAIRED_DEVICE_LINK_KEYS_UPDATE_EVT:
        {
            WPRINT_LIB_INFO(("[SmartBridge] Paired Device Link Keys Update Event\n"));
            memcpy(&device_link_keys, &p_event_data->paired_device_link_keys_request.key_data, sizeof(wiced_bt_device_sec_keys_t));
            wiced_bt_smartbridge_bond_info_update( p_event_data->paired_device_link_keys_update );
            device_link_key_updated  = WICED_TRUE;
            break;
        }

        case BTM_PAIRED_DEVICE_LINK_KEYS_REQUEST_EVT:
        {
            WPRINT_LIB_INFO( ("[SmartBridge] Paired Device Link Keys Request Event\n") );
            if ( device_link_key_updated == WICED_TRUE )
            {
                memcpy( &p_event_data->paired_device_link_keys_request.key_data, &device_link_keys, sizeof(wiced_bt_device_sec_keys_t));
                return WICED_BT_SUCCESS;
            }
            status = WICED_BT_ERROR;
            break;
        }

        case BTM_PAIRING_IO_CAPABILITIES_BLE_REQUEST_EVT:
        {
            WPRINT_LIB_INFO( ("[SmartBridge] Pairing IO capabilities Request Event\n") );

            extern wiced_bt_dev_ble_io_caps_req_t  default_io_caps_ble;
            /* Peer requested for I/O capabilities. Copy local I/O caps to stack */
            WPRINT_LIB_INFO( ( "Getting local I/O capabilities\n" ) );
            p_event_data->pairing_io_capabilities_ble_request.local_io_cap = default_io_caps_ble.local_io_cap;
            p_event_data->pairing_io_capabilities_ble_request.oob_data     = default_io_caps_ble.oob_data;
            p_event_data->pairing_io_capabilities_ble_request.auth_req     = default_io_caps_ble.auth_req;
            p_event_data->pairing_io_capabilities_ble_request.max_key_size = default_io_caps_ble.max_key_size;
            p_event_data->pairing_io_capabilities_ble_request.init_keys    = default_io_caps_ble.init_keys;
            p_event_data->pairing_io_capabilities_ble_request.resp_keys    = default_io_caps_ble.resp_keys;
            break;
        }

        case BTM_PAIRING_COMPLETE_EVT:
        {
            WPRINT_LIB_INFO( ( "[SmartBridge] Pairing complete status=%i, reason=0x%x.\n", p_event_data->pairing_complete.pairing_complete_info.ble.status, p_event_data->pairing_complete.pairing_complete_info.ble.reason ) );
            if ( p_event_data->pairing_complete.pairing_complete_info.ble.status == WICED_SUCCESS )
            {
            }
            /* Notify app thread that pairing is complete, regardless of  */
            wiced_rtos_set_semaphore( &connecting_socket->semaphore );
            break;
        }

        case BTM_ENCRYPTION_STATUS_EVT:
        {
            WPRINT_LIB_INFO( ( "[SmartBridge] encryption status = %i\n", p_event_data->encryption_status.result ) );

            if ( p_event_data->encryption_status.result == WICED_BT_SUCCESS )
            {
                /* Update state of the socket to encrypted */
                connecting_socket->state = SOCKET_STATE_LINK_ENCRYPTED;
            }
            wiced_rtos_set_semaphore( &connecting_socket->semaphore );
            break;
        }

        case BTM_PASSKEY_NOTIFICATION_EVT:
        {
            WPRINT_LIB_INFO( ("[SmartBridge] Passkey Notification event( PassKey generated is: %u )\n", (unsigned int)p_event_data->user_passkey_notification.passkey ) );
            break;
        }

        case BTM_PASSKEY_REQUEST_EVT:
        case BTM_SMP_REMOTE_OOB_DATA_REQUEST_EVT:
        case BTM_USER_CONFIRMATION_REQUEST_EVT:
        case BTM_SMP_SC_LOCAL_OOB_DATA_NOTIFICATION_EVT:
        case BTM_SMP_SC_REMOTE_OOB_DATA_REQUEST_EVT:
        default:
            WPRINT_LIB_INFO(("[SmartBridge] Unhandled Bluetooth Stack Callback event :%d\n", event));
            break;
    }

    return status;
}

static wiced_result_t bt_management_override_device_address( void )
{
    wiced_bt_device_address_t   new_address;
    /* If application decides to override the address, read the default address and override */
    if ( bt_address_is_overriden == WICED_FALSE )
    {
        /* No override */
        return WICED_BT_SUCCESS;
    }
    wiced_bt_dev_read_local_addr( bt_address );

    WPRINT_LIB_INFO(( "[SmartBridge] Local Bluetooth Address: [%02X:%02X:%02X:%02X:%02X:%02X]\n", bt_address[0], bt_address[1], bt_address[2], bt_address[3], bt_address[4], bt_address[5]) );
    /* Override address */
    new_address[0] = ( bt_address[0] & ~( bt_override_address_mask[0] ) ) | ( bt_override_address[0] & bt_override_address_mask[0] );
    new_address[1] = ( bt_address[1] & ~( bt_override_address_mask[1] ) ) | ( bt_override_address[1] & bt_override_address_mask[1] );
    new_address[2] = ( bt_address[2] & ~( bt_override_address_mask[2] ) ) | ( bt_override_address[2] & bt_override_address_mask[2] );
    new_address[3] = ( bt_address[3] & ~( bt_override_address_mask[3] ) ) | ( bt_override_address[3] & bt_override_address_mask[3] );
    new_address[4] = ( bt_address[4] & ~( bt_override_address_mask[4] ) ) | ( bt_override_address[4] & bt_override_address_mask[4] );
    new_address[5] = ( bt_address[5] & ~( bt_override_address_mask[5] ) ) | ( bt_override_address[5] & bt_override_address_mask[5] );

    wiced_bt_set_local_bdaddr( new_address );
    wiced_bt_dev_read_local_addr( new_address );
    WPRINT_LIB_INFO(( "[SmartBridge] New Bluetooth Address: [%02X:%02X:%02X:%02X:%02X:%02X]\n", new_address[0], new_address[1], new_address[2], new_address[3], new_address[4], new_address[5]) );

    return WICED_BT_SUCCESS;
}
