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

#define MAX_GATT_ATTRIBUTE_LENGTH  ( 512 )

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

/**
 * Initialise Bluetooth Internet Gateway (BIG)
 *
 * @param[in] callback : callback function that will be called when application using BIG needs to load or store local keys
 *
 * @return @ref wiced_result_t
 */
wiced_result_t bt_internet_gateway_init( big_local_keys_callback_t callback );

/**
 * Deinitialise Bluetooth Internet Gateway
 *
 * @return @ref wiced_result_t
 */
wiced_result_t bt_internet_gateway_deinit( void );

#ifdef __cplusplus
} /* extern "C" */
#endif
