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
 *
 * GPIO-button implementation
 *
 */

#include "wwd_assert.h"
#include "wiced_platform.h"
#include "gpio_button.h"

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

static void gpio_button_interrupt_handler( void* args );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static gpio_button_state_change_callback_t button_state_change_callback;

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t gpio_button_init( const gpio_button_t* button )
{
    wiced_assert( "Bad args", (button != NULL) );
    return wiced_gpio_init( button->gpio, ( button->polarity == WICED_ACTIVE_HIGH )? INPUT_PULL_UP: INPUT_PULL_DOWN );
}

wiced_result_t gpio_button_deinit( const gpio_button_t* button )
{
    return wiced_gpio_deinit( button->gpio );
}

wiced_result_t gpio_button_register_state_change_callback( gpio_button_state_change_callback_t callback )
{
    if ( !callback )
    {
        return WICED_BADARG;
    }

    button_state_change_callback = callback;

    return WICED_SUCCESS;
}

wiced_result_t gpio_button_enable( const gpio_button_t* button )
{
    wiced_gpio_irq_trigger_t trigger;

    wiced_assert( "Bad args", (button != NULL) );

    if ( button->trigger == 0 )
    {
        trigger = ( ( button->polarity == WICED_ACTIVE_LOW ) ? IRQ_TRIGGER_RISING_EDGE : IRQ_TRIGGER_FALLING_EDGE );
    }
    else
    {
        trigger = button->trigger;
    }

    return wiced_gpio_input_irq_enable( button->gpio, trigger, gpio_button_interrupt_handler, (void*)button );
}

wiced_result_t gpio_button_disable( const gpio_button_t* button )
{
    wiced_assert( "Bad args", (button != NULL) );

    return wiced_gpio_input_irq_disable( button->gpio );
}

wiced_bool_t gpio_button_get_value( const gpio_button_t* button )
{
    wiced_bool_t value;

    wiced_assert( "Bad args", (button != NULL) );

    value = wiced_gpio_input_get( button->gpio );
    return value;
}


static void gpio_button_interrupt_handler( void* args )
{
    const gpio_button_t* button = (const gpio_button_t*)args;
    wiced_bool_t   gpio_state;
    wiced_bool_t   is_pressed;

    if( !button_state_change_callback || !button )
    {
        return;
    }

    gpio_state = wiced_gpio_input_get( button->gpio );

    is_pressed = ( button->polarity == WICED_ACTIVE_HIGH ) ? ( (gpio_state  == WICED_FALSE ) ? WICED_TRUE : WICED_FALSE ) : ( (gpio_state == WICED_FALSE ) ? WICED_FALSE : WICED_TRUE );

    button_state_change_callback( (void*)button, is_pressed );
}
