/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "wps_host.h"
#include "wps_structures.h"

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

extern wiced_result_t besl_wps_init                        ( wps_agent_t* workspace, const besl_wps_device_detail_t* details, wps_agent_type_t type, wiced_interface_t interface );
extern wiced_result_t besl_wps_get_result                  ( wps_agent_t* workspace );
extern wiced_result_t besl_wps_deinit                      ( wps_agent_t* workspace );
extern wiced_result_t besl_wps_start                       ( wps_agent_t* workspace, besl_wps_mode_t mode, char* password, besl_wps_credential_t* credentials, uint16_t credential_length );
extern wiced_result_t besl_wps_restart                     ( wps_agent_t* workspace );
extern wiced_result_t besl_wps_wait_till_complete          ( wps_agent_t* workspace );
extern wiced_result_t besl_wps_abort                       ( wps_agent_t* workspace );
extern wiced_result_t besl_wps_management_set_event_handler( wps_agent_t* workspace, wiced_bool_t enable );
