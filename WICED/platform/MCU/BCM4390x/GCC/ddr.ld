/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

.ddr_text :
{
    . = ALIGN(32);
    *(.ddr_text*)
} > DDR

.ddr_data :
{
    . = ALIGN(32);
    *(.ddr_data*)
} > DDR

.ddr_bss (NOLOAD) : /* Zero initialised memory used for zero initialised variables */
{
    . = ALIGN(4);

    link_ddr_bss_location = .;
    *(.ddr_bss*)
    link_ddr_bss_end = ALIGN(4);
} > DDR

.ddr_heap (NOLOAD) :
{
    . = ALIGN(4);

    link_ddr_heap_location = .;
    KEEP(*(.ddr_heap*))
    link_ddr_heap_end = ALIGN(4);

    link_ddr_free_location = ALIGN(4);
} > DDR
