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
 */

#include "wiced_platform.h"
#include "wiced_rtos.h"
#include "wiced_audio.h"
#include "spdif.h"
#include "platform_i2s.h"

/******************************************************
 *                      Macros
 ******************************************************/

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

/******************************************************
 *               Function Declarations
 ******************************************************/

static wiced_result_t spdif_init(void* device_data);
static wiced_result_t spdif_deinit(void* device_data);
static wiced_result_t spdif_configure(void* device_data, wiced_audio_config_t* config, uint32_t* mclk);
static wiced_result_t spdif_start_play(void* device_data);
static wiced_result_t spdif_stop_play(void* device_data);
static wiced_result_t spdif_set_volume(void* device_data, double decibles);
static wiced_result_t spdif_get_volume_range(void* device_data, double* min_volume_decibels, double* max_volume_decibels);

/******************************************************
 *               Variables Definitions
 ******************************************************/

wiced_audio_device_interface_t spdif_interface =
{
        .audio_device_init             = spdif_init,
        .audio_device_deinit           = spdif_deinit,
        .audio_device_configure        = spdif_configure,
        .audio_device_start_streaming  = spdif_start_play,
        .audio_device_stop_streaming   = spdif_stop_play,
        .audio_device_set_volume       = spdif_set_volume,
        .audio_device_set_treble       = NULL,
        .audio_device_set_bass         = NULL,
        .audio_device_get_volume_range = spdif_get_volume_range
};

/******************************************************
 *               Function Definitions
 ******************************************************/

static wiced_result_t spdif_init(void* device_data)
{
    (void)device_data;

    return WICED_SUCCESS;
}

static wiced_result_t spdif_deinit(void* device_data)
{
    (void)device_data;

    return WICED_SUCCESS;
}

static wiced_result_t spdif_configure(void* device_data, wiced_audio_config_t* config, uint32_t* mclk)
{
    (void)device_data;
    (void)config;
    (void)mclk;

    return WICED_SUCCESS;
}

static wiced_result_t spdif_start_play(void* device_data)
{
    (void)device_data;

    return WICED_SUCCESS;
}

static wiced_result_t spdif_stop_play(void* device_data)
{
    (void)device_data;

    return WICED_SUCCESS;
}

static wiced_result_t spdif_set_volume(void* device_data, double decibles)
{
    (void)device_data;
    (void)decibles;

    return WICED_SUCCESS;
}

static wiced_result_t spdif_get_volume_range(void* device_data, double* min_volume_decibels, double* max_volume_decibels)
{
    (void)device_data;

    *min_volume_decibels = 0.0;
    *max_volume_decibels = 100.0;

    return WICED_SUCCESS;
}


/* This function can only be called from the platform initialization routine */
wiced_result_t spdif_device_register( spdif_device_data_t* device_data, platform_audio_device_id_t device_id )
{
    if( device_data == NULL )
    {
        return WICED_BADARG;
    }
    spdif_interface.audio_device_driver_specific = device_data;
    spdif_interface.device_id = device_id;

    /* Register a device to the audio device list and keep device data internally from this point */
    return wiced_register_audio_device(device_id, &spdif_interface);
}
