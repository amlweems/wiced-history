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
 *
 */
#include "sam4s_platform.h"
#include "wiced_platform.h"
#include "wiced_utilities.h"

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

wiced_result_t wiced_pwm_init( wiced_pwm_t pwm_peripheral, uint32_t frequency, float duty_cycle )
{
    UNUSED_PARAMETER( pwm_peripheral );
    UNUSED_PARAMETER( frequency );
    UNUSED_PARAMETER( duty_cycle );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_pwm_start( wiced_pwm_t pwm )
{
    UNUSED_PARAMETER( pwm );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_pwm_stop( wiced_pwm_t pwm )
{
    UNUSED_PARAMETER( pwm );
    return WICED_UNSUPPORTED;
}
