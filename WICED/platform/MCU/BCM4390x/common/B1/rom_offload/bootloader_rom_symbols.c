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
 * This file supplements the GCC_rom_bootloader_symbols.ld
 * all the functions in this file should have the same names as in GCC_rom_bootloader_symbols.ld*
 * * (Except for built-in functions, which are not included in this file because
 *    - they will be always present in the SDK , so linker knows that the symbol is a function
 *    - and adding them to this file results in compilation error: conflicting type of built-in function)
 *
 * GCC_rom_bootloader_symbols.ld defines a list of symbols which should be linked from ROM,
 * this file tells the linker that the symbols defined in GCC_rom_bootloader_symbols.ld are functions.
 * To ensure the arm thumb interworking logic is correct in the generated output file.
 */

#include "platform_toolchain.h"

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
 *               Function Declarations
 ******************************************************/

void platform_nonexistant_function (void );

/* Define the functions to be linked from ROM to have weak attribute
 * To ensure that linker treats the symbols as functions.
 */

WEAK void  hwcrypto_compute_sha256hmac_inner_outer_hashcontext( void );
WEAK void  hwcrypto_core_enable ( void );
WEAK void  hwcrypto_dcache_clean_dma_input( void );
WEAK void  hwcrypto_split_dma_data ( void );
WEAK void  hwcrypto_unprotected_blocking_dma_transfer ( void );
WEAK void  platform_hwcrypto_aes128cbc_decrypt ( void );
WEAK void  platform_hwcrypto_aescbc_decrypt_sha256_hmac ( void );
WEAK void  platform_hwcrypto_execute ( void );
WEAK void  platform_hwcrypto_init ( void );
WEAK void  platform_hwcrypto_sha256_hash ( void );
WEAK void  platform_hwcrypto_sha256_hmac ( void );
WEAK void  platform_hwcrypto_sha256_hmac_final ( void );
WEAK void  platform_hwcrypto_sha256_hmac_init ( void );
WEAK void  platform_hwcrypto_sha256_hmac_update ( void );
WEAK void  platform_hwcrypto_sha256_incremental ( void );

WEAK void  memcpy ( void );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

/* All the functions below call platform_nonexistant_function(), which is not defined intentionally
 * To ensure that this definition of the function is not linked ever.
 */
void  hwcrypto_compute_sha256hmac_inner_outer_hashcontext( void ) { platform_nonexistant_function(); }
void  hwcrypto_core_enable ( void ) { platform_nonexistant_function(); }
void  hwcrypto_dcache_clean_dma_input( void ) { platform_nonexistant_function(); }
void  hwcrypto_split_dma_data ( void ) { platform_nonexistant_function(); }
void  hwcrypto_unprotected_blocking_dma_transfer ( void ) { platform_nonexistant_function(); }
void  platform_hwcrypto_aes128cbc_decrypt ( void ) { platform_nonexistant_function(); }
void  platform_hwcrypto_aescbc_decrypt_sha256_hmac ( void ) { platform_nonexistant_function(); }
void  platform_hwcrypto_execute ( void ) { platform_nonexistant_function(); }
void  platform_hwcrypto_init ( void ) { platform_nonexistant_function(); }
void  platform_hwcrypto_sha256_hash ( void ) { platform_nonexistant_function(); }
void  platform_hwcrypto_sha256_hmac ( void ) { platform_nonexistant_function(); }
void  platform_hwcrypto_sha256_hmac_final ( void ) { platform_nonexistant_function(); }
void  platform_hwcrypto_sha256_hmac_init ( void ) { platform_nonexistant_function(); }
void  platform_hwcrypto_sha256_hmac_update ( void ) { platform_nonexistant_function(); }
void  platform_hwcrypto_sha256_incremental ( void ) { platform_nonexistant_function(); }

void  memcpy ( void ) { platform_nonexistant_function(); }
