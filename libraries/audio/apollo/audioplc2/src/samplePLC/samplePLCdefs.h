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
 * samplePLCdefs.h
 *
 *
 * Robert Zopf
 * Broadcom Corp.
 * Office of the CTO
 * 08/13/2015
 *****************************************************************************/

#ifndef _SAMPLEPLCDEFS
#define _SAMPLEPLCDEFS


#ifdef __cplusplus
extern "C" {
#endif

#define ENABLE_FLOATING 0           // for fixed point debugging

#define ONE_OVER_WS     ( 100 )     // one over Window Size in seconds
#define SF_MAX          ( 96000 )   // Max sampling frequency
#define WIN_MAX         ( SF_MAX / ONE_OVER_WS )
#define SL_MAX          ( 18 )
#define LPCORD_MAX      ( 10 )        // SL_MAX
#define LPCORD          ( 10 )

#define ANAWINL         ( 0.01 )
#define MAXNEPW         ( 4 )         // Number of errors per window

// #define MAX_BUF_LEN     3000

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // _SAMPLEPLCDEFS
