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

wiced_result_t wiced_gpio_init( wiced_gpio_t gpio, wiced_gpio_config_t configuration )
{
    sam4s_gpio_pin_config_t gpio_pin_config;

    sam4s_powersave_clocks_needed();

    switch ( configuration )
    {
        case INPUT_PULL_UP:
        {
            gpio_pin_config.direction = IOPORT_DIR_INPUT;
            gpio_pin_config.mode      = IOPORT_MODE_PULLUP;
            break;
        }
        case INPUT_PULL_DOWN:
        {
            gpio_pin_config.direction = IOPORT_DIR_INPUT;
            gpio_pin_config.mode      = IOPORT_MODE_PULLDOWN;
            break;
        }
        case INPUT_HIGH_IMPEDANCE:
        {
            gpio_pin_config.direction = IOPORT_DIR_INPUT;
            gpio_pin_config.mode      = 0;
            break;
        }
        case OUTPUT_PUSH_PULL:
        {
            gpio_pin_config.direction = IOPORT_DIR_OUTPUT;
            gpio_pin_config.mode      = 0;
            break;
        }
        case OUTPUT_OPEN_DRAIN_NO_PULL:
        {
            gpio_pin_config.direction = IOPORT_DIR_OUTPUT;
            gpio_pin_config.mode      = IOPORT_MODE_OPEN_DRAIN;
            break;
        }
        case OUTPUT_OPEN_DRAIN_PULL_UP:
        {
            gpio_pin_config.direction = IOPORT_DIR_OUTPUT;
            gpio_pin_config.mode      = IOPORT_MODE_OPEN_DRAIN | IOPORT_MODE_PULLUP;
            break;
        }
        default:
        {
            sam4s_powersave_clocks_not_needed();
            return WICED_BADARG;
        }
    }

    sam4s_gpio_pin_init( &platform_gpio_pin_mapping[gpio], &gpio_pin_config );

    sam4s_powersave_clocks_not_needed();

    return WICED_SUCCESS;
}

wiced_result_t wiced_gpio_output_high( wiced_gpio_t gpio )
{
    sam4s_powersave_clocks_needed();
    sam4s_gpio_output_high( &platform_gpio_pin_mapping[gpio] );
    sam4s_powersave_clocks_not_needed();
    return WICED_SUCCESS;
}

wiced_result_t wiced_gpio_output_low( wiced_gpio_t gpio )
{
    sam4s_powersave_clocks_needed();
    sam4s_gpio_output_low( &platform_gpio_pin_mapping[gpio] );
    sam4s_powersave_clocks_not_needed();
    return WICED_SUCCESS;
}

wiced_bool_t wiced_gpio_input_get( wiced_gpio_t gpio )
{
    wiced_bool_t value;

    sam4s_powersave_clocks_needed( );
    value = sam4s_gpio_get_input( &platform_gpio_pin_mapping[gpio] );
    sam4s_powersave_clocks_not_needed( );
    return value;
}

wiced_result_t wiced_gpio_input_irq_enable( wiced_gpio_t gpio, wiced_gpio_irq_trigger_t trigger, wiced_gpio_irq_handler_t handler, void* arg )
{
    wiced_result_t            result;
    sam4s_gpio_irq_config_t   config;
    sam4s_wakeup_pin_config_t wakeup_config;

    sam4s_powersave_clocks_needed( );

    switch ( trigger )
    {
        case IRQ_TRIGGER_RISING_EDGE:
        {
            config.trigger = IOPORT_SENSE_RISING;
            break;
        }
        case IRQ_TRIGGER_FALLING_EDGE:
        {
            config.trigger = IOPORT_SENSE_FALLING;
            break;
        }
        case IRQ_TRIGGER_BOTH_EDGES:
        {
            config.trigger = IOPORT_SENSE_BOTHEDGES;
            break;
        }
        default:
        {
            return WICED_BADARG;
        }
    }

    config.wakeup_pin = platform_wakeup_pin_config[gpio].is_wakeup_pin;
    config.arg        = arg;
    config.callback   = handler;

    result = sam4s_gpio_irq_enable( &platform_gpio_pin_mapping[gpio], &config );

    wakeup_config.is_wakeup_pin     = platform_wakeup_pin_config[gpio].is_wakeup_pin;
    wakeup_config.wakeup_pin_number = platform_wakeup_pin_config[gpio].wakeup_pin_number;
    wakeup_config.trigger           = ( IRQ_TRIGGER_RISING_EDGE | trigger ) ? IOPORT_SENSE_RISING : IOPORT_SENSE_FALLING;

    sam4s_powersave_enable_wakeup_pin( &wakeup_config );

    sam4s_powersave_clocks_not_needed( );

    return result;
}

wiced_result_t wiced_gpio_input_irq_disable( wiced_gpio_t gpio )
{
    wiced_result_t result;

    sam4s_powersave_clocks_needed( );

    result = sam4s_gpio_irq_disable( &platform_gpio_pin_mapping[gpio] );

    sam4s_powersave_disable_wakeup_pin( &platform_wakeup_pin_config[gpio] );

    sam4s_powersave_clocks_not_needed( );

    return result;
}
