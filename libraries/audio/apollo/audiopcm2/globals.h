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
 * @file globals.h
 *
 */

#ifndef _H_AUDIOPCM_GLOBALS_H_
#define _H_AUDIOPCM_GLOBALS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* general no error value for return function */
#define NO_ERR ( (int8_t) ( 0 ) )
#define TRUE ( (int8_t) ( 1 ) )
#define FALSE ( (int8_t) ( 0 ) )


/* audiopcm control register status flags */
#define APCM_CTRL_ONOFF_STATUS            ( 1 << 0 ) /**< Audiopcm CTRL register bit 0 mask */
#define APCM_CTRL_INPUT_STATUS            ( 1 << 1 ) /**< Audiopcm CTRL register bit 1 mask */
#define APCM_CTRL_RTP_STATUS              ( 1 << 2 ) /**< Audiopcm CTRL register bit 2 mask */
#define APCM_CTRL_DSP_STATUS              ( 1 << 3 ) /**< Audiopcm CTRL register bit 3 mask */
#define APCM_CTRL_OUTPUT_STATUS           ( 1 << 4 ) /**< Audiopcm CTRL register bit 4 mask */
#define APCM_CTRL_CLOCK_STATUS            ( 1 << 5 ) /**< Audiopcm CTRL register bit 5 mask */
#define APCM_CTRL_WATCHDOG_STATUS         ( 1 << 6 ) /**< Audiopcm CTRL register bit 6 mask */
#define APCM_CTRL_AUTORESET_REQUEST       ( 1 << 7 ) /**< Audiopcm CTRL register bit 6 mask */

/* inner components watermarking
 * definitions are GLOBALS to force uniqueness of the WMARKs
 */
#define APCM_WMARK       ( (uint32_t) ( 0xefb0bb1e ) )

/* defines for MEDIA_CLOCK_UNIT (MCU) support */
#define APCM_MEDIA_CLOCK_UNIT_NS              ( 0 )
#define APCM_MEDIA_CLOCK_UNIT_PPB             ( 1 )
#define APCM_MEDIA_CLOCK_UNIT_TK              ( 2 )
#define APCM_MEDIA_CLOCK_UNIT_USR             ( 3 )

#define THREAD_DEFAULT_PRIORITY ( 4 )

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* _H_AUDIOPCM_GLOBALS_H_ */
