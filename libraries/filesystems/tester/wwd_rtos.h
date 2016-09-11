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
 *
 */

#ifndef INCLUDED_WWD_RTOS_H_
#define INCLUDED_WWD_RTOS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define RTOS_HIGHER_PRIORTIY_THAN(x)     (0)
#define RTOS_LOWER_PRIORTIY_THAN(x)      (0)
#define RTOS_LOWEST_PRIORITY             (0)
#define RTOS_HIGHEST_PRIORITY            (0)
#define RTOS_DEFAULT_THREAD_PRIORITY     (0)

#define RTOS_USE_DYNAMIC_THREAD_STACK
#define WWD_THREAD_STACK_SIZE            (0)

/*
 * The number of system ticks per second
 */
#define SYSTICK_FREQUENCY  (1)

typedef struct
{
    uint8_t info;    /* not supported yet */
} host_rtos_thread_config_type_t;

/******************************************************
 *             Structures
 ******************************************************/

typedef unsigned char   host_semaphore_type_t;
typedef int             host_thread_type_t;
typedef char            host_queue_type_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_RTOS_H_ */
