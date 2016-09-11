/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include <stdint.h>
#include "tx_api.h"
#include "tx_initialize.h"
#include "platform_assert.h"
#include "wwd_rtos.h"

#define CYCLES_PER_SYSTICK ( ( CPU_CLOCK_HZ / SYSTICK_FREQUENCY ) - 1 )



typedef struct
{
    uint32_t CTRL;
    uint32_t LOAD;
    uint32_t VAL;
    uint32_t CALIB;
} STK_t;


#define STK                 ((STK_t   *) 0xE000E010)
#define SCB_SHP             ((uint8_t *) 0xE000ED18)
#define SYSTICK_ENABLE      (1)
#define SYSTICK_INT_ENABLE  (2)
#define SYSTICK_SOURCE      (4)

/*@external@*/ extern void* _vectors[];
extern void* _tx_initialize_unused_memory;
extern void* _tx_thread_system_stack_ptr;

void _tx_initialize_low_level( void )
{

    /* Setup some ThreadX internal values */
    _tx_initialize_unused_memory = (void*)0xbaadbaad;  /* TODO : add proper value here */
    _tx_thread_system_stack_ptr = _vectors[0];

    /* Enable cycle counting register */
    *((unsigned long*)0xE0001000) |= 1;

    /* Setup the system handler priorities */
    SCB_SHP[ 0] = (uint8_t) 0x00; /* Mem Manage system handler priority */
    SCB_SHP[ 1] = (uint8_t) 0x00; /* Bus Fault system handler priority */
    SCB_SHP[ 2] = (uint8_t) 0x00; /* Usage Fault system handler priority */
    SCB_SHP[ 3] = (uint8_t) 0x00; /* Reserved */
    SCB_SHP[ 4] = (uint8_t) 0x00; /* Reserved */
    SCB_SHP[ 5] = (uint8_t) 0x00; /* Reserved */
    SCB_SHP[ 6] = (uint8_t) 0x00; /* Reserved */
    SCB_SHP[ 7] = (uint8_t) 0xF0; /* SVCall system handler priority */
    SCB_SHP[ 8] = (uint8_t) 0x00; /* Debug Monitor system handler priority */
    SCB_SHP[ 9] = (uint8_t) 0x00; /* Reserved */
    SCB_SHP[10] = (uint8_t) 0xF0; /* PendSV system handler priority */
    SCB_SHP[11] = (uint8_t) 0x40; /* SysTick system handler priority */


    /* Setup the system tick */
    STK->LOAD = (uint32_t) ( CYCLES_PER_SYSTICK );
    STK->CTRL = (uint32_t) ( SYSTICK_INT_ENABLE | SYSTICK_SOURCE );  /* clock source is processor clock - AHB */

}
