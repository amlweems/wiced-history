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
 * BCM43909 ROM common
 */

#include "platform_assert.h"

/******************************************************
 *               Function Declarations
 ******************************************************/

void ramfnstub_unimplemented_handler(void);

/******************************************************
 *               Function Definitions
 ******************************************************/

void ramfnstub_unimplemented_handler(void)
{
    /* This means that a ROM function was called, which in turn
     * called a RAM stub function for which no implementation
     * was provided.  This should never happen.
     */
    WICED_TRIGGER_BREAKPOINT();

    while (1)
    {
        /* Loop forever */
    }
}
