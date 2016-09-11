/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 */
#pragma once

#include "wiced_result.h"
#include <stdint.h>
#include "nfa_api.h"
#include "nfa_ce_api.h"
#include "nfa_rw_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#if 0
#define WICED_NFC_TRACE_ENABLED
#define WICED_NFC_ASSERT_ENABLED
#endif

#ifdef WICED_NFC_TRACE_ENABLED
#define WICED_NFC_TRACE_INFO( s, ... ) WPRINT_APP_INFO( ( s, ##__VA_ARGS__ ))
#else
#define WICED_NFC_TRACE_INFO( s, ... )
#endif

#ifdef WICED_NFC_ASSERT_ENABLED
#define WICED_NFC_ASSERT( a, b )  wiced_assert( a, b )
#else
#define WICED_NFC_ASSERT( a, b )
#endif

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/* NFC tag message types */
typedef enum
{
    WICED_NFC_TAG_TYPE_TEXT,
    WICED_NFC_TAG_TYPE_WSC
} wiced_nfc_tag_type_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_nfc_tag_type_t type;
    uint8_t*             buffer;
    uint32_t             buffer_length;
} wiced_nfc_tag_msg_t;

/******************************************************
 *               Function Declarations
 ******************************************************/

tHAL_NFC_ENTRY* nfc_fwk_get_hal_functions( void );
int             nfc_fwk_boot_entry       ( void );
void            nfc_utils_trace_array    ( uint8_t *p, char *p_title, uint32_t len );
wiced_result_t  nfc_fwk_ndef_build       ( wiced_nfc_tag_msg_t *p_ndef_msg, uint8_t **pp_ndef_buf, uint32_t *p_ndef_size );
void*           nfc_fwk_mem_co_alloc     ( UINT32 num_bytes);
void            nfc_fwk_mem_co_free      ( void *p_buf);
void            GKI_shutdown             ( void );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
