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
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t wiced_adc_init( wiced_adc_t adc, uint32_t sampling_cycle )
{
    wiced_result_t result;

    sam4s_powersave_clocks_needed();

    result = sam4s_adc_init( &platform_adc[adc], (uint8_t)sampling_cycle );

    sam4s_powersave_clocks_not_needed();

    return result;
}

wiced_result_t wiced_adc_take_sample( wiced_adc_t adc, uint16_t* output )
{
    wiced_result_t result;

    sam4s_powersave_clocks_needed();

    result = sam4s_adc_start_software_conversion( &platform_adc[adc], output );

    sam4s_powersave_clocks_not_needed();

    return result;
}

wiced_result_t wiced_adc_take_sample_stream( wiced_adc_t adc, void* buffer, uint16_t buffer_length )
{
    UNUSED_PARAMETER( adc );
    UNUSED_PARAMETER( buffer );
    UNUSED_PARAMETER( buffer_length );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_adc_deinit( wiced_adc_t adc )
{
    wiced_result_t result;

    sam4s_powersave_clocks_needed();

    result = sam4s_adc_deinit( &platform_adc[adc] );

    sam4s_powersave_clocks_not_needed();

    return result;
}
