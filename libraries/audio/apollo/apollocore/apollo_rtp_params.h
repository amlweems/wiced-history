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
#define RTP_EXT_SIZE_2_0                32

#define RTP_HEADER_SIZE_2_0             (RTP_BASE_HEADER_SIZE + RTP_EXT_SIZE_2_0)

#define RTP_HEADER_SIZE                 RTP_HEADER_SIZE_2_0                         /* The "current" header size */
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
#define RTP_AUDIO_EXT_AUDIO_LENGTH_2_0  7           /* Length of extension header in 32 bit units */

#define RTP_AUDIO_EXT_ID_FEC_XOR        0x00000000
#define RTP_AUDIO_EXT_FEC_LENGTH        1           /* Length of extension header in 32 bit units */

#define RTP_AUDIO_EXT_VER_MAJOR_SHIFT   28
#define RTP_AUDIO_EXT_VER_MINOR_SHIFT   24

#define RTP_AUDIO_EXT_VERSION_MJR       (3)
#define RTP_AUDIO_EXT_VERSION_MIN       (0)
#define RTP_AUDIO_EXT_VERSION_MJRMIN    ( ((RTP_AUDIO_EXT_VERSION_MJR & 0x0F)<<28) | ((RTP_AUDIO_EXT_VERSION_MIN & 0x0F)<<24) )

#define RTP_AUDIO_BL_MAX_LENGTH         36

/*
 * Audio format definitions.
 */

#define AUDIO_FORMAT_BPS_16                 (0 << 31)
#define AUDIO_FORMAT_BPS_24                 (1 << 31)

#define AUDIO_FORMAT_CONTAINER_NATIVE       (0 << 30)
#define AUDIO_FORMAT_CONTAINER_32BIT        (1 << 30)

#define AUDIO_FORMAT_LITTLE_ENDIAN          (0 << 29)
#define AUDIO_FORMAT_BIG_ENDIAN             (1 << 29)

#define AUDIO_FORMAT_UNSIGNED               (0 << 28)
#define AUDIO_FORMAT_SIGNED                 (1 << 28)

#define AUDIO_FORMAT_SPATIAL_INTERLEAVING   (0 << 27)
#define AUDIO_FORMAT_TIME_INTERLEAVING      (1 << 27)

#define AUDIO_FORMAT_INTEGER                (0 << 26)
#define AUDIO_FORMAT_FLOATING_POINT         (1 << 26)

#define AUDIO_FORMAT_SAMPLE_RATE_441        (0 << 24)
#define AUDIO_FORMAT_SAMPLE_RATE_48         (1 << 24)
#define AUDIO_FORMAT_SAMPLE_RATE_96         (2 << 24)
#define AUDIO_FORMAT_SAMPLE_RATE_192        (3 << 24)
#define AUDIO_FORMAT_SAMPLE_RATE_MASK       (3 << 24)

#define AUDIO_FORMAT_CUSTOM_CHMAP           (1 << 23)

#define AUDIO_FORMAT_CHMAP_MASK             (0x3F)
#define AUDIO_FORMAT_CHMAP_SHIFT            17


#define AUDIO_FORMAT_LATENCY_MASK           (0x3FF)
#define AUDIO_FORMAT_BL_MASK                (0xFF)
#define AUDIO_FORMAT_BLSYNC_SHIFT           (24)
#define AUDIO_FORMAT_BL_SHIFT               (16)
#define AUDIO_FORMAT_SL_MASK                (0xFF)
#define AUDIO_FORMAT_SL_SHIFT               (16)

#define AUDIO_FORMAT_MCLOCK_UNIT_MASK       (0x3)
#define AUDIO_FORMAT_MCLOCK_UNIT_NSECS      (0)
#define AUDIO_FORMAT_MCLOCK_UNIT_PPB        (1)
#define AUDIO_FORMAT_MCLOCK_UNIT_TICKS      (2)
#define AUDIO_FORMAT_MCLOCK_UNIT_USR        (3)

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
#define RTP_AUDIO_OFFSET_BL            10
#define RTP_AUDIO_OFFSET_MCLOCK_UNIT   10


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
