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
 * Audio timer clock functionality for the BCM43909.
 */

#include "wiced_result.h"
#include "wiced_utilities.h"
#include "platform_peripheral.h"
#include "platform_ascu.h"

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

static wiced_bool_t          audio_timer_enabled   = WICED_FALSE;
static host_semaphore_type_t audio_timer_semaphore;

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t platform_audio_timer_enable( uint32_t audio_frame_count )
{
    wiced_result_t result = WICED_ERROR;

    wiced_jump_when_not_true( (audio_timer_enabled == WICED_FALSE), _exit );

    result = host_rtos_init_semaphore( &audio_timer_semaphore );
    wiced_jump_when_not_true( (result == WICED_SUCCESS), _exit );

    platform_ascu_disable_interrupts   ( ASCU_ASTP_INT_MASK );
    if ( audio_frame_count > 0 )
    {
        platform_ascu_set_frame_sync_period( audio_frame_count );
        platform_ascu_enable_interrupts    ( ASCU_ASTP_INT_MASK );
    }
    platform_pmu_chipcontrol(PMU_CHIPCONTROL_PWM_CLK_ASCU_REG, 0, PMU_CHIPCONTROL_PWM_CLK_ASCU_MASK);

    audio_timer_enabled = WICED_TRUE;

_exit:
    return result;
}


wiced_result_t platform_audio_timer_disable( void )
{
    wiced_result_t result = WICED_SUCCESS;

    wiced_jump_when_not_true( (audio_timer_enabled == WICED_TRUE), _exit );

    audio_timer_enabled = WICED_FALSE;

    platform_ascu_disable_interrupts   ( ASCU_ASTP_INT_MASK );

    host_rtos_set_semaphore( &audio_timer_semaphore, WICED_FALSE );
    result = host_rtos_deinit_semaphore( &audio_timer_semaphore );

_exit:
    return result;
}


wiced_result_t platform_audio_timer_get_frame_sync( uint32_t timeout_msecs )
{
    wiced_result_t result = WICED_ERROR;

    wiced_jump_when_not_true( (audio_timer_enabled == WICED_TRUE), _exit );

    result = host_rtos_get_semaphore( &audio_timer_semaphore, timeout_msecs, WICED_TRUE );

_exit:
    return result;
}


wiced_result_t platform_audio_timer_set_frame_sync( void )
{
    return host_rtos_set_semaphore( &audio_timer_semaphore, WICED_TRUE );
}


wiced_result_t platform_audio_timer_get_time( uint32_t *time_hi, uint32_t *time_lo )
{
    return (wiced_result_t) platform_ascu_read_fw_audio_timer( time_hi, time_lo );
}


wiced_result_t platform_audio_timer_get_resolution( uint32_t audio_sample_rate, uint32_t *ticks_per_sec )
{
    return (wiced_result_t) platform_ascu_get_audio_timer_resolution( audio_sample_rate, ticks_per_sec );
}
