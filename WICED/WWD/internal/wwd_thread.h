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
 *  The Wiced Thread allows thread safe access to the Wiced hardware bus
 *  This is an Wiced internal file and should not be used by functions outside Wiced.
 *
 *  This file provides prototypes for functions which allow multiple threads to use the Wiced hardware bus (SDIO or SPI)
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
 */

#ifndef INCLUDED_WWD_THREAD_H_
#define INCLUDED_WWD_THREAD_H_

#include "wwd_buffer.h"
#include "wwd_constants.h"

#define WICED_THREAD_PRIORITY   RTOS_HIGHEST_PRIORITY

/** Initialises the Wiced Thread
 *
 * Initialises the Wiced thread, and its flags/semaphores,
 * then starts it running
 *
 * @return    WICED_SUCCESS : if initialisation succeeds
 *            WICED_ERROR   : otherwise
 */
extern wiced_result_t wiced_thread_init( void )  /*@modifies internalState@*/;


/** Terminates the Wiced Thread
 *
 * Sets a flag then wakes the Wiced Thread to force it to terminate.
 *
 */
extern void wiced_thread_quit( void );


extern void wiced_thread_notify( void );
extern wiced_result_t wiced_get_packet_to_send(wiced_buffer_t* buffer);

#endif /* ifndef INCLUDED_WWD_THREAD_H_ */
