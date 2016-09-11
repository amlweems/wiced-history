/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/**
 * @file Apollo common RTP definitions and parameters
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/


#define RTP_DEFAULT_PORT                5004

#define RTP_PACKET_MAX_SIZE             1500
#define RTP_IP_HEADER_SIZE              28
#define RTP_BASE_HEADER_SIZE            12
#define RTP_EXT_SIZE_1_0                20
#define RTP_EXT_SIZE_1_1                28
#define RTP_HEADER_SIZE_1_0             (RTP_BASE_HEADER_SIZE + RTP_EXT_SIZE_1_0)
#define RTP_HEADER_SIZE_1_1             (RTP_BASE_HEADER_SIZE + RTP_EXT_SIZE_1_1)
#define RTP_HEADER_SIZE                 RTP_HEADER_SIZE_1_1                         /* The "current" header size */
#define RTP_PACKET_MAX_DATA             (RTP_PACKET_MAX_SIZE - RTP_HEADER_SIZE - RTP_IP_HEADER_SIZE)
#define RTP_PACKET_MIN_DATA             64

#define RTP_PAYLOAD_AUDIO               98
#define RTP_PAYLOAD_AUDIO_DUP           99
#define RTP_PAYLOAD_FEC                 100
#define RTP_MAX_SEQ_NUM                 ((uint32_t)0x0000FFFF)

/*
 * RTP Audio extension header definitions.
 */

#define RTP_AUDIO_EXT_ID_LPCM           0x00010000
#define RTP_AUDIO_EXT_AUDIO_LENGTH_1_0  4           /* Length of extension header in 32 bit units */
#define RTP_AUDIO_EXT_AUDIO_LENGTH_1_1  6           /* Length of extension header in 32 bit units */

#define RTP_AUDIO_EXT_ID_FEC_XOR        0x00000000
#define RTP_AUDIO_EXT_FEC_LENGTH        1           /* Length of extension header in 32 bit units */

#define RTP_AUDIO_EXT_VER_MAJOR_SHIFT   28
#define RTP_AUDIO_EXT_VER_MINOR_SHIFT   24
#define RTP_AUDIO_EXT_VERSION_1_0       (0x10000000)
#define RTP_AUDIO_EXT_VERSION_1_1       (0x11000000)

#define RTP_AUDIO_SPATIAL_INTERLEAVING  0x00000000
#define RTP_AUDIO_TIME_INTERLEAVING     0x00200000

#define RTP_AUDIO_FEC_PRIOR             0x00100000
#define RTP_AUDIO_FEC_POST              0x00000000
#define RTP_AUDIO_FEC_MAX_LENGTH        16

#define RTP_AUDIO_SAMPLE_RATE_441       0x00000000
#define RTP_AUDIO_SAMPLE_RATE_48        0x20000000
#define RTP_AUDIO_SAMPLE_RATE_96        0x40000000
#define RTP_AUDIO_SAMPLE_RATE_192       0x60000000
#define RTP_AUDIO_SAMPLE_RATE_MASK      0xE0000000

#define RTP_AUDIO_BPS_16                0x00000000
#define RTP_AUDIO_BPS_24                0x04000000

#define RTP_AUDIO_BL_MASK               0x000F0000

#define RTP_AUDIO_CHANNEL_SHIFT         22

/*
 * Uint32 offsets within the RTP packet.
 */

#define RTP_AUDIO_OFFSET_BASE           0
#define RTP_AUDIO_OFFSET_TIMESTAMP      1
#define RTP_AUDIO_OFFSET_SSRC           2
#define RTP_AUDIO_OFFSET_ID             3
#define RTP_AUDIO_OFFSET_VERSION        4
#define RTP_AUDIO_OFFSET_FEC_REF        4
#define RTP_AUDIO_OFFSET_SCLOCK_LO      5
#define RTP_AUDIO_OFFSET_SCLOCK_HI      6
#define RTP_AUDIO_OFFSET_FORMAT         7
#define RTP_AUDIO_OFFSET_MCLOCK_LO      8
#define RTP_AUDIO_OFFSET_MCLOCK_HI      9


/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
