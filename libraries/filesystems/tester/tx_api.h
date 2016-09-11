/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *
 */

#ifndef INCLUDED_TX_API_H_
#define INCLUDED_TX_API_H_

#ifdef __cplusplus
extern "C"
{
#endif


#define VOID                                    void
typedef char                                    CHAR;
typedef unsigned char                           UCHAR;
typedef int                                     INT;
typedef unsigned int                            UINT;
typedef long                                    LONG;
typedef unsigned long                           ULONG;
typedef short                                   SHORT;
typedef unsigned short                          USHORT;

#define TX_INTERRUPT_SAVE_AREA
#define TX_DISABLE
#define TX_RESTORE

#define TX_TRUE                         1
#define TX_FALSE                        0
#define TX_NULL                         (void *) 0


typedef struct
{
        VOID                *tx_thread_filex_ptr;
} TX_THREAD;


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_TX_API_H_ */
