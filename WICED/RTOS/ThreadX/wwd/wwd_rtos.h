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
 *  Definitions for the ThreadX implementation of the Wiced RTOS
 *  abstraction layer.
 *
 */

#ifndef INCLUDED_WWD_RTOS_H_
#define INCLUDED_WWD_RTOS_H_

#include "tx_api.h"

#define malloc_get_current_thread( ) tx_thread_identify( )
typedef TX_THREAD* malloc_thread_handle;
#define wiced_thread_to_malloc_thread( thread ) ((malloc_thread_handle)(&(thread)->handle))


#define RTOS_HIGHER_PRIORTIY_THAN(x)     ((x) < RTOS_HIGHEST_PRIORITY ? (x)-1 : RTOS_HIGHEST_PRIORITY)
#define RTOS_LOWER_PRIORTIY_THAN(x)      ((x) > RTOS_LOWEST_PRIORITY  ? (x)+1 : RTOS_LOWEST_PRIORITY )
#define RTOS_LOWEST_PRIORITY             (1023)
#define RTOS_HIGHEST_PRIORITY            (0)
#define RTOS_DEFAULT_THREAD_PRIORITY     (4)

#define RTOS_USE_STATIC_THREAD_STACK

#define WICED_END_OF_THREAD(thread)                     (void)(thread)

#ifdef DEBUG
#define WICED_THREAD_STACK_SIZE        (632)   /* Stack checking requires a larger stack */
#else /* ifdef DEBUG */
#define WICED_THREAD_STACK_SIZE        (544)
#endif /* ifdef DEBUG */


/*
 * The number of system ticks per second
 */
#define SYSTICK_FREQUENCY  (1000)

/******************************************************
 *             Structures
 ******************************************************/

typedef TX_SEMAPHORE  host_semaphore_type_t; /** ThreadX definition of a semaphore */
typedef TX_THREAD     host_thread_type_t;    /** ThreadX definition of a thread handle */
typedef TX_QUEUE      host_queue_type_t;     /** ThreadX definition of a message queue */

#endif /* ifndef INCLUDED_WWD_RTOS_H_ */
