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
 *
 */

#include "wiced_rtos.h"
#include "wiced_time.h"
#include "wiced_defaults.h"
#include "tx_api.h"
#include "RTOS/wwd_rtos_interface.h"
#include "wiced_utilities.h"
#include "wwd_debug.h"
#include "internal/wiced_internal_api.h"
#include "tx_thread.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define WORKER_THREAD_MONITOR_UPDATE(worker, delay)    do { worker->monitor_info.last_update = host_rtos_get_time(); worker->monitor_info.longest_delay = delay; } while(0)

#define TX_TIMEOUT(timeout_ms)   ((timeout_ms != WICED_NEVER_TIMEOUT) ? ((ULONG)(timeout_ms / ms_to_tick_ratio)) : TX_WAIT_FOREVER )

/******************************************************
 *                    Constants
 ******************************************************/

#ifndef APPLICATION_STACK_SIZE
#define APPLICATION_STACK_SIZE   WICED_DEFAULT_APPLICATION_STACK_SIZE
#endif

#define SYSTEM_MONITOR_THREAD_STACK_SIZE   512


/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef VOID (*native_thread_t)(ULONG);
typedef void (*native_timer_handler_t)(ULONG);

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    event_handler_t function;
    void*           arg;
} wiced_event_message_t;

/******************************************************
 *               Function Declarations
 ******************************************************/

static void application_thread_main    ( ULONG thread_input );
static void application_thread_cleanup ( TX_THREAD* thread_ptr, UINT condition );
static void worker_thread_main         ( uint32_t arg );
static void timed_event_handler        ( void* arg );
extern void system_monitor_thread_main ( void* arg );

/******************************************************
 *               Variables Definitions
 ******************************************************/

const uint32_t        ms_to_tick_ratio = 1000 / SYSTICK_FREQUENCY;
wiced_worker_thread_t wiced_hardware_io_worker_thread;
wiced_worker_thread_t wiced_networking_worker_thread;
#ifndef WICED_DISABLE_WATCHDOG
static wiced_thread_t system_monitor_thread_handle;
#endif /* WICED_DISABLE_WATCHDOG */
/******************************************************
 *               Function Definitions
 ******************************************************/

/**
 *  Main function - starts ThreadX
 *  Called from the crt0 _start function
 *
 */
int main( void )
{
#if defined ( __IAR_SYSTEMS_ICC__ )
/* IAR allows init functions in __low_level_init(), but it is run before global
 * variables have been initialised, so the following init still needs to be done
 * When using GCC, this is done in crt0_GCC.c
 */
    extern void init_platform( void );
    extern void init_architecture( void );

    init_architecture( );
    init_platform( );
#endif /* #elif defined ( __IAR_SYSTEMS_ICC__ ) */    /* Enter the ThreadX kernel.  */
    tx_kernel_enter( );
    return 0;
}

/**
 *  Application Define function - creates and starts the application thread
 *  Called by ThreadX whilst it is initialising
 *
 *  @param first_unused_memory: unused parameter - required to match prototype
 */
void tx_application_define( void *first_unused_memory )
{
	TX_THREAD* app_thread_handle;
	char*      app_thread_stack;

    UNUSED_PARAMETER(first_unused_memory);

    /* Create the application thread.  */
    app_thread_handle = (TX_THREAD*)MALLOC_OBJECT("app thread", TX_THREAD);
    app_thread_stack  = (char*)malloc_named("app stack", APPLICATION_STACK_SIZE);

    tx_thread_create( app_thread_handle, (char*)"app thread", application_thread_main, 0, app_thread_stack, APPLICATION_STACK_SIZE, WICED_APPLICATION_PRIORITY, WICED_APPLICATION_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START );
    tx_thread_entry_exit_notify( app_thread_handle, application_thread_cleanup );
}

void application_thread_cleanup( TX_THREAD *thread_ptr, UINT condition )
{
    /* Determine if the thread was exited. */
    if ( thread_ptr && condition == TX_THREAD_EXIT )
    {
        malloc_transfer_to_curr_thread(thread_ptr->tx_thread_stack_start);
        malloc_transfer_to_curr_thread(thread_ptr);
        tx_thread_terminate( thread_ptr );
        tx_thread_delete( thread_ptr );
        free( thread_ptr->tx_thread_stack_start );
        free( thread_ptr );
        malloc_thread_leak_check(thread_ptr);
    }
}

void application_thread_main( ULONG thread_input )
{
    UNUSED_PARAMETER( thread_input);

#ifndef WICED_DISABLE_WATCHDOG
    /* Start the watchdog kicking thread */
    wiced_rtos_create_thread( &system_monitor_thread_handle, RTOS_HIGHEST_PRIORITY, "system monitor", (wiced_thread_function_t)system_monitor_thread_main, SYSTEM_MONITOR_THREAD_STACK_SIZE, NULL );
#endif /* WICED_DISABLE_WATCHDOG */

    application_start( );
}

wiced_result_t wiced_rtos_init( void )
{
    wiced_result_t result = WICED_SUCCESS;

    WPRINT_RTOS_INFO(("Started ThreadX " ThreadX_VERSION "\r\n"));

    if ( wiced_rtos_create_worker_thread( WICED_HARDWARE_IO_WORKER_THREAD, WICED_DEFAULT_WORKER_PRIORITY, HARDWARE_IO_WORKER_THREAD_STACK_SIZE, HARDWARE_IO_WORKER_THREAD_QUEUE_SIZE ) != WICED_SUCCESS )
    {
        WPRINT_RTOS_INFO(("Failed to create WICED_HARDWARE_IO_WORKER_THREAD\r\n"));
        result = WICED_ERROR;
    }

    if ( wiced_rtos_create_worker_thread( WICED_NETWORKING_WORKER_THREAD,  WICED_NETWORK_WORKER_PRIORITY,  NETWORKING_WORKER_THREAD_STACK_SIZE,  NETWORKING_WORKER_THREAD_QUEUE_SIZE  ) != WICED_SUCCESS )
    {
        WPRINT_RTOS_INFO(("Failed to create WICED_NETWORKING_WORKER_THREAD\r\n"));
        result = WICED_ERROR;
    }

    return result;
}

wiced_result_t wiced_rtos_deinit( void )
{
    wiced_rtos_delete_worker_thread(WICED_HARDWARE_IO_WORKER_THREAD);
    wiced_rtos_delete_worker_thread(WICED_NETWORKING_WORKER_THREAD);

    return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_create_thread( wiced_thread_t* thread, uint8_t priority, const char* name, wiced_thread_function_t function, uint32_t stack_size, void* arg )
{
    thread->stack = malloc_named("stack",stack_size);
    if (thread->stack == NULL)
    {
        return WICED_NOMEM;
    }
    malloc_transfer_to_curr_thread(thread->stack);

	if ( tx_thread_create( &thread->handle, (CHAR*) name, (native_thread_t) function, (ULONG) arg, thread->stack, stack_size, priority, priority, TX_NO_TIME_SLICE, TX_AUTO_START ) != TX_SUCCESS )
    {
        free( thread->stack );
        thread->stack = NULL;
        malloc_thread_leak_check(thread);
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_delete_thread( wiced_thread_t* thread )
{
    tx_thread_terminate( &thread->handle );

    malloc_transfer_to_curr_thread(thread->stack);

    if ( tx_thread_delete( &thread->handle ) != TX_SUCCESS )
    {
        malloc_thread_leak_check(thread);
        return WICED_ERROR;
    }

    free( thread->stack );
    thread->stack = NULL;
    malloc_thread_leak_check(thread);
    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_thread_join( wiced_thread_t* thread )
{
    while ( thread->handle.tx_thread_state != TX_COMPLETED )
    {
        wiced_rtos_delay_milliseconds( 10 );
    }
    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_is_current_thread( wiced_thread_t* thread )
{
    if ( tx_thread_identify( ) == &thread->handle )
    {
        return WICED_SUCCESS;
    }
    else
    {
        return WICED_ERROR;
    }
}


wiced_result_t wiced_rtos_check_stack( void )
{
    // TODO: Add stack checking here.

    // TX_THREAD* current_thread;
    // TX_THREAD_GET_CURRENT(current_thread);
    // TX_THREAD_STACK_CHECK(current_thread);

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_thread_force_awake( wiced_thread_t* thread )
{
    if ( tx_thread_wait_abort( &thread->handle ) == TX_SUCCESS )
    {
        return WICED_SUCCESS;
    }
    else
    {
        return WICED_ERROR;
    }
}


wiced_result_t wiced_rtos_delay_milliseconds( uint32_t milliseconds )
{
    if ( ( tx_thread_sleep( (ULONG) milliseconds / ms_to_tick_ratio ) ) != TX_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_time_get_time( wiced_time_t* time_ptr )
{
    *time_ptr = (wiced_time_t) ( tx_time_get( ) * ms_to_tick_ratio );
    return WICED_SUCCESS;
}


wiced_result_t wiced_time_set_time( wiced_time_t* time_ptr )
{
    tx_time_set( ( *time_ptr ) / ms_to_tick_ratio );
    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_init_semaphore( wiced_semaphore_t* semaphore )
{
    if ( tx_semaphore_create( semaphore, (CHAR*) "", 0 ) != TX_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_set_semaphore( wiced_semaphore_t* semaphore )
{
    if ( tx_semaphore_put( semaphore ) != TX_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_get_semaphore( wiced_semaphore_t* semaphore, uint32_t timeout_ms )
{
    UINT result = tx_semaphore_get( semaphore, TX_TIMEOUT( timeout_ms ) );

    if (result == TX_SUCCESS )
    {
        return WICED_SUCCESS;
    }
    else if (result == TX_NO_INSTANCE)
    {
        return WICED_TIMEOUT;
    }
    else
    {
        return WICED_ERROR;
    }
}


wiced_result_t wiced_rtos_deinit_semaphore( wiced_semaphore_t* semaphore )
{
    if ( tx_semaphore_delete( semaphore ) != TX_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_init_mutex( wiced_mutex_t* mutex )
{
    /* Mutex uses priority inheritance */
    if ( tx_mutex_create( mutex, (CHAR*) "", TX_INHERIT ) != TX_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_lock_mutex( wiced_mutex_t* mutex )
{
    if ( tx_mutex_get( mutex, TX_WAIT_FOREVER ) != TX_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_unlock_mutex( wiced_mutex_t* mutex )
{
    if ( tx_mutex_put( mutex ) != TX_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_deinit_mutex( wiced_mutex_t* mutex )
{
    if ( tx_mutex_delete( mutex ) != TX_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_init_queue( wiced_queue_t* queue, const char* name, uint32_t message_size, uint32_t number_of_messages )
{
    uint32_t queue_size = message_size * number_of_messages;

    if ( ( message_size % 4 ) > 0 )
    {
        return WICED_ERROR;
    }

    queue->buffer = (void*) malloc_named("queue", queue_size);
    malloc_transfer_to_curr_thread(queue->buffer);

    if ( tx_queue_create( &queue->handle, (CHAR*) name, (UINT) ( message_size / 4 ), (VOID *) queue->buffer, (ULONG) queue_size ) != TX_SUCCESS )
    {
        free( queue->buffer );
        queue->buffer = NULL;
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_push_to_queue( wiced_queue_t* queue, void* message, uint32_t timeout_ms )
{
    if ( tx_queue_send( &queue->handle, (VOID*)message, TX_TIMEOUT( timeout_ms ) ) != TX_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_pop_from_queue( wiced_queue_t* queue, void* message, uint32_t timeout_ms )
{
    if ( tx_queue_receive( &queue->handle, (VOID*)message, TX_TIMEOUT( timeout_ms ) ) != TX_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_deinit_queue( wiced_queue_t* queue )
{
    malloc_transfer_to_curr_thread( queue->buffer );

    if ( tx_queue_delete( &queue->handle ) != TX_SUCCESS )
    {
        return WICED_ERROR;
    }

    free( queue->buffer );
    queue->buffer = NULL;

    return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_is_queue_empty( wiced_queue_t* queue )
{
    return ( queue->handle.tx_queue_enqueued == 0 ) ? WICED_SUCCESS : WICED_ERROR;
}

wiced_result_t wiced_rtos_is_queue_full( wiced_queue_t* queue )
{
    return ( queue->handle.tx_queue_available_storage == 0 ) ? WICED_SUCCESS : WICED_ERROR;
}

wiced_result_t wiced_rtos_init_timer( wiced_timer_t* timer, uint32_t time_ms, timer_handler_t function, void* arg )
{
    if ( tx_timer_create( timer, (char*)"", (native_timer_handler_t)function, (ULONG)arg, time_ms / ms_to_tick_ratio, time_ms / ms_to_tick_ratio, 0 ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_start_timer( wiced_timer_t* timer )
{
    if ( tx_timer_activate( timer ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_stop_timer( wiced_timer_t* timer )
{
    if ( tx_timer_deactivate( timer ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_reload_timer( wiced_timer_t* timer )
{
    if ( tx_timer_change( timer, timer->tx_timer_internal.tx_timer_internal_re_initialize_ticks, timer->tx_timer_internal.tx_timer_internal_re_initialize_ticks ) != TX_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_deinit_timer( wiced_timer_t* timer )
{
    if ( tx_timer_delete( timer ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_is_timer_running( wiced_timer_t* timer )
{
    return ( (void*) timer->tx_timer_internal.tx_timer_internal_list_head != TX_NULL ) ? WICED_SUCCESS : WICED_ERROR;
}


static void worker_thread_main( uint32_t arg )
{
    wiced_worker_thread_t* worker_thread = (wiced_worker_thread_t*) arg;

    while ( 1 )
    {
        wiced_event_message_t message;

        if ( wiced_rtos_pop_from_queue( &worker_thread->event_queue, &message, WICED_WAIT_FOREVER ) == WICED_SUCCESS )
        {
            message.function( message.arg );
        }
    }

    WICED_END_OF_THREAD(NULL);
}

wiced_result_t wiced_rtos_create_worker_thread( wiced_worker_thread_t* worker_thread, uint8_t priority, uint32_t stack_size, uint32_t event_queue_size )
{
    memset( worker_thread, 0, sizeof( *worker_thread ) );

    if ( wiced_rtos_init_queue( &worker_thread->event_queue, "worker queue", sizeof(wiced_event_message_t), event_queue_size ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    if ( wiced_rtos_create_thread( &worker_thread->thread, priority, "worker thread", worker_thread_main, stack_size, (void*) worker_thread ) != WICED_SUCCESS )
    {
        wiced_rtos_deinit_queue( &worker_thread->event_queue );
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_delete_worker_thread( wiced_worker_thread_t* worker_thread )
{
    tx_thread_terminate( &worker_thread->thread.handle );

    if ( WICED_SUCCESS != wiced_rtos_deinit_queue( &worker_thread->event_queue ) )
    {
        return WICED_ERROR;
    }

    if ( WICED_SUCCESS != wiced_rtos_delete_thread( &worker_thread->thread ) )
    {
        return WICED_ERROR;
    }

    malloc_thread_leak_check(&worker_thread->thread);

    return WICED_SUCCESS;
}


static void timed_event_handler( void* arg )
{
    wiced_timed_event_t*  event_object = (wiced_timed_event_t*) arg;
    wiced_event_message_t message;

    message.function = event_object->function;
    message.arg      = event_object->arg;

    wiced_rtos_push_to_queue( &event_object->thread->event_queue, &message, WICED_NO_WAIT );
}

wiced_result_t wiced_rtos_register_timed_event( wiced_timed_event_t* event_object, wiced_worker_thread_t* worker_thread, event_handler_t function, uint32_t time_ms, void* arg )
{
    if ( wiced_rtos_init_timer( &event_object->timer, time_ms, timed_event_handler, (void*) event_object ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    event_object->function   = function;
    event_object->thread     = worker_thread;
    event_object->arg        = arg;

    if ( wiced_rtos_start_timer( &event_object->timer ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_deregister_timed_event( wiced_timed_event_t* event_object )
{
    if ( wiced_rtos_deinit_timer( &event_object->timer ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    memset( event_object, 0, sizeof( *event_object ) );

    return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_send_asynchronous_event( wiced_worker_thread_t* worker_thread, event_handler_t function, void* arg )
{
    wiced_event_message_t message;

    message.function = function;
    message.arg      = arg;

    return wiced_rtos_push_to_queue( &worker_thread->event_queue, &message, WICED_NO_WAIT );
}

wiced_result_t wiced_rtos_init_event_flags( wiced_event_flags_t* event_flags )
{
    if ( tx_event_flags_create( event_flags, (CHAR*)"Event flags" ) != TX_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_wait_for_event_flags( wiced_event_flags_t* event_flags, uint32_t flags_to_wait_for, uint32_t* flags_set, wiced_bool_t clear_set_flags, wiced_event_flags_wait_option_t wait_option, uint32_t timeout_ms )
{
    UINT option = 0;

    option |= ( clear_set_flags == WICED_TRUE )      ? ( 1 << 0 ) : 0;
    option |= ( wait_option == WAIT_FOR_ALL_EVENTS ) ? ( 1 << 1 ) : 0;

    if ( tx_event_flags_get( event_flags, flags_to_wait_for, option, (ULONG*)flags_set, TX_TIMEOUT( timeout_ms ) ) != TX_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_set_event_flags( wiced_event_flags_t* event_flags, uint32_t flags_to_set )
{
    if ( tx_event_flags_set( event_flags, flags_to_set, TX_OR ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_deinit_event_flags( wiced_event_flags_t* event_flags )
{
    if ( tx_event_flags_delete( event_flags ) != TX_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}
