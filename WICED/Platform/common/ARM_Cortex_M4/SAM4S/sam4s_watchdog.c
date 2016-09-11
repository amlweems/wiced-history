/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "sam4s_platform.h"
#include "wiced_defaults.h"
#include "wiced_utilities.h"

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
 *             WICED Function Definitions
 ******************************************************/

/******************************************************
 *             SAM4S Function Definitions
 ******************************************************/

#ifndef WICED_DISABLE_WATCHDOG
wiced_result_t sam4s_watchdog_init( void )
{
    /* WARNING: SAM4S watchdog is NOT implemented yet. Watchdog is disabled at start-up */
    wdt_disable( WDT );
    return WICED_SUCCESS;
}

wiced_result_t sam4s_watchdog_kick( void )
{
    /* WARNING: SAM4S watchdog is NOT implemented yet */
    return WICED_UNSUPPORTED;
}

wiced_bool_t sam4s_watchdog_check_last_reset( void )
{
    /* WARNING: SAM4S watchdog is NOT implemented yet */
    return WICED_FALSE;
}
#else
wiced_result_t sam4s_watchdog_init( void )
{
    wdt_disable( WDT );
    return WICED_SUCCESS;
}

wiced_result_t sam4s_watchdog_kick( void )
{
    return WICED_SUCCESS;
}

wiced_bool_t sam4s_watchdog_check_last_reset( void )
{
    return WICED_FALSE;
}
#endif
