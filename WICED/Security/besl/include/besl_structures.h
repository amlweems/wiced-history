/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

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
    BESL_SUCCESS,
    BESL_ERROR,
    BESL_OUT_OF_MEMORY,
    BESL_IN_PROGRESS,

    BESL_TLS_DECRYPTION_FAIL,
    BESL_TLS_ENCRYPTION_FAIL,
    BESL_TLS_HMAC_CHECK_FAIL,

    BESL_WPS_PBC_OVERLAP,
    BESL_WPS_HMAC_CHECK_FAIL,

    BESL_P2P_TIMEOUT,
    BESL_P2P_FAILED,
} besl_result_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef uint8_t   besl_bool_t;

/******************************************************
 *                    Structures
 ******************************************************/

#pragma pack(1)

typedef struct
{
    uint8_t octet[6];
} besl_mac_t;

typedef struct
{
    uint8_t* data;
    uint32_t length;
    uint32_t packet_mask;
} besl_ie_t;

#pragma pack()

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
