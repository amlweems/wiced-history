/*
 * Copyright 2013, Broadcom Corporation
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
#include <stdint.h>
#include "platform.h"
#include "k60_gpio.h"
#include "wwd_constants.h"
#include "wwd_assert.h"
#include "platform_common_config.h"
#include "Platform/wwd_platform_interface.h"
#include "core_cm4.h"

#ifndef WL_RESET_BANK
#error Missing WL_RESET_BANK definition
#endif
#ifndef WL_REG_ON_BANK
#error Missing WL_REG_ON_BANK definition
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

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t host_platform_init( void )
{
    /* Configure WLAN Reset Pin. Hold WLAN chip at reset (Low)  */
    k60_gpio_init( WL_RESET_BANK, WL_RESET_PIN, OUTPUT_PUSH_PULL );
    host_platform_reset_wifi( WICED_TRUE );

    /* Configure WLAN Regulator On Pin. Set regulator off (High) */
    k60_gpio_init( WL_REG_ON_BANK, WL_REG_ON_PIN, OUTPUT_PUSH_PULL );
    host_platform_power_wifi( WICED_FALSE );

    return WICED_SUCCESS;
}

wiced_result_t host_platform_deinit( void )
{
    /* Configure WLAN Reset Pin. Hold WLAN chip at reset (Low)  */
    k60_gpio_init( WL_RESET_BANK, WL_RESET_PIN, OUTPUT_PUSH_PULL );
    host_platform_reset_wifi( WICED_TRUE );

    /* Configure WLAN Regulator On Pin. Set regulator off (High) */
    k60_gpio_init( WL_REG_ON_BANK, WL_REG_ON_PIN, OUTPUT_PUSH_PULL );
    host_platform_power_wifi( WICED_FALSE );

    return WICED_SUCCESS;
}

uint32_t host_platform_get_cycle_count( void )
{
    /* From the ARM Cortex-M3 Techinical Reference Manual
     * 0xE0001004  DWT_CYCCNT  RW  0x00000000  Cycle Count Register */
    return *( (uint32_t*) 0xE0001004 );
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

