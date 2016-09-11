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

#include "wiced_result.h"
#include "big_stack_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* Total number of active cached values supported by REST Smart Server */
#ifndef TOTAL_CACHED_VALUES
#define TOTAL_CACHED_VALUES ( 5 )
#endif

/* Total number of simultaneous pairing in progress supported by REST Smart Server */
#ifndef TOTAL_PAIRING_INFOS
#define TOTAL_PAIRING_INFOS ( 5 )
#endif

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

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t restful_smart_server_start( big_peer_device_link_keys_callback_t callback );
wiced_result_t restful_smart_server_stop ( void );

#ifdef __cplusplus
} /* extern "C" */
#endif
