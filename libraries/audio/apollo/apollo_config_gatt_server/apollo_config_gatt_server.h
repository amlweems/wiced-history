/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *
 * This file provides definitions and function prototypes for Apollo config
 * device
 *
 */
#pragma once

#include "wiced_result.h"
#include "wiced_bt_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#ifndef BD_ADDR_LEN
#define BD_ADDR_LEN     6
#endif /* BD_ADDR_LEN */

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    APOLLO_CONFIG_GATT_EVENT_DCT_READ,
    APOLLO_CONFIG_GATT_EVENT_DCT_WRITE,
    APOLLO_CONFIG_GATT_EVENT_DCT_WRITE_COMPLETED
} apollo_config_gatt_event_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct
{
    char spk_name[22];          //apollo_audio_dct_t->spkr_name
    uint8_t mode;
    uint8_t is_configured;
// network configuration
    wiced_security_t security;  //dct_collection_t->wiced_config_soft_ap_t->security
    char nw_ssid_name[22];      //dct_collection_t->dct_wifi->stored_ap_list[0].details.SSID.value
    char nw_pass_phrase[22];
// speaker configuration
    //char spk_name[22];          //apollo_audio_dct_t->spkr_name
    uint8_t spk_channel_map;    //apollo_audio_dct_t->channel
    uint8_t spk_vol;            //apollo_audio_dct_t->vol
// source configuration
    uint8_t src_type;           //apollo_sender_dct_t-> source_type
    uint8_t src_vol;            //apollo_sender_dct_t->volume
} apollo_config_gatt_server_dct_t;

typedef wiced_result_t (*apollo_config_gatt_event_callback)(apollo_config_gatt_event_t event, apollo_config_gatt_server_dct_t *dct,  void *user_context);

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
        apollo_config_gatt_event_callback event_cbf;
        void                             *user_context;
        wiced_bool_t                      init_bt_stack;
        uint8_t                           bt_device_address[BD_ADDR_LEN];
} apollo_config_gatt_server_params_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t apollo_config_gatt_server_start( apollo_config_gatt_server_params_t *params );

wiced_result_t apollo_config_gatt_server_stop ( void );

#ifdef __cplusplus
} /* extern "C" */
#endif
