/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
/** @file
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

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
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

extern platform_result_t platform_init_audio( void );
extern platform_result_t platform_deinit_audio( void );

/**
 * Enable audio timer
 *
 * @param[in]  audio_frame_count : audio timer interrupts period expressed in number of audio samples/frames
 *
 * @return @ref wiced_result_t
 */
wiced_result_t platform_audio_timer_enable( uint32_t frame_count );


/**
 * Disable audio timer
 *
 * @return @ref wiced_result_t
 */
wiced_result_t platform_audio_timer_disable( void );


/**
 * Wait for audio timer frame sync event
 *
 * @param[in]  timeout_msecs     : timeout value in msecs; WICED_NO_WAIT or WICED_WAIT_FOREVER otherwise.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t platform_audio_timer_get_frame_sync( uint32_t timeout_msecs );


/**
 * Read audio timer value (tick count)
 *
 * @param[out] time_hi           : upper 32-bit of 64-bit audio timer ticks
 * @param[out] time_lo           : lower 32-bit of 64-bit audio timer ticks
 *
 * @return @ref wiced_result_t
 */
wiced_result_t platform_audio_timer_get_time( uint32_t *time_hi, uint32_t *time_lo );


/**
 * Get audio timer resolution (ticks per second)
 *
 * @param[in]  audio_sample_rate : audio sample rate
 * @param[out] ticks_per_sec     : returned audio timer resolution
 *
 * @return @ref wiced_result_t
 */
wiced_result_t platform_audio_timer_get_resolution( uint32_t audio_sample_rate, uint32_t *ticks_per_sec );


#ifdef __cplusplus
} /*extern "C" */
#endif
