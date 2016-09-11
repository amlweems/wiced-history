/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/**
 * @file Audio PLL tuning
 */

#pragma once

#include "wiced_result.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/*
 * set of default values appropriate for initialization of audio_pll_tuner_init_params_t
 */

#define AUDIO_PLL_TUNER_DEFAULT_ADJ_PPM_MAX                    (+2000.0f)
#define AUDIO_PLL_TUNER_DEFAULT_ADJ_PPM_MIN                    (-2000.0f)
#define AUDIO_PLL_TUNER_DEFAULT_ADJ_RATE_PPM_PER_MSEC          (2.0f)
#define AUDIO_PLL_TUNER_DEFAULT_ADJ_ATTACK_RATE                (0.020f)
#define AUDIO_PLL_TUNER_DEFAULT_ADJ_DECAY_RATE                 (0.015f)
#define AUDIO_PLL_TUNER_DEFAULT_LEVEL_CORRECTION_THRES_MAX     (+2000.0f)
#define AUDIO_PLL_TUNER_DEFAULT_LEVEL_CORRECTION_THRES_MIN     (-2000.0f)
#define AUDIO_PLL_TUNER_DEFAULT_FREQUENCY_CORRECTION_INCREMENT (5.0f)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    AUDIO_PLL_TUNER_TIME_UNIT_UNKNOWN          = -1,
    AUDIO_PLL_TUNER_TIME_UNIT_NANOSECONDS      =  0,
    AUDIO_PLL_TUNER_TIME_UNIT_PART_PER_BILLION =  1,
} audio_pll_tuner_time_unit_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct audio_pll_tuner_s *audio_pll_tuner_ref;

typedef struct
{
    uint64_t                    ts_reference;
    uint64_t                    ts_audio_timer;
    uint32_t                    payload_size;
    audio_pll_tuner_time_unit_t ts_audio_timer_unit;
} audio_pll_tuner_timestamp_t;

typedef wiced_result_t (*audio_pll_tuner_start_timer_cb_t)       (uint32_t audio_sample_count,     void *user_context);
typedef wiced_result_t (*audio_pll_tuner_stop_timer_cb_t)        (                                 void *user_context);
typedef wiced_result_t (*audio_pll_tuner_wait_for_period_cb_t)   (uint32_t timeout_ms,             void *user_context);
typedef wiced_result_t (*audio_pll_tuner_get_buffer_level_cb_t)  (uint32_t *level_in_bytes,        void *user_context);
typedef wiced_result_t (*audio_pll_tuner_set_ppm_cb_t)           (float ppm,                       void *user_context);
typedef wiced_result_t (*audio_pll_tuner_get_time_cb_t)          (audio_pll_tuner_timestamp_t *ts, void *user_context);


/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    float                                   adjustment_ppm_max;
    float                                   adjustment_ppm_min;
    float                                   adjustment_ppm_per_msec;
    float                                   adjustment_attack_rate;
    float                                   adjustment_decay_rate;
    float                                   level_correction_threshold_high;
    float                                   level_correction_threshold_low;
    float                                   frequency_correction_ppm_increment;
    void                                   *user_context;
    audio_pll_tuner_start_timer_cb_t        timer_start;
    audio_pll_tuner_stop_timer_cb_t         timer_stop;
    audio_pll_tuner_wait_for_period_cb_t    period_wait;
    audio_pll_tuner_get_buffer_level_cb_t   buffer_level_get;
    audio_pll_tuner_set_ppm_cb_t            ppm_set;
    audio_pll_tuner_get_time_cb_t           get_time;
} audio_pll_tuner_init_params_t;

typedef struct
{
    uint32_t target_buffer_level_bytes;
    uint32_t sample_rate;
    uint8_t  bits_per_sample;
    uint8_t  channels;
} audio_pll_tuner_start_params_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/** Initialize audio PLL tuner library
 *
 * @param[ in] params          : Audio PLL tuning input parameters
 * @param[out] pll_tuner_ptr   : Storage for Audio PLL tuner handle @ref audio_pll_tuner_ref
 *
 * @return @ref wiced_result_t
 */
wiced_result_t audio_pll_tuner_init( audio_pll_tuner_init_params_t *params, audio_pll_tuner_ref *pll_tuner_ptr );


/** Clean up audio PLL tuner library
 *
 * @param[ in] pll_tuner       : Audio PLL tuner handle
 *
 * @return @ref wiced_result_t
 */
wiced_result_t audio_pll_tuner_deinit( audio_pll_tuner_ref pll_tuner );


/** Start audio PLL tuning
 *
 * @param[ in] pll_tuner       : Audio PLL tuner handle
 * @param[ in] sample_rate     : Audio sample rate
 * @param[ in] bits_per_sample : Audio sample size (in bits)
 * @param[ in] channels        : Audio channel count
 *
 * @return @ref wiced_result_t
 */
wiced_result_t audio_pll_tuner_start( audio_pll_tuner_ref pll_tuner,  audio_pll_tuner_start_params_t *params );


/** Stop audio PLL tuning
 *
 * @param pll_tuner            : Audio PLL tuner handle
 *
 * @return @ref wiced_result_t
 */
wiced_result_t audio_pll_tuner_stop( audio_pll_tuner_ref pll_tuner );


/** Provide reference and audio timer timestamps to drive audio PLL tuning algorithm
 *
 * @param pll_tuner            : Audio PLL tuner handle
 * @param ts                   : Audio timestamps
 *
 * @return @ref wiced_result_t
 */
wiced_result_t audio_pll_tuner_push_timestamp( audio_pll_tuner_ref pll_tuner, audio_pll_tuner_timestamp_t *ts );

#ifdef __cplusplus
} /* extern "C" */
#endif
