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
 */
#include "platform_peripheral.h"
#include "wwd_assert.h"

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

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

platform_result_t platform_adc_init( const platform_adc_t* adc, uint32_t sample_cycle )
{
    UNUSED_PARAMETER( adc );
    UNUSED_PARAMETER( sample_cycle );
    return PLATFORM_UNSUPPORTED;
}

platform_result_t platform_adc_deinit( const platform_adc_t* adc )
{
    UNUSED_PARAMETER( adc );
    return PLATFORM_UNSUPPORTED;
}

platform_result_t platform_adc_take_sample( const platform_adc_t* adc, uint16_t* output )
{
    UNUSED_PARAMETER( adc );
    wiced_assert( "output is NULL", output != NULL );
    *output = 0;
    return PLATFORM_UNSUPPORTED;
}

platform_result_t platform_adc_take_sample_stream( const platform_adc_t* adc, void* buffer, uint16_t buffer_length )
{
    UNUSED_PARAMETER( adc );
    UNUSED_PARAMETER( buffer );
    UNUSED_PARAMETER( buffer_length );
    return PLATFORM_UNSUPPORTED;
}
