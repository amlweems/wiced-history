/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Definitions of the Wiced RTOS abstraction layer for the special case
 *  of having no RTOS
 *
 */

#ifndef INCLUDED_WICED_RTOS_H_
#define INCLUDED_WICED_RTOS_H_

#define RTOS_HIGHER_PRIORTIY_THAN(x)     (x)
#define RTOS_LOWER_PRIORTIY_THAN(x)      (x)
#define RTOS_LOWEST_PRIORITY             (0)
#define RTOS_HIGHEST_PRIORITY            (0)
#define RTOS_DEFAULT_THREAD_PRIORITY     (0)

#define RTOS_USE_DYNAMIC_THREAD_STACK
#define WICED_THREAD_STACK_SIZE        (544)

#define WICED_END_OF_THREAD(thread)   (void)(thread)

#define malloc_get_current_thread( ) (NULL)
typedef void* malloc_thread_handle;
#define wiced_thread_to_malloc_thread( thread ) (thread)

/*
 * The number of system ticks per second
 */
#define SYSTICK_FREQUENCY  (1000)

/******************************************************
 *             Structures
 ******************************************************/

typedef volatile unsigned char   host_semaphore_type_t;  /** NoOS definition of a semaphore */
typedef volatile unsigned char   host_thread_type_t;     /** NoOS definition of a thread handle - Would be declared void but that is not allowed. */
typedef volatile unsigned char   host_queue_type_t;      /** NoOS definition of a message queue */

/*@external@*/ extern void NoOS_setup_timing( void );
/*@external@*/ extern void NoOS_stop_timing( void );
/*@external@*/ extern void NoOS_systick_irq( void );

#endif /* ifndef INCLUDED_WICED_RTOS_H_ */
