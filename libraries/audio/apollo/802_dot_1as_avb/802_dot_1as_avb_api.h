/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/**
 * @file 802.1as AVB API
 */

#pragma once

#include "wiced_result.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    AVB_FRAME_TYPE_NONE,
    AVB_FRAME_TYPE_ETHERNET_MULTICAST,
    AVB_FRAME_TYPE_ETHERNET_UNICAST,
    AVB_FRAME_TYPE_IP_UDP_MULTICAST
} avb_frame_type_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct avb_timestamp_s
{
        uint32_t    lock;           /* Lock flag, writing when locked */
        uint32_t    avb_ts_ref1;    /* Reference 1 write-back */
        uint16_t    pad;            /* alignment padding */
        uint16_t    ts_sec_h;       /* high 16-bits of timestamp seconds */
        uint32_t    ts_sec_l;       /* low 32-bits of timestamp seconds */
        uint32_t    ts_nsec;        /* timestamp nanoseconds */
        uint32_t    avb_ts;         /* AVB timer reading corresponding to timestamp*/
        uint32_t    avb_ts_ref2;    /* Reference2 write-back */
} avb_timestamp_t;

/******************************************************************************
* @purpose Initialize the AVB library and network.
*
* @param    frame_type   Level 2 Ethernet multicast/unicast PTP or UDP/IP multicast frames
* @param    iface        WICED network interface @ref wiced_interface_t
* @param    client_addr  WICED IPv4 address
* @param    udp_port     UDP port
* @returns  @ref wiced_result_t
*
* @comments
*
*
* @end
*******************************************************************************/
wiced_result_t avblib_init(avb_frame_type_t frame_type, wiced_interface_t iface, wiced_ip_address_t *client_addr, uint16_t udp_port);


/******************************************************************************
* @purpose  Shut down the AVB library
*
* @param    none
*
* @returns  @ref wiced_result_t
*
* @comments
*
*
* @end
*******************************************************************************/
wiced_result_t avblib_deinit(void);


/******************************************************************************
* @purpose  Send a multicast 802.1as Sync Message
*
* @param    none
*
* @returns  @ref wiced_result_t
*
* @comments
*
*
* @end
*******************************************************************************/
wiced_result_t avblib_sync_send(void);


/******************************************************************************
* @purpose  Get the AVB timestamp
*
* @param    unsigned int *AVB_timestamp
*
* @returns  @ref wiced_result_t
*
* @comments
*
*
* @end
*******************************************************************************/
wiced_result_t avblib_ts_get(avb_timestamp_t *avb_timestamp);


#ifdef __cplusplus
} /* extern "C" */
#endif
