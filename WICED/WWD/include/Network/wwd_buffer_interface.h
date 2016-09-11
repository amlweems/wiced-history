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
 *  Defines the WICED Buffer Interface.
 *
 *  Provides prototypes for functions that allow WICED to use packet
 *  buffers in an abstract way.
 *
 */

#ifndef INCLUDED_WWD_BUFFER_INTERFACE_H_
#define INCLUDED_WWD_BUFFER_INTERFACE_H_

#include "wwd_buffer.h"
#include "wwd_constants.h"
#include "wwd_bus_protocol.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 * @cond       Constants
 ******************************************************/

typedef enum
{
    WICED_NETWORK_TX,
    WICED_NETWORK_RX
} wiced_buffer_dir_t;


typedef wiced_buffer_t  wiced_buffer_queue_ptr_t;

#pragma pack(1)

typedef struct
{
    /*@owned@*/  wiced_buffer_queue_ptr_t  queue_next;
#ifdef WICED_BUS_HAS_HEADER
                 wiced_bus_header_t        bus_header;
#endif /* ifdef WICED_BUS_HAS_HEADER */
} wiced_buffer_header_t;

#pragma pack()

/** @endcond */

/** @addtogroup buffif Buffer Interface
 * Allows WICED to use packet buffers in an abstract way.
 *  @{
 */

/******************************************************
 *             Function declarations
 ******************************************************/

/**
 * Initialize the packet buffer interface
 *
 * Implemented in the WICED buffer interface which is specific to the
 * buffering scheme in use.
 * Some implementations of the packet buffer interface may need additional
 * information for initialization, especially the location of packet buffer
 * pool(s). These can be passed via the 'native_arg' parameter. The @ref wiced_management_init
 * function passes the value directly from it's parameters.
 *
 * @param native_arg  An implementation specific argument passed from @ref wiced_management_init
 *
 * @return WICED_SUCCESS = Success, WICED_ERROR = Failure
 */

extern wiced_result_t host_buffer_init( void* native_arg );

/**
 * @brief Allocates a packet buffer
 *
 * Implemented in the WICED buffer interface which is specific to the
 * buffering scheme in use.
 * Attempts to allocate a packet buffer of the size requested. It can do this
 * by allocating a pre-existing packet from a pool, using a static buffer,
 * or by dynamically allocating memory. The method of allocation does not
 * concern WICED, however it must match the way the network stack expects packet
 * buffers to be allocated.
 *
 * @param buffer     A pointer which receives the allocated packet buffer handle
 * @param direction : Indicates transmit/receive direction that the packet buffer is
 *                    used for. This may be needed if tx/rx pools are separate.
 * @param size      : The number of bytes to allocate.
 * @param wait      : Whether to wait for a packet buffer to be available
 *
 * @return WICED_SUCCESS = Success, WICED_ERROR = Failure
 *
 */
extern wiced_result_t host_buffer_get( /*@out@*/ wiced_buffer_t* buffer, wiced_buffer_dir_t direction, unsigned short size, wiced_bool_t wait );

/**
 * Releases a packet buffer
 *
 * Implemented in the Wiced buffer interface, which will be specific to the
 * buffering scheme in use.
 * This function is used by WICED to indicate that it no longer requires
 * a packet buffer. The buffer can then be released back into a pool for
 * reuse, or the dynamically allocated memory can be freed, according to
 * how the packet was allocated.
 * Returns void since WICED cannot do anything about failures
 *
 * @param buffer    : the handle of the packet buffer to be released
 * @param direction : indicates transmit/receive direction that the packet buffer has
 *                    been used for. This might be needed if tx/rx pools are separate.
 *
 */
extern void host_buffer_release( /*@only@*/ wiced_buffer_t buffer, wiced_buffer_dir_t direction );

/**
 * Retrieves the current pointer of a packet buffer
 *
 * Implemented in the WICED buffer interface which is specific to the
 * buffering scheme in use.
 * Since packet buffers usually need to be created with space at the
 * front for additional headers, this function allows WICED to get
 * the current 'front' location pointer.
 *
 * @param buffer : The handle of the packet buffer whose pointer is to be retrieved
 *
 * @return The packet buffer's current pointer.
 */
extern /*@exposed@*/ uint8_t* host_buffer_get_current_piece_data_pointer( wiced_buffer_t buffer );

/**
 * Retrieves the size of a packet buffer
 *
 * Implemented in the WICED buffer interface which is specific to the
 * buffering scheme in use.
 * Since packet buffers usually need to be created with space at the
 * front for additional headers, the memory block used to contain a packet buffer
 * will often be larger than the current size of the packet buffer data.
 * This function allows WICED to retrieve the current size of a packet buffer's data.
 *
 * @param buffer : The handle of the packet buffer whose size is to be retrieved
 *
 * @return The size of the packet buffer.
 */
extern uint16_t host_buffer_get_current_piece_size( wiced_buffer_t buffer );

/**
 * Retrieves the next piece of a set of daisy chained packet buffers
 *
 * Implemented in the WICED buffer interface which is specific to the
 * buffering scheme in use.
 * Some buffering schemes allow buffers to be daisy chained into linked lists.
 * This allows more flexibility with packet buffers and avoids memory copies.
 * It does however require scatter-gather DMA for the hardware bus.
 * This function retrieves the next buffer in a daisy chain of packet buffers.
 *
 * @param buffer : The handle of the packet buffer whose next buffer is to be retrieved
 *
 * @return The handle of the next buffer, or NULL if there is none.
 */
extern /*@exposed@*/ /*@null@*/ wiced_buffer_t host_buffer_get_next_piece( wiced_buffer_t buffer );

/**
 * Moves the current pointer of a packet buffer
 *
 * Implemented in the WICED buffer interface which is specific to the
 * buffering scheme in use.
 * Since packet buffers usually need to be created with space at the
 * front for additional headers, this function allows WICED to move
 * the current 'front' location pointer so that it has space to add headers
 * to transmit packets, and so that the network stack does not see the
 * internal WICED headers on received packets.
 *
 * @param buffer    : A pointer to the handle of the current packet buffer
 *                    for which the current pointer will be moved. On return
 *                    this may contain a pointer to a newly allocated packet
 *                    buffer which has been daisy chained to the front of the
 *                    given one. This would be the case if the given packet buffer
 *                    didn't have enough space at the front.
 * @param add_remove_amount : This is the number of bytes to move the current pointer
 *                            of the packet buffer - a negative value increases the space
 *                            for headers at the front of the packet, a positive value
 *                            decreases the space.
 * @return WICED_SUCCESS = Success, WICED_ERROR = Failure
 */
extern wiced_result_t host_buffer_add_remove_at_front( wiced_buffer_t* buffer, int32_t add_remove_amount ); /* Adds or removes buffer parts as needed (and returns new handle) - new bytes must be contiguous with each other, but not necessarily with original bytes */


/**
 * Checks for buffers that have been leaked
 *
 * Implemented in the WICED buffer interface which is specific to the
 * buffering scheme in use.
 * This function must only be used when all buffers are expected to have been
 * released. Function triggers an assertion if any buffers are in use.
 *
 * @return WICED_SUCCESS = Success, WICED_ERROR = Failure
 */
extern wiced_result_t host_buffer_check_leaked( void );


extern wiced_result_t host_buffer_set_data_end( wiced_buffer_t buffer, uint8_t* end_of_data );

/** @} */

#ifdef __cplusplus
} /*extern "C" */
#endif
#endif /* ifndef INCLUDED_WWD_BUFFER_INTERFACE_H_ */
