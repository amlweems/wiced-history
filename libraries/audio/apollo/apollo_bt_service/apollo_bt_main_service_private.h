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
 * This file provides definitions and function prototypes for Apollo BT main service
 * device
 *
 */
#pragma once

#include "wiced_bt_stack.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define BT_DEVICE_ADDRESS { 0x11, 0x22, 0x33, 0xAA, 0xBB, 0xCC }

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

extern       wiced_bt_cfg_settings_t wiced_bt_cfg_settings;

extern const wiced_bt_cfg_buf_pool_t wiced_bt_cfg_buf_pools[ ];
extern const uint8_t                 sdp_database[];
extern const uint16_t                wiced_bt_sdp_db_size;

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_bt_management_evt_data_t *apollo_bt_service_get_management_evt_data(void);


/**
 * Reconnect to last-connected Bluetooth A2DP audio source.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t apollo_bt_a2dp_sink_connect( void );

wiced_result_t apollo_bt_service_reconnection_timer_start(void);
wiced_result_t apollo_bt_service_reconnection_timer_stop(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
