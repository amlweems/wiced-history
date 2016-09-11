/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include <stdio.h>

#include "apollo_bt_api.h"
#include "wiced_utilities.h"
#include "wiced_rtos.h"
#include "wiced_audio.h"
#include "wiced_platform.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define PRINT_ERR(x,...)                                      \
    do                                                        \
    {                                                         \
        fprintf(stderr, "[ERROR] %s: ", __PRETTY_FUNCTION__); \
        fprintf(stderr, x, ##__VA_ARGS__);                    \
    } while(0)

#define BYTES_TO_NANOSECS(number_of_bytes, sample_rate, block_align) \
    ( (NANOSECS_PER_SEC_ULL / (sample_rate * block_align)) * number_of_bytes )

#define BYTES_TO_AUDIO_TIMER_TICKS(number_of_bytes, audio_timer_factor, block_align) \
    ( (audio_timer_factor * number_of_bytes) / block_align )

/******************************************************
 *                    Constants
 ******************************************************/

#define APOLLO_BT_PACKET_QUEUE_DEPTH    (16)

#define NANOSECS_PER_SEC_ULL            (1000000000ULL)

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

/******************************************************
 *               Variable Definitions
 ******************************************************/

static volatile int              reference_count = 0;

static apollo_bt_packet_params_t apollo_bt_packet_params;
static wiced_queue_t             apollo_bt_audio_packet_queue;
static wiced_queue_t             apollo_bt_empty_packet_queue;
static apollo_bt_packet_t        apollo_bt_current_packet;
static wiced_event_flags_t       apollo_bt_events;
static wiced_audio_config_t      apollo_bt_audio_config;
static uint32_t                  apollo_bt_frame_size;
static uint32_t                  apollo_bt_audio_timer_clock_factor;

/******************************************************
 *               Function Definitions
 ******************************************************/


wiced_result_t apollo_bt_sync_init( apollo_bt_packet_params_t* params )
{
    wiced_result_t result = WICED_ERROR;

    reference_count++;

    if ( reference_count == 1 )
    {
        memset( &apollo_bt_audio_config, 0, sizeof(apollo_bt_audio_config) );
        memset( &apollo_bt_current_packet, 0, sizeof(apollo_bt_current_packet) );
        memcpy( &apollo_bt_packet_params, params, sizeof(*params) );

        result = wiced_rtos_init_queue( &apollo_bt_audio_packet_queue, "audio_packet_queue", sizeof(apollo_bt_packet_t), APOLLO_BT_PACKET_QUEUE_DEPTH );
        wiced_action_jump_when_not_true( (result == WICED_SUCCESS), _exit, PRINT_ERR("wiced_rtos_init_queue() on audio packet queue failed !\r\n") );

        result = wiced_rtos_init_queue( &apollo_bt_empty_packet_queue, "empty_packet_queue", sizeof(apollo_bt_packet_t), APOLLO_BT_PACKET_QUEUE_DEPTH );
        wiced_action_jump_when_not_true( (result == WICED_SUCCESS), _exit, PRINT_ERR("wiced_rtos_init_queue() on empty packet queue failed !\r\n") );

        result = wiced_rtos_init_event_flags( &apollo_bt_events );
        wiced_action_jump_when_not_true( (result == WICED_SUCCESS), _exit, PRINT_ERR("wiced_rtos_init_event_flags() failed !\r\n") );
    }
    else if ( reference_count > 1 )
    {
        result = WICED_SUCCESS;
    }

_exit:
    return result;
}


static void apollo_bt_flush_queue( wiced_queue_t *queue )
{
    wiced_result_t result = WICED_ERROR;
    while ( wiced_rtos_is_queue_empty( queue ) == WICED_ERROR )
    {
        result = wiced_rtos_pop_from_queue( queue, &apollo_bt_current_packet, WICED_NO_WAIT );
        if ( result == WICED_SUCCESS )
        {
            wiced_packet_delete( apollo_bt_current_packet.packet );
            apollo_bt_current_packet.packet = NULL;
        }
    }
}


wiced_result_t apollo_bt_sync_deinit( void )
{
    wiced_result_t result = WICED_ERROR;

    reference_count--;
    if ( reference_count == 0 )
    {
        if ( apollo_bt_current_packet.packet != NULL )
        {
            wiced_packet_delete( apollo_bt_current_packet.packet );
            apollo_bt_current_packet.packet = NULL;
        }

        apollo_bt_flush_queue( &apollo_bt_audio_packet_queue );
        apollo_bt_flush_queue( &apollo_bt_empty_packet_queue );

        result  = wiced_rtos_deinit_queue( &apollo_bt_audio_packet_queue );
        result |= wiced_rtos_deinit_queue( &apollo_bt_empty_packet_queue );
        result |= wiced_rtos_deinit_event_flags( &apollo_bt_events );

        memset( &apollo_bt_current_packet, 0, sizeof(apollo_bt_current_packet) );
        memset( &apollo_bt_audio_config, 0, sizeof(apollo_bt_audio_config) );
    }
    else if ( reference_count < 0 )
    {
        reference_count = 0;
    }
    else
    {
        result = WICED_SUCCESS;
    }

    return result;
}


wiced_result_t apollo_bt_set_event( apollo_bt_event_t event, const void *event_data, uint32_t event_data_size )
{
    wiced_result_t result = WICED_ERROR;
    uint32_t num_frames;

    wiced_jump_when_not_true( (reference_count > 0), _exit );

    switch ( event )
    {
        case APOLLO_BT_EVENT_AUDIO_CONFIG:
            wiced_jump_when_not_true( (event_data_size == sizeof(apollo_bt_audio_config)), _exit );
            memcpy( &apollo_bt_audio_config, event_data, event_data_size );
            apollo_bt_frame_size = ( apollo_bt_audio_config.channels * apollo_bt_audio_config.bits_per_sample ) >> 3;
            num_frames = apollo_bt_packet_params.max_data / apollo_bt_frame_size;
            if ( apollo_bt_packet_params.even_frame_count == WICED_TRUE )
            {
                num_frames &= ~0x01;
            }
            apollo_bt_packet_params.max_data = num_frames * apollo_bt_frame_size;
            /*
             * The ASCU audio timer clock rate is a power-of-2 multiple of the audio sample rate
             * Let's find out what that factor is
             */
            wiced_audio_timer_get_resolution( apollo_bt_audio_config.sample_rate, &apollo_bt_audio_timer_clock_factor );
            apollo_bt_audio_timer_clock_factor /= apollo_bt_audio_config.sample_rate;
            break;

        case APOLLO_BT_EVENT_AUDIO_START:
        case APOLLO_BT_EVENT_AUDIO_STOP:
        case APOLLO_BT_EVENT_AUDIO_NEW_PACKET:
            break;

        default:
            event = APOLLO_BT_EVENT_NONE;
            break;
    }

    if ( event != APOLLO_BT_EVENT_NONE )
    {
        result = wiced_rtos_set_event_flags( &apollo_bt_events, event );
    }

_exit:
    return result;
}


wiced_result_t apollo_bt_get_event( apollo_bt_event_t event, void *event_data, uint32_t event_data_size )
{
    wiced_result_t result = WICED_ERROR;

    wiced_jump_when_not_true( (reference_count > 0), _exit );

    switch ( event )
    {
        case APOLLO_BT_EVENT_AUDIO_CONFIG:
            wiced_jump_when_not_true( (event_data_size == sizeof(apollo_bt_audio_config)), _exit );
            memcpy( event_data, &apollo_bt_audio_config, event_data_size );
            result = WICED_SUCCESS;
            break;

        default:
            break;
    }

_exit:
    return result;
}


wiced_result_t apollo_bt_wait_for_event( uint32_t event_mask, uint32_t *events, uint32_t timeout_ms )
{
    wiced_result_t result = WICED_ERROR;

    wiced_jump_when_not_true( (reference_count > 0), _exit );

    *events = 0;
    result = wiced_rtos_wait_for_event_flags( &apollo_bt_events, event_mask, events, WICED_TRUE, WAIT_FOR_ANY_EVENT, timeout_ms );

_exit:
    return result;
}


wiced_result_t apollo_bt_provide_empty_packet( apollo_bt_packet_t* bt_empty_packet )
{
    wiced_result_t result = WICED_ERROR;

    wiced_jump_when_not_true( (reference_count > 0), _exit );

    result = wiced_rtos_push_to_queue( &apollo_bt_empty_packet_queue , bt_empty_packet, WICED_NO_WAIT );
    wiced_action_jump_when_not_true( (result == WICED_SUCCESS), _exit, PRINT_ERR("%s(): wiced_rtos_push_to_queue() failed !\r\n", __FUNCTION__) );

_exit:
    return result;
}


wiced_result_t apollo_bt_read_data( apollo_bt_packet_t* bt_packet, uint32_t timeout_ms )
{
    wiced_result_t result = WICED_ERROR;

    wiced_jump_when_not_true( (reference_count > 0), _exit );

    result = wiced_rtos_pop_from_queue( &apollo_bt_audio_packet_queue, bt_packet, timeout_ms );

_exit:
    return result;
}


wiced_result_t apollo_bt_write_data( const void* buffer, uint32_t bytes_to_write, uint64_t ts_nsecs, uint32_t* bytes_written )
{
    wiced_result_t result = WICED_ERROR;
    uint32_t write_count = 0;
    uint8_t* write_buf = (uint8_t*)buffer;

    wiced_jump_when_not_true( (reference_count > 0), _exit );


    while ( write_count < bytes_to_write )
    {
        uint32_t written_to_packet = 0;

        if ( apollo_bt_current_packet.packet == NULL )
        {
            result = wiced_rtos_pop_from_queue( &apollo_bt_empty_packet_queue, &apollo_bt_current_packet, WICED_WAIT_FOREVER );
            wiced_action_jump_when_not_true( (result == WICED_SUCCESS), _exit,
                                             PRINT_ERR("%s(): wiced_rtos_pop_from_queue() failed !\r\n", __FUNCTION__) );

            wiced_packet_set_data_end( apollo_bt_current_packet.packet, &apollo_bt_current_packet.data[apollo_bt_packet_params.offset + apollo_bt_packet_params.max_data] );

            apollo_bt_current_packet.datalen = apollo_bt_packet_params.offset;

            if ( write_count == 0 )
            {
                apollo_bt_current_packet.ts_nsecs = ts_nsecs;
            }
            else
            {

                // apollo_bt_current_packet.ts_nsecs = ts_nsecs + BYTES_TO_NANOSECS( write_count, apollo_bt_audio_config.sample_rate, apollo_bt_frame_size );
                apollo_bt_current_packet.ts_nsecs = ts_nsecs + BYTES_TO_AUDIO_TIMER_TICKS( write_count, apollo_bt_audio_timer_clock_factor, apollo_bt_frame_size );
            }
        }

        written_to_packet = MIN( (bytes_to_write - write_count), ((apollo_bt_packet_params.offset + apollo_bt_packet_params.max_data) - apollo_bt_current_packet.datalen) );
        memcpy( &apollo_bt_current_packet.data[apollo_bt_current_packet.datalen], &write_buf[ write_count ], written_to_packet );

        write_count += written_to_packet;
        apollo_bt_current_packet.datalen += written_to_packet;

        if ( apollo_bt_current_packet.datalen == apollo_bt_packet_params.offset + apollo_bt_packet_params.max_data )
        {
            result = wiced_rtos_push_to_queue( &apollo_bt_audio_packet_queue , &apollo_bt_current_packet, WICED_NO_WAIT );
            wiced_action_jump_when_not_true( (result == WICED_SUCCESS), _exit, PRINT_ERR("%s(): wiced_rtos_push_to_queue() failed !\r\n", __FUNCTION__) );
            apollo_bt_current_packet.packet = NULL;
        }
    }

_exit:
    *bytes_written = write_count;
    return result;
}
