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
 * Bluetooth Remote Control interface
 *
 */
#pragma once

#include "wiced_result.h"
#include "wiced_codec_if.h"
#include "wiced_bt_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define APOLLO_BT_SONG_ARTIST_STR_SIZE (65)
#define APOLLO_BT_SONG_TITLE_STR_SIZE  (257)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct
{
    apollo_bt_a2dp_sink_t    *a2dp_sink_ctx;
    wiced_bool_t              rc_is_initialized;
    wiced_bt_device_address_t rc_peer_address;
    uint32_t                  rc_peer_features;
    wiced_bool_t              rc_is_connected;
    uint8_t                   rc_volume;
    uint32_t                  track_duration;
    int8_t                    song_artist[APOLLO_BT_SONG_ARTIST_STR_SIZE];
    int8_t                    song_title[APOLLO_BT_SONG_TITLE_STR_SIZE];
} apollo_bt_rc_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t apollo_bt_remote_control_init( void );
wiced_result_t apollo_bt_remote_control_deinit( void );
wiced_result_t apollo_bt_remote_control_connect( void );
void apollo_bt_remote_control_peer_address_reset( void );
void apollo_bt_remote_control_peer_address_set( wiced_bt_device_address_t peer_addr );

#ifdef __cplusplus
} /* extern "C" */
#endif
