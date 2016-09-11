/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/*****************************************************************************
 * samplePLC.h
 *
 *
 * Robert Zopf
 * Broadcom Corp.
 * Office of the CTO
 * 08/13/2015
 *****************************************************************************/

#ifndef _SAMPLEPLC_H
#define _SAMPLEPLC_H


#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SWLSAR, LINEAR
} SAMPLE_CONCEALMENT_METHOD;


typedef struct
{
#if ENABLE_FLOATING
    double*                   aswin; // assymetric window for LPC analysis
    double                    Emax;
    double                    a[ LPCORD_MAX + 1 ];
#endif
    int32_t                   SF;  // sampling frequency in Hz
    int16_t                   WL;  // window length
    int16_t                   SL;  // shuffling length

    // int16_t    hbuf[(int32_t)(SF_MAX*WS)];     // history buffer
    int16_t*                  hbuf; // this will now point into the xq buffer in audioPLC
    int16_t                   hq;   // number of samples to the point of the beginning of the current frame to fix
    int16_t*                  hbufplc;
    int16_t                   a_16[ LPCORD_MAX + 1 ];
    int16_t                   Q_a_16;
    int16_t                   lastsamplefixed;
    SAMPLE_CONCEALMENT_METHOD Method;

    int32_t                   Emax_32;
    int16_t*                  aswin_16; // assymetric window for LPC analysis
} T_SAMPLEPLC_STRUCT;

typedef struct
{
    T_SAMPLEPLC_STRUCT* samplePLC_state[ MAX_CHAN ];
} T_SAMPLE_CONCEALMENT_STRUCT;

int initSamplePLC( T_SAMPLEPLC_STRUCT* mem, int32_t sf, int16_t SL );
int allocSamplePLC( T_SAMPLEPLC_STRUCT** mem );
int freeSamplePLC( T_SAMPLEPLC_STRUCT* mem );

void UpdateSamplePLC( T_SAMPLEPLC_STRUCT* mem, int16_t* inbuf, int16_t L );
int  samplePLC( T_SAMPLEPLC_STRUCT* mem, int16_t* inbuf, int16_t L, int16_t SL, int16_t* SampleStatus, int16_t flag );
int  samplePLC32( T_SAMPLEPLC_STRUCT* mem, int32_t* inbuf, int16_t L, int16_t SL, int16_t* SampleStatus, int16_t flag );
int  initSampleConcealment( T_SAMPLE_CONCEALMENT_STRUCT* mem, int32_t sf, int16_t SL, int nchan );
int  allocSampleConcealment( T_SAMPLE_CONCEALMENT_STRUCT** mem, int nchan );
int  freeSampleConcealment( T_SAMPLE_CONCEALMENT_STRUCT* mem, int nchan );


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* _SAMPLEPLC_H */
