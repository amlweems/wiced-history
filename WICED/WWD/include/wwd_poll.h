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
 *  Header for using WICED with no RTOS or network stack
 *
 *  It is possible to use these WICED without any operating system. To do this,
 *  the user application is required to periodically use the functions in this
 *  file to allow WICED to send and receive data across the SPI/SDIO bus.
 *
 */

#ifndef INCLUDED_WWD_POLL_H
#define INCLUDED_WWD_POLL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *             Function declarations
 ******************************************************/

/*@-exportlocal@*/

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
extern int8_t wiced_send_one_packet( void )  /*@modifies internalState @*/;


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
extern int8_t wiced_receive_one_packet( void )  /*@modifies internalState @*/;


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
extern void wiced_poll_all( void ) /*@modifies internalState@*/;

/*@+exportlocal@*/

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ifndef INCLUDED_WWD_POLL_H */
