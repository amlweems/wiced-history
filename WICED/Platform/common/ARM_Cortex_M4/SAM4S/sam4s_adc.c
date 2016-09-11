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

static const sam4s_gpio_pin_config_t standard_adc_pin_config =
{
    .direction = IOPORT_DIR_INPUT,
    .mode      = 0,
};

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t sam4s_adc_init( const sam4s_adc_t* adc, uint8_t sampling_cycle )
{
    sysclk_enable_peripheral_clock( adc->peripheral_id );

    sam4s_gpio_pin_init( adc->adc_pin, &standard_adc_pin_config );

    adc_init( adc->peripheral, CPU_CLOCK_HZ, adc->adc_clock_hz, 8 );

    /* Maximum track time is 16 cycles */
    if ( sampling_cycle > 16 )
    {
        sampling_cycle = 16;
    }

    /* Tracking time = TRACKTIM + 1 */
    sampling_cycle--;
    adc_configure_timing( adc->peripheral, sampling_cycle, adc->settling_time, 1 );

    adc_set_resolution( adc->peripheral, adc->resolution );

    adc_enable_channel( adc->peripheral, adc->channel );

    adc_configure_trigger( adc->peripheral, adc->trigger, 0 );

    return WICED_SUCCESS;
}

wiced_result_t sam4s_adc_deinit( const sam4s_adc_t* adc )
{
    adc_disable_channel( adc->peripheral, adc->channel );

    sam4s_pin_deinit( adc->adc_pin );

    return WICED_SUCCESS;
}

wiced_result_t sam4s_adc_start_software_conversion( const sam4s_adc_t* adc, uint16_t* value )
{
    uint32_t temp;

    adc_start( adc->peripheral );

    while ( ( adc_get_status( adc->peripheral ) & ADC_ISR_DRDY ) == 0 )
    {
    }

    temp = adc_get_latest_value( ADC );

    *value = (uint16_t)( temp & 0xffffffffU ) ;

    return WICED_SUCCESS;
}
