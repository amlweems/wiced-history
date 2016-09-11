/*
 * Copyright 2014, Broadcom Corporation
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
#include "sam4s_platform.h"
#include "Platform/wwd_platform_interface.h"
#include "wwd_constants.h"
#include "wwd_assert.h"
#include "wiced_platform.h"
#include <stdint.h>
#include "string.h"
#ifndef WICED_DISABLE_BOOTLOADER
#include "bootloader_app.h"
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#ifdef __GNUC__
#define TRIGGER_BREAKPOINT() __asm__("bkpt")
#elif defined ( __IAR_SYSTEMS_ICC__ )
#define TRIGGER_BREAKPOINT() __asm("bkpt 0")
#endif

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
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

static const sam4s_pin_t wlan_reg_on_pin  = WL_REG_ON_PIN;
static const sam4s_pin_t wlan_reset_pin   = WL_RESET_PIN;

/******************************************************
 *               Function Definitions
 ******************************************************/

void host_platform_reset_wifi( wiced_bool_t reset_asserted )
{
    if ( reset_asserted == WICED_TRUE )
    {
        ioport_set_pin_level( wlan_reset_pin, IOPORT_PIN_LEVEL_LOW );
    }
    else
    {
        ioport_set_pin_level( wlan_reset_pin, IOPORT_PIN_LEVEL_HIGH );
    }
}

void host_platform_power_wifi( wiced_bool_t power_enabled )
{
    if ( power_enabled == WICED_TRUE )
    {
        ioport_set_pin_level( wlan_reg_on_pin, IOPORT_PIN_LEVEL_LOW );
    }
    else
    {
        ioport_set_pin_level( wlan_reg_on_pin, IOPORT_PIN_LEVEL_HIGH );
    }
}

wiced_result_t host_platform_init( void )
{
    /* Configure WLAN Reset Pin. Hold WLAN chip at reset (Low)  */
    ioport_enable_pin( wlan_reset_pin );
    ioport_set_pin_dir( wlan_reset_pin, IOPORT_DIR_OUTPUT );
    ioport_set_pin_mode( wlan_reset_pin, 0 );
    host_platform_reset_wifi( WICED_TRUE );

    /* Configure WLAN Regulator On Pin. Set regulator off (High) */
    ioport_enable_pin( wlan_reg_on_pin );
    ioport_set_pin_dir( wlan_reg_on_pin, IOPORT_DIR_OUTPUT );
    ioport_set_pin_mode( wlan_reg_on_pin, 0 );
    host_platform_power_wifi( WICED_FALSE );

    return WICED_SUCCESS;
}

wiced_result_t host_platform_deinit( void )
{
    /* Configure WLAN Reset Pin. Hold WLAN chip at reset (Low)  */
    host_platform_reset_wifi( WICED_TRUE );

    /* Configure WLAN Regulator On Pin. Set regulator off (High) */
    host_platform_power_wifi( WICED_FALSE );

    return WICED_SUCCESS;
}

uint32_t host_platform_get_cycle_count( void )
{
    /* From the ARM Cortex-M3 Techinical Reference Manual
     * 0xE0001004  DWT_CYCCNT  RW  0x00000000  Cycle Count Register */
    return *( (uint32_t*) 0xE0001004 );
}

void host_platform_get_mac_address( wiced_mac_t* mac )
{
#ifndef WICED_DISABLE_BOOTLOADER
    memcpy(mac->octet, bootloader_api->get_wifi_config_dct()->mac_address.octet, sizeof(wiced_mac_t));
#else
    UNUSED_PARAMETER( mac );
#endif
}

wiced_bool_t host_platform_is_in_interrupt_context( void )
{
    /* From the ARM Cortex-M3 Techinical Reference Manual
     * 0xE000ED04   ICSR    RW [a]  Privileged  0x00000000  Interrupt Control and State Register */
    uint32_t active_interrupt_vector = (uint32_t)( SCB ->ICSR & 0x3fU );

    if ( active_interrupt_vector != 0 )
    {
        return WICED_TRUE;
    }
    else
    {
        return WICED_FALSE;
    }
}

wiced_result_t host_platform_init_wlan_powersave_clock( void )
{
    return WICED_UNSUPPORTED;
}

wiced_result_t host_platform_deinit_wlan_powersave_clock( void )
{
    return WICED_UNSUPPORTED;
}
