/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Define cryptographic functions
 */

#pragma once

#include <stdint.h>
#include "wiced_result.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gets a 16 bit random numbers.
 *
 * Allows user applications to retrieve 16 bit random numbers.
 *
 * @param buffer : pointer to the buffer which will receive the
 *                 generated random data
 * @param buffer_length : size of the buffer
 *
 * @return WICED_SUCCESS or Error code
 */
extern wiced_result_t wiced_crypto_get_random( void* buffer, uint16_t buffer_length );

/**
 * Feed entropy into random number generator.
 *
 * @param buffer : pointer to the buffer which contains random data
 * @param buffer_length : size of the buffer
 *
 * @return WICED_SUCCESS or Error code
 */
extern wiced_result_t wiced_crypto_add_entropy( const void* buffer, uint16_t buffer_length );

#ifdef __cplusplus
} /*extern "C" */
#endif
