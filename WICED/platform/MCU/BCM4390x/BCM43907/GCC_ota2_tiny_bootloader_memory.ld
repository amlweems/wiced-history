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
    AONRAM           (rwx)  : ORIGIN = 0x0069F000, LENGTH = 9K /* Use half of 8K AON memory as hard limit for tiny bootloader */
    RAM              (rwx)  : ORIGIN = 0x00690000, LENGTH = 60K /* Uses the top 60K of SRAM */
}

