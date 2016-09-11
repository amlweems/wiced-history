/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "tls_suite.h"

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

int ecc_parse_server_key_exchange( ssl_context *ssl, const uint8_t* data, uint32_t data_length, tls_digitally_signed_signature_algorithm_t input_signature_algorithm );

int ecc_tls_create_premaster_secret( void*          key_context,
                                     uint8_t        is_ssl_3,
                                     uint16_t       max_version,
                                     uint8_t*       premaster_secret_out,
                                     uint32_t*      pms_length_out,
                                     int32_t        (*f_rng)(void *),
                                     void*          p_rng,
                                     uint8_t*       encrypted_output,
                                     uint32_t*      encrypted_length_out );

#ifdef __cplusplus
} /* extern "C" */
#endif
