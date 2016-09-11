/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef INCLUDED_WWD_INTERNAL_H
#define INCLUDED_WWD_INTERNAL_H

#include <stdint.h>
#include "wwd_constants.h" /* for wiced_result_t */
#include "Network/wwd_network_interface.h"

/******************************************************
 *             Constants
 ******************************************************/

typedef enum
{
    // Note : If changing this, core_base_address must be changed also
    ARM_CORE    = 0,
    SOCRAM_CORE = 1,
    SDIOD_CORE  = 2
} device_core_t;

typedef enum
{
    WLAN_DOWN,
    WLAN_UP
} wlan_state_t;

/******************************************************
 *             Structures
 ******************************************************/

typedef struct
{
    wlan_state_t         state;
    wiced_country_code_t country_code;
    uint32_t             keep_wlan_awake;
} wiced_wlan_status_t;

/******************************************************
 *             Function declarations
 ******************************************************/

extern wiced_result_t wiced_set_backplane_window ( uint32_t addr );

/* Device core control functions */
extern wiced_result_t wiced_disable_device_core  ( device_core_t core_id );
extern wiced_result_t wiced_reset_device_core    ( device_core_t core_id );
extern wiced_result_t wiced_device_core_is_up    ( device_core_t core_id );

extern wiced_result_t wiced_wifi_set_down(wiced_interface_t interface);

extern void wiced_set_country(wiced_country_code_t code);

/******************************************************
 *             Global variables
 ******************************************************/

extern wiced_wlan_status_t wiced_wlan_status;

#endif /* ifndef INCLUDED_WWD_INTERNAL_H */
