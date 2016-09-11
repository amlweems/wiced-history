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
 * Bluetooth Audio AVDT Sink Service
 *
 *
 *
 * Notes: Currently supports 44.1kHz and 48kHz audio
 */

#include <stdlib.h>
#include "wiced_rtos.h"
#include "wiced_result.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_remote_control.h"
#include "apollo_bt_a2dp_sink_private.h"
#include "apollo_bt_service.h"
#include "apollo_bt_main_service_private.h"
#include "apollo_log.h"
#include "apollo_bt_a2dp_sink_profiling.h"
#include  "apollo_bt_remote_control_private.h"
#include "apollo_bt_nv.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define BLUETOOTH_DEVICE_NAME           "Apollo"

/*  Recommended max_bitpool for high quality audio */
#define BT_AUDIO_A2DP_SBC_MAX_BITPOOL   53

#define AVRC_LOCAL_FEATURES             (REMOTE_CONTROL_FEATURE_CONTROLLER | REMOTE_CONTROL_FEATURE_TARGET)

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

static uint8_t uuid_list[] =
{
    0x08, 0x11, /* Headset */
    0x1E, 0x11, /* Handsfree */
    0x0E, 0x11, /* AV Remote Control */
    0x0B, 0x11, /* Audio Sink */
};

static wiced_bt_a2dp_codec_info_t bt_audio_codec_capabilities =
{
    .codec_id = WICED_BT_A2DP_SINK_CODEC_SBC,
    .cie =
        {
            .sbc =
            {
                (A2D_SBC_IE_SAMP_FREQ_44 | A2D_SBC_IE_SAMP_FREQ_48),    /* samp_freq */
                (A2D_SBC_IE_CH_MD_MONO | A2D_SBC_IE_CH_MD_STEREO |
                 A2D_SBC_IE_CH_MD_JOINT | A2D_SBC_IE_CH_MD_DUAL),       /* ch_mode */
                (A2D_SBC_IE_BLOCKS_16 | A2D_SBC_IE_BLOCKS_12 |
                 A2D_SBC_IE_BLOCKS_8 | A2D_SBC_IE_BLOCKS_4),            /* block_len */
                (A2D_SBC_IE_SUBBAND_4 | A2D_SBC_IE_SUBBAND_8),          /* num_subbands */
                (A2D_SBC_IE_ALLOC_MD_L | A2D_SBC_IE_ALLOC_MD_S),        /* alloc_mthd */
                BT_AUDIO_A2DP_SBC_MAX_BITPOOL,          /* max_bitpool for high quality audio */
                A2D_SBC_IE_MIN_BITPOOL                                  /* min_bitpool */
            }
        }
};

static wiced_bt_a2dp_config_data_t bt_audio_codec_config =
{
    .feature_mask = 0,
    .codec_capabilities =
    {
        .count = 1,
        .info = &bt_audio_codec_capabilities,
    }
};

static apollo_bt_a2dp_sink_t  g_apollo_bt_a2dp_sink;
static apollo_bt_a2dp_sink_t *g_p_apollo_bt_a2dp_sink = NULL;

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

/*Utility Functions */
wiced_result_t bt_audio_get_config_from_cie( wiced_bt_a2dp_codec_info_t* p_codec_config, bt_audio_config_t* p_audio_config)
{
    wiced_result_t result = WICED_BADARG;

    if(p_codec_config == NULL || p_audio_config == NULL)
    {
        return result;
    }

    if(p_codec_config->codec_id == WICED_BT_A2DP_SINK_CODEC_SBC)
    {
        if(p_codec_config->cie.sbc.samp_freq == A2D_SBC_IE_SAMP_FREQ_44)
        {
            p_audio_config->sample_rate = 44100;
        }
        else if(p_codec_config->cie.sbc.samp_freq == A2D_SBC_IE_SAMP_FREQ_48)
        {
            p_audio_config->sample_rate = 48000;
        }
        else if(p_codec_config->cie.sbc.samp_freq == A2D_SBC_IE_SAMP_FREQ_32)
        {
            p_audio_config->sample_rate = 32000;
        }
        else if(p_codec_config->cie.sbc.samp_freq == A2D_SBC_IE_SAMP_FREQ_16)
        {
            p_audio_config->sample_rate = 16000;
        }
        else
        {
            return WICED_ERROR;
        }

        if(p_codec_config->cie.sbc.ch_mode == A2D_SBC_IE_CH_MD_MONO)
        {
            p_audio_config->channels = 1;
        }
        else
        {
            p_audio_config->channels = 2;
        }

        p_audio_config->bits_per_sample = 16;

        p_audio_config->frame_size = (p_audio_config->channels * p_audio_config->bits_per_sample) / 8;

        result = WICED_SUCCESS;
    }

    return result;
}


/* DM Functions */
wiced_result_t bt_audio_write_eir( uint8_t *device_name )
{
    uint8_t eir_cfg[EIR_DATA_LENGTH] = {0};
    uint8_t* p = eir_cfg;
    uint8_t name_len = strlen((char*)device_name);

    *p++ = (uint8_t)(name_len+1);                 /* Length */
    *p++ = (uint8_t)EIR_COMPLETE_LOCAL_NAME_TYPE; /* EIR Data Type */
    memcpy(p, device_name, name_len);   /* Name string */
    p += name_len;

    *p++ = sizeof(uuid_list)+1;
    *p++ = (uint8_t) EIR_COMPLETE_16BITS_UUID_TYPE;
    memcpy(p, uuid_list, sizeof(uuid_list));

    return wiced_bt_dev_write_eir(eir_cfg, EIR_DATA_LENGTH);
}


void bt_audio_sink_data_cb( wiced_bt_a2dp_sink_codec_t codec_type, wiced_bt_a2dp_sink_audio_data_t* p_audio_data )
{
    bt_audio_codec_data_t* audio = NULL;
    uint8_t* in_audio;
    uint16_t in_length;
    wiced_result_t result;

    p_audio_data->p_pkt->len--;
    p_audio_data->p_pkt->offset++;
    in_length = p_audio_data->p_pkt->len;

    APOLLO_BT_A2DP_SINK_PROFILING_WRITE();

#ifdef BT_AUDIO_USE_MEM_POOL
    audio = bt_buffer_pool_allocate_buffer(g_p_apollo_bt_a2dp_sink->mem_pool);
#else
    audio = malloc( sizeof(bt_audio_codec_data_t)+in_length );
#endif
    if(audio == NULL)
    {
        apollo_log_msg(APOLLO_LOG_ERR,"bt_audio_sink_data_cb: buffer allocation failed !\n");
        return;
    }

    in_audio = ((uint8_t*)(p_audio_data->p_pkt+1))+p_audio_data->p_pkt->offset;
    memcpy(audio->data, in_audio, in_length);
    audio->length = in_length;
    audio->offset = 0;
    result = bt_audio_write_to_decoder_queue(g_p_apollo_bt_a2dp_sink, audio);
    if (result != WICED_SUCCESS)
    {
        /*
         * Failure writing to the decoder queue. Free the audio buffer
         * before we return so we don't leak memory.
         */
#ifdef BT_AUDIO_USE_MEM_POOL
        bt_buffer_pool_free_buffer(audio);
#else
        free(audio);
#endif
    }
}


/*Audio Sink Functions*/
static void bt_audio_sink_control_cb( wiced_bt_a2dp_sink_event_t event, wiced_bt_a2dp_sink_event_data_t* p_data)
{
    switch(event)
    {
        case WICED_BT_A2DP_SINK_CONNECT_EVT:
        {
            apollo_log_msg(APOLLO_LOG_INFO,"bt_audio_sink_control_cb:CONNECT EVENT \nstatus = %d\n", p_data->connect.result);

            if (p_data->connect.result == WICED_SUCCESS)
            {
                apollo_bt_remote_control_peer_address_set(p_data->connect.bd_addr);
                apollo_log_msg(APOLLO_LOG_INFO,"bt_audio_sink_control_cb: Remote Bluetooth Address: [%02X:%02X:%02X:%02X:%02X:%02X]\n",
                               p_data->connect.bd_addr[0], p_data->connect.bd_addr[1], p_data->connect.bd_addr[2],
                               p_data->connect.bd_addr[3], p_data->connect.bd_addr[4], p_data->connect.bd_addr[5]);
                if ( g_p_apollo_bt_a2dp_sink->user_params.event_cbf != NULL )
                {
                    g_p_apollo_bt_a2dp_sink->user_params.event_cbf( APOLLO_BT_A2DP_EVENT_CONNECTED, NULL, g_p_apollo_bt_a2dp_sink->user_params.user_context );
                }
            }
        }
        break;

        case WICED_BT_A2DP_SINK_DISCONNECT_EVT:
        {
            wiced_bt_device_address_t bda;

            memcpy( bda, p_data->disconnect.bd_addr, sizeof(wiced_bt_device_address_t) );
            apollo_log_msg(APOLLO_LOG_INFO,"bt_audio_sink_control_cb:DISCONNECTED EVENT \nreason = %d \nRemote Bluetooth Address: [%02X:%02X:%02X:%02X:%02X:%02X]\n",
                                            p_data->disconnect.result, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
            apollo_bt_remote_control_peer_address_reset();
            if ( g_p_apollo_bt_a2dp_sink->user_params.event_cbf != NULL )
            {
                g_p_apollo_bt_a2dp_sink->user_params.event_cbf( APOLLO_BT_A2DP_EVENT_DISCONNECTED, NULL, g_p_apollo_bt_a2dp_sink->user_params.user_context );
            }
#ifdef BT_AUDIO_USE_MEM_POOL
            bt_buffer_pool_print_debug_info( g_p_apollo_bt_a2dp_sink->mem_pool );
#endif
        }
        break;

        case WICED_BT_A2DP_SINK_START_EVT:
        {
            apollo_log_msg(APOLLO_LOG_INFO,"bt_audio_sink_control_cb:STREAM START EVENT \nstatus = %d\n", p_data->start.result);
            bt_audio_configure_decoder( g_p_apollo_bt_a2dp_sink, &g_p_apollo_bt_a2dp_sink->decoder_config );
            if ( g_p_apollo_bt_a2dp_sink->user_params.event_cbf != NULL )
            {
                g_p_apollo_bt_a2dp_sink->user_params.event_cbf( APOLLO_BT_A2DP_EVENT_STARTED, NULL, g_p_apollo_bt_a2dp_sink->user_params.user_context );
            }
        }
        break;

        case WICED_BT_A2DP_SINK_SUSPEND_EVT:
        {
            apollo_log_msg(APOLLO_LOG_INFO,"bt_audio_sink_control_cb:STREAM SUSPEND EVENT \nstatus = %d\n", p_data->start.result);
            if ( g_p_apollo_bt_a2dp_sink->user_params.event_cbf != NULL )
            {
                g_p_apollo_bt_a2dp_sink->user_params.event_cbf( APOLLO_BT_A2DP_EVENT_STOPPED, NULL, g_p_apollo_bt_a2dp_sink->user_params.user_context );
            }
            bt_audio_reset_decoder_config( g_p_apollo_bt_a2dp_sink );
#ifdef BT_AUDIO_USE_MEM_POOL
            bt_buffer_pool_print_debug_info( g_p_apollo_bt_a2dp_sink->mem_pool );
#endif
            apollo_bt_a2dp_sink_profiling_dump();
        }
        break;

        case WICED_BT_A2DP_SINK_CODEC_CONFIG_EVT:
        {
            apollo_log_msg(APOLLO_LOG_INFO,"bt_audio_sink_control_cb:CODEC CONFIG \nCODEC ID:0x%02X  FS:0x%02X  CH_MODE:0x%02X  BLOCK_LEN:0x%02X  NUM_SUBBANDS:0x%02X  ALLOC_METHOD:0x%02X  MAX_BITPOOL:0x%02X  MIN_BITPOOL:0x%02X\n",
                           p_data->codec_config.codec_id, p_data->codec_config.cie.sbc.samp_freq, p_data->codec_config.cie.sbc.ch_mode, p_data->codec_config.cie.sbc.block_len,
                           p_data->codec_config.cie.sbc.num_subbands, p_data->codec_config.cie.sbc.alloc_mthd, p_data->codec_config.cie.sbc.max_bitpool, p_data->codec_config.cie.sbc.min_bitpool);

            memcpy( &g_p_apollo_bt_a2dp_sink->decoder_config, &p_data->codec_config, sizeof(g_p_apollo_bt_a2dp_sink->decoder_config) );

            bt_audio_get_config_from_cie(&p_data->codec_config, &g_p_apollo_bt_a2dp_sink->audio_config);

            memset( &g_p_apollo_bt_a2dp_sink->user_event_data.codec_config, 0, sizeof(g_p_apollo_bt_a2dp_sink->user_event_data.codec_config));
            g_p_apollo_bt_a2dp_sink->user_event_data.codec_config.sample_rate     = g_p_apollo_bt_a2dp_sink->audio_config.sample_rate;
            g_p_apollo_bt_a2dp_sink->user_event_data.codec_config.bits_per_sample = g_p_apollo_bt_a2dp_sink->audio_config.bits_per_sample;
            g_p_apollo_bt_a2dp_sink->user_event_data.codec_config.channels        = g_p_apollo_bt_a2dp_sink->audio_config.channels;
            g_p_apollo_bt_a2dp_sink->user_event_data.codec_config.frame_size      = g_p_apollo_bt_a2dp_sink->audio_config.frame_size;

            if ( g_p_apollo_bt_a2dp_sink->user_params.event_cbf != NULL )
            {
                g_p_apollo_bt_a2dp_sink->user_params.event_cbf( APOLLO_BT_A2DP_EVENT_CODEC_CONFIG, &g_p_apollo_bt_a2dp_sink->user_event_data, g_p_apollo_bt_a2dp_sink->user_params.user_context );
            }
        }
        break;
    }
}


wiced_result_t apollo_bt_a2dp_sink_init( apollo_bt_a2dp_sink_init_params_t *params )
{
    wiced_result_t                 result = WICED_ERROR;
    apollo_bt_paired_device_info_t out_device_unused;

    wiced_action_jump_when_not_true(g_apollo_bt_a2dp_sink.is_initialized == WICED_FALSE, _exit,
                                    apollo_log_msg(APOLLO_LOG_ERR, "Apollo BT A2DP Sink: already initialized !\n"));

    g_apollo_bt_a2dp_sink.p_event_data = apollo_bt_service_get_management_evt_data();
    wiced_action_jump_when_not_true(g_apollo_bt_a2dp_sink.p_event_data != NULL, _exit,
                                    apollo_log_msg(APOLLO_LOG_ERR, "Apollo BT A2DP Sink: BT service is not initialized !\r\n"));
    wiced_action_jump_when_not_true(g_apollo_bt_a2dp_sink.p_event_data->enabled.status == WICED_BT_SUCCESS, _exit,
                                    apollo_log_msg(APOLLO_LOG_ERR, "Apollo BT A2DP Sink: BT service has not successfully started !\n"));

    g_p_apollo_bt_a2dp_sink = &g_apollo_bt_a2dp_sink;

    memcpy( &g_p_apollo_bt_a2dp_sink->user_params, params, sizeof(g_p_apollo_bt_a2dp_sink->user_params) );
    g_p_apollo_bt_a2dp_sink->codec_config = bt_audio_codec_config;
    g_p_apollo_bt_a2dp_sink->cfg_settings = wiced_bt_cfg_settings;

#ifdef BT_AUDIO_USE_MEM_POOL
    apollo_log_msg(APOLLO_LOG_DEBUG0, "Apollo BT A2DP Sink: using BT MEMPOOL instead of malloc()\r\n");
    result = bt_buffer_pool_init(&g_p_apollo_bt_a2dp_sink->mem_pool, MEM_POOL_BUFFER_COUNT, MEM_POOL_BUFFER_SIZE);
    wiced_action_jump_when_not_true(result == WICED_SUCCESS, _exit,
                                    apollo_log_msg(APOLLO_LOG_ERR, "Apollo BT A2DP Sink: BT buffer pool init failed !\n"));
#endif

    result = bt_audio_decoder_context_init( g_p_apollo_bt_a2dp_sink );
    wiced_action_jump_when_not_true(result == WICED_SUCCESS, _exit,
                                    apollo_log_msg(APOLLO_LOG_ERR, "Apollo BT A2DP Sink: audio decoder init failed !\n"));

#ifdef WICED_DCT_INCLUDE_BT_CONFIG
    /* Configure the Device Name and Class of Device from the DCT */
    platform_dct_bt_config_t* dct_bt_config = NULL;
    // Read config
    result = wiced_dct_read_lock( (void**) &dct_bt_config, WICED_TRUE, DCT_BT_CONFIG_SECTION, 0, sizeof(platform_dct_bt_config_t) );
    if ( result != WICED_SUCCESS )
    {
        result = bt_audio_write_eir( ( uint8_t* )BLUETOOTH_DEVICE_NAME );
        apollo_log_msg(APOLLO_LOG_ERR, "Apollo BT A2DP Sink: wiced_dct_read_lock(DCT_BT_CONFIG_SECTION) failed !!\n");
    }
    else
    {
        apollo_log_msg(APOLLO_LOG_INFO,"Apollo BT A2DP Sink: WICED DCT BT ADDR [%x:%x:%x:%x:%x:%x] \r\n",
                dct_bt_config->bluetooth_device_address[0], dct_bt_config->bluetooth_device_address[1],
                dct_bt_config->bluetooth_device_address[2], dct_bt_config->bluetooth_device_address[3],
                dct_bt_config->bluetooth_device_address[4], dct_bt_config->bluetooth_device_address[5]);
        result = bt_audio_write_eir(dct_bt_config->bluetooth_device_name);
        wiced_dct_read_unlock( (void*) dct_bt_config, WICED_TRUE );
    }
#else
    result = bt_audio_write_eir( ( uint8_t* )BLUETOOTH_DEVICE_NAME );
#endif

    apollo_log_msg(APOLLO_LOG_INFO,"Apollo BT A2DP Sink: wiced_bt_dev_write_eir() result = 0x%x\n", (unsigned int)result);

    wiced_bt_dev_read_local_addr( g_p_apollo_bt_a2dp_sink->local_address );
    apollo_log_msg(APOLLO_LOG_INFO,"Apollo BT A2DP Sink: Local Bluetooth Address is [%02X:%02X:%02X:%02X:%02X:%02X]\n",
                   g_p_apollo_bt_a2dp_sink->local_address[0], g_p_apollo_bt_a2dp_sink->local_address[1],
                   g_p_apollo_bt_a2dp_sink->local_address[2], g_p_apollo_bt_a2dp_sink->local_address[3],
                   g_p_apollo_bt_a2dp_sink->local_address[4], g_p_apollo_bt_a2dp_sink->local_address[5]);

    result = wiced_bt_sdp_db_init( (UINT8 *)sdp_database, wiced_bt_sdp_db_size );
    apollo_log_msg( APOLLO_LOG_INFO, "Apollo BT A2DP Sink: wiced_bt_sdp_db_init() result (bool) = 0x%x\n", ( unsigned int ) result );

    result = wiced_bt_dev_set_discoverability(BTM_GENERAL_DISCOVERABLE,
                                              g_p_apollo_bt_a2dp_sink->cfg_settings.br_edr_scan_cfg.inquiry_scan_window,
                                              g_p_apollo_bt_a2dp_sink->cfg_settings.br_edr_scan_cfg.inquiry_scan_interval);
    apollo_log_msg(APOLLO_LOG_INFO, "Apollo BT A2DP Sink: discoverability result = 0x%x\r\n", (unsigned int)result);

    result = wiced_bt_dev_set_connectability(BTM_CONNECTABLE,
                                             g_p_apollo_bt_a2dp_sink->cfg_settings.br_edr_scan_cfg.page_scan_window,
                                             g_p_apollo_bt_a2dp_sink->cfg_settings.br_edr_scan_cfg.page_scan_interval);
    apollo_log_msg(APOLLO_LOG_INFO, "Apollo BT A2DP Sink: connectability result = 0x%x\r\n", (unsigned int)result);

    result = wiced_bt_a2dp_sink_init(&g_p_apollo_bt_a2dp_sink->codec_config, (wiced_bt_a2dp_sink_control_cb_t) bt_audio_sink_control_cb, bt_audio_sink_data_cb);
    g_apollo_bt_a2dp_sink.is_initialized = WICED_TRUE;
    if ( result == WICED_SUCCESS )
    {
        if ( apollo_bt_remote_control_init() != WICED_SUCCESS )
        {
            apollo_log_msg(APOLLO_LOG_INFO, "Apollo BT A2DP Sink: wiced_bt_remote_control_init() failed with result = 0x%x\n", (unsigned int)result);
        }
    }
    wiced_action_jump_when_not_true(result == WICED_SUCCESS, _exit,
                                    apollo_log_msg(APOLLO_LOG_ERR, "Apollo BT A2DP Sink: wiced_bt_a2dp_sink_init() failed with result = 0x%x!\n", (unsigned int)result));

    if ( apollo_bt_nv_get_device_info_by_index(0, &out_device_unused) == WICED_SUCCESS )
    {
        apollo_bt_service_reconnection_timer_start();
    }

 _exit:
    g_apollo_bt_a2dp_sink.init_result = result;
    return result;
}


wiced_result_t apollo_bt_a2dp_sink_deinit( void )
{
    wiced_result_t result = WICED_ERROR;

    wiced_action_jump_when_not_true( g_apollo_bt_a2dp_sink.is_initialized == WICED_TRUE, _exit, apollo_log_msg(APOLLO_LOG_ERR,"Apollo BT A2DP Sink: not yet initialized !\n") );

    result = wiced_bt_a2dp_sink_deinit();

    bt_audio_decoder_context_deinit( g_p_apollo_bt_a2dp_sink );

#ifdef BT_AUDIO_USE_MEM_POOL
    bt_buffer_pool_deinit(g_p_apollo_bt_a2dp_sink->mem_pool);
#endif

    memset( g_p_apollo_bt_a2dp_sink, 0, sizeof(g_apollo_bt_a2dp_sink) );
    g_p_apollo_bt_a2dp_sink = NULL;
 _exit:
    return result;

}


wiced_result_t apollo_bt_a2dp_sink_connect( void )
{
    wiced_result_t                 result     = WICED_ERROR;
    apollo_bt_paired_device_info_t out_device;

    wiced_action_jump_when_not_true( (g_apollo_bt_a2dp_sink.is_initialized == WICED_TRUE) && (g_apollo_bt_a2dp_sink.init_result == WICED_SUCCESS), _exit,
                                     apollo_log_msg(APOLLO_LOG_ERR,"Apollo BT A2DP Sink: not yet initialized !\n") );

    result = apollo_bt_nv_get_device_info_by_index(0, &out_device);
    wiced_action_jump_when_not_true( result == WICED_SUCCESS, _exit, apollo_log_msg(APOLLO_LOG_NOTICE,"Apollo BT A2DP Sink: no previously connected device exist !\n") );

    result = wiced_bt_a2dp_sink_connect(out_device.device_link.bd_addr);
    wiced_action_jump_when_not_true( result == WICED_SUCCESS, _exit, apollo_log_msg(APOLLO_LOG_NOTICE,"Apollo BT A2DP Sink: wiced_bt_a2dp_sink_connect() failed !\n"));

 _exit:
    return result;;
}


apollo_bt_a2dp_sink_t *apollo_bt_a2dp_get_context( void )
{
    return g_p_apollo_bt_a2dp_sink;
}
