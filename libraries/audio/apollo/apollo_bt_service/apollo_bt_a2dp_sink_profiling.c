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
 * Bluetooth Audio AVDT Sink Service profiling
 */

#include "wiced_platform.h"
#include "apollo_bt_a2dp_sink_profiling.h"

/******************************************************
 *                      Macros
 ******************************************************/

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

#ifdef APOLLO_BT_PROFILE
#if defined(WICED_TRACEX_BUFFER_ADDRESS) && defined(WICED_TRACEX_BUFFER_SIZE)
a2dp_chunk_t *a2dp_dump_array       = (a2dp_chunk_t *)((uint8_t *)(WICED_TRACEX_BUFFER_ADDRESS + WICED_TRACEX_BUFFER_SIZE));
#else
a2dp_chunk_t *a2dp_dump_array       = (a2dp_chunk_t *)((uint8_t *)PLATFORM_DDR_BASE(0));
#endif
uint32_t      a2dp_dump_array_index = 0;
#endif /* APOLLO_BT_PROFILE */

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/
