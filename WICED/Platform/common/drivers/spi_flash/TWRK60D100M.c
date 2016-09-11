/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include "spi_flash_platform_interface.h"
#include "MK60N512VMD100.h"

int sflash_platform_init( int peripheral_id, void** platform_peripheral_out )
{
    return 0;
}

int sflash_platform_send_recv_byte( void* platform_peripheral, unsigned char MOSI_val, void* MISO_addr )
{
    return 0;
}

int sflash_platform_chip_select( void* platform_peripheral )
{
    return 0;
}

int sflash_platform_chip_deselect( void* platform_peripheral )
{
    return 0;
}

