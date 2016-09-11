/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "sam4s_platform.h"
#include "string.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define PINS_PER_PORT (32) /* Px00 to Px31 */
#define TOTAL_PORTS   ( 3) /* PIOA to C    */

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

#pragma pack(1)
typedef struct
{
    wiced_bool_t              wakeup_pin;
    sam4s_gpio_irq_callback_t callback;
    void*                     arg;
} sam4s_gpio_irq_data_t;
#pragma pack()

/******************************************************
 *               Function Declarations
 ******************************************************/

void gpio_irq( void );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/* Flag to indicate whether active GPIO IRQ data is initialised */
static wiced_bool_t active_gpio_irq_data_initted = WICED_FALSE;

/* Pointer to active GPIO IRQs */
static sam4s_gpio_irq_data_t active_gpio_irq_data[TOTAL_PORTS * PINS_PER_PORT];

/* GPIO IRQ interrupt vectors */
static const IRQn_Type const irq_vectors[] =
{
    [0] = PIOA_IRQn,
    [1] = PIOB_IRQn,
    [2] = PIOC_IRQn,
};

/******************************************************
 *             SAM4S Function Definitions
 ******************************************************/

wiced_result_t sam4s_gpio_pin_init( const sam4s_pin_t* gpio_pin, const sam4s_gpio_pin_config_t* config )
{
    ioport_enable_pin( *gpio_pin );
    ioport_set_pin_mode( *gpio_pin, config->mode );
    ioport_set_pin_dir( *gpio_pin, config->direction );
    return WICED_SUCCESS;
}

wiced_result_t sam4s_peripheral_pin_init( const sam4s_pin_t* peripheral_pin, const sam4s_peripheral_pin_config_t* config )
{
    /* Set pin mode and disable GPIO */
    ioport_set_pin_mode( *peripheral_pin, *config );
    ioport_disable_pin( *peripheral_pin );

    return WICED_SUCCESS;
}

wiced_result_t sam4s_pin_deinit( const sam4s_pin_t* pin )
{
    ioport_disable_pin( *pin );
    return WICED_SUCCESS;
}

wiced_result_t sam4s_gpio_output_high( const sam4s_pin_t* gpio_pin )
{
    ioport_set_pin_level( *gpio_pin, IOPORT_PIN_LEVEL_HIGH );
    return WICED_SUCCESS;
}

wiced_result_t sam4s_gpio_output_low ( const sam4s_pin_t* gpio_pin )
{
    ioport_set_pin_level( *gpio_pin, IOPORT_PIN_LEVEL_LOW );
    return WICED_SUCCESS;
}

wiced_bool_t sam4s_gpio_get_input( const sam4s_pin_t* gpio_pin )
{
    return ( ioport_get_pin_level( *gpio_pin ) == false ) ? WICED_FALSE : WICED_TRUE;
}

wiced_result_t sam4s_gpio_irq_enable ( const sam4s_pin_t* gpio_pin, const sam4s_gpio_irq_config_t* config )
{
    ioport_port_mask_t mask          = ioport_pin_to_mask( *gpio_pin );
    ioport_port_t      port          = ioport_pin_to_port_id( *gpio_pin );
    volatile Pio*      port_register = arch_ioport_port_to_base( port );
    uint32_t           temp;

    UNUSED_VARIABLE( temp );

    if ( active_gpio_irq_data_initted == WICED_FALSE )
    {
        /* Initialise the first time this function is called */
        memset( &active_gpio_irq_data_initted, 0, sizeof( active_gpio_irq_data_initted ) );
        active_gpio_irq_data_initted = WICED_TRUE;
    }

    /* Reset and enable associated CPU interrupt vector */
    NVIC_DisableIRQ( irq_vectors[port] );
    NVIC_ClearPendingIRQ( irq_vectors[port] );
    NVIC_SetPriority( irq_vectors[port], SAM4S_GPIO_IRQ_PRIO );
    NVIC_EnableIRQ( irq_vectors[port] );

    active_gpio_irq_data[*gpio_pin].wakeup_pin = config->wakeup_pin;
    active_gpio_irq_data[*gpio_pin].arg        = config->arg;
    active_gpio_irq_data[*gpio_pin].callback   = config->callback;

    if ( config->trigger == IOPORT_SENSE_RISING || config->trigger == IOPORT_SENSE_BOTHEDGES )
    {
       port_register->PIO_AIMER  |= mask;
       port_register->PIO_ESR    |= mask;
       port_register->PIO_REHLSR |= mask;
    }

    if ( config->trigger == IOPORT_SENSE_FALLING || config->trigger == IOPORT_SENSE_BOTHEDGES )
    {
        port_register->PIO_AIMER  |= mask;
        port_register->PIO_ESR    |= mask;
        port_register->PIO_FELLSR |= mask;
    }

    /* Read ISR to clear residual interrupt status */
    temp = port_register->PIO_ISR;

    /* Enable interrupt source */
    port_register->PIO_IER |= mask;

    return WICED_SUCCESS;
}

wiced_result_t sam4s_gpio_irq_disable( const sam4s_pin_t* gpio_pin )
{
    ioport_port_mask_t mask          = ioport_pin_to_mask( *gpio_pin );
    ioport_port_t      port          = ioport_pin_to_port_id( *gpio_pin );
    volatile Pio*      port_register = arch_ioport_port_to_base( port );

    /* Disable interrupt on pin */
    port_register->PIO_IDR = mask;

    /* Disable Cortex-M interrupt vector as well if no pin interrupt is enabled */
    if ( port_register->PIO_IMR == 0 )
    {
        NVIC_DisableIRQ( irq_vectors[port] );
    }

    return WICED_SUCCESS;
}

/******************************************************
 *                  ISR Definitions
 ******************************************************/

void gpio_irq( void )
{
    ioport_port_t port;

    for ( port = 0; port < TOTAL_PORTS; port++ )
    {
        volatile Pio* port_register = arch_ioport_port_to_base( port );
        uint32_t      status        = port_register->PIO_ISR; /* Get interrupt status. Read clears the interrupt */
        uint32_t      mask          = port_register->PIO_IMR;
        uint32_t      iter          = 0;

        if ( ( status != 0 ) && ( mask != 0 ) )
        {
            /* Call the respective GPIO interrupt handler/callback */
            for ( iter = 0; iter < PINS_PER_PORT; iter++, status >>= 1, mask >>= 1 )
            {
                uint32_t gpio_irq_data_index = (port * PINS_PER_PORT) + iter;

                if ( ( ( mask & 0x1 ) != 0 ) && ( ( status & 0x1 ) != 0 ) && ( active_gpio_irq_data[gpio_irq_data_index].callback != NULL ) )
                {
                    if ( active_gpio_irq_data[gpio_irq_data_index].wakeup_pin == WICED_TRUE )
                    {
                        sam4s_powersave_wake_up_interrupt_notify();
                    }

                    active_gpio_irq_data[gpio_irq_data_index].callback( active_gpio_irq_data[gpio_irq_data_index].arg );
                }
            }
        }
    }
}
