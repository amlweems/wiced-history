/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef INCLUDED_SPI_FLASH_PLATFORM_INTERFACE_H
#define INCLUDED_SPI_FLASH_PLATFORM_INTERFACE_H

#ifdef __cplusplus
 extern "C" {
#endif

extern int sflash_platform_init          ( int peripheral_id, void** platform_peripheral_out );
extern int sflash_platform_send_recv_byte( void* platform_peripheral, unsigned char MOSI_val, void* MISO_addr );
extern int sflash_platform_chip_select   ( void* platform_peripheral );
extern int sflash_platform_chip_deselect ( void* platform_peripheral );


#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_SPI_FLASH_PLATFORM_INTERFACE_H */
