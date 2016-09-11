/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "MK60N512VMD100.h"
#include "wiced_defaults.h"
#include "wiced_platform.h"
#include "wwd_constants.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    K60_GPIO_INPUT,
    K60_GPIO_OUTPUT,
}k60_gpio_direction_t;

typedef enum
{
    K60_GPIO_A,
    K60_GPIO_B,
    K60_GPIO_C,
    K60_GPIO_D,
    K60_GPIO_E
}k60_gpio_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef void (*k60_gpio_irq_callback)( void* );
typedef wiced_gpio_irq_trigger_t gpio_irq_trigger_t;
typedef wiced_gpio_config_t      gpio_config_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t k60_gpio_init        ( k60_gpio_t gpio, uint8_t pin, gpio_config_t config );

wiced_result_t k60_gpio_select_mux  ( k60_gpio_t gpio, uint8_t pin, uint8_t pin_control_register_mux );

wiced_result_t k60_gpio_output_high ( k60_gpio_t gpio, uint8_t pin );

wiced_result_t k60_gpio_output_low  ( k60_gpio_t gpio, uint8_t pin );

wiced_bool_t   k60_gpio_input_get   ( k60_gpio_t gpio, uint8_t pin );

wiced_result_t k60_gpio_irq_enable  ( k60_gpio_t gpio, uint8_t pin, gpio_irq_trigger_t trigger, k60_gpio_irq_callback callback, void* arg );

wiced_result_t k60_gpio_irq_disable ( k60_gpio_t gpio, uint8_t pin );
