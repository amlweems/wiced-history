/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

MEMORY
{
    /* internal flash addresses */
    BTLDR_VECTORS    (rx)  : ORIGIN = 0x08000000, LENGTH = 512
    SHARED_API       (rx)  : ORIGIN = 0x08000200, LENGTH = 512
    BTLDR_CODE       (rx)  : ORIGIN = 0x08000400, LENGTH = 0x09000     /* 36K */
    APP_CODE         (rx)  : ORIGIN = 0x0800C000, LENGTH = 0x80000  /* 512K */

    /* external flash offsets (not addresses ) - same as ota2_dct.ld */
    APPS_LUT                   (rw)  : ORIGIN = 0x00000000, LENGTH = 4K
    DCT1_FLASH                 (rw)  : ORIGIN = 0x00004000, LENGTH = 16K
    DCT2_FLASH                 (rw)  : ORIGIN = 0x00008000, LENGTH = 16K
    SAVE_FLASH                 (rw)  : ORIGIN = 0x0000c000, LENGTH = 16K

    /* sram addresses */
    SRAM             (rwx) : ORIGIN = 0x20000000, LENGTH = 96K
    BTLDR_SRAM       (rwx) : ORIGIN = 0x20010000, LENGTH = 32k      /* Boot loader stack at the end. */
}

