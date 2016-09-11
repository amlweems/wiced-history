/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "wiced_result.h"
#include "wiced_rtos.h"
#include "wiced_codec_sbc_params.h"
#include "apollo_bt_a2dp_sink_private.h"
#include "apollo_log.h"

/*****************************************************************************
**
**  Name:           bluetooth_audio_decoder.c
**
**  Description:    Implements interfacing functions to invoke decoders.
**
*****************************************************************************/

/******************************************************
 *                      Macros
 ******************************************************/

#define BT_AUDIO_INIT_CODEC_CBS                                         \
{                                                                       \
        .alloc_output_buffer_fp = bt_audio_decoder_alloc_output_buffer, \
        .read_encoded_data_fp = bt_audio_decoder_read_encoded_data,     \
        .write_decoded_data_fp = bt_audio_decoder_write_decoded_data    \
}

#ifdef BT_AUDIO_USE_MEM_POOL
#define BT_AUDIO_FREE_BUFFER(buffer) bt_buffer_pool_free_buffer(buffer)
#else  /* !BT_AUDIO_USE_MEM_POOL */
#define BT_AUDIO_FREE_BUFFER(buffer) free(buffer)
#endif /*  BT_AUDIO_USE_MEM_POOL */

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

static apollo_bt_a2dp_sink_t *g_a2dp_sink = NULL;

/******************************************************
 *               Function Declarations
 ******************************************************/

unsigned short bt_audio_decoder_alloc_output_buffer( int16_t** buffer, uint16_t length );
unsigned short bt_audio_decoder_read_encoded_data  ( uint8_t* data, uint16_t length );
unsigned short bt_audio_decoder_write_decoded_data ( int16_t* data, uint16_t length );

/******************************************************
*               Function Definitions
******************************************************/

static wiced_result_t bt_audio_decoder_task( uint32_t arg )
{
    wiced_result_t         result;
    uint32_t               flags_set;
    apollo_bt_a2dp_sink_t *a2dp_sink = (apollo_bt_a2dp_sink_t *)arg;

    apollo_log_msg( APOLLO_LOG_INFO, "Apollo BT A2DP sink: starting decoder task...\r\n" );

    do
    {
        flags_set = 0;
        result = wiced_rtos_wait_for_event_flags( &a2dp_sink->decoder.events, BT_AUDIO_CODEC_EVENT_ALL, &flags_set, WICED_TRUE, WAIT_FOR_ANY_EVENT, WICED_WAIT_FOREVER );
        if ( result != WICED_SUCCESS )
        {
            apollo_log_msg( APOLLO_LOG_DEBUG0, "Apollo BT A2DP sink: wiced_rtos_wait_for_event_flags() failed with %d !\r\n", ( int )result );
            continue;
        }

        if( flags_set & BT_AUDIO_CODEC_EVENT_QUIT )
        {
            break;
        }

        if ( flags_set & BT_AUDIO_CODEC_EVENT_DECODE )
        {
            while( (a2dp_sink->decoder_thread_quit == WICED_FALSE) && (wiced_rtos_is_queue_empty( &a2dp_sink->decoder.queue ) != WICED_SUCCESS) )
            {
                if( a2dp_sink->decoder.wiced_sbc_if->configured == 0 )
                {
                    apollo_log_msg( APOLLO_LOG_ERR, "Apollo BT A2DP sink: decoder not configured\r\n" );
                    break;
                }
                result = a2dp_sink->decoder.wiced_sbc_if->decode( );
                if ( result != WICED_SUCCESS )
                {
                    apollo_log_msg( APOLLO_LOG_DEBUG0, "Apollo BT A2DP sink: decoder failed with %d !\r\n", ( int )result );
                }
            }
        }

    } while (a2dp_sink->decoder_thread_quit == WICED_FALSE);

    apollo_log_msg( APOLLO_LOG_INFO, "Apollo BT A2DP sink: exiting decoder task !\r\n" );

    return result;
}


wiced_result_t bt_audio_decoder_context_init( apollo_bt_a2dp_sink_t *a2dp_sink )
{
    wiced_result_t                  result      = WICED_ERROR;
    wiced_codec_data_transfer_api_t decoder_cbs = BT_AUDIO_INIT_CODEC_CBS;

    wiced_action_jump_when_not_true( g_a2dp_sink == NULL, _exit, apollo_log_msg( APOLLO_LOG_ERR,"Apollo BT A2DP sink: audio decoder already initialized !\r\n" ) );
    g_a2dp_sink = a2dp_sink;

    a2dp_sink->decoder.wiced_sbc_if = wiced_get_codec_by_type( WICED_CODEC_SBC );
    wiced_action_jump_when_not_true(a2dp_sink->decoder.wiced_sbc_if != NULL, _exit, apollo_log_msg( APOLLO_LOG_ERR,"Apollo BT A2DP sink: wiced_get_codec_by_type() failed !\r\n" ) );

    a2dp_sink->decoder.wiced_sbc_if->init( &decoder_cbs, NULL );

    result = wiced_rtos_init_event_flags( &a2dp_sink->decoder.events );
    wiced_action_jump_when_not_true(result == WICED_SUCCESS, _exit, apollo_log_msg( APOLLO_LOG_ERR,"Apollo BT A2DP sink: wiced_rtos_init_event_flags() failed !\r\n" ) );

    result = wiced_rtos_init_queue( &a2dp_sink->decoder.queue, "CODEC_QUEUE", sizeof(bt_audio_codec_data_t*), BT_AUDIO_DECODER_QUEUE_MAX_SIZE );
    wiced_action_jump_when_not_true(result == WICED_SUCCESS, _exit, apollo_log_msg( APOLLO_LOG_ERR,"Apollo BT A2DP sink: wiced_rtos_init_event_flags() failed !\r\n" ) );

    a2dp_sink->decoder_thread_quit = WICED_FALSE;
    result = wiced_rtos_create_thread_with_stack( &a2dp_sink->decoder_thread, BT_AUDIO_DECODER_TASK_PRIORITY, "bt_a2dp_dec",
                                                 (wiced_thread_function_t) bt_audio_decoder_task, a2dp_sink->decoder_stack, BT_AUDIO_DECODER_STACK_SIZE, a2dp_sink );
    wiced_action_jump_when_not_true(result == WICED_SUCCESS, _exit, apollo_log_msg( APOLLO_LOG_ERR,"Apollo BT A2DP sink: wiced_rtos_create_thread_with_stack() failed !\r\n" ) );
    a2dp_sink->decoder_thread_ptr = &a2dp_sink->decoder_thread;

 _exit:
    return result;
}


wiced_result_t bt_audio_decoder_context_deinit( apollo_bt_a2dp_sink_t *a2dp_sink )
{
    wiced_result_t result = WICED_ERROR;

    wiced_action_jump_when_not_true( g_a2dp_sink != NULL, _exit, apollo_log_msg( APOLLO_LOG_ERR,"Apollo BT A2DP sink: audio decoder not initialized !\r\n" ) );

    a2dp_sink->decoder_thread_quit = WICED_TRUE;
    wiced_rtos_set_event_flags( &a2dp_sink->decoder.events, BT_AUDIO_CODEC_EVENT_QUIT );

    if ( a2dp_sink->decoder_thread_ptr != NULL )
    {
        wiced_rtos_thread_join( &a2dp_sink->decoder_thread );
        result = wiced_rtos_delete_thread( &a2dp_sink->decoder_thread );
        a2dp_sink->decoder_thread_ptr = NULL;
    }

    wiced_rtos_deinit_queue( &a2dp_sink->decoder.queue );
    wiced_rtos_deinit_event_flags( &a2dp_sink->decoder.events );

    a2dp_sink->decoder.wiced_sbc_if->close();
    a2dp_sink->decoder.wiced_sbc_if = NULL;

    g_a2dp_sink = NULL;

 _exit:
    return result;
}


wiced_result_t bt_audio_write_to_decoder_queue( apollo_bt_a2dp_sink_t *a2dp_sink, bt_audio_codec_data_t* audio )
{
    wiced_result_t result;

    result = wiced_rtos_push_to_queue( &a2dp_sink->decoder.queue, &audio, WICED_NO_WAIT );
    if ( result != WICED_SUCCESS )
    {
        BT_AUDIO_FREE_BUFFER(audio);
        apollo_log_msg( APOLLO_LOG_ERR,"bt_audio_write_to_decoder_queue: wiced_rtos_push_to_queue() failed !\r\n" );
    }
    else
    {
        result = wiced_rtos_set_event_flags( &a2dp_sink->decoder.events, BT_AUDIO_CODEC_EVENT_DECODE );
    }

    return result;
}


wiced_result_t bt_audio_reset_decoder_config( apollo_bt_a2dp_sink_t *a2dp_sink )
{
    if( a2dp_sink->decoder_input_buffer != NULL )
    {
        BT_AUDIO_FREE_BUFFER(a2dp_sink->decoder_input_buffer);
        a2dp_sink->decoder_input_buffer = NULL;
    }
    return WICED_SUCCESS;
}


wiced_result_t bt_audio_configure_decoder( apollo_bt_a2dp_sink_t *a2dp_sink, wiced_bt_a2dp_codec_info_t* decoder_config )
{
    wiced_result_t result = WICED_ERROR;

    if( decoder_config->codec_id == WICED_BT_A2DP_SINK_CODEC_SBC )
    {
        a2dp_sink->decoder.wiced_sbc_if->set_configuration( (void*) &decoder_config->cie.sbc );
        result = WICED_SUCCESS;
    }
    else
    {
        apollo_log_msg( APOLLO_LOG_ERR, "bt_audio_configure_decoder: unsupported codec !\r\n" );
    }

    return result;
}


unsigned short bt_audio_decoder_alloc_output_buffer( int16_t** buffer, uint16_t length )
{
    bt_audio_codec_data_t* pcm;

#ifdef BT_AUDIO_USE_MEM_POOL
    pcm = bt_buffer_pool_allocate_buffer( g_a2dp_sink->mem_pool );
#else
    pcm = malloc( sizeof( bt_audio_codec_data_t ) + length );
#endif
    if( pcm == NULL )
    {
        apollo_log_msg( APOLLO_LOG_ERR,"bt_audio_decoder_alloc_output_buffer: buffer allocation failed !\r\n" );
        return 0;
    }

    pcm->length = length;
    pcm->offset = 0;
    *buffer = (int16_t*)pcm->data;

    return length;
}


unsigned short bt_audio_decoder_read_encoded_data( uint8_t* data, uint16_t length )
{
    wiced_result_t result;
    uint16_t out_length = 0;

    if( g_a2dp_sink->decoder_input_buffer == NULL )
    {
        result = wiced_rtos_pop_from_queue( &g_a2dp_sink->decoder.queue, &g_a2dp_sink->decoder_input_buffer, WICED_NEVER_TIMEOUT );
        if( result != WICED_SUCCESS )
        {
            apollo_log_msg( APOLLO_LOG_ERR, "bt_audio_decoder_task: wiced_rtos_pop_from_queue() failed !\r\n" );
            return 0;
        }
    }

    out_length = MIN( g_a2dp_sink->decoder_input_buffer->length, length );

    /* Read Data from the input stream */
    memcpy ( data, ( g_a2dp_sink->decoder_input_buffer->data + g_a2dp_sink->decoder_input_buffer->offset ), out_length );

    /* update buffer pointer */
    g_a2dp_sink->decoder_input_buffer->length -= out_length;
    g_a2dp_sink->decoder_input_buffer->offset += out_length;

    if ( g_a2dp_sink->decoder_input_buffer->length == 0 )
    {
        BT_AUDIO_FREE_BUFFER(g_a2dp_sink->decoder_input_buffer);
        g_a2dp_sink->decoder_input_buffer = NULL;
    }

    return( out_length );
}


unsigned short bt_audio_decoder_write_decoded_data( int16_t* data, uint16_t length )
{
    if ( data != NULL )
    {
        bt_audio_codec_data_t* pcm = ( bt_audio_codec_data_t* ) ( ( ( uint8_t* ) data )- sizeof( bt_audio_codec_data_t ) );

        if ( length != 0 )
        {
            if ( g_a2dp_sink->user_params.data_cbf != NULL )
            {
                g_a2dp_sink->user_params.data_cbf( ( uint8_t * )data, ( length * sizeof( uint16_t ) ), g_a2dp_sink->user_params.user_context );
            }
        }
        BT_AUDIO_FREE_BUFFER(pcm);
    }

    return 1;
}
