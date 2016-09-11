/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include "wiced_defaults.h"
#include "watchdog.h"
#include "wdog/wdog.h"
#include "MK60N512VMD100.h"

#ifndef WICED_DISABLE_WATCHDOG

/** @file
 * TODO: Update this to Freescale K60x
 *
 * Debugging Watchdog Reset
 */

#if (defined(APPLICATION_WATCHDOG_TIMEOUT_SECONDS) && (APPLICATION_WATCHDOG_TIMEOUT_SECONDS > MAX_WATCHDOG_TIMEOUT_SECONDS))
#error APPLICATION_WATCHDOG_TIMEOUT_SECONDS must NOT be larger than 22 seconds
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

#define WATCHDOG_PRESCALER              (0)
#define WATCHDOG_TIMEOUT_MULTIPLIER     (0)
#define DBG_WATCHDOG_TIMEOUT_MULTIPLIER (0)
#define DBG_WATCHDOG_PRESCALER          (0)

#ifdef APPLICATION_WATCHDOG_TIMEOUT_SECONDS
#define WATCHDOG_TIMEOUT                (APPLICATION_WATCHDOG_TIMEOUT_SECONDS * WATCHDOG_TIMEOUT_MULTIPLIER)
#define DBG_WATCHDOG_TIMEOUT            (APPLICATION_WATCHDOG_TIMEOUT_SECONDS * DBG_WATCHDOG_TIMEOUT_MULTIPLIER)
#else
#define WATCHDOG_TIMEOUT                (MAX_WATCHDOG_TIMEOUT_SECONDS * WATCHDOG_TIMEOUT_MULTIPLIER)
#define DBG_WATCHDOG_TIMEOUT            (MAX_WATCHDOG_TIMEOUT_SECONDS * DBG_WATCHDOG_TIMEOUT_MULTIPLIER)
#endif /* APPLICATION_WATCHDOG_TIMEOUT_SECONDS */

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

wiced_result_t watchdog_init( void )
{
    /* Disable watchdog for now */
	wdog_disable();
    return WICED_SUCCESS;
}

wiced_result_t watchdog_kick( void )
{
    return WICED_SUCCESS;
}

wiced_bool_t watchdog_check_last_reset( void )
{
    return WICED_FALSE;
}

#else
wiced_result_t  watchdog_init( void )             { return WICED_SUCCESS; }
wiced_result_t  watchdog_kick( void )             { return WICED_SUCCESS; }
wiced_bool_t    watchdog_check_last_reset( void ) { return WICED_FALSE;   }
#endif
