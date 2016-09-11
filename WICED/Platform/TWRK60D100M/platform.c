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
#include "platform.h"
#include "wiced_platform.h"
#include "MK60N512VMD100.h"
#include "watchdog.h"
#include "stdio.h"
#include "string.h"
#include "wwd_assert.h"
#include "k60_gpio.h"
#include "platform_common_config.h"
#include "Platform/wwd_platform_interface.h"

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
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t wiced_platform_init( void )
{
    WPRINT_PLATFORM_INFO( ("\r\nPlatform " PLATFORM " initialised\r\n") );

    if ( WICED_TRUE == watchdog_check_last_reset() )
    {
        WPRINT_PLATFORM_ERROR(("WARNING: Watchdog reset occured previously. Please see watchdog.c for debugging instructions.\r\n"));
    }

    return WICED_SUCCESS;
}

void init_platform( void )
{
    /* stub */
}

void host_platform_reset_wifi( wiced_bool_t reset_asserted )
{
    if ( reset_asserted == WICED_TRUE )
    {
        k60_gpio_output_low( WL_RESET_BANK, WL_RESET_PIN );
    }
    else
    {
        k60_gpio_output_high( WL_RESET_BANK, WL_RESET_PIN );
    }
}

void host_platform_power_wifi( wiced_bool_t power_enabled )
{
    if ( power_enabled == WICED_TRUE )
    {
        k60_gpio_output_low( WL_REG_ON_BANK, WL_REG_ON_PIN );
    }
    else
    {
        k60_gpio_output_high( WL_REG_ON_BANK, WL_REG_ON_PIN );
    }
}
