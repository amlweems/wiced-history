/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

/** @file
 *  Smartbridge's Helper Function Headers
 */

#include "wiced_utilities.h"
#include "wiced_bt_smartbridge.h"
#include "wiced_bt_smart_interface.h"

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

wiced_result_t smartbridge_helper_delete_scan_result_list( void );
wiced_result_t smartbridge_helper_add_scan_result_to_list( wiced_bt_smart_scan_result_t* result );
wiced_result_t smartbridge_helper_find_device_in_scan_result_list( wiced_bt_device_address_t* address, wiced_bt_smart_address_type_t type,  wiced_bt_smart_scan_result_t** result );
wiced_result_t smartbridge_helper_get_scan_results( wiced_bt_smart_scan_result_t** result_list, uint32_t* count );

wiced_bool_t smartbridge_helper_socket_check_actions_enabled( wiced_bt_smartbridge_socket_t* socket, uint8_t action_bits );
wiced_bool_t smartbridge_helper_socket_check_actions_disabled( wiced_bt_smartbridge_socket_t* socket, uint8_t action_bits );
void smartbridge_helper_socket_set_actions( wiced_bt_smartbridge_socket_t* socket, uint8_t action_bits );
void smartbridge_helper_socket_clear_actions( wiced_bt_smartbridge_socket_t* socket, uint8_t action_bits );

#ifdef __cplusplus
} /* extern "C" */
#endif
