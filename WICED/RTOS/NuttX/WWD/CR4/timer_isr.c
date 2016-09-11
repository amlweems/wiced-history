/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * BCM43909 timer isr
 */
#include "platform_toolchain.h"
#include "platform_isr.h"
#include "wwd_rtos_isr.h"

#include "nuttx/config.h"
#include "nuttx/arch.h"

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

WWD_RTOS_DEFINE_ISR( platform_tick_isr )
{
    if ( platform_tick_irq_handler( ) )
    {
        sched_process_timer( );
    }
}

WWD_RTOS_MAP_ISR( platform_tick_isr, Timer_ISR )
