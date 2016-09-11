/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/*
 * This software has been developed by Broadcom Corporation.
 *
 * Copyright (c)  Broadcom Corporation 2006.  All rights reserved.
 *
 * File: typedef.h
 * Version: 0.1
 * Date: Dec 21, 2006
 * Author: Jes Thyssen
 */

#ifndef    __TYPEDEF__
#define    __TYPEDEF__

#ifdef __cplusplus
extern "C" {
#endif

#define USE_NEW_OPS 1

#include <stdint.h>
#ifndef WIN32
typedef int64_t __int64;
#endif
typedef double  Float;
typedef int16_t Word16;
typedef int32_t Word32;
typedef int     Flag;

#define __int64 int64_t

#ifdef __cplusplus
}
#endif
#endif
