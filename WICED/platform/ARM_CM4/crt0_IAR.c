/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "platform_init.h"
#include "platform_cmsis.h"


extern void __iar_program_start( void );
extern void platform_init_system_clocks(void);
extern void platform_init_memory(void);


int __low_level_init( void );

/* This is the code that gets called on processor reset. To initialize the */
/* device. */
#pragma section=".intvec"
 int __low_level_init( void )
{
     /* IAR allows init functions in __low_level_init(), but it is run before global
      * variables have been initialised, so the following init still needs to be done
      * When using GCC, this is done in crt0_GCC.c
      */
     /* Setup the interrupt vectors address */
     SCB->VTOR = (unsigned long )__section_begin(".intvec");

     /* Enable CPU Cycle counting */
     CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; /* Global Enable for DWT */
     DWT->CYCCNT = 0;                                /* Reset the counter */
     DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;            /* Enable cycle counter */

     platform_init_system_clocks();
     platform_init_memory();
     return 1; /* return 1 to force memory init */
}


/* Use stackless  because we might have arrived here
 * because the PC was set by a debugger.  If the vector
 * table isn't installed, then the SP will be bogus.
 * Setup the stack before calling the IAR entry point.
 */
#pragma section="CSTACK"
__stackless void _start( void )
{
    /* Stack pointer is usually set up by boot process, but if program was loaded via jtag in RAM, that might not have happened. */
    __set_MSP((unsigned long)__section_end("CSTACK"));

    /* SP should be setup by this time. */
    __iar_program_start();
}
