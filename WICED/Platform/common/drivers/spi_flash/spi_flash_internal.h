/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef INCLUDED_SPI_FLASH_INTERNAL_H
#define INCLUDED_SPI_FLASH_INTERNAL_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "spi_flash.h"
#include <stdint.h>


/* Status Register bit definitions */
#define SFLASH_STATUS_REGISTER_BUSY                          ( 0x01 )
#define SFLASH_STATUS_REGISTER_WRITE_ENABLED                 ( 0x02 )
#define SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_0             ( 0x04 )
#define SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_1             ( 0x08 )
#define SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_2             ( 0x10 ) /* SST Only */
#define SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_3             ( 0x20 ) /* SST Only */
#define SFLASH_STATUS_REGISTER_AUTO_ADDRESS_INCREMENT        ( 0x40 ) /* SST Only */
#define SFLASH_STATUS_REGISTER_BLOCK_PROTECT_BITS_READ_ONLY  ( 0x80 ) /* SST Only */


/* Command definitions */
typedef enum
{

    SFLASH_WRITE_STATUS_REGISTER        = 0x01,  /* WRSR                  */
    SFLASH_WRITE                        = 0x02,
    SFLASH_READ                         = 0x03,
    SFLASH_WRITE_DISABLE                = 0x04, /* WRDI                   */
    SFLASH_READ_STATUS_REGISTER         = 0x05, /* RDSR                   */
    SFLASH_WRITE_ENABLE                 = 0x06, /* WREN                   */
    SFLASH_FAST_READ                    = 0x0B,
    SFLASH_SECTOR_ERASE                 = 0x20, /* SE                     */
    SFLASH_BLOCK_ERASE_MID              = 0x52, /* SE                     */
    SFLASH_BLOCK_ERASE_LARGE            = 0xD8, /* SE                     */
    SFLASH_READ_ID1                     = 0x90, /* data size varies       */
    SFLASH_READ_ID2                     = 0xAB, /* data size varies       */
    SFLASH_READ_JEDEC_ID                = 0x9F, /* RDID                   */
    SFLASH_CHIP_ERASE1                  = 0x60, /* CE                     */
    SFLASH_CHIP_ERASE2                  = 0xC7, /* CE                     */
    SFLASH_ENABLE_WRITE_STATUS_REGISTER = 0x50, /* EWSR   - SST only      */
    SFLASH_READ_SECURITY_REGISTER       = 0x2B, /* RDSCUR - Macronix only */
    SFLASH_WRITE_SECURITY_REGISTER      = 0x2F, /* WRSCUR - Macronix only */
    SFLASH_ENTER_SECURED_OTP            = 0xB1, /* ENSO   - Macronix only */
    SFLASH_EXIT_SECURED_OTP             = 0xC1, /* EXSO   - Macronix only */
    SFLASH_DEEP_POWER_DOWN              = 0xB9, /* DP     - Macronix only */
    SFLASH_RELEASE_DEEP_POWER_DOWN      = 0xAB, /* RDP    - Macronix only */


} sflash_command_t;


#define SFLASH_DUMMY_BYTE ( 0xA5 )



#define SFLASH_MANUFACTURER( id ) ( ( (id) & 0x00ff0000 ) >> 16 )

#define SFLASH_MANUFACTURER_SST        ( 0xBF )
#define SFLASH_MANUFACTURER_MACRONIX   ( 0xC2 )

#define SFLASH_ID_MX25L8006E           ( 0xC22014 )
#define SFLASH_ID_SST25VF080B          ( 0xBF258E )

int sflash_read_ID              ( const sflash_handle_t* handle, void* data_addr );
int sflash_write_enable         ( const sflash_handle_t* handle );
int sflash_read_status_register ( const sflash_handle_t* handle, void* dest_addr );
int sflash_write_status_register( const sflash_handle_t* handle, char value );
int generic_sflash_command      ( const sflash_handle_t* handle, sflash_command_t cmd, unsigned int num_initial_parameter_bytes, const void* parameter_bytes, int num_data_bytes, const void* const data_MOSI, void* const data_MISO );






#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_SPI_FLASH_INTERNAL_H */
