/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef INCLUDED_PLATFORM_TOOLCHAIN_H
#define INCLUDED_PLATFORM_TOOLCHAIN_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/
#ifndef WEAK
#define WEAK
#endif /* WEAK */

#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE
#endif

#define PLATFORM_L1_CACHE_BYTES (0)
#define PLATFORM_L1_CACHE_PTR_ROUND_UP(a)   (a)

/******************************************************
 *                    Constants
 ******************************************************/

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
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* #ifndef INCLUDED_PLATFORM_TOOLCHAIN_H */
