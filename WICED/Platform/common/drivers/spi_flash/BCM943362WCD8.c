/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include "sam4s_platform.h"
#include "spi_flash_platform_interface.h"

static const sam4s_spi_config_t spi_flash_config =
{
    .speed_hz = 100000,
    .polarity = 1,
    .phase    = 0
};

int sflash_platform_init( int peripheral_id, void** platform_peripheral_out )
{
    sam4s_spi_init( &platform_spi[SPI_FLASH], &spi_flash_config );
    *platform_peripheral_out = (void*)platform_spi[SPI_FLASH].peripheral;
    return 0;
}

int sflash_platform_send_recv_byte( void* platform_peripheral, unsigned char MOSI_val, void* MISO_addr )
{
    sam4s_spi_transfer( &platform_spi[SPI_FLASH], (const uint8_t*)&MOSI_val, (uint8_t*)MISO_addr, 1 );
    return 0;
}

int sflash_platform_chip_select( void* platform_peripheral )
{
    sam4s_spi_assert_chip_select( &platform_spi[SPI_FLASH] );
    return 0;
}

int sflash_platform_chip_deselect( void* platform_peripheral )
{
    sam4s_spi_deassert_chip_select( &platform_spi[SPI_FLASH] );
    return 0;
}

