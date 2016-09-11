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
 * Defines macros for defining asserts for ARM-Cortex-M3 CPU
 */
#pragma once


#ifdef WIN32
#include <stdint.h>
#include <windows.h>
#undef interface
#elif defined(linux) || defined(__linux) || defined(__linux__)
#include <signal.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

void platform_error(void );

#ifdef WIN32
#define WICED_ASSERTION_FAIL_ACTION( )   platform_error(); DebugBreak()
#elif defined(linux) || defined(__linux) || defined(__linux__)
#define WICED_ASSERTION_FAIL_ACTION( )   raise(SIGTRAP)
#endif


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
} /*extern "C" */
#endif
