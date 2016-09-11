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
 *  Implementation of wiced_rtos.c for FreeRTOS
 *
 *  This is the FreeRTOS implementation of the Wiced RTOS
 *  abstraction layer.
 *  It provides Wiced with standard ways of using threads,
 *  semaphores and time functions
 *
 */

#include "wwd_rtos.h"
#include <stdint.h>
#include "wwd_constants.h"
#include "wwd_assert.h"
#include "RTOS/wwd_rtos_interface.h"
#include "StackMacros.h"

/******************************************************
 *             Constants
 ******************************************************/

#define     WICED_THREAD_PRIORITY           ((unsigned long)configMAX_PRIORITIES-1)  /** in FreeRTOS, a higher number is a higher priority */


const uint32_t  ms_to_tick_ratio = 1000 / configTICK_RATE_HZ;

extern void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed portCHAR *pcTaskName );

/******************************************************
 *             Function definitions
 ******************************************************/

/**
 * Creates a new thread
 *
 * @param thread         : pointer to variable which will receive handle of created thread
 * @param entry_function : main thread function
 * @param name           : a string thread name used for a debugger
 *
 * @returns WICED_SUCCESS on success, WICED_ERROR otherwise
 */
wiced_result_t host_rtos_create_thread( /*@out@*/ host_thread_type_t* thread, void(*entry_function)( uint32_t ), const char* name, /*@null@*/ void* stack, uint32_t stack_size, uint32_t priority )
{
    return host_rtos_create_thread_with_arg( thread, entry_function, name, stack, stack_size, priority, 0 );
}

wiced_result_t host_rtos_create_thread_with_arg( /*@out@*/ host_thread_type_t* thread, void(*entry_function)( uint32_t ), const char* name, /*@null@*/ void* stack, uint32_t stack_size, uint32_t priority, uint32_t arg )
{
    signed portBASE_TYPE result;
    /*@-noeffect@*/
    UNUSED_PARAMETER( stack );   /* Unused in release mode */
    /*@+noeffect@*/

    wiced_assert( "Warning: FreeRTOS does not utilize pre-allocated thread stacks. allocated space is wasted\r\n", stack == NULL );

    result = xTaskCreate( (pdTASK_CODE)entry_function, (const signed char * )name, (unsigned short)(stack_size / sizeof( portSTACK_TYPE )), (void*)arg, (unsigned portBASE_TYPE) priority, thread );

    return ( result == (signed portBASE_TYPE) pdPASS ) ? WICED_SUCCESS : WICED_ERROR;
}

/**
 * Terminates the current thread
 *
 * @param thread         : handle of the thread to terminate
 *
 * @returns WICED_SUCCESS on success, WICED_ERROR otherwise
 */
wiced_result_t host_rtos_finish_thread( host_thread_type_t* thread )
{
    vTaskDelete( *thread );

    return WICED_SUCCESS;
}


/**
 * Blocks the current thread until the indicated thread is complete
 *
 * @param thread         : handle of the thread to terminate
 *
 * @returns WICED_SUCCESS on success, WICED_ERROR otherwise
 */
wiced_result_t host_rtos_join_thread( host_thread_type_t* thread )
{
    while ( xTaskIsTaskFinished( *thread ) != pdTRUE )
    {
        host_rtos_delay_milliseconds( 10 );
    }
    return WICED_SUCCESS;
}

/**
 * Deletes a terminated thread
 *
 * FreeRTOS does not require that another thread deletes any terminated thread
 *
 * @param thread         : handle of the terminated thread to delete
 *
 * @returns WICED_SUCCESS on success, WICED_ERROR otherwise
 */
wiced_result_t host_rtos_delete_terminated_thread( host_thread_type_t* thread )
{
    /*@-noeffect@*/
    UNUSED_PARAMETER( thread );
    /*@+noeffect@*/
    return WICED_SUCCESS;
}


/**
 * Creates a semaphore
 *
 * In FreeRTOS a semaphore is represented with a counting semaphore
 *
 * @param semaphore         : pointer to variable which will receive handle of created semaphore
 *
 * @returns WICED_SUCCESS on success, WICED_ERROR otherwise
 */
wiced_result_t host_rtos_init_semaphore( /*@out@*/ host_semaphore_type_t* semaphore )
{
    *semaphore = xSemaphoreCreateCounting( (unsigned portBASE_TYPE) 0x7fffffff, (unsigned portBASE_TYPE) 0 );

    return ( *semaphore != NULL ) ? WICED_SUCCESS : WICED_ERROR;
}


/**
 * Gets a semaphore
 *
 * If value of semaphore is larger than zero, then the semaphore is decremented and function returns
 * Else If value of semaphore is zero, then current thread is suspended until semaphore is set.
 * Value of semaphore should never be below zero
 *
 * Must not be called from interrupt context, since it could block, and since an interrupt is not a
 * normal thread, so could cause RTOS problems if it tries to suspend it.
 *
 * @param semaphore       : Pointer to variable which will receive handle of created semaphore
 * @param timeout_ms      : Maximum period to block for. Can be passed NEVER_TIMEOUT to request no timeout
 * @param will_set_in_isr : True if the semaphore will be set in an ISR. Currently only used for NoOS/NoNS
 *
 */

wiced_result_t host_rtos_get_semaphore( host_semaphore_type_t* semaphore, uint32_t timeout_ms, /*@unused@*/ wiced_bool_t will_set_in_isr )
{
    /*@-noeffect@*/
    UNUSED_PARAMETER( will_set_in_isr );
    /*@+noeffect@*/

    if ( pdTRUE == xSemaphoreTake( *semaphore, (portTickType) ( timeout_ms * ( 1000/configTICK_RATE_HZ ) ) ) )
    {
        return WICED_SUCCESS;
    }
    else
    {
        return WICED_TIMEOUT;
    }
}


/**
 * Sets a semaphore
 *
 * If any threads are waiting on the semaphore, the first thread is resumed
 * Else increment semaphore.
 *
 * Can be called from interrupt context, so must be able to handle resuming other
 * threads from interrupt context.
 *
 * @param semaphore       : Pointer to variable which will receive handle of created semaphore
 * @param called_from_ISR : Value of WICED_TRUE indicates calling from interrupt context
 *                          Value of WICED_FALSE indicates calling from normal thread context
 *
 * @return wiced_result_t : WICED_SUCCESS if semaphore was successfully set
 *                        : WICED_ERROR if an error occurred
 *
 */

wiced_result_t host_rtos_set_semaphore( host_semaphore_type_t* semaphore, wiced_bool_t called_from_ISR )
{
    signed portBASE_TYPE result;

    if ( called_from_ISR == WICED_TRUE )
    {
        signed portBASE_TYPE xHigherPriorityTaskWoken;
        result = xSemaphoreGiveFromISR( *semaphore, &xHigherPriorityTaskWoken );

        wiced_assert( "Unable to set semaphore", result == pdTRUE );

        /* If xSemaphoreGiveFromISR() unblocked a task, and the unblocked task has
         * a higher priority than the currently executing task, then
         * xHigherPriorityTaskWoken will have been set to pdTRUE and this ISR should
         * return directly to the higher priority unblocked task.
         */
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    }
    else
    {
        result = xSemaphoreGive( *semaphore );
        wiced_assert( "Unable to set semaphore", result == pdTRUE );
    }

    return ( result == pdPASS )? WICED_SUCCESS : WICED_ERROR;
}


/**
 * Deletes a semaphore
 *
 * WICED uses this function to delete a semaphore.
 *
 * @param semaphore         : Pointer to the semaphore handle
 *
 * @return wiced_result_t : WICED_SUCCESS if semaphore was successfully deleted
 *                        : WICED_ERROR if an error occurred
 *
 */

wiced_result_t host_rtos_deinit_semaphore( host_semaphore_type_t* semaphore )
{
    if (semaphore != NULL)
    {
        vQueueDelete( *semaphore );
        *semaphore = NULL;
    }
    return WICED_SUCCESS;
}


/**
 * Gets time in milliseconds since RTOS start
 *
 * @Note: since this is only 32 bits, it will roll over every 49 days, 17 hours.
 *
 * @returns Time in milliseconds since RTOS started.
 */
wiced_time_t host_rtos_get_time( void )  /*@modifies internalState@*/
{
    return (wiced_time_t) ( xTaskGetTickCount( ) * ( 1000 / configTICK_RATE_HZ ) );
}


/**
 * Delay for a number of milliseconds
 *
 * Processing of this function depends on the minimum sleep
 * time resolution of the RTOS.
 * The current thread sleeps for the longest period possible which
 * is less than the delay required, then makes up the difference
 * with a tight loop
 *
 * @return wiced_result_t : WICED_SUCCESS if delay was successful
 *                        : WICED_ERROR if an error occurred
 *
 */
wiced_result_t host_rtos_delay_milliseconds( uint32_t num_ms )
{
    uint32_t remainder;

    if ( ( num_ms / ( 1000 / configTICK_RATE_HZ ) ) != 0 )
    {
        vTaskDelay( (portTickType) ( num_ms / ( 1000 / configTICK_RATE_HZ ) ) );
    }

    remainder = num_ms % ( 1000 / configTICK_RATE_HZ );

    if ( remainder != 0 )
    {
        volatile uint32_t clock_in_kilohertz = (uint32_t) ( configCPU_CLOCK_HZ / 1000 );
        for ( ; clock_in_kilohertz != 0; clock_in_kilohertz-- )
        {
            volatile uint32_t tmp_ms = remainder;
            for ( ; tmp_ms != 0; tmp_ms-- )
            {
                /* do nothing */
            }
        }
    }
    return WICED_SUCCESS;

}

void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed portCHAR *pcTaskName )
{
    /*@-noeffect@*/
    UNUSED_PARAMETER( pxTask );
    UNUSED_PARAMETER( pcTaskName ); /* unused parameter in release build */
    /*@+noeffect@*/

    WPRINT_RTOS_DEBUG(("Stack Overflow Detected in task %s\r\n",pcTaskName));
}

void vApplicationMallocFailedHook( void )
{
    WPRINT_RTOS_DEBUG(("Heap is out of memory during malloc\r\n"));
}


wiced_result_t host_rtos_init_queue( host_queue_type_t* queue, void* buffer, uint32_t buffer_size, uint32_t message_size )
{
    UNUSED_PARAMETER(buffer);
    if ( ( *queue = xQueueCreate( buffer_size / message_size, message_size ) ) == NULL )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t host_rtos_push_to_queue( host_queue_type_t* queue, void* message, uint32_t timeout_ms )
{
    if ( xQueueSendToBack( *queue, message, (portTickType) ( timeout_ms / ms_to_tick_ratio ) ) != pdPASS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t host_rtos_pop_from_queue( host_queue_type_t* queue, void* message, uint32_t timeout_ms )
{
    if ( xQueueReceive( *queue, message, ( timeout_ms / ms_to_tick_ratio ) ) != pdPASS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t host_rtos_deinit_queue( host_queue_type_t* queue )
{
    vQueueDelete( *queue );
    return WICED_SUCCESS;
}
