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
#include "wiced_platform.h"
#include "platform_dct.h"
#include "Platform/wwd_platform_interface.h"
#include "wwd_assert.h"
#include "stdio.h"
#include "string.h"
#include "crt0.h"
#include "core_cm4.h"

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

typedef uint32_t flash_write_t;

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

void init_clocks( void )
{
    sysclk_init();

    /* Switch Slow Clock source to external 32K crystal */
    pmc_switch_sclk_to_32kxtal( 0 );
    while( pmc_osc_is_ready_32kxtal( ) == 0 )
    {
    }

    pmc_disable_udpck( );
}

void init_memory( void )
{

}

void init_architecture( void )
{
	uint32_t a;

#ifdef INTERRUPT_VECTORS_IN_RAM
    SCB->VTOR = 0x1FFF0000; /* Change the vector table to point to start of SRAM */
#endif /* ifdef INTERRUPT_VECTORS_IN_RAM */

    /* Initialise watchdog. */
    sam4s_watchdog_init( );

    /* Initialise the interrupt priorities to a priority lower than 0 so that the BASEPRI register can mask them */
    for ( a = 0; a < PERIPH_COUNT_IRQn; a++ )
    {
    	/* SAM4S uses __NVIC_PRIO_BITS equals 4 i.e. supports 16 different pre-emption priorities ranging from 0x0 to 0xf.
    	 * Set all priorities to lowest value here and let each individual module to bump it up to the appropriate priority.
    	 */
        NVIC_SetPriority( a, 0xf );
    }

    /* Enable PIO peripheral clocks */
    ioport_init();

    /* Initialise powersave
     * Note: This MUST preceed any other peripheral init functions
     */
    sam4s_powersave_init();

    /* Initialise standard I/O UART */
    platform_stdio_init( );

    /* Ensure 802.11 device is in reset. */
    host_platform_init( );

    /* Disable MCU powersave at start-up. Application must explicitly enable MCU powersave if desired */
    sam4s_powersave_clocks_needed();
}
