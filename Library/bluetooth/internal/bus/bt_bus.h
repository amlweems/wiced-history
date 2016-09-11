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

#include "wiced_utilities.h"

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

typedef void (*bt_bus_isr)( void );

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t bt_bus_init( void );

wiced_result_t bt_bus_deinit( void );

wiced_result_t bt_bus_enable_irq( bt_bus_isr isr );

wiced_result_t bt_bus_disable_irq( void );

wiced_result_t bt_bus_transmit( const uint8_t* data_out, uint32_t size );

wiced_result_t bt_bus_receive( uint8_t* data_in, uint32_t size, uint32_t timeout_ms );

wiced_bool_t   bt_bus_is_ready( void );

wiced_bool_t   bt_bus_is_on( void );
