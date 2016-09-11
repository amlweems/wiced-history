/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/**
 * @file
 *
 * GPIO-based Button APIs
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                     Macros
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

typedef struct
{
    wiced_gpio_t                 gpio;
    wiced_active_state_t         polarity;
    wiced_gpio_irq_trigger_t     trigger;
} gpio_button_t;

typedef void (*gpio_button_state_change_callback_t)( gpio_button_t* button, wiced_bool_t new_state );

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t gpio_button_init     ( const gpio_button_t* button );
wiced_result_t gpio_button_deinit   ( const gpio_button_t* button );
wiced_result_t gpio_button_enable   ( const gpio_button_t* button );
wiced_result_t gpio_button_disable  ( const gpio_button_t* button );
wiced_bool_t   gpio_button_get_value( const gpio_button_t* button );
wiced_result_t gpio_button_register_state_change_callback( gpio_button_state_change_callback_t callback );

#ifdef __cplusplus
} /*extern "C" */
#endif
