/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/*************************************************************************
 * AudioStreamingConcealment
 *
 *************************************************************************/

#ifndef _AUDIOSTREAMINGCONCEALMENT_H
#define _AUDIOSTREAMINGCONCEALMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#define _16BIT 0
#define _24BIT 1

typedef struct
{
    int16_t                      ValidMemCheck;
    int16_t                      slcMemCheck;
    int16_t                      plcMemCheck;
    T_SAMPLE_CONCEALMENT_STRUCT* sampleConcealmentmem;
    struct AUDIOPLC_STRUCT*      audioPLCmem;
    int16_t                      SL;
    int16_t                      nChan;
    int16_t                      frsz;
    int16_t                      IOformat;
} T_AUDIOSTREAMINGCONCEALMENT_STRUCT;

int initAudioStreamingConcealment( T_AUDIOSTREAMINGCONCEALMENT_STRUCT* mem, int32_t sf, int16_t SL, int frsz, int nchan, int slc_enable, int16_t IOformat );
int freeAudioStreamingConcealment( T_AUDIOSTREAMINGCONCEALMENT_STRUCT* mem, int nchan );
int allocAudioStreamingConcealment( T_AUDIOSTREAMINGCONCEALMENT_STRUCT** mem, int nchan, int sf, int frsz, int slc_enable );
int AudioStreamingConcealment( T_AUDIOSTREAMINGCONCEALMENT_STRUCT* mem, void* inbuf[], void* outbuf[], char* FrameStatus );

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // _AUDIOSTREAMINGCONCEALMENT_H
