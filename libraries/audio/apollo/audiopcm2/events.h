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
 * @file audiopcm_events.h
 *
 *
 */
#ifndef _H_AUDIOPCM_EVENTS_H_
#define _H_AUDIOPCM_EVENTS_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "audiopcm_types.h"


/** documenting events following "5 Ws" rules
 * http://en.wikipedia.org/wiki/Five_Ws
 *
 * for now only 3 Ws (who, what, where)
 */

/**
 * audiopcm event: component ("who")
 */
typedef enum
{
    AUDIOPCM_E_CMP_UNKNOWN = 0,
    AUDIOPCM_E_CMP_ANYTYPE,
    AUDIOPCM_E_CMP_RESERVED,
    AUDIOPCM_E_CMP_AUDIO_SYNTAX,
    AUDIOPCM_E_CMP_INPUT_THREAD,
    AUDIOPCM_E_CMP_OUTPUT_THREAD,
    AUDIOPCM_E_CMP_RTP_THREAD,
    AUDIOPCM_E_CMP_DSP_THREAD,
    AUDIOPCM_E_CMP_CLK_THREAD,
    AUDIOPCM_E_CMP_MAX,
} event_component_t;

/**
 * audiopcm event: object ("where")
 */
typedef enum
{
    AUDIOPCM_E_OBJ_UNKNOWN = 0,
    AUDIOPCM_E_OBJ_ANYTYPE,
    AUDIOPCM_E_OBJ_RESERVED,
    AUDIOPCM_E_OBJ_AUDIO_STREAM,
    AUDIOPCM_E_OBJ_AUDIO_STREAM_SAMPLERATE,
    AUDIOPCM_E_OBJ_AUDIO_STREAM_BITPERSAMPLE,
    AUDIOPCM_E_OBJ_AUDIO_STREAM_BURSTLENGHT,
    AUDIOPCM_E_OBJ_AUDIO_RTP_BUF,
    AUDIOPCM_E_OBJ_AUDIO_RTP_QUEUE,
    AUDIOPCM_E_OBJ_AUDIO_DSP_BUF,
    AUDIOPCM_E_OBJ_AUDIO_DSP_QUEUE,
    AUDIOPCM_E_OBJ_AUDIO_SAMPLE_QUEUE,
    AUDIOPCM_E_OBJ_AUDIO_STEREO_BUF,
    AUDIOPCM_E_OBJ_AUDIO_RENDER_CBAK,
    AUDIOPCM_E_OBJ_AUDIO_OUT_BUF,
    AUDIOPCM_E_OBJ_AUDIO_OUT_QUEUE,
    AUDIOPCM_E_OBJ_MAX,
} event_object_t;

/**
 * audiopcm event: info ("what")
 */
typedef enum
{
    AUDIOPCM_E_INFO_UNKNOWN = 0,
    AUDIOPCM_E_INFO_ANYTYPE,
    AUDIOPCM_E_INFO_RESERVED,
    AUDIOPCM_E_INFO_ILLEGAL,
    AUDIOPCM_E_INFO_OVERFLOW,
    AUDIOPCM_E_INFO_UNDERFLOW,
    AUDIOPCM_E_INFO_CHANGE,
    AUDIOPCM_E_INFO_INVALID,
    AUDIOPCM_E_INFO_DISCONTINUITY,
    AUDIOPCM_E_INFO_BIG_GAP,
    AUDIOPCM_E_INFO_BL_SYNC_FAIL,
    AUDIOPCM_E_INFO_BL_SYNC_MISMATCH,
    AUDIOPCM_E_INFO_PLC_INIT_FAIL,
    AUDIOPCM_E_INFO_PLC_DECODE_FAIL,
    AUDIOPCM_E_INFO_OUTOFORDER,
    AUDIOPCM_E_INFO_MAX,
} event_info_t;


/**
 * audiopcm event: info ("what")
 */
typedef enum
{
    AUDIOPCM_E_FILE_UNKNOWN = 0,
    AUDIOPCM_E_FILE_ANYTYPE,
    AUDIOPCM_E_FILE_RESERVED,
    AUDIOPCM_E_FILE_AUDIOPCM,
    AUDIOPCM_E_FILE_RTP,
    AUDIOPCM_E_FILE_DSP,
    AUDIOPCM_E_FILE_OUT,
    AUDIOPCM_E_FILE_CLK,
    AUDIOPCM_E_FILE_WDOG,
    AUDIOPCM_E_FILE_CONCEALMENT,
} event_file_t;

/**
 * audiopcm event: who/where/what
 */
typedef struct event_
{
    uint32_t          id;

    /* who, where, why */
    event_component_t comp;
    event_object_t    obj;
    event_info_t      info;

    /* internal debug */
    event_file_t      dbg_file;
    uint32_t          dbg_line;
} event_t;

/*
 * define the string type
 */
typedef char event_str_t[ 256 ];



int8_t get_audiopcm_event_str( event_str_t event_str, event_t* event );



#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* _H_AUDIOPCM_EVENTS_H_ */
