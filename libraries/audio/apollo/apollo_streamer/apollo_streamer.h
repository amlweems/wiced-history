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
 * @file Apollo Audio Streamer
 */

#pragma once

#include "wiced_result.h"
#include "wiced_tcpip.h"
#include "apollo_rtp_params.h"
#include "platform_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define APOLLO_STREAMER_BURST_AUTO_SLC      (0xF0F0F0F0)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    APOLLO_AUDIO_SOURCE_CAPTURE, /*!< Audio from WICED audio capture/RS device */
    APOLLO_AUDIO_SOURCE_BT       /*!< Bluetooth audio source - A2DP only - */
} apollo_audio_source_type_t;

typedef enum
{
    APOLLO_STREAMER_EVENT_BT_CONNECTED,
    APOLLO_STREAMER_EVENT_BT_DISCONNECTED,
    APOLLO_STREAMER_EVENT_BT_PLAYBACK_STARTED,
    APOLLO_STREAMER_EVENT_BT_PLAYBACK_STOPPED,
    APOLLO_STREAMER_EVENT_BT_VOLUME_CHANGED,
    APOLLO_STREAMER_EVENT_BT_PLAYBACK_STATUS,
    APOLLO_STREAMER_EVENT_BT_TRACK_METADATA
} apollo_streamer_event_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct
{
    uint32_t position_msecs;                               /*!< Playback position in milliseconds (relative to track duration below)    */
    uint32_t duration_msecs;                               /*!< Track duration in milliseconds                                          */
} apollo_streamer_track_playback_status_t;

typedef struct
{
    int8_t* artist_utf8_str;                               /*!< NULL-terminated, UTF-8 encoded, track artist string                       */
    int8_t* title_utf8_str;                                /*!< NULL-terminated, UTF-8 encoded, track title string                        */
} apollo_streamer_track_metadata_t;

typedef union
{
    uint8_t                                 volume;        /*!< Audio volume (0 - 100)                                                    */
    apollo_streamer_track_playback_status_t playback;
    apollo_streamer_track_metadata_t        metadata;
} apollo_streamer_event_data_t;

typedef struct apollo_streamer_s* apollo_streamer_ref;

typedef int (*apollo_streamer_event_callback)(apollo_streamer_ref handle, void* user_context, apollo_streamer_event_t event, apollo_streamer_event_data_t* event_data);

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct apollo_streamer_params_s
{
    apollo_streamer_event_callback event_cb;               /*!< Application event callback                                                */
    void*                          user_context;           /*!< User context for event callback                                           */
    apollo_audio_source_type_t     source_type;            /*!< Audio source type @ref apollo_audio_source_type_t                         */
    wiced_interface_t              iface;                  /*!< network interface @ref wiced_interface_t                                  */
    wiced_ip_address_t             clientaddr;             /*!< IPv4 destination address (can be multicast) @ref wiced_ip_address_t       */
    uint16_t                       port;                   /*!< RTP/UDP port number, enter 0 to use default                               */
    int                            num_pool_packets;       /*!< Total number of packets to be allocated for streamer buffering            */
    int                            num_packets;            /*!< Number of packets streamer may use for audio buffering                    */
    int                            max_payload_size;       /*!< RTP maximum payload size (audio data size)                                */
    int                            burst_length;           /*!< Burst length of protected audio packets                                   */
    int                            shuffle_length;         /*!< SLC shuffle length                                                        */
    platform_audio_device_id_t     audio_device_rx;        /*!< Input  / capture  device ID @ref platform_audio_device_id_t               */
    platform_audio_device_id_t     audio_device_tx;        /*!< Output / playback device ID @ref platform_audio_device_id_t               */

    /* Audio render playback parameters */

    int                            volume;                 /*!< Audio volume (0 - 100)                                                    */
    uint32_t                       buffer_nodes;           /*!< Number of buffer nodes for audio render to allocate                       */
    uint32_t                       buffer_ms;              /*!< Buffering (pre-roll) time that audio render should use                    */
    uint32_t                       threshold_ms;           /*!< Threshold in ms for adding silence/dropping audio frames                  */
    int                            clock_enable;           /*!< 0 = disable (blind push), 1 = enable                                      */

    /* Audio PLL tuning control parameters */

    int                            pll_tuning_enable;      /*!< 0 = disable audio PLL tuning, 1 = enable                                  */

    /* Input device audio format */

    platform_audio_sample_sizes_t  input_sample_size;      /*!< Input sample size for audio_device_rx                                     */
    platform_audio_sample_rates_t  input_sample_rate;      /*!< Input sample rate for audio_device_rx                                     */
    uint8_t                        input_channel_count;    /*!< Input channel count for audio_device_rx                                   */

} apollo_streamer_params_t;

/******************************************************
 *               Function Declarations
 ******************************************************/

/**
 * Perform the main initialization and initiate streaming
 *
 * @param[in]  params       Streaming engine parameters       @ref apollo_streamer_params_t
 * @param[out] streamer_ptr Pointer to Apollo streamer handle @ref apollo_streamer_ref
 *
 * @return @ref wiced_result_t
 */
wiced_result_t apollo_streamer_init  ( apollo_streamer_params_t* params, apollo_streamer_ref* streamer_ptr );


/**
 * Perform the main initialization for the audio streamer.
 *
 * @param[in]  streamer     Apollo streamer handle
 *
 * @return @ref wiced_result_t
 */
wiced_result_t apollo_streamer_deinit( apollo_streamer_ref streamer );


/** Set the volume for the apollo streamer library.
 *
 * @param[in] streamer    Apollo streamer handle
 * @param[in] volume      New volume level (0-100).
 *
 * @return @ref wiced_result_t
 */
wiced_result_t apollo_streamer_set_volume(apollo_streamer_ref streamer, int volume);


#ifdef __cplusplus
} /* extern "C" */
#endif
