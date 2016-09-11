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
 *  Allows thread safe access to the Wiced hardware bus
 *
 *  This file provides functions which allow multiple threads to use the Wiced hardware bus (SDIO or SPI)
 *  This is achieved by having a single thread (the "Wiced Thread") which queues messages to be sent, sending
 *  them sequentially, as well as receiving messages as they arrive.
 *
 *  Messages to be sent come from the wiced_send_sdpcm_common function in SDPCM.c .  The messages already
 *  contain SDPCM headers, but not any bus headers (GSPI), and are passed to the wiced_thread_send_data function.
 *  This function can be called from any thread.
 *
 *  Messages are received by way of a callback supplied by in SDPCM.c - wiced_process_sdpcm
 *  Received messages are delivered in the context of the Wiced Thread, so the callback function needs to avoid blocking.
 *
 *  It is also possible to use these functions without any operating system, by periodically calling the wiced_send_one_packet,
 *  wiced_receive_one_packet or wiced_poll_all functions
 *
 */

#include <string.h>
#include "wwd_assert.h"
#include "wwd_logging.h"
#include "wwd_poll.h"
#include "RTOS/wwd_rtos_interface.h"
#include "Network/wwd_buffer_interface.h"
#include "Network/wwd_network_interface.h"
#include "Platform/wwd_bus_interface.h"
#include "internal/wwd_thread.h"
#include "internal/SDPCM.h"
#include "internal/wwd_internal.h"
#include "internal/Bus_protocols/wwd_bus_protocol_interface.h"
#include "wwd_rtos.h"

#define WICED_THREAD_POLL_TIMEOUT      (NEVER_TIMEOUT)

#ifdef RTOS_USE_STATIC_THREAD_STACK
static uint8_t wiced_thread_stack[WICED_THREAD_STACK_SIZE];
#define WICED_THREAD_STACK     wiced_thread_stack
#else
#ifdef RTOS_USE_DYNAMIC_THREAD_STACK
#define WICED_THREAD_STACK     NULL
#else
#error RTOS_USE_STATIC_THREAD_STACK or RTOS_USE_DYNAMIC_THREAD_STACK must be defined
#endif
#endif


/******************************************************
 *             Static Variables
 ******************************************************/

static char                  wiced_thread_quit_flag = (char) 0;
static char                  wiced_inited           = (char) 0;
static host_thread_type_t    wiced_thread;
static host_semaphore_type_t wiced_transceive_semaphore;

static wiced_bool_t wiced_bus_interrupt;

/******************************************************
 *             Static Function Prototypes
 ******************************************************/

static void wiced_thread_func( /*@unused@*/ uint32_t thread_input )   /*@globals wiced_thread, wiced_packet_send_queue_mutex, wiced_transceive_semaphore@*/ /*@modifies wiced_thread_quit_flag, wiced_inited@*/;

/******************************************************
 *             Global Functions
 ******************************************************/


/** Initialises the Wiced Thread
 *
 * Initialises the Wiced thread, and its flags/semaphores,
 * then starts it running
 *
 * @return    WICED_SUCCESS : if initialisation succeeds
 *            WICED_ERROR   : otherwise
 */
wiced_result_t wiced_thread_init( void ) /*@globals undef wiced_thread, undef wiced_packet_send_queue_mutex, undef wiced_transceive_semaphore@*/ /*@modifies wiced_inited@*/
{
    if ( wiced_init_sdpcm( ) != WICED_SUCCESS )
    {
        WPRINT_WWD_ERROR(("Could not initialize SDPCM codec\r\n"));
        /*@-globstate@*/
        return WICED_ERROR;
        /*@+globstate@*/
    }

    /* Create the event flag which signals the Wiced thread needs to wake up */
    if ( host_rtos_init_semaphore( &wiced_transceive_semaphore ) != WICED_SUCCESS )
    {
        WPRINT_WWD_ERROR(("Could not initialize WICED thread semaphore\r\n"));
        /*@-globstate@*/
        return WICED_ERROR;
        /*@+globstate@*/
    }

    if ( WICED_SUCCESS != host_rtos_create_thread( &wiced_thread, wiced_thread_func, "WICED", WICED_THREAD_STACK, (uint32_t) WICED_THREAD_STACK_SIZE, (uint32_t) WICED_THREAD_PRIORITY ) )
    {
        /* could not start wiced main thread */
        WPRINT_WWD_ERROR(("Could not start WICED thread\r\n"));
        return WICED_ERROR;
    }

    wiced_inited = (char) 1;
    return WICED_SUCCESS;
}

/** Sends the first queued packet
 *
 * Checks the queue to determine if there is any packets waiting
 * to be sent. If there are, then it sends the first one.
 *
 * This function is normally used by the Wiced Thread, but can be
 * called periodically by systems which have no RTOS to ensure
 * packets get sent.
 *
 * @return    1 : packet was sent
 *            0 : no packet sent
 */
int8_t wiced_send_one_packet( void ) /*@modifies internalState, wiced_packet_send_queue_head, wiced_packet_send_queue_tail@*/
{
    wiced_buffer_t tmp_buf_hnd = NULL;
    int8_t ret = 0;

    if (wiced_get_packet_to_send(&tmp_buf_hnd) == WICED_SUCCESS )
    {
        /* Ensure the wlan backplane bus is up */
        if ( WICED_SUCCESS == wiced_bus_ensure_wlan_bus_is_up() )
        {
            WPRINT_WWD_DEBUG(("Wcd:> Sending pkt 0x%08X\n\r", (unsigned int)tmp_buf_hnd ));
            if ( WICED_SUCCESS == wiced_bus_transfer_buffer( BUS_WRITE, WLAN_FUNCTION, 0, tmp_buf_hnd ) )
            {
                ret = (int8_t) 1;
            }
        }
        else
        {
            wiced_assert("Could not bring bus back up", 0 != 0 );
        }

        host_buffer_release( tmp_buf_hnd, WICED_NETWORK_TX );
    }

    return ret;
}

/** Receives a packet if one is waiting
 *
 * Checks the wifi chip fifo to determine if there is any packets waiting
 * to be received. If there are, then it receives the first one, and calls
 * the callback wiced_process_sdpcm (in SDPCM.c).
 *
 * This function is normally used by the Wiced Thread, but can be
 * called periodically by systems which have no RTOS to ensure
 * packets get received properly.
 *
 * @return    1 : packet was received
 *            0 : no packet waiting
 */
int8_t wiced_receive_one_packet( void )
{
    /* Check if there is a packet ready to be received */
    wiced_buffer_t recv_buffer;
    if ( wiced_read_frame( &recv_buffer ) == WICED_SUCCESS)
    {
        if ( recv_buffer != NULL )
        {
            WICED_LOG(("Wcd:< Rcvd pkt 0x%08X\n", (unsigned int)recv_buffer ));
            /* Send received buffer up to SDPCM layer */
            wiced_process_sdpcm( recv_buffer );
        }
        return (int8_t) 1;
    }
    return 0;
}

/** Sends and Receives all waiting packets
 *
 * Repeatedly calls wiced_send_one_packet and wiced_receive_one_packet
 * to send and receive packets, until there are no more packets waiting to
 * be transferred.
 *
 * This function is normally used by the Wiced Thread, but can be
 * called periodically by systems which have no RTOS to ensure
 * packets get send and received properly.
 *
 */
void wiced_poll_all( void ) /*@modifies internalState@*/
{
    do
    {
        /* Send queued outgoing packets */
        while ( wiced_send_one_packet( ) != 0 )
        {
            /* loop whist packets still queued */
        }
    } while ( wiced_receive_one_packet( ) != 0 );
}

/** Terminates the Wiced Thread
 *
 * Sets a flag then wakes the Wiced Thread to force it to terminate.
 *
 */
void wiced_thread_quit( void )
{
    /* signal main thread and wake it */
    wiced_thread_quit_flag = (char) 1;
    host_rtos_set_semaphore( &wiced_transceive_semaphore, WICED_FALSE );

    /* Wait for the Wiced thread to end */
    while ( wiced_inited != 0 )
    {
        host_rtos_delay_milliseconds( 1 );
    }
    host_rtos_delete_terminated_thread( &wiced_thread );
}

/**
 * Informs Wiced of an interrupt
 *
 * This function should be called from the SDIO/SPI interrupt function
 * and usually indicates newly received data is available.
 * It wakes the Wiced Thread, forcing it to check the send/receive
 *
 */
void wiced_platform_notify_irq( void )
{
    /* just wake up the main thread and let it deal with the data */
    if ( wiced_inited == (char) 1 )
    {
        wiced_bus_interrupt = WICED_TRUE;
        host_rtos_set_semaphore( &wiced_transceive_semaphore, WICED_TRUE );
    }
}

void wiced_thread_notify( void )
{
    /* just wake up the main thread and let it deal with the data */
    if ( wiced_inited == (char) 1 )
    {
        host_rtos_set_semaphore( &wiced_transceive_semaphore, WICED_FALSE );
    }
}

/******************************************************
 *             Static Functions
 ******************************************************/

/** The Wiced Thread function
 *
 *  This is the main loop of the Wiced Thread.
 *  It simply calls wiced_poll_all to send/receive all waiting packets, then goes
 *  to sleep.  The sleep has a 100ms timeout, causing the send/receive queues to be
 *  checked 10 times per second in case an interrupt is missed.
 *  Once the quit flag has been set, flags/mutexes are cleaned up, and the function exits.
 *
 * @param thread_input  : unused parameter needed to match thread prototype.
 *
 */
static void wiced_thread_func( uint32_t /*@unused@*/thread_input )   /*@globals wiced_thread, wiced_packet_send_queue_mutex, wiced_transceive_semaphore, wiced_wlan_status@*/ /*@modifies wiced_thread_quit_flag, wiced_inited@*/
{
    uint32_t       int_status;
    int8_t         rx_status;
    int8_t         tx_status;

    wiced_result_t result;

    /*@-noeffect@*/
    UNUSED_PARAMETER(thread_input);
    /*@+noeffect@*/

    wiced_bus_interrupt = WICED_FALSE;

    while ( wiced_thread_quit_flag != (char) 1 )
    {
        /* Check if we were woken by interrupt */
        if ( ( wiced_bus_interrupt == WICED_TRUE ) || WICED_BUS_USE_STATUS_REPORT_SCHEME )
        {
            wiced_bus_interrupt = WICED_FALSE;
            int_status = wiced_bus_process_interrupt();

            /* Check if the interrupt indicated there is a packet to read */
            if ( WICED_BUS_PACKET_AVAILABLE_TO_READ(int_status) != 0)
            {
                /* Receive all available packets */
                do
                {
                    rx_status = wiced_receive_one_packet( );
                } while ( rx_status != 0 );
            }
        }

        /* Send all queued packets */
        do
        {
            tx_status = wiced_send_one_packet( );
        }
        while (tx_status != 0);

        /* Check if we have run out of bus credits */
        if ( wiced_get_available_bus_credits( ) == 0 )
        {
            /* Keep poking the WLAN until it gives us more credits */
            wiced_bus_poke_wlan( );
            result = host_rtos_get_semaphore( &wiced_transceive_semaphore, (uint32_t) 100, WICED_FALSE );
        }
        else
        {
            /* Put the bus to sleep and wait for something else to do */
            if ( wiced_wlan_status.keep_wlan_awake == 0 )
            {
                result = wiced_bus_allow_wlan_bus_to_sleep( );
                wiced_assert( "Error setting wlan sleep", result == WICED_SUCCESS );
            }

            result = host_rtos_get_semaphore( &wiced_transceive_semaphore, (uint32_t) WICED_THREAD_POLL_TIMEOUT, WICED_FALSE );
            REFERENCE_DEBUG_ONLY_VARIABLE(result);
        } wiced_assert("Could not get wiced sleep semaphore\r\n", (result == WICED_SUCCESS)||(result == WICED_TIMEOUT) );
    }

    /* Reset the quit flag */
    wiced_thread_quit_flag = (char) 0;

    /* Delete the semaphore */
    host_rtos_deinit_semaphore( &wiced_transceive_semaphore );

    wiced_quit_sdpcm( );
    wiced_inited = (char) 0;

    if ( WICED_SUCCESS != host_rtos_finish_thread( &wiced_thread ) )
    {
        WPRINT_WWD_DEBUG(("Could not close WICED thread\r\n"));
    }

    WICED_END_OF_THREAD(NULL);
}

