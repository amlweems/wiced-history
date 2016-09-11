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
 *  Implementation of wiced_rtos.c for the case where no RTOS is used
 *
 *  This is a special implementation of the Wiced RTOS
 *  abstraction layer, which is designed to make Wiced work even with
 *  no RTOS.
 *  It provides Wiced with alternates to the functions for threads,
 *  semaphores and time functions.
 *
 *  Semaphores are simple integers, since accesses to an integer will be atomic.
 *
 */


#include "wwd_rtos.h"
#include <stdint.h>


volatile uint32_t noos_total_time = 0;



typedef struct
{
    uint32_t CTRL;
    uint32_t LOAD;
    uint32_t VAL;
    uint32_t CALIB;
} STK_t;


#define CYCLES_PER_SYSTICK  ( ( CPU_CLOCK_HZ / SYSTICK_FREQUENCY ) - 1 )
#define STK                 ((STK_t   *) 0xE000E010)
#define SCB_SHP             ((uint8_t *) 0xE000ED18)
#define SYSTICK_ENABLE      (1)
#define SYSTICK_INT_ENABLE  (2)
#define SYSTICK_SOURCE      (4)


void NoOS_setup_timing( void )
{
    /* Setup the system handler priorities */
    SCB_SHP[11] = (uint8_t) 0x40; /* SysTick system handler priority */


    /* Setup the system tick */
    STK->LOAD = (uint32_t) ( CYCLES_PER_SYSTICK );
    STK->CTRL = (uint32_t) ( SYSTICK_ENABLE | SYSTICK_INT_ENABLE | SYSTICK_SOURCE );  /* clock source is processor clock - AHB */
}


void NoOS_stop_timing( void )
{
    /* Setup the system handler priorities */
    SCB_SHP[11] = (uint8_t) 0x40; /* SysTick system handler priority */


    /* Setup the system tick */
    STK->CTRL &= (uint32_t) ~( SYSTICK_ENABLE | SYSTICK_INT_ENABLE );
}

void NoOS_systick_irq( void )
{
    noos_total_time++;
}

