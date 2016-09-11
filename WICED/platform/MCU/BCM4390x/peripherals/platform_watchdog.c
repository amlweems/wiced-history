/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "platform_peripheral.h"
#include "platform_appscr4.h"
#include "platform_assert.h"
#include "platform_peripheral.h"

#include "cr4.h"

#include "typedefs.h"
#include "sbchipc.h"

#include "wiced_defaults.h"
#include "wiced_osl.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/


#define WATCHDOG_TIMEOUT_MULTIPLIER     platform_reference_clock_get_freq( PLATFORM_REFERENCE_CLOCK_ILP )

#ifdef APPLICATION_WATCHDOG_TIMEOUT_SECONDS
#define WATCHDOG_TIMEOUT                (APPLICATION_WATCHDOG_TIMEOUT_SECONDS * WATCHDOG_TIMEOUT_MULTIPLIER)
#else
#define WATCHDOG_TIMEOUT                (MAX_WATCHDOG_TIMEOUT_SECONDS * WATCHDOG_TIMEOUT_MULTIPLIER)
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

static void
platform_watchdog_set( uint32_t timeout )
{
    PLATFORM_PMU->pmuwatchdog = timeout;
}

platform_result_t platform_watchdog_init( void )
{
#ifndef WICED_DISABLE_WATCHDOG
    return platform_watchdog_kick();
#else
    platform_watchdog_deinit();
    return PLATFORM_FEATURE_DISABLED;
#endif
}

platform_result_t platform_watchdog_deinit( void )
{
    platform_watchdog_set( 0 );
    return PLATFORM_SUCCESS;
}

platform_result_t platform_watchdog_kick( void )
{
#ifndef WICED_DISABLE_WATCHDOG
    platform_watchdog_set( WATCHDOG_TIMEOUT );
    return PLATFORM_SUCCESS;
#else
    return PLATFORM_FEATURE_DISABLED;
#endif
}

wiced_bool_t platform_watchdog_check_last_reset( void )
{
#ifndef WICED_DISABLE_WATCHDOG
    if ( PLATFORM_PMU->pmustatus & PST_WDRESET )
    {
        /* Clear the flag and return */
        PLATFORM_PMU->pmustatus = PST_WDRESET;
        return WICED_TRUE;
    }
#endif

    return WICED_FALSE;
}

void platform_mcu_reset( void )
{
    WICED_DISABLE_INTERRUPTS();

    /* Fire PMU watchdog far in a future, use hard-coded ILP clock to reduce application sizes which do not use watchdog. */
    PLATFORM_PMU->pmuwatchdog = 30 * ILP_CLOCK;

    /* Make sure board not enters into endless reset cycle */
    while( ( cr4_get_cycle_counter() < CPU_CLOCK_HZ ) && !cr4_is_cycle_counter_overflowed() )
    {
    }

    /* Define PMU's reaction to backplane reset which is about to be issued */
    platform_common_chipcontrol( &PLATFORM_PMU->pmucontrol,
                                 PMU_CONTROL_RESETCONTROL_MASK,
                                 PMU_CONTROL_RESETCONTROL_RESET );

    /* Set watchdog to reset system on next tick */
    PLATFORM_CHIPCOMMON->clock_control.watchdog_counter = 1;

    /* Loop forever */
    while (1)
    {
    }
}
