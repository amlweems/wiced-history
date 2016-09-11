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
#include <stdint.h>
#include "stm32f10x.h"
#include "wwd_constants.h"
#include "wwd_assert.h"
#include "gpio_irq.h"
#include "watchdog.h"
#include "platform_common_config.h"
#include "wiced_platform.h"
#include "Platform/wwd_platform_interface.h"

#ifndef WL_RESET_BANK
#error Missing WL_RESET_BANK definition
#endif
#ifndef WL_REG_ON_BANK
#error Missing WL_REG_ON_BANK definition
#endif

/******************************************************
 *                      Macros
 ******************************************************/
#ifdef __GNUC__
#define TRIGGER_BREAKPOINT() __asm__("bkpt")
#elif defined ( __IAR_SYSTEMS_ICC__ )
#define TRIGGER_BREAKPOINT() __asm("bkpt 0")
#endif

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

wiced_result_t host_platform_init( void )
{
    GPIO_InitTypeDef gpio_init_structure;

    /* Enable the GPIO peripherals related to the reset and reg_on pins */
    RCC_APB2PeriphClockCmd( WL_RESET_BANK_CLK | WL_REG_ON_BANK_CLK, ENABLE );

    /* Setup the reset pin */
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init_structure.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init_structure.GPIO_Pin = WL_RESET_PIN;
    GPIO_Init( WL_RESET_BANK, &gpio_init_structure );
    host_platform_reset_wifi( WICED_TRUE ); /* Start wifi chip in reset */

    gpio_init_structure.GPIO_Pin = WL_REG_ON_PIN;
    GPIO_Init( WL_REG_ON_BANK, &gpio_init_structure );
    host_platform_power_wifi( WICED_FALSE ); /* Start wifi chip with regulators off */

    return WICED_SUCCESS;
}

wiced_result_t host_platform_deinit( void )
{
    GPIO_InitTypeDef gpio_init_structure;

    /* Re-Setup the reset pin and REG_ON pin - these need to be held low to keep the chip powered down */
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init_structure.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init_structure.GPIO_Pin = WL_RESET_PIN;
    GPIO_Init( WL_RESET_BANK, &gpio_init_structure );
    host_platform_reset_wifi( WICED_TRUE ); /* Start wifi chip in reset */

    gpio_init_structure.GPIO_Pin = WL_REG_ON_PIN;
    GPIO_Init( WL_REG_ON_BANK, &gpio_init_structure );
    host_platform_power_wifi( WICED_FALSE ); /* Start wifi chip with regulators off */

    return WICED_SUCCESS;
}

uint32_t host_platform_get_cycle_count(void)
{
/* From the ARM Cortex-M3 Techinical Reference Manual
 * 0xE0001004  DWT_CYCCNT  RW  0x00000000  Cycle Count Register */
    return *((volatile uint32_t*)0xE0001004);
}

wiced_bool_t host_platform_is_in_interrupt_context( void )
{
    /* From the ARM Cortex-M3 Techinical Reference Manual
     * 0xE000ED04   ICSR    RW [a]  Privileged  0x00000000  Interrupt Control and State Register */
    uint32_t active_interrupt_vector = (uint32_t)( SCB ->ICSR & 0x3fU );

    if ( active_interrupt_vector != 0 )
    {
        return WICED_TRUE;
    }
    else
    {
        return WICED_FALSE;
    }
}

wiced_result_t host_platform_init_wlan_powersave_clock( void )
{
    return WICED_UNSUPPORTED;
}

wiced_result_t host_platform_deinit_wlan_powersave_clock( void )
{
    return WICED_UNSUPPORTED;
}
