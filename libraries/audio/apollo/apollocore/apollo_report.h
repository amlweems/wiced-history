/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define APOLLO_REPORT_MAGIC_TAG         (0x5CAFF01D)

#define APOLLO_REPORT_VERSION           (1)
#define APOLLO_REPORT_STATS_VERSION     (1)

#define APOLLO_REPORT_PORT              (19705)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    APOLLO_REPORT_MSG_STATS = 1,
} APOLLO_REPORT_MSG_T;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    uint16_t version;
    int16_t  rssi;
    uint32_t speaker_channel;
    uint64_t rtp_packets_received;
    uint64_t rtp_packets_dropped;
    uint64_t audio_frames_played;
    uint64_t audio_frames_dropped;
} apollo_report_stats_t;

typedef struct
{
    uint32_t                magic;          /* Magic tag                        */
    uint16_t                version;        /* Apollo report header version     */
    uint8_t                 mac_addr[6];    /* MAC address of message sender    */
    uint32_t                msg_type;       /* APOLLO_REPORT_MSG_T message type */
    uint32_t                msg_length;     /* Length of message data           */
    uint8_t                 msg_data[0];    /* Message data                     */
} apollo_report_msg_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/


#ifdef __cplusplus
} /* extern "C" */
#endif
