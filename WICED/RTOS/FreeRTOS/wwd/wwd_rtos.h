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
 *  Definitions for the FreeRTOS implementation of the Wiced RTOS
 *  abstraction layer.
 *
 */

#ifndef INCLUDED_WICED_RTOS_H_
#define INCLUDED_WICED_RTOS_H_


#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "wwd_FreeRTOS_systick.h"

#define RTOS_HIGHER_PRIORTIY_THAN(x)     (x < RTOS_HIGHEST_PRIORITY ? x+1 : RTOS_HIGHEST_PRIORITY)
#define RTOS_LOWER_PRIORTIY_THAN(x)      (x > RTOS_LOWEST_PRIORITY ? x-1 : RTOS_LOWEST_PRIORITY)
#define RTOS_LOWEST_PRIORITY             (0)
#define RTOS_HIGHEST_PRIORITY            (configMAX_PRIORITIES-1)
#define RTOS_DEFAULT_THREAD_PRIORITY     (1)

#define WICED_END_OF_THREAD( thread )       vTaskDelete(NULL)

#define RTOS_USE_DYNAMIC_THREAD_STACK

#define malloc_get_current_thread( ) xTaskGetCurrentTaskHandle()
typedef xTaskHandle malloc_thread_handle;

#ifdef DEBUG
#define WICED_THREAD_STACK_SIZE        (632)   /* Stack checking requires a larger stack */
#else /* ifdef DEBUG */
#define WICED_THREAD_STACK_SIZE        (544)
#endif /* ifdef DEBUG */

/******************************************************
 *             Structures
 ******************************************************/

typedef xSemaphoreHandle    host_semaphore_type_t;  /** FreeRTOS definition of a semaphore */
typedef xTaskHandle         host_thread_type_t;     /** FreeRTOS definition of a thread handle */
typedef xQueueHandle        host_queue_type_t;      /** FreeRTOS definition of a message queue */

/*@external@*/ extern void vApplicationMallocFailedHook( void );
/*@external@*/ extern void vApplicationIdleSleepHook( void );

#endif /* ifndef INCLUDED_WICED_RTOS_H_ */
