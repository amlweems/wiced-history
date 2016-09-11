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

#include "wiced_audio.h"
#include "wiced_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define ak4954_pll_master       { ak4954_mpll, 1, 1 }
#define ak4954_pll_slave        { ak4954_spll, 1, 0 }
#define ak4954_ext_master       { ak4954_ext,  0, 1 }
#define ak4954_ext_slave        { ak4954_ext,  0, 0 }

/******************************************************
 *            Constants
 ******************************************************/

/* Maximum supported AK4954 devices. */
#define AK4954_MAX_DEVICES      1

/* standard audio device information */
#define AK4954_ADC_NAME         "ak4954_adc"
#define AK4954_ADC_DIRECTION    PLATFORM_AUDIO_DEVICE_INPUT
#define AK4954_ADC_PORT_TYPE    PLATFORM_AUDIO_LINE
#define AK4954_ADC_CHANNELS     2
#define AK4954_ADC_SIZES        (PLATFORM_AUDIO_SAMPLE_SIZE_16_BIT | PLATFORM_AUDIO_SAMPLE_SIZE_24_BIT)
#define AK4954_ADC_RATES        (PLATFORM_AUDIO_SAMPLE_RATE_8KHZ | PLATFORM_AUDIO_SAMPLE_RATE_16KHZ |      \
                                 PLATFORM_AUDIO_SAMPLE_RATE_22_05KHZ | PLATFORM_AUDIO_SAMPLE_RATE_32KHZ |  \
                                 PLATFORM_AUDIO_SAMPLE_RATE_44_1KHZ | PLATFORM_AUDIO_SAMPLE_RATE_48KHZ |   \
                                 PLATFORM_AUDIO_SAMPLE_RATE_96KHZ)

#define AK4954_DAC_NAME         "ak4954_dac"
#define AK4954_DAC_DIRECTION    PLATFORM_AUDIO_DEVICE_OUTPUT
#define AK4954_DAC_PORT_TYPE    PLATFORM_AUDIO_LINE
#define AK4954_DAC_CHANNELS     2
#define AK4954_DAC_SIZES        (PLATFORM_AUDIO_SAMPLE_SIZE_16_BIT | PLATFORM_AUDIO_SAMPLE_SIZE_24_BIT | PLATFORM_AUDIO_SAMPLE_SIZE_32_BIT)
#define AK4954_DAC_RATES        (PLATFORM_AUDIO_SAMPLE_RATE_8KHZ | PLATFORM_AUDIO_SAMPLE_RATE_16KHZ |      \
                                 PLATFORM_AUDIO_SAMPLE_RATE_22_05KHZ | PLATFORM_AUDIO_SAMPLE_RATE_32KHZ |  \
                                 PLATFORM_AUDIO_SAMPLE_RATE_44_1KHZ | PLATFORM_AUDIO_SAMPLE_RATE_48KHZ |   \
                                 PLATFORM_AUDIO_SAMPLE_RATE_96KHZ)

#define AUDIO_DEVICE_ID_AK4954_ADC_LINE_INFO                    \
        { AUDIO_DEVICE_ID_AK4954_ADC_LINE, AK4954_ADC_NAME,     \
          AK4954_ADC_DESCRIPTION, AK4954_ADC_DIRECTION,         \
          AK4954_ADC_PORT_TYPE, AK4954_ADC_CHANNELS,            \
          AK4954_ADC_SIZES, AK4954_ADC_RATES }

#define AUDIO_DEVICE_ID_AK4954_DAC_LINE_INFO                    \
        { AUDIO_DEVICE_ID_AK4954_DAC_LINE, AK4954_DAC_NAME,     \
          AK4954_DAC_DESCRIPTION, AK4954_DAC_DIRECTION,         \
          AK4954_DAC_PORT_TYPE, AK4954_DAC_CHANNELS,            \
          AK4954_DAC_SIZES, AK4954_DAC_RATES }

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef enum ak4954_device_id ak4954_device_id_t;

typedef struct ak4954_clock_settings ak4954_clock_settings_t;
typedef struct ak4954_device_cmn_data ak4954_device_cmn_data_t;
typedef struct ak4954_device_route ak4954_device_route_t;
typedef struct ak4954_device_data ak4954_device_data_t;
typedef struct ak4954_audio_device_interface ak4954_audio_device_interface_t;

typedef wiced_result_t (ak4954_ckcfg_fn_t)(ak4954_device_data_t *dd, wiced_audio_config_t *config, uint32_t mclk);

/******************************************************
 *            Enumerations
 ******************************************************/

enum ak4954_device_id
{
    AK4954_DEVICE_ID_0              = 0,

    /* Not a device id! */
    AK4954_DEVICE_ID_MAX,
};

/******************************************************
 *             Structures
 ******************************************************/

struct ak4954_clock_settings
{
    ak4954_ckcfg_fn_t               *fn;
    uint8_t                         pll_enab        : 1;
    uint8_t                         is_frame_master : 1;
};

struct ak4954_device_cmn_data
{
    ak4954_device_id_t              id;
    wiced_i2c_device_t *            i2c_data;
    ak4954_clock_settings_t         ck;
    wiced_gpio_t                    pdn;
};

struct ak4954_device_data
{
    const ak4954_device_route_t     *route;
    ak4954_device_cmn_data_t        *cmn;
    wiced_i2s_t                     data_port;
};

/******************************************************
 *             Variable declarations
 ******************************************************/

extern const ak4954_device_route_t ak4954_dac_hp;
extern const ak4954_device_route_t ak4954_dac_spkr;
extern const ak4954_device_route_t ak4954_adc_mic;

/******************************************************
 *             Function declarations
 ******************************************************/

wiced_result_t ak4954_platform_configure( ak4954_device_data_t *device_data, uint32_t mclk, uint32_t fs, uint8_t width );

wiced_result_t ak4954_device_register( ak4954_device_data_t *device_data, const platform_audio_device_id_t device_id );

/* Don't use these functions directly; use macros provided. */
ak4954_ckcfg_fn_t   ak4954_mpll, ak4954_spll, ak4954_ext;

#ifdef __cplusplus
} /* extern "C" */
#endif
