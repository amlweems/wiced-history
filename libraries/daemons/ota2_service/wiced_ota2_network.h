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
 * WICED Over The Air 2 Background Network interface (OTA2)
 *
 *        ***  PRELIMINARY - SUBJECT TO CHANGE  ***
 *
 *  This API allows for disconnecting from current network
 *      and connecting to an alternate network for accessing
 *      the internet and downloading an OTA2 Image File
 */

#pragma once

#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "wiced.h"
#include "platform_dct.h"
#include "../../utilities/mini_printf/mini_printf.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                  Enumerations
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/


/******************************************************
 *               Variables Definitions
 ******************************************************/

/****************************************************************
 *  Internal functions
 ****************************************************************/

/****************************************************************
 *  External functions
 ****************************************************************/
/**
 * Disconnect from current network
 *  Does not change any firmware settings
 *
 * @param    N/A
 *
 * @return - WICED_SUCCESS
 *           WICED_ERROR
 */
wiced_result_t wiced_ota2_network_down( void );

/**
 * Connect to a specific AP
 *  Set APSTA, AP, MPC, rmc_ackreq to 0x00
 *  Set channel
 *  Connect to designated AP
 *  get IP address using DHCP Client
 *
 * @param[in]  ap_info - structure defining AP to connect to
 *
 * @return - WICED_SUCCESS
 *           WICED_ERROR
 *           WICED_BADARG
 */
wiced_result_t wiced_ota2_network_up( wiced_config_ap_entry_t* ap_info );


#ifdef __cplusplus
} /*extern "C" */
#endif

