/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
/*
 *
 * */

/* Copyright 2015 Broadcom Corporation.  All Rights Reserved. */
/* Author: Rob Zopf                                           */
/* April 27, 2015                                             */

#ifndef AUDIOPLCERRS_H
#define AUDIOPLCERRS_H

#ifdef __cplusplus
extern "C" {
#endif

#define INIT_AUDIO_PLC_TRUE 12345  /* a random number to check */
#define INIT_LCPLC_TRUE     23456

#define AUDIOPLC_NO_ERROR                          0
#define AUDIOPLC_AUDIOPLCSTRUCT_NOT_INIT           1    /* AUDIOPLC_STRUCT not initialized and audioDecoding() called */
#define AUDIOPLC_SAMPLING_RATE_FS_NOT_SUPPORTED    2    /* Combination of sampling rate and frame size not supported */
#define AUDIOPLC_PLC_STATE_XQ_MALLOC_FAIL          4
#define AUDIOPLC_FREE_LC_PLC_WITHOUT_INIT          8
#define AUDIOPLC_PLC_STATE_XQ_NULL_FREE            16
#define AUDIOPLC_LCPLCSTRUCT_NOT_INIT              32
#define AUDIOPLC_AUDIOPLCSTRUCT_FREE_NO_INIT       64

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif
