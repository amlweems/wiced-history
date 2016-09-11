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
 * Bluetooth Audio AVDT Sink Service profiling
 */
#pragma once

#include <stdint.h>
#include <stdio.h>
#include "wiced_time.h"
#include "wiced_rtos.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#ifdef APOLLO_BT_PROFILE
#define APOLLO_BT_A2DP_SINK_PROFILING_WRITE( )                                                  \
        do                                                                                      \
        {                                                                                       \
            a2dp_dump_array[a2dp_dump_array_index].seq_num   = p_audio_data->seq_num;           \
            a2dp_dump_array[a2dp_dump_array_index].length    = in_length;                       \
            a2dp_dump_array[a2dp_dump_array_index].timestamp = p_audio_data->timestamp;         \
            wiced_time_get_time(&a2dp_dump_array[a2dp_dump_array_index].systime_ms);            \
            a2dp_dump_array[a2dp_dump_array_index].systime   = host_platform_get_cycle_count(); \
            a2dp_dump_array_index++;                                                            \
        } while(0)
#else
#define APOLLO_BT_A2DP_SINK_PROFILING_WRITE( )
#endif /* APOLLO_BT_PROFILE */

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

typedef struct
{
    uint16_t seq_num;
    uint16_t length;
    uint32_t timestamp;
    uint32_t systime;
    uint32_t systime_ms;
} a2dp_chunk_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

#ifdef APOLLO_BT_PROFILE
extern a2dp_chunk_t *a2dp_dump_array;
extern uint32_t      a2dp_dump_array_index;
#endif /* APOLLO_BT_PROFILE */

/******************************************************
 *               Function Declarations
 ******************************************************/

extern uint32_t host_platform_get_cycle_count( void );

static inline void apollo_bt_a2dp_sink_profiling_dump(void)
{
#ifdef APOLLO_BT_PROFILE
    uint32_t i;
    double  jitter_msec = 0;
    uint32_t jitter_max_msec = 0;
    uint32_t jitter_min_msec = 0xffffffff;
    double average_jitter_msec = 0.0f;
    uint32_t systime_ms = 0;
    uint32_t timestamp = 0;

    printf("packet_seq_num, packet_time_of_arrival_msec, jitter_msec, avg_jitter_msec, packet_timestamp_msec\n");
    for( i = 0; i < a2dp_dump_array_index; i++ )
    {
        jitter_msec    = (i > 0) ? ((double)((uint32_t)(a2dp_dump_array[i].systime - a2dp_dump_array[i-1].systime))) / (double)320000 : 0;

        if ( i > 0 )
        {
            average_jitter_msec += jitter_msec;

            systime_ms = (a2dp_dump_array[i].systime_ms - a2dp_dump_array[0].systime_ms);
            timestamp  = (a2dp_dump_array[i].timestamp - a2dp_dump_array[0].timestamp);

            if ( (uint32_t)jitter_msec > jitter_max_msec )
            {
                jitter_max_msec = (uint32_t)jitter_msec;
            }
            if ( (uint32_t)jitter_msec < jitter_min_msec )
            {
                jitter_min_msec = (uint32_t)jitter_msec;
            }
        }

        printf("%hu,%lu,%f,%f,%lu\n",
                a2dp_dump_array[i].seq_num,
                systime_ms,
                jitter_msec,
                (i > 0) ? (average_jitter_msec / (double)i) : -1.0f,
                timestamp);

        wiced_rtos_delay_milliseconds(10);
    }
    a2dp_dump_array_index = 0;
    printf("\r\njitter_min=%lu, jitter_max=%lu\n", jitter_min_msec, jitter_max_msec);
#endif /* APOLLO_BT_PROFILE */
    return;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
