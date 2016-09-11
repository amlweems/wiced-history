/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *
 */
#include <string.h>
#include "wwd_ring_buffer.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define MIN(x,y)  ((x) < (y) ? (x) : (y))

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
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/


wiced_result_t ring_buffer_init( wiced_ring_buffer_t* ring_buffer, uint8_t* buffer, uint32_t size )
{
    ring_buffer->buffer     = (uint8_t*)buffer;
    ring_buffer->size       = size;
    ring_buffer->head       = 0;
    ring_buffer->tail       = 0;
    return WICED_SUCCESS;
}

wiced_result_t ring_buffer_deinit( wiced_ring_buffer_t* ring_buffer )
{
    UNUSED_PARAMETER(ring_buffer);
    return WICED_SUCCESS;
}

uint32_t ring_buffer_write( wiced_ring_buffer_t* ring_buffer, const uint8_t* data, uint32_t data_length )
{
    uint32_t tail_to_end = ring_buffer->size - ring_buffer->tail;

    /* Calculate the maximum amount we can copy */
    uint32_t amount_to_copy = MIN(data_length, (ring_buffer->tail == ring_buffer->head) ? ring_buffer->size : (tail_to_end + ring_buffer->head) % ring_buffer->size);

    /* Copy as much as we can until we fall off the end of the buffer */
    memcpy(&ring_buffer->buffer[ring_buffer->tail], data, MIN(amount_to_copy, tail_to_end));

    /* Check if we have more to copy to the front of the buffer */
    if (tail_to_end < amount_to_copy)
    {
        memcpy(&ring_buffer->buffer[0], data + tail_to_end, amount_to_copy - tail_to_end);
    }

    /* Update the tail */
    ring_buffer->tail = (ring_buffer->tail + amount_to_copy) % ring_buffer->size;

    return amount_to_copy;
}

wiced_result_t ring_buffer_get_data( wiced_ring_buffer_t* ring_buffer, uint8_t** data, uint32_t* contiguous_bytes )
{
    uint32_t head_to_end = ring_buffer->size - ring_buffer->head;

    *data = &ring_buffer->buffer[ring_buffer->head];
//    ring_buffer->head = ( ring_buffer->head + 1 ) % ring_buffer->size;
    *contiguous_bytes = MIN(head_to_end, (head_to_end + ring_buffer->tail) % ring_buffer->size);
    return WICED_SUCCESS;
}

wiced_result_t ring_buffer_consume( wiced_ring_buffer_t* ring_buffer, uint32_t bytes_consumed )
{
    ring_buffer->head = (ring_buffer->head + bytes_consumed) % ring_buffer->size;
    return WICED_SUCCESS;
}

//uint32_t ring_buffer_read( wiced_ring_buffer_t* ring_buffer, uint8_t* data, uint32_t n, uint32_t timeout_ms )
//{
//    uint32_t i;
//
//    for ( i = 0; i != n; i++ )
//    {
//        if ( wiced_rtos_get_semaphore( &ring_buffer->semaphore, timeout_ms ) != WICED_SUCCESS )
//        {
//            goto exit;
//        }
//
//        data[i] = ring_buffer->buffer[ring_buffer->tail];
//        ring_buffer->tail = ( ring_buffer->tail + 1 ) % ( ring_buffer->size );
//    }
//
//    exit:
//    ring_buffer->remaining += i;
//    return i;
//}

uint32_t ring_buffer_free_space( wiced_ring_buffer_t* ring_buffer )
{
    uint32_t tail_to_end = ring_buffer->size - ring_buffer->tail;
    return ((tail_to_end + ring_buffer->head) % ring_buffer->size);
}

uint32_t ring_buffer_used_space( wiced_ring_buffer_t* ring_buffer )
{
    uint32_t head_to_end = ring_buffer->size - ring_buffer->head;
    return ((head_to_end + ring_buffer->tail) % ring_buffer->size);
}

//wiced_result_t ring_buffer_clear( wiced_ring_buffer_t* ring_buffer )
//{
//    ring_buffer->head      = 0;
//    ring_buffer->tail      = 0;
//    ring_buffer->overrun   = WICED_FALSE;
//    ring_buffer->remaining = ring_buffer->size;
//    return WICED_SUCCESS;
//}

//wiced_bool_t ring_buffer_is_full( wiced_ring_buffer_t* ring_buffer )
//{
//    return ( ring_buffer->remaining == 0 ) ? WICED_TRUE : WICED_FALSE;
//}

//wiced_bool_t ring_buffer_is_empty( wiced_ring_buffer_t* ring_buffer )
//{
//    return ( ring_buffer->remaining == ring_buffer->size ) ? WICED_TRUE : WICED_FALSE;
//}

//wiced_result_t ring_buffer_increment_head( wiced_ring_buffer_t* ring_buffer )
//{
//    ring_buffer->head = ( ring_buffer->head + 1 ) % ( ring_buffer->size );
//
//    if ( ring_buffer->remaining )
//    {
//        ring_buffer->remaining--;
//        wiced_rtos_set_semaphore( &ring_buffer->semaphore );
//    }
//    else
//    {
//        ring_buffer->overrun = WICED_TRUE;
//    }
//
//    return WICED_SUCCESS;
//}

//wiced_result_t ring_buffer_increment_tail( wiced_ring_buffer_t* ring_buffer )
//{
//    if ( ring_buffer->remaining < ring_buffer->size )
//    {
//        if ( wiced_rtos_get_semaphore( &ring_buffer->semaphore, WICED_NO_WAIT ) != WICED_SUCCESS )
//        {
//            return WICED_TIMEOUT;
//        }
//
//        ring_buffer->remaining++;
//        ring_buffer->tail = ( ring_buffer->tail + 1 ) % ( ring_buffer->size );
//    }
//
//    return WICED_SUCCESS;
//}
