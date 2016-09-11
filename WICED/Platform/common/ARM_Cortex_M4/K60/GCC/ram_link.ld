/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

reset_handler = _start;

/* Mapping of platform interrupts over the top of generic versions */ 

SVC_irq              = 1 + ( DEFINED( vPortSVCHandler       ) ? vPortSVCHandler : 
                             DEFINED( __tx_SVCallHandler    ) ? __tx_SVCallHandler : 
                                                              UnhandledInterrupt );

PENDSV_irq           = 1 + ( DEFINED( xPortPendSVHandler    ) ? xPortPendSVHandler : 
                             DEFINED( __tx_PendSVHandler    ) ? __tx_PendSVHandler : 
                                                                UnhandledInterrupt );

SYSTICK_irq          = 1 + ( DEFINED( xPortSysTickHandler   ) ? xPortSysTickHandler :
                             DEFINED( __tx_SysTickHandler   ) ? __tx_SysTickHandler :
                             DEFINED( NoOS_systick_irq      ) ? NoOS_systick_irq    :
                                                                UnhandledInterrupt );

PortA_irq            = 1 + ( DEFINED( gpio_rtos_irq         ) ? gpio_rtos_irq :
                             DEFINED( gpio_irq              ) ? gpio_irq      :
                                                                UnhandledInterrupt );

PortB_irq            = 1 + ( DEFINED( gpio_rtos_irq         ) ? gpio_rtos_irq :
                             DEFINED( gpio_irq              ) ? gpio_irq      :
                                                                UnhandledInterrupt );  
 
PortC_irq            = 1 + ( DEFINED( gpio_rtos_irq         ) ? gpio_rtos_irq :
                             DEFINED( gpio_irq              ) ? gpio_irq      :
                                                                UnhandledInterrupt );  

PortD_irq            = 1 + ( DEFINED( gpio_rtos_irq         ) ? gpio_rtos_irq :
                             DEFINED( gpio_irq              ) ? gpio_irq      :
                                                                UnhandledInterrupt );  
                                                                
PortE_irq            = 1 + ( DEFINED( gpio_rtos_irq         ) ? gpio_rtos_irq :
                             DEFINED( gpio_irq              ) ? gpio_irq      :
                                                                UnhandledInterrupt );                                                                                                                                                                                                                                                                          
ENTRY( _start );

MEMORY
{
	/* OTA app resides in RAM only */
    SRAM (rwx)   : ORIGIN = 0x1FFF0000, LENGTH = 128K
    OTA_HDR (rx) : ORIGIN = 0x1FFEFE00, LENGTH = 512
}

SECTIONS
{

    .text1 :
    {
        bootloader_app_header_location = .;
        KEEP(*(*.bootloader_app_header));
    }>OTA_HDR  AT> OTA_HDR


	/* Data transfer area for serial flash writing app  - at start of memory */
    .sflash_trnsf :
    {
        KEEP(*(*.data_config))
        KEEP(*(*.data_transfer))
    }>SRAM AT> SRAM


    .vectors :
    {
        link_interrupt_vectors_location = .;
        KEEP(*(.vectors))
    }>SRAM AT> SRAM

    .text :
    {
        . = ALIGN(4);
        
        link_code_location = .;

        *(.text .text.* .gnu.linkonce.t.*)

        KEEP(*(.text.vPortSVCHandler    .text.xPortPendSVHandler .text.xPortSysTickHandler ))
        KEEP(*(.text.__tx_SVCallHandler .text.__tx_PendSVHandler .text.__tx_SysTickHandler ))
        KEEP(*(.text.NoOS_systick_irq ))
	KEEP(*(.text.gpio_rtos_irq .text.gpio_irq ))
        link_code_end = .;
        
        . = ALIGN(0x4);
        
        link_const_variable_data_location = .;
        wifi_firmware_image_location = .;
        *(.rodata.wifi_firmware_image)
        wifi_firmware_image_end = .;
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        link_const_variable_data_end = .;

        . = ALIGN(0x4);

        link_constructors_location = .;
        KEEP(*(.preinit_array))
        KEEP(*(.init_array))
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
        link_constructors_end = .;

        . = ALIGN(0x4);

        link_destructors_location = .;
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*crtend.o(.dtors))
        link_destructors_end = .;

        . = ALIGN(16);

    }>SRAM AT> SRAM

    /*
     * The .ARM.exidx and .ARM.exidx sections are used for C++ exception handling.
     * It is located here for completeness. Bare-metal ARM projects 
     * typically cannot afford the overhead associated with C++ 
     * exceptions handling.
     */
    .ARM.exidx :
    {
        __exidx_start = ALIGN(4);
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } >SRAM AT> SRAM
    
    .ARM.extab :
    {
    	__extab_start = ALIGN(4);
        *(.ARM.extab*)
        __extab_end = .;
    } > SRAM  AT> SRAM

    .fast : /* This section contains code that is run from RAM after being loaded from flash - functions can be put in this section with the C attribute: __attribute__ ((section (".fast"))) */
    {
        link_run_from_ram_code_flash_location = LOADADDR( .fast ); /* This is the location in flash of the code */
        link_run_from_ram_code_ram_location = .;
        *(.fast .fast.* .text.fastcode)
        link_run_from_ram_code_ram_end = .;
    }>SRAM AT> SRAM

    .data : /* Contains the non-zero initialised global variables */
    {
        link_global_data_initial_values = LOADADDR( .data ); /* This is the location in flash of the initial values of global variables */
        link_global_data_start = .;
        *(.data*)
        link_global_data_end = .;
        . = ALIGN(., 4);
    }>SRAM AT> SRAM


    .bss : /* Zero initialised memory used for zero initialised variables */
    {
        link_bss_location = ALIGN(., 4);
        *(.bss*)
        *(COMMON)
        link_bss_end = .;
        . = ALIGN(., 4);
    }> SRAM AT>SRAM


    .stack : /* Contains the initial stack */
    {
        link_stack_location = ALIGN(., 4);
        *(.stack)
        . = ALIGN(MAX(link_stack_location + __STACKSIZE__ , .), 4);
        link_stack_end = .;
    }> SRAM AT>SRAM

    /DISCARD/ :
    {
        *(.ARM.attributes*)
        *(.comment)
        *(.init)
        *(.preinit)
        *(.fini)
        *(.fini_array)
        *(.ARM.exidx*)
        *(.gnu.linkonce.armexidx.*)
        *(.eh_frame_hdr)
        *(.eh_frame)
        *(.gnu.linkonce.armextab.*)
        *(.v4_bx)
        *(.vfp11_veneer)
        *(.gcc_except_table)
        *(.eh_frame_hdr)
        *(.eh_frame)
        *(.glue*)
    }

}

/* Declare libc Heap to start at end of allocated RAM */

PROVIDE( _heap = link_stack_end );

/* End of the heap is top of RAM, aligned 8 byte */

PROVIDE( _eheap = ALIGN( ORIGIN( SRAM ) + LENGTH( SRAM ) - 8, 8 ) );
PROVIDE( total_app_image_size = LOADADDR( .data ) - ORIGIN( SRAM ) + link_global_data_end - link_global_data_start );
PROVIDE( wifi_firmware_image_size_from_link = wifi_firmware_image_end - wifi_firmware_image_location );
PROVIDE( vectors_offset = link_interrupt_vectors_location - bootloader_app_header_location );

/* ThreadX aliases */
PROVIDE( __RAM_segment_used_end__ = link_stack_end );
PROVIDE( __tx_free_memory_start = link_stack_end );
PROVIDE( __tx_vectors = link_interrupt_vectors_location );

PROVIDE( wifi_firmware_image = 0 );
PROVIDE( bootloader_api_addr_loc = 0x00000410);



