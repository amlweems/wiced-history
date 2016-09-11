/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "fx_api.h"
#include "network/wwd_network_constants.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define FILEX_MEDIA_BUFFER_SIZE              (1024)
/* Padded size includes spacing to ensure that data access to buffer can always be within cache line boundaries */
#define FILEX_MEDIA_BUFFER_SIZE_PADDED       (FILEX_MEDIA_BUFFER_SIZE + PLATFORM_L1_CACHE_BYTES)


/* Only enable ONE of the following options */

/* Enable to make File-X read-only */
//#define FILEX_WRITE_STRATEGY  BLOCK_DEVICE_READ_ONLY

#define FILEX_WRITE_STRATEGY  BLOCK_DEVICE_WRITE_IMMEDIATELY

/* Enable to allow write-behind in File-X */
//#define FILEX_WRITE_STRATEGY  BLOCK_DEVICE_WRITE_BEHIND_ALLOWED

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

/******************************************************
 *               Function Definitions
 ******************************************************/

VOID  wiced_filex_driver( FX_MEDIA* media_ptr );

#ifdef __cplusplus
} /*extern "C" */
#endif
