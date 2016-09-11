/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "wiced_audio.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define AUDIO_RENDER_VOLUME_MIN     (0)
#define AUDIO_RENDER_VOLUME_MAX     (100)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/**
 * Audio buffer structure.
 */

typedef struct wiced_audio_render_buf_s
{
    int64_t  pts;
    uint8_t* data_buf;
    uint32_t data_offset;
    uint32_t data_length;
    void*    opaque;
} wiced_audio_render_buf_t;

/**
 * Callback for releasing audio buffers.
 */

typedef wiced_result_t (*wiced_audio_render_buf_release_cb_t)(wiced_audio_render_buf_t* buf, void* session_ptr);

/**
 * Callback for providing reference time in nanoseconds
 */

typedef wiced_result_t (*wiced_audio_render_time_get_cb_t)(int64_t *time_in_nanosecs, void* session_ptr);

/**
 * Audio queue weight
 */

typedef struct wiced_audio_render_queue_weight_s
{
    uint32_t input_queue_weight;
    uint32_t output_buffer_weight;
    uint32_t output_buffer_size;
} wiced_audio_render_queue_weight_t;

/**
 * Audio playback statistics
 */

typedef struct
{
    uint64_t    audio_frames_played;
    uint64_t    audio_frames_dropped;
    uint64_t    audio_frames_inserted;
    int64_t     audio_max_early;
    int64_t     audio_max_late;
} wiced_audio_render_stats_t;

/******************************************************
 *                    Structures
 ******************************************************/

/**
 * Configuration parameters for starting audio render.
 */

typedef struct wiced_audio_render_params_s {
    uint32_t                                buffer_nodes;   /* Number of buffer nodes for audio render to allocate                      */
    uint32_t                                buffer_ms;      /* Buffering (pre-roll) time that audio render should use                   */
    uint32_t                                threshold_ms;   /* Threshold in ms for adding silence/dropping audio frames                 */
    int                                     clock_enable;   /* 0 = disable (blind push), 1 = enable                                     */
    platform_audio_device_id_t              device_id;      /* device identifier                                                        */
    void*                                   session_ptr;    /* Application session pointer passed back in the following callbacks       */
    wiced_audio_render_buf_release_cb_t     cb_buf_release; /* Audio buffer release callback                                            */
    wiced_audio_render_time_get_cb_t        cb_time_get;    /* OPTIONAL - reference nanosecond time callback                            */
} wiced_audio_render_params_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

typedef struct wiced_audio_render_s *wiced_audio_render_ref;

/** Initialize the audio render library.
 *
 * @param[in] params : Pointer to the audio configuration parameters.
 *
 * @return Pointer to the audio render instance or NULL
 */
wiced_audio_render_ref wiced_audio_render_init(wiced_audio_render_params_t* params);

/** Deinitialize the audio render library.
 *
 * @param[in] audio  : Pointer to the audio render instance.
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_deinit(wiced_audio_render_ref audio);

/** Configure the audio render audio format.
 *
 * @param[in] audio  : Pointer to the audio render instance.
 * @param[in] config : Pointer to the audio configuration.
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_configure(wiced_audio_render_ref audio, wiced_audio_config_t* config);

/** Put the audio render in play state.
 * @note this functionality still needs to be implemented.
 *
 * @param[in] audio  : Pointer to the audio render instance.
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_play(wiced_audio_render_ref audio);

/** Put the audio render in pause state.
 * @note this functionality still needs to be implemented.
 *
 * @param[in] audio  : Pointer to the audio render instance.
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_pause(wiced_audio_render_ref audio);

/** Flush the audio render.
 *
 * @param[in] audio  : Pointer to the audio render instance.
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_flush(wiced_audio_render_ref audio);

/** Stop the audio render playback.
 * @note this functionality still needs to be implemented.
 *
 * @param[in] audio  : Pointer to the audio render instance.
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_stop(wiced_audio_render_ref audio);

/** Push an audio buffer to the audio render library.
 * @note The audio render becomes owner of the buffer pointed to
 * by the data_buf member until it is release via the buffer release
 * callback.
 *
 * @param[in] audio  : Pointer to the audio render instance.
 * @param[in] buf    : Pointer to the audio buffer.
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_push(wiced_audio_render_ref audio, wiced_audio_render_buf_t* buf);

/** Set the output volume for the audio render library.
 *
 * @param[in] audio  : Pointer to the audio render instance.
 * @param[in] volume : The new volume (0-100)
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_set_volume(wiced_audio_render_ref audio, uint8_t volume);

/** Adjust audio rendering speed in part per million of default/normal speed
 *
 * @param[in] audio        : Pointer to the audio render instance.
 * @param[in] speed_in_ppm : Audio playback speed adjustment in part-per-million (PPM)
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_set_speed(wiced_audio_render_ref audio, float speed_in_ppm);

/** Get audio render internal queue depth
 *
 * @param[in]  audio  : Pointer to the audio render instance.
 * @param[out] weight : Pointer to @ref wiced_audio_render_queue_depth_t structure
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_get_queue_weight(wiced_audio_render_ref audio, wiced_audio_render_queue_weight_t *weight);

/** Get the current audio render playback statistics.
 *
 * @param[in]  audio  : Pointer to the audio render instance.
 * @param[out] stats  : Pointer to @ref wiced_audio_render_stats_t structure
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_get_stats(wiced_audio_render_ref audio, wiced_audio_render_stats_t* stats);

#ifdef __cplusplus
} /* extern "C" */
#endif
