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

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    APOLLO_DO_NOT_CACHE    = 0,       /* Do not cache or store        */
    APOLLO_CACHE_TO_MEMORY = 1 << 0,  /* Cache to memory              */
    APOLLO_STORE_TO_NVRAM  = 1 << 1   /* Store to non-volatile memory */
} APOLLO_CACHING_MODE_T;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/**
 * Audio volume structure passed with
 * APOLLO_CMD_SENDER_COMMAND_VOLUME and APOLLO_CMD_EVENT_SET_VOLUME
 */
typedef struct apollo_volume_s
{
  int volume;
  APOLLO_CACHING_MODE_T caching_mode;
} apollo_volume_t;

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
