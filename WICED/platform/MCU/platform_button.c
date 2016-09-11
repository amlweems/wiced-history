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
 * Common Button implementation for platforms that only use GPIO buttons.
 * If a platform has other, non-GPIO based buttons it must override all the functions defined within this file.
 */

#include "wiced_platform.h"
#include "platform.h"
#include "platform_button.h"
#include "gpio_button.h"
#include "wwd_debug.h"

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
 *               Static Function Declarations
 ******************************************************/

static void platform_button_state_change_callback( gpio_button_t* button, wiced_bool_t new_state );

/******************************************************
 *               Variable Definitions
 ******************************************************/

extern const gpio_button_t platform_gpio_buttons[PLATFORM_BUTTON_MAX];

static platform_button_state_change_callback_t user_callback;

/******************************************************
 *               Function Definitions
 ******************************************************/

WEAK platform_result_t platform_button_init( platform_button_t button )
{
    return (platform_result_t)gpio_button_init( &platform_gpio_buttons[ button ] );
}

WEAK platform_result_t  platform_button_deinit( platform_button_t button )
{
    return (platform_result_t)gpio_button_deinit( &platform_gpio_buttons[ button ] );
}

WEAK platform_result_t platform_button_enable( platform_button_t button )
{
    return (platform_result_t)gpio_button_enable( &platform_gpio_buttons[ button ] );
}

WEAK platform_result_t platform_button_disable( platform_button_t button )
{
    return (platform_result_t)gpio_button_disable( &platform_gpio_buttons[ button ] );
}

WEAK wiced_bool_t platform_button_get_value( platform_button_t button )
{
    return gpio_button_get_value( &platform_gpio_buttons[ button ] );
}

WEAK platform_result_t platform_button_register_state_change_callback( platform_button_state_change_callback_t callback )
{
    user_callback = callback;
    return (platform_result_t)gpio_button_register_state_change_callback( platform_button_state_change_callback );
}

static void platform_button_state_change_callback( gpio_button_t* button, wiced_bool_t new_state )
{
    user_callback( ( platform_button_t ) ARRAY_POSITION( platform_gpio_buttons, button ), new_state );
}
