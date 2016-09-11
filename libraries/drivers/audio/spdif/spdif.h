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
/* standard audio device info */

/* standard audio device information */
#define SPDIF_ADC_NAME         "SPDIF IN"
#define SPDIF_ADC_DIRECTION    PLATFORM_AUDIO_DEVICE_INPUT
#define SPDIF_ADC_PORT_TYPE    PLATFORM_AUDIO_LINE
#define SPDIF_ADC_CHANNELS     6
#define SPDIF_ADC_SIZES        (PLATFORM_AUDIO_SAMPLE_SIZE_16_BIT | PLATFORM_AUDIO_SAMPLE_SIZE_24_BIT | PLATFORM_AUDIO_SAMPLE_SIZE_32_BIT)
#define SPDIF_ADC_RATES        (PLATFORM_AUDIO_SAMPLE_RATE_8KHZ    | PLATFORM_AUDIO_SAMPLE_RATE_32KHZ |    \
                                PLATFORM_AUDIO_SAMPLE_RATE_44_1KHZ | PLATFORM_AUDIO_SAMPLE_RATE_48KHZ |    \
                                PLATFORM_AUDIO_SAMPLE_RATE_96KHZ )

#define SPDIF_DAC_NAME         "SPDIF OUT"
#define SPDIF_DAC_DIRECTION    PLATFORM_AUDIO_DEVICE_OUTPUT
#define SPDIF_DAC_PORT_TYPE    PLATFORM_AUDIO_LINE
#define SPDIF_DAC_CHANNELS     6
#define SPDIF_DAC_SIZES        (PLATFORM_AUDIO_SAMPLE_SIZE_16_BIT | PLATFORM_AUDIO_SAMPLE_SIZE_24_BIT | PLATFORM_AUDIO_SAMPLE_SIZE_32_BIT)
#define SPDIF_DAC_RATES        (PLATFORM_AUDIO_SAMPLE_RATE_8KHZ    | PLATFORM_AUDIO_SAMPLE_RATE_32KHZ |    \
                                PLATFORM_AUDIO_SAMPLE_RATE_44_1KHZ | PLATFORM_AUDIO_SAMPLE_RATE_48KHZ |    \
                                PLATFORM_AUDIO_SAMPLE_RATE_96KHZ )


#define AUDIO_DEVICE_ID_SPDIF_ADC_INFO                                      \
    {   AUDIO_DEVICE_ID_SPDIF_ADC, SPDIF_ADC_NAME, SPDIF_ADC_DESCRIPTION,   \
        SPDIF_ADC_DIRECTION, SPDIF_ADC_PORT_TYPE,                       \
        SPDIF_ADC_CHANNELS,  SPDIF_ADC_SIZES, SPDIF_ADC_RATES    }

#define AUDIO_DEVICE_ID_SPDIF_DAC_INFO                                      \
    {   AUDIO_DEVICE_ID_SPDIF_DAC, SPDIF_DAC_NAME, SPDIF_DAC_DESCRIPTION,   \
        SPDIF_DAC_DIRECTION, SPDIF_DAC_PORT_TYPE,                       \
        SPDIF_DAC_CHANNELS,  SPDIF_DAC_SIZES, SPDIF_DAC_RATES    }

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct
{
    wiced_i2s_t             data_port;
} spdif_device_data_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/


wiced_result_t spdif_device_register( spdif_device_data_t* device_data, platform_audio_device_id_t device_id );


#ifdef __cplusplus
} /* extern "C" */
#endif
