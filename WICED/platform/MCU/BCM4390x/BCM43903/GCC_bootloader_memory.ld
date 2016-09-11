/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

MEMORY
{
    ROM              (rx)   : ORIGIN = 0x00400000, LENGTH = 640K
    FLOPS            (rwx)  : ORIGIN = 0x00000000, LENGTH = 32
    BOOTLOADER_SRAM  (rwx)  : ORIGIN = 0x00598000, LENGTH = 32K  /* Uses the top 32K of SRAM */
}
