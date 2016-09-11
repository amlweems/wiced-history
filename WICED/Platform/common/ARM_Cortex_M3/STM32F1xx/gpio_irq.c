/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "gpio_irq.h"
#include "string.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define NUMBER_OF_GPIO_IRQ_LINES         (16)
#define EXICR_BASE_ADDRESS  ( AFIO_BASE + 8 )

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    gpio_port_t*       owner_port;
    gpio_irq_handler_t handler;
    void*              arg;
} gpio_irq_data_t;

/******************************************************
 *               Function Declarations
 ******************************************************/

static uint8_t        convert_port_to_port_number( gpio_port_t* port );
static wiced_result_t gpio_irq_line_select( gpio_port_t* gpio_port, gpio_pin_number_t gpio_pin_number );

/******************************************************
 *               Variables Definitions
 ******************************************************/

static volatile gpio_irq_data_t gpio_irq_data[NUMBER_OF_GPIO_IRQ_LINES];
static uint8_t                  gpio_irq_management_initted = 0;

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t gpio_irq_enable( gpio_port_t* gpio_port, gpio_pin_number_t gpio_pin_number, gpio_irq_trigger_t trigger, gpio_irq_handler_t handler, void* arg )
{
    uint32_t interrupt_line = (uint32_t) ( 1 << gpio_pin_number );

    if( gpio_pin_number > 15 )
    {
        return WICED_BADARG;
    }

	if ( gpio_irq_management_initted == 0 )
    {
        memset( (void*)gpio_irq_data, 0, sizeof( gpio_irq_data ) );

        gpio_irq_management_initted = 1;
    }

    if ( ( EXTI->IMR & interrupt_line ) == 0 )
    {
        NVIC_InitTypeDef nvic_init_structure;
        EXTI_InitTypeDef exti_init_structure;

        gpio_irq_line_select( gpio_port, gpio_pin_number );

        if ( trigger == ( IRQ_TRIGGER_FALLING_EDGE | IRQ_TRIGGER_RISING_EDGE ) )
        {
            exti_init_structure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
        }
        else if ( trigger == IRQ_TRIGGER_FALLING_EDGE )
        {
            exti_init_structure.EXTI_Trigger = EXTI_Trigger_Falling;
        }
        else if ( trigger == IRQ_TRIGGER_RISING_EDGE )
        {
            exti_init_structure.EXTI_Trigger = EXTI_Trigger_Rising;
        }

        exti_init_structure.EXTI_Line    = interrupt_line;
        exti_init_structure.EXTI_Mode    = EXTI_Mode_Interrupt;
        exti_init_structure.EXTI_LineCmd = ENABLE;
        EXTI_Init( &exti_init_structure );

        if ( gpio_pin_number <= 4 )
        {
            nvic_init_structure.NVIC_IRQChannel = (uint8_t) ( EXTI0_IRQn + gpio_pin_number );
        }
        else if ( gpio_pin_number <= 9 )
        {
            nvic_init_structure.NVIC_IRQChannel = EXTI9_5_IRQn;
        }
        else if ( gpio_pin_number <= 15 )
        {
            nvic_init_structure.NVIC_IRQChannel = EXTI15_10_IRQn;
        }

        /* Must be lower priority than the value of configMAX_SYSCALL_INTERRUPT_PRIORITY otherwise FreeRTOS will not be able to mask the interrupt */
        nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0xE;
        nvic_init_structure.NVIC_IRQChannelSubPriority        = 0xC;
        nvic_init_structure.NVIC_IRQChannelCmd                = ENABLE;
        NVIC_Init( &nvic_init_structure );

        gpio_irq_data[gpio_pin_number].owner_port = gpio_port;
        gpio_irq_data[gpio_pin_number].handler    = handler;
        gpio_irq_data[gpio_pin_number].arg        = arg;
        return WICED_SUCCESS;
    }

    return WICED_ERROR;
}

wiced_result_t gpio_irq_disable( gpio_port_t* gpio_port, gpio_pin_number_t gpio_pin_number )
{
    uint32_t interrupt_line     = (uint32_t) ( 1 << gpio_pin_number );
    uint8_t interrupt_line_used = 0;

    if ( ( EXTI->IMR & interrupt_line ) && gpio_irq_data[gpio_pin_number].owner_port == gpio_port )
    {
        NVIC_InitTypeDef nvic_init_structure;
        EXTI_InitTypeDef exti_init_structure;

        /* Disable EXTI interrupt line */
        exti_init_structure.EXTI_Line    = (uint32_t) ( 1 << gpio_pin_number );
        exti_init_structure.EXTI_LineCmd = DISABLE;
        exti_init_structure.EXTI_Mode    = EXTI_Mode_Interrupt;
        exti_init_structure.EXTI_Trigger = EXTI_Trigger_Rising;
        EXTI_Init( &exti_init_structure );
        exti_init_structure.EXTI_Trigger = EXTI_Trigger_Falling;
        EXTI_Init( &exti_init_structure );

        /* Disable NVIC interrupt */
        if ( gpio_pin_number <= 4 )
        {
            nvic_init_structure.NVIC_IRQChannel = (uint8_t) ( EXTI0_IRQn + gpio_pin_number );
            interrupt_line_used = 0;
        }
        else if ( gpio_pin_number <= 9 )
        {
            nvic_init_structure.NVIC_IRQChannel = EXTI9_5_IRQn;
            interrupt_line_used = ( ( EXTI->IMR & 0x3e0U ) != 0 );
        }
        else if ( gpio_pin_number <= 15 )
        {
            nvic_init_structure.NVIC_IRQChannel = EXTI15_10_IRQn;
            interrupt_line_used = ( ( EXTI->IMR & 0xfc00U ) != 0 );
        }

        if ( !interrupt_line_used )
        {
            nvic_init_structure.NVIC_IRQChannelCmd                = DISABLE;
            nvic_init_structure.NVIC_IRQChannelPreemptionPriority = 0;
            nvic_init_structure.NVIC_IRQChannelSubPriority        = 0;
            NVIC_Init( &nvic_init_structure );
        }

        gpio_irq_data[gpio_pin_number].owner_port = 0;
        gpio_irq_data[gpio_pin_number].handler    = 0;
        gpio_irq_data[gpio_pin_number].arg        = 0;
        return WICED_SUCCESS;
    }

    return WICED_ERROR;
}

void gpio_irq( void )
{
    uint32_t active_interrupt_vector = (uint32_t) ( ( SCB ->ICSR & 0x3fU ) - 16 );
    uint32_t gpio_number;
    uint32_t interrupt_line;

    switch ( active_interrupt_vector )
    {
        case EXTI0_IRQn:
            interrupt_line = EXTI_Line0;
            gpio_number = 0;
            break;
        case EXTI1_IRQn:
            interrupt_line = EXTI_Line1;
            gpio_number = 1;
            break;
        case EXTI2_IRQn:
            interrupt_line = EXTI_Line2;
            gpio_number = 2;
            break;
        case EXTI3_IRQn:
            interrupt_line = EXTI_Line3;
            gpio_number = 3;
            break;
        case EXTI4_IRQn:
            interrupt_line = EXTI_Line4;
            gpio_number = 4;
            break;
        case EXTI9_5_IRQn:
            interrupt_line = EXTI_Line5;
            for ( gpio_number = 5; gpio_number < 10 && ( EXTI ->PR & interrupt_line ) == 0; gpio_number++ )
            {
                interrupt_line <<= 1;
            }
            break;
        case EXTI15_10_IRQn:
            interrupt_line = EXTI_Line10;
            for ( gpio_number = 10; gpio_number < 16 && ( EXTI ->PR & interrupt_line ) == 0; gpio_number++ )
            {
                interrupt_line <<= 1;
            }
            break;
        default:
            return;
    }

    /* Clear interrupt flag */EXTI ->PR = interrupt_line;

    /* Call the respective GPIO interrupt handler/callback */
    if ( gpio_irq_data[gpio_number].handler != NULL )
    {
        gpio_irq_data[gpio_number].handler( gpio_irq_data[gpio_number].arg );
    }
}

static uint8_t convert_port_to_port_number( gpio_port_t* port )
{
    switch ( (int) port )
    {
        case GPIOA_BASE :
            return 0;
        case GPIOB_BASE :
            return 1;
        case GPIOC_BASE :
            return 2;
        case GPIOD_BASE :
            return 3;
        case GPIOE_BASE :
            return 4;
        case GPIOF_BASE :
            return 5;
        case GPIOG_BASE :
            return 6;
        default:
            return 7;
    }

    return 0;
}

static wiced_result_t gpio_irq_line_select( gpio_port_t* gpio_port, gpio_pin_number_t gpio_pin_number )
{
	uint8_t port_number = 0;
    uint32_t* exicr = (uint32_t*) EXICR_BASE_ADDRESS;

    port_number = convert_port_to_port_number( gpio_port );
    if ( port_number > 6 )
    {
        return WICED_BADARG;
    }
    /* get register address that has to be updated */
    exicr += gpio_pin_number / 4;
    /* connect external interrupt line to the gpio pin by writing the proper EXTICR register */
    *exicr = *exicr  |  (uint32_t) ( ( port_number & 0x0F ) << ( ( gpio_pin_number % 4 ) * 4 ) );
    return WICED_SUCCESS;
}
