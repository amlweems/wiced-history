/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef INCLUDED_WWD_RING_BUFFER__H
#define INCLUDED_WWD_RING_BUFFER__H

#include <stdint.h>
#include "wwd_constants.h"
#include "RTOS/wwd_rtos_interface.h"

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

typedef struct
{
    uint8_t*  buffer;
    uint32_t  size;
    uint32_t  head;
    uint32_t  tail;
} wiced_ring_buffer_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/* Ring Buffer API */
wiced_result_t ring_buffer_init             ( wiced_ring_buffer_t* ring_buffer, uint8_t* buffer, uint32_t size );
wiced_result_t ring_buffer_deinit           ( wiced_ring_buffer_t* ring_buffer );
uint32_t       ring_buffer_write            ( wiced_ring_buffer_t* ring_buffer, const uint8_t* data, uint32_t data_length );
uint32_t       ring_buffer_used_space       ( wiced_ring_buffer_t* ring_buffer );
uint32_t       ring_buffer_free_space       ( wiced_ring_buffer_t* ring_buffer );
wiced_result_t ring_buffer_get_data         ( wiced_ring_buffer_t* ring_buffer, uint8_t** data, uint32_t* contiguous_bytes );
wiced_result_t ring_buffer_consume          ( wiced_ring_buffer_t* ring_buffer, uint32_t bytes_consumed );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_RING_BUFFER__H */
