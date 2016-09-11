/*
 * Copyright 2015, Broadcom Corporation
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

typedef enum
{
    APOLLO_AUDIO_SOURCE_CAPTURE, /*!< Audio from WICED audio capture/RS device */
    APOLLO_AUDIO_SOURCE_BT       /*!< Bluetooth audio source - A2DP only - */
} apollo_audio_source_type_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct apollo_streamer_s* apollo_streamer_ref;

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct apollo_streamer_params_s
{
    apollo_audio_source_type_t source_type;            /*!< Audio source type @ref apollo_audio_source_type_t */
    wiced_interface_t          iface;                  /*!< network interface @ref wiced_interface_t */
    wiced_ip_address_t         clientaddr;             /*!< IPv4 destination address (can be multicast) @ref wiced_ip_address_t */
    uint16_t                   port;                   /*!< RTP/UDP port number, enter 0 to use default */
    int                        num_packets;
    int                        payload_size;
    int                        fec_length;
    uint32_t                   fec_order;              /*!< RTP_AUDIO_FEC_PRIOR or RTP_AUDIO_FEC_PRIOR defined in apollo_rtp_params.h */
    wiced_bool_t               swap;
    char                      *audio_device_rx;
    char                      *audio_device_tx;
    uint32_t                   app_dct_offset_for_bt;
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
wiced_result_t apollo_streamer_init  ( apollo_streamer_params_t *params, apollo_streamer_ref *streamer_ptr );


/**
 * Perform the main initialization for the audio streamer.
 *
 * @param[in]  streamer     Apollo streamer handle
 *
 * @return @ref wiced_result_t
 */
wiced_result_t apollo_streamer_deinit( apollo_streamer_ref streamer );


#ifdef __cplusplus
} /* extern "C" */
#endif
