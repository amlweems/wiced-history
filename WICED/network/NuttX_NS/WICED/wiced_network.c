/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Wiced NuttX networking layer
 */

#include <nuttx/config.h>

#include <arch/chip/wifi.h>

#include "wwd_assert.h"
#include "wwd_buffer_interface.h"
#include "internal/wiced_internal_api.h"

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
 *                 Static Variables
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t wiced_network_init( void )
{
    WPRINT_NETWORK_INFO(("Initialising NuttX networking " RTOS_VERSION "\n"));

    WPRINT_NETWORK_INFO(("Creating Packet pools\n"));
    if ( wwd_buffer_init( NULL ) != WWD_SUCCESS )
    {
        WPRINT_NETWORK_ERROR(("Could not initialize buffer interface\n"));
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_network_deinit( void )
{
    WPRINT_NETWORK_INFO(("Deinitialising NuttX networking " RTOS_VERSION "\n"));

    WPRINT_NETWORK_INFO(("Destroying Packet pools\n"));
    if ( wwd_buffer_deinit( ) != WWD_SUCCESS )
    {
        WPRINT_NETWORK_ERROR(("Could not deinitialize buffer interface\n"));
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_ip_up( wiced_interface_t interface, wiced_network_config_t config, const wiced_ip_setting_t* ip_settings )
{
    wiced_bool_t ip_up = WICED_FALSE;

    UNUSED_PARAMETER( config );
    UNUSED_PARAMETER( ip_settings );

    if ( wifi_driver_is_valid( interface ) )
    {
        if ( wifi_driver_ip_up( interface ) == OK )
        {
            ip_up = WICED_TRUE;
        }
    }
    if ( !ip_up )
    {
        return WICED_ERROR;
    }

    SET_IP_NETWORK_INITED( interface, WICED_TRUE );

    return WICED_SUCCESS;
}

wiced_result_t wiced_ip_down( wiced_interface_t interface )
{
    wiced_bool_t ip_down = WICED_FALSE;

    if ( wifi_driver_is_valid( interface ) )
    {
        if ( wifi_driver_ip_down( interface ) == OK )
        {
            ip_down = WICED_TRUE;
        }
    }
    if ( !ip_down )
    {
        return WICED_ERROR;
    }

    SET_IP_NETWORK_INITED( interface, WICED_FALSE );

    return WICED_SUCCESS;
}
