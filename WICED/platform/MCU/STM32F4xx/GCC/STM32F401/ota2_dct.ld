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
    /* external flash offsets - same as ota2_memory_with_bootloader.ld */
    APPS_LUT                   (rw)  : ORIGIN = 0x00000000, LENGTH = 4K
    DCT1_FLASH                 (rw)  : ORIGIN = 0x00004000, LENGTH = 16K
    DCT2_FLASH                 (rw)  : ORIGIN = 0x00008000, LENGTH = 16K
    SAVE_FLASH                 (rw)  : ORIGIN = 0x0000c000, LENGTH = 16K
}

PHDRS
{
    rodata PT_LOAD ;
}

SECTIONS
{
    .rodata :
    {
        dct_location = .;
        KEEP(*(.rodata.initial_dct))
        KEEP(*(.rodata.wiced_apps_lut))
        KEEP(*(.rodata.wiced_apps_lut_fill))
        dct_end = .;

    } >DCT1_FLASH  AT> DCT1_FLASH :rodata
}

PROVIDE( dct_full_size_loc       = LENGTH( DCT1_FLASH ));
PROVIDE( dct_used_size_loc       = (dct_end - ORIGIN( DCT1_FLASH )));
PROVIDE( dct1_start_addr_loc     = ORIGIN( DCT1_FLASH ));
PROVIDE( dct1_size_loc           = LENGTH( DCT1_FLASH ));
PROVIDE( dct2_start_addr_loc     = ORIGIN( DCT2_FLASH ));
PROVIDE( dct2_size_loc           = LENGTH( DCT2_FLASH ));
PROVIDE( dct_save_start_addr_loc = ORIGIN( SAVE_FLASH ));
PROVIDE( dct_save_size_loc       = LENGTH( SAVE_FLASH ));

