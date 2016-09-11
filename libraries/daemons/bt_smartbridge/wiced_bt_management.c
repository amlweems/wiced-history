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
#include "wiced_bt.h"
#include "wiced_bt_smartbridge_constants.h"
#include "wiced_bt_types.h"
#include "wiced_bt_stack.h"
#include "bt_bus.h"
#include "bt_packet_internal.h"
#ifdef BT_MFGTEST_MODE
#include "bt_mfgtest.h"
#endif

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

static wiced_bt_dev_status_t smartbridge_bt_stack_management_callback( wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data );
extern wiced_bt_gatt_status_t smartbridge_gatt_callback( wiced_bt_gatt_evt_t event, wiced_bt_gatt_event_data_t *p_event_data );

/******************************************************
 *               Variable Definitions
 ******************************************************/

extern wiced_bt_cfg_settings_t           wiced_bt_cfg_settings;
extern const wiced_bt_cfg_buf_pool_t     wiced_bt_cfg_buf_pools[];
static char                              bt_device_name[BT_DEVICE_NAME_MAX_LENGTH + 1] = { 0 };
wiced_bool_t                             bt_initialised           = WICED_FALSE;
static wiced_bt_device_address_t         bt_address               = { 0 };

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
    if ( bt_initialised == WICED_FALSE )
    {
        return WICED_BT_SUCCESS;
    }

    memset( bt_device_name, 0, sizeof( bt_device_name ) );
    bt_initialised = WICED_FALSE;
    return WICED_BT_SUCCESS;
}

wiced_result_t wiced_bt_init_address( const wiced_bt_device_address_t* address, const wiced_bt_device_address_t* mask )
{
    UNUSED_PARAMETER(address);
    UNUSED_PARAMETER(mask);
    return WICED_BT_UNSUPPORTED;
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
    BD_ADDR address;

    switch(event)
    {
        case BTM_ENABLED_EVT:
        {
            /* Initialize GATT REST API Server once Bluetooth controller and host stack is enabled */
            if ( ( status = p_event_data->enabled.status ) == WICED_BT_SUCCESS )
            {
                wiced_bt_dev_read_local_addr(address);
                WPRINT_BT_APP_INFO(( "[SmartBridge] Local Bluetooth Address: [%02X:%02X:%02X:%02X:%02X:%02X]\n", address[0], address[1], address[2], address[3], address[4], address[5]) );
                /* Register for GATT event notifications */
                wiced_bt_gatt_register( smartbridge_gatt_callback );
                bt_initialised = WICED_TRUE;
            }
        }

        case BTM_SECURITY_REQUEST_EVT:
        case BTM_PAIRING_IO_CAPABILITIES_BLE_REQUEST_EVT:
        case BTM_PASSKEY_REQUEST_EVT:
        case BTM_SMP_REMOTE_OOB_DATA_REQUEST_EVT:
        case BTM_USER_CONFIRMATION_REQUEST_EVT:
        case BTM_PASSKEY_NOTIFICATION_EVT:
        case BTM_PAIRING_COMPLETE_EVT:
        case BTM_SMP_SC_LOCAL_OOB_DATA_NOTIFICATION_EVT:
        case BTM_SMP_SC_REMOTE_OOB_DATA_REQUEST_EVT:
        default:
            WPRINT_LIB_INFO(("[SmartBridge] Stack Callback event :%x\n", event));
            break;
    }

    return status;
}
