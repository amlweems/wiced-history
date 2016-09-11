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

#define APOLLO_CONFIG_GATT_ATTRIBUTE_SIZE (22)

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
    char                spk_name[APOLLO_CONFIG_GATT_ATTRIBUTE_SIZE];
    uint8_t             mode;
    uint8_t             is_configured;
    wiced_security_t    security;
    char                nw_ssid_name[APOLLO_CONFIG_GATT_ATTRIBUTE_SIZE];
    char                nw_pass_phrase[APOLLO_CONFIG_GATT_ATTRIBUTE_SIZE];
    uint8_t             spk_channel_map;
    uint8_t             spk_vol;
    uint8_t             src_type;
    uint8_t             src_vol;
} apollo_config_gatt_server_dct_t;

typedef wiced_result_t ( *gatt_event_cbf )( apollo_config_gatt_event_t event, apollo_config_gatt_server_dct_t *dct, void *user_context );

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    gatt_event_cbf  gatt_event_cbf;   /*!< GATT configuration service event callback @apollo_config_gatt_event_t */
    void           *user_context;     /*!< User context pointer                                                  */
} apollo_config_gatt_server_params_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/**
 * Start Bluetooth GATT configuration services.
 * apollo_bt_service_init() must be called first; otherwise, this call will fail.
 *
 * @param[in]  params          GATT server initialization parameters @apollo_config_gatt_server_params_t
 *
 * @return @ref wiced_result_t
 */
wiced_result_t apollo_config_gatt_server_start( apollo_config_gatt_server_params_t *params );


/**
 * Shutdown and cleanup GATT configuration service.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t apollo_config_gatt_server_stop ( void );

#ifdef __cplusplus
} /* extern "C" */
#endif
