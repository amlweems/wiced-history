/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef INCLUDED_WWD_BUS_PROTOCOL_INTERFACE_H_
#define INCLUDED_WWD_BUS_PROTOCOL_INTERFACE_H_

#include "wwd_constants.h"
#include "wwd_buffer.h"
#include "wwd_bus_protocol.h"

/******************************************************
 *             Constants
 ******************************************************/

typedef enum
{
    BUS_FUNCTION       = 0,
    BACKPLANE_FUNCTION = 1,
    WLAN_FUNCTION      = 2
} bus_function_t;

#define BUS_FUNCTION_MASK (0x3)  /* Update this if adding functions */

/******************************************************
 *             Structures
 ******************************************************/

#pragma pack(1)

typedef struct
{
#ifdef WICED_BUS_HAS_HEADER
                 wiced_bus_header_t  bus_header;
#endif /* ifdef WICED_BUS_HAS_HEADER */
                 uint32_t data[1];
} wiced_transfer_bytes_packet_t;

#pragma pack()

/******************************************************
 *             Function declarations
 ******************************************************/

/* Initialisation functions */
extern wiced_result_t  wiced_bus_init   ( void );
extern wiced_result_t  wiced_bus_deinit ( void );


/* Device register access functions */
extern wiced_result_t  wiced_write_backplane_value ( uint32_t address, uint8_t register_length, uint32_t value );
extern wiced_result_t  wiced_read_backplane_value  ( uint32_t address, uint8_t register_length, /*@out@*/ uint8_t* value );
extern wiced_result_t  wiced_write_register_value  ( bus_function_t function, uint32_t address, uint8_t value_length, uint32_t value );

/* Device data transfer functions */
extern wiced_result_t  wiced_bus_transfer_buffer ( bus_transfer_direction_t direction, bus_function_t function, uint32_t address, wiced_buffer_t buffer );
extern wiced_result_t  wiced_bus_transfer_bytes  ( bus_transfer_direction_t direction, bus_function_t function, uint32_t address, uint16_t size, /*@in@*/ /*@out@*/ wiced_transfer_bytes_packet_t* data );

/* Frame transfer function */
extern /*@only@*/ /*@null@*/ wiced_result_t wiced_read_frame( wiced_buffer_t* buffer );

/* Bus energy saving functions */
extern wiced_result_t wiced_bus_allow_wlan_bus_to_sleep( void );
extern wiced_result_t wiced_bus_ensure_wlan_bus_is_up( void );

extern wiced_result_t wiced_bus_poke_wlan         ( void );
extern wiced_result_t wiced_bus_set_flow_control  ( uint8_t value );
extern wiced_bool_t   wiced_bus_is_flow_controlled( void );
extern uint32_t       wiced_bus_process_interrupt ( void );
extern wiced_result_t wiced_bus_ack_interrupt     ( uint32_t intstatus );

/******************************************************
 *             Global variables
 ******************************************************/

#endif /* ifndef INCLUDED_WWD_BUS_PROTOCOL_INTERFACE_H_ */
