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
 *  Provides packet size constants which are useful for implementations of the
 *  network interface and buffer interface.
 */


#ifndef INCLUDED_WWD_NETWORK_CONSTANTS_H_
#define INCLUDED_WWD_NETWORK_CONSTANTS_H_

#include "wwd_buffer_interface.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 * @cond       Constants
 ******************************************************/

/**
 * The space in bytes required for headers in front of the Ethernet header.
 */
#define WICED_LINK_OVERHEAD_BELOW_ETHERNET_FRAME ( sizeof(wiced_buffer_header_t) + 12 + 4 + 2 )

/**
 * The maximum space in bytes required for headers in front of the Ethernet header.
 * This definition allows WICED to use a pre-built bus-generic network stack library regardless of choice of bus.
 * Note: adjust accordingly if a new bus is added.
 */
#define WICED_LINK_OVERHEAD_BELOW_ETHERNET_FRAME_MAX ( 8 + 12 + 4 + 2 )

/**
 * The space in bytes required after the end of an Ethernet packet
 */
#define WICED_LINK_TAIL_AFTER_ETHERNET_FRAME     ( 0 )

/**
 * The size of an Ethernet header
 */
#define WICED_ETHERNET_SIZE         (14)

/**
 * The size in bytes of the Link layer header i.e. the Wiced specific headers and the Ethernet header
 */
#define WICED_PHYSICAL_HEADER       (WICED_LINK_OVERHEAD_BELOW_ETHERNET_FRAME_MAX + WICED_ETHERNET_SIZE)

/**
 * The size in bytes of the data after Link layer packet
 */
#define WICED_PHYSICAL_TRAILER      (WICED_LINK_TAIL_AFTER_ETHERNET_FRAME)

/**
 * The maximum size in bytes of the data part of an Ethernet frame
 */
#ifndef WICED_PAYLOAD_MTU
#define WICED_PAYLOAD_MTU           (1500)
#endif

/**
 * The maximum size in bytes of a packet used within Wiced
 */
#define WICED_LINK_MTU              (WICED_PAYLOAD_MTU + WICED_PHYSICAL_HEADER + WICED_PHYSICAL_TRAILER)


/**
 * Ethernet Ethertypes
 */
#define WICED_ETHERTYPE_IPv4    0x0800
#define WICED_ETHERTYPE_IPv6    0x86DD
#define WICED_ETHERTYPE_ARP     0x0806
#define WICED_ETHERTYPE_RARP    0x8035
#define WICED_ETHERTYPE_EAPOL   0x888E

/** @endcond */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ifndef INCLUDED_WWD_NETWORK_CONSTANTS_H_ */
