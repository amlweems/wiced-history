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
 * Audio timer clock functionality
 */

#include "wiced_result.h"
#include "platform_audio.h"


/******************************************************
 *                      Macros
 ******************************************************/

#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER( x ) ( (void)(x) )
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

wiced_result_t platform_audio_timer_enable( uint32_t audio_frame_count )
{
    UNUSED_PARAMETER( audio_frame_count );
    return WICED_UNSUPPORTED;
}


wiced_result_t platform_audio_timer_disable( void )
{
    return WICED_UNSUPPORTED;
}


wiced_result_t platform_audio_timer_get_frame_sync( uint32_t timeout_msecs )
{
    UNUSED_PARAMETER( timeout_msecs );
    return WICED_UNSUPPORTED;
}


wiced_result_t platform_audio_timer_set_frame_sync( void )
{
    return WICED_UNSUPPORTED;
}


wiced_result_t platform_audio_timer_get_time( uint32_t *time_hi, uint32_t *time_lo )
{
    UNUSED_PARAMETER( time_hi );
    UNUSED_PARAMETER( time_lo );
    return WICED_UNSUPPORTED;
}


wiced_result_t platform_audio_timer_get_resolution( uint32_t audio_sample_rate, uint32_t *ticks_per_sec )
{
    UNUSED_PARAMETER( audio_sample_rate );
    UNUSED_PARAMETER( ticks_per_sec );
    return WICED_UNSUPPORTED;
}
