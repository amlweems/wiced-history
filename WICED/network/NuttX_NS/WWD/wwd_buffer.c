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
 *  Wiced NuttX networking layer
 */

#include "wwd_buffer.h"
#include "wwd_assert.h"
#include "wwd_network_constants.h"
#include "wwd_rtos_interface.h"
#include "network/wwd_buffer_interface.h"
#include "platform/wwd_bus_interface.h"
#include "wiced_utilities.h"

#include "platform_toolchain.h"

/******************************************************
 *                      Macros
 ******************************************************/

#ifdef SUPPORT_BUFFER_CHAINING
#error Not supported
#endif

#define NUM_BUFFERS_POOL_SIZE(x)    (WICED_LINK_MTU_ALIGNED * (x))

/******************************************************
 *                    Constants
 ******************************************************/

#ifndef TX_PACKET_POOL_SIZE
#define TX_PACKET_POOL_SIZE         (7)
#endif

#ifndef RX_PACKET_POOL_SIZE
#define RX_PACKET_POOL_SIZE         (7)
#endif

#define TX_BUFFER_POOL_SIZE         NUM_BUFFERS_POOL_SIZE(TX_PACKET_POOL_SIZE)
#define RX_BUFFER_POOL_SIZE         NUM_BUFFERS_POOL_SIZE(RX_PACKET_POOL_SIZE)

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    struct sq_queue_s     queue;
    uint16_t              len;
    host_semaphore_type_t semaphore;
} host_buffer_pool_t;

/******************************************************
 *                 Static Variables
 ******************************************************/

static uint8_t               tx_buffer_pool_memory[TX_BUFFER_POOL_SIZE] ALIGNED(PLATFORM_L1_CACHE_BYTES);
static uint8_t               rx_buffer_pool_memory[RX_BUFFER_POOL_SIZE] ALIGNED(PLATFORM_L1_CACHE_BYTES);

static wiced_buffer_impl_t   tx_buffers[TX_PACKET_POOL_SIZE];
static wiced_buffer_impl_t   rx_buffers[RX_PACKET_POOL_SIZE];

static host_buffer_pool_t    rx_buffer_pool;
static host_buffer_pool_t    tx_buffer_pool;

static wiced_bool_t          buffers_inited = WICED_FALSE;

/******************************************************
 *            Static Function Definitions
 ******************************************************/

static host_buffer_pool_t* host_buffer_pool_for_direction( wwd_buffer_dir_t direction )
{
    switch ( direction )
    {
        case WWD_NETWORK_TX:
            return &tx_buffer_pool;

        case WWD_NETWORK_RX:
            return &rx_buffer_pool;

        default:
            wiced_assert( "not handled", 0 );
            return NULL;
    }
}

static wwd_buffer_dir_t host_buffer_direction_for_pool( host_buffer_pool_t* pool )
{
    if ( pool == &tx_buffer_pool )
    {
        return WWD_NETWORK_TX;
    }
    else if ( pool == &rx_buffer_pool )
    {
        return WWD_NETWORK_RX;
    }
    else
    {
        wiced_assert( "not handled", 0 );
        return WWD_NETWORK_RX;
    }
}

static void host_buffer_pool_put( host_buffer_pool_t* pool, wiced_buffer_t buffer )
{
    wwd_result_t wwd_result;
    irqstate_t   flags;

    UNUSED_VARIABLE( wwd_result );

    flags = irqsave( );

    sq_addlast( &buffer->node, &pool->queue );
    pool->len++;
    wiced_assert( "must not wrap around", pool->len != 0 );

    irqrestore( flags );

    wwd_result = host_rtos_set_semaphore( &pool->semaphore, WICED_FALSE );
    wiced_assert( "semaphore set failed", wwd_result == WWD_SUCCESS );
}

static wwd_result_t host_buffer_pool_get( host_buffer_pool_t* pool, uint16_t size, uint32_t timeout_ms, wiced_buffer_t* buffer )
{
    wwd_result_t result;
    irqstate_t   flags;

    wiced_assert( "bad size", size <= WICED_LINK_MTU );

    result = host_rtos_get_semaphore( &pool->semaphore, timeout_ms, WICED_FALSE );
    if ( result != WWD_SUCCESS )
    {
        return result;
    }

    flags = irqsave( );

    wiced_assert( "must be not empty", pool->len != 0 );
    pool->len--;
    *buffer = (wiced_buffer_t)sq_remfirst( &pool->queue );

    irqrestore( flags );

    wiced_assert( "must be never NULL", *buffer != NULL );

    (*buffer)->size   = size;
    (*buffer)->offset = 0;
    (*buffer)->pool   = pool;

    return WWD_SUCCESS;
}

static wwd_result_t host_buffer_pool_get_for_direction( wwd_buffer_dir_t direction, uint16_t size, uint32_t timeout_ms, wiced_buffer_t* buffer )
{
    return host_buffer_pool_get( host_buffer_pool_for_direction( direction ), size, timeout_ms, buffer );
}

static void host_buffer_pool_init( host_buffer_pool_t* pool, wiced_buffer_impl_t* buffers, unsigned buffers_num, uint8_t* pool_memory )
{
    wwd_result_t wwd_result;
    unsigned     i;

    UNUSED_VARIABLE( wwd_result );

    sq_init( &pool->queue );

    wwd_result = host_rtos_init_semaphore( &pool->semaphore );
    wiced_assert( "semaphore init failed", wwd_result == WWD_SUCCESS );

    for ( i = 0; i < buffers_num; i++ )
    {
        buffers[ i ].buffer = pool_memory + WICED_LINK_MTU_ALIGNED * i;
        host_buffer_pool_put( pool, &buffers[ i ] );
    }
}

static void host_buffer_pool_deinit( host_buffer_pool_t* pool )
{
    wwd_result_t wwd_result;

    UNUSED_VARIABLE ( wwd_result );

    wwd_result = host_rtos_deinit_semaphore( &pool->semaphore );
    wiced_assert( "semaphore deinit failed", wwd_result == WWD_SUCCESS );
}

static void host_buffer_release_notify( host_buffer_pool_t* pool )
{
#ifdef PLAT_NOTIFY_FREE
    wwd_buffer_dir_t pool_direction = host_buffer_direction_for_pool( pool );

    host_platform_bus_buffer_freed( pool_direction );
    host_platform_bus_buffer_freed_nuttx( pool_direction == WWD_NETWORK_TX );
#else
    UNUSED_PARAMETER( pool );
#endif /* ifdef PLAT_NOTIFY_FREE */
}

/******************************************************
 *               Function Definitions
 ******************************************************/

wwd_result_t wwd_buffer_init( void* native_arg )
{
    UNUSED_PARAMETER( native_arg );

    if ( buffers_inited )
    {
        wiced_assert( "already inited", 0 );
        return WWD_BUFFER_ALLOC_FAIL;
    }

    host_buffer_pool_init( &rx_buffer_pool, rx_buffers, ARRAY_SIZE( rx_buffers ), rx_buffer_pool_memory );
    host_buffer_pool_init( &tx_buffer_pool, tx_buffers, ARRAY_SIZE( tx_buffers ), tx_buffer_pool_memory );

    buffers_inited = WICED_TRUE;

    return WWD_SUCCESS;
}

wwd_result_t wwd_buffer_deinit( void )
{
    wwd_result_t result;

    if ( !buffers_inited )
    {
        wiced_assert( "not yet inited", 0 );
        return WWD_BUFFER_ALLOC_FAIL;
    }

    result = host_buffer_check_leaked( );
    if ( result != WWD_SUCCESS )
    {
        return result;
    }

    host_buffer_pool_deinit( &tx_buffer_pool );
    host_buffer_pool_deinit( &rx_buffer_pool );

    buffers_inited = WICED_FALSE;

    return WWD_SUCCESS;
}

wwd_result_t host_buffer_check_leaked( void )
{
    wiced_assert( "TX pool leakage", tx_buffer_pool.len == TX_PACKET_POOL_SIZE );
    wiced_assert( "RX pool leakage", rx_buffer_pool.len == RX_PACKET_POOL_SIZE );
    return WWD_SUCCESS;
}

wwd_result_t internal_host_buffer_get( wiced_buffer_t* buffer, wwd_buffer_dir_t direction, unsigned short size, unsigned long timeout_ms )
{
    return host_buffer_pool_get_for_direction( direction, size, timeout_ms, buffer );
}

wwd_result_t host_buffer_get( wiced_buffer_t* buffer, wwd_buffer_dir_t direction, unsigned short size, wiced_bool_t wait )
{
    return host_buffer_pool_get_for_direction( direction, size, wait ? NEVER_TIMEOUT : 0, buffer );
}

void host_buffer_release( wiced_buffer_t buffer, wwd_buffer_dir_t direction )
{
    host_buffer_pool_t* pool = buffer->pool;

    UNUSED_PARAMETER( direction );

    host_buffer_pool_put( pool, buffer );
    host_buffer_release_notify( pool );
}

uint8_t* host_buffer_get_current_piece_data_pointer( wiced_buffer_t buffer )
{
    return ( buffer->buffer + buffer->offset );
}

uint16_t host_buffer_get_current_piece_size( wiced_buffer_t buffer )
{
    return buffer->size;
}

wiced_buffer_t host_buffer_get_next_piece( wiced_buffer_t buffer )
{
    UNUSED_PARAMETER(buffer);
    wiced_assert("not implemented", WICED_FALSE);
    return NULL;
}

wwd_result_t host_buffer_add_remove_at_front( wiced_buffer_t* buffer, int32_t add_remove_amount )
{
    const uint16_t size = (*buffer)->size;
    wiced_assert("bad delta", ( add_remove_amount > 0 ) ? ( add_remove_amount <= size ) : ( -add_remove_amount <= (WICED_LINK_MTU - size ) ) );
    (*buffer)->offset = (uint16_t)( (*buffer)->offset + add_remove_amount );
    (*buffer)->size   = (uint16_t)( (*buffer)->size   - add_remove_amount );
    return WWD_SUCCESS;
}

wwd_result_t host_buffer_set_size( wiced_buffer_t buffer, unsigned short size )
{
    wiced_assert( "bad size", size <= WICED_LINK_MTU );
    buffer->size = size;
    return WWD_SUCCESS;
}

void host_buffer_init_fifo( wiced_buffer_fifo_t* fifo )
{
    sq_init( fifo );
}

void host_buffer_push_to_fifo( wiced_buffer_fifo_t* fifo, wiced_buffer_t buffer, wwd_interface_t interface )
{
    irqstate_t flags;

    buffer->interface = interface;

    flags = irqsave( );
    sq_addlast( &buffer->node, fifo );
    irqrestore( flags );
}

wiced_buffer_t host_buffer_pop_from_fifo( wiced_buffer_fifo_t* fifo, wwd_interface_t* interface )
{
    irqstate_t flags;
    wiced_buffer_t buffer;

    flags = irqsave( );
    buffer = (wiced_buffer_t)sq_remfirst( fifo );
    irqrestore( flags );

    if ( buffer && interface )
    {
        *interface = buffer->interface;
    }

    return buffer;
}
