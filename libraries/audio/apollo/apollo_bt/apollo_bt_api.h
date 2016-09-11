/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#pragma once

#include <stdint.h>
#include "wiced_result.h"
#include "wiced_tcpip.h"

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
    APOLLO_BT_EVENT_NONE              =  0x0,
    APOLLO_BT_EVENT_AUDIO_CONFIG      = (1 << 0),     /* using wiced_audio_config_t */
    APOLLO_BT_EVENT_AUDIO_START       = (1 << 1),
    APOLLO_BT_EVENT_AUDIO_STOP        = (1 << 2),
    APOLLO_BT_EVENT_AUDIO_NEW_PACKET  = (1 << 3),

    APOLLO_BT_EVENT_ALL           =  0xFFFFFFFF
} apollo_bt_event_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct apollo_bt_packet_params_s
{
    uint16_t        offset;                      /* offset from start of wiced_packet_t; reserved for RTP header           */
    uint16_t        max_data;                    /* maximum of data stored in wiced_packet_t, including offset above       */
    wiced_bool_t    even_frame_count;            /* use an even number of audio frames in each buffer                      */
} apollo_bt_packet_params_t;

typedef struct apollo_bt_packet_s
{
    uint64_t        ts_nsecs;                    /* timestamp in nanosecs */
    wiced_packet_t* packet;                      /* pointer to wiced_packet_t                                              */
    uint8_t*        data;                        /* pointer to storage buffer from wiced_packet_t                          */
    uint16_t        datalen;                     /* hold amount of data stored within buffer above, not to exceed max_data */
} apollo_bt_packet_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/


/** Initialize the synchronization primitives
 *
 * @param[in]   params : @ref apollo_bt_packet_params_t initialization parameters
 *
 * @return  @ref wiced_result_t
 */
wiced_result_t apollo_bt_sync_init           ( apollo_bt_packet_params_t* params );


/** Release resources associated with synchronization primitives
 *
 * @return  @ref wiced_result_t
 */
wiced_result_t apollo_bt_sync_deinit         ( void );


/** Set an event
 *
 * @param[in]   event           : @ref APOLLO_BT_EVENT_T event
 * @param[in]   event_data      : pointer to a set of data related to the event
 * @param[in]   event_data_size : size of the data set
 *
 * @return  @ref wiced_result_t
 */
wiced_result_t apollo_bt_set_event           ( apollo_bt_event_t event, const void *event_data, uint32_t event_data_size );


/** Get an event
 *
 * @param[in]   event           : @ref APOLLO_BT_EVENT_T event
 * @param[out]  event_data      : pointer to a storage buffer for the data set related to the event
 * @param[in]   event_data_size : size of the storage buffer
 *
 * @return  @ref wiced_result_t
 */
wiced_result_t apollo_bt_get_event           ( apollo_bt_event_t event, void *event_data, uint32_t event_data_size );


/** Wait for event(s)
 *
 * @param[in]   event_mask      : bitmask of APOLLO_BT_EVENT_T the caller will be waiting for
 * @param[out]  events          : pointer to an unsigned integer to store a bitmask of APOLLO_BT_EVENT_T that have been triggered / satisfied
 * @param[in]   timeout_ms      : timeout value in msecs; or WICED_NO_WAIT / WICED_WAIT_FOREVER
 *
 * @return  @ref wiced_result_t
 */
wiced_result_t apollo_bt_wait_for_event      ( uint32_t event_mask, uint32_t *events, uint32_t timeout_ms );


/**
 * Provide empty packet to be used by @ref apollo_bt_write_data
 *
 * @param[in]  bt_empty_packet : @ref apollo_bt_packet_t buffer header
 *
 * @return  @ref wiced_result_t
 */
wiced_result_t apollo_bt_provide_empty_packet( apollo_bt_packet_t* bt_empty_packet );


/** Write audio data into shared storage
 *
 * @param[in]   buffer         : buffer with the data.
 * @param[in]   bytes_to_write : number of bytes to be written.
 * @param[in]   ts_nsecs       : timestamp in nanosecs accompanying the data
 * @param[out]  bytes_written  : number of bytes actually written.
 *
 * @return  @ref wiced_result_t
 */
wiced_result_t apollo_bt_write_data          ( const void* buffer, uint32_t bytes_to_write, uint64_t ts_nsecs, uint32_t* bytes_written );


/**
 * Get audio data out of shared storage
 *
 * @param[out]  bt_packet      : @ref apollo_bt_packet_t buffer header
 * @param[in]   timeout_ms     : timeout value in msecs; or WICED_NO_WAIT / WICED_WAIT_FOREVER
 *
 * @return  @ref wiced_result_t
 */
wiced_result_t apollo_bt_read_data           ( apollo_bt_packet_t* bt_packet, uint32_t timeout_ms );

#ifdef __cplusplus
} /* extern "C" */
#endif
