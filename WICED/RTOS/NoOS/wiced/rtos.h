/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "wwd_constants.h"

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


typedef unsigned char    wiced_event_flags_t;

typedef unsigned char    wiced_semaphore_t;

typedef unsigned char    wiced_mutex_t;

typedef void (*timer_handler_t)( void* arg );

typedef unsigned char    wiced_timer_t;

typedef unsigned char    wiced_thread_t;

typedef unsigned char    wiced_queue_t;

typedef unsigned char   wiced_worker_thread_t;

typedef wiced_result_t (*event_handler_t)( void* arg );

typedef unsigned char   wiced_timed_event_t;


/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

