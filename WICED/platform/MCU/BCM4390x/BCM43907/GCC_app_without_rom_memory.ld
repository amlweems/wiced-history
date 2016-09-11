/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

BOOTLOADER_SRAM = 0x696C00;

MEMORY
{
    ROM              (rx)   : ORIGIN = 0x00400000, LENGTH = 640K
    FLOPS            (rwx)  : ORIGIN = 0x00000000, LENGTH = 32
    SRAM             (rwx)  : ORIGIN = 0x004A0000, LENGTH = 2048K
    AONRAM           (rwx)  : ORIGIN = 0x006A0000, LENGTH = 8K
    DDR              (rwx)  : ORIGIN = 0x40000000, LENGTH = 0
}
