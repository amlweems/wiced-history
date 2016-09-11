/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

reset_handler = _start;

/* Mapping of platform interrupts over the top of generic versions */ 

SVC_irq              = 1 + ( DEFINED( vPortSVCHandler        ) ? vPortSVCHandler : 
                             DEFINED( __tx_SVCallHandler     ) ? __tx_SVCallHandler : 
                                                               UnhandledInterrupt );
                                                             
PENDSV_irq           = 1 + ( DEFINED( xPortPendSVHandler     ) ? xPortPendSVHandler : 
                             DEFINED( __tx_PendSVHandler     ) ? __tx_PendSVHandler : 
                                                                 UnhandledInterrupt );
                                                             
SYSTICK_irq          = 1 + ( DEFINED( xPortSysTickHandler    ) ? xPortSysTickHandler :
                             DEFINED( __tx_SysTickHandler    ) ? __tx_SysTickHandler :
                             DEFINED( NoOS_systick_irq       ) ? NoOS_systick_irq    :
                                                                 UnhandledInterrupt );
                                                             
SDIO_irq             = 1 + ( DEFINED( sdio_rtos_irq          ) ? sdio_rtos_irq : 
                             DEFINED( sdio_irq               ) ? sdio_irq : 
                                                                 UnhandledInterrupt );
                                                             
EXTI0_irq            = 1 + ( DEFINED( gpio_rtos_irq          ) ? gpio_rtos_irq : 
                             DEFINED( gpio_irq               ) ? gpio_irq : 
                                                                 UnhandledInterrupt );
                                                             
EXTI1_irq            = 1 + ( DEFINED( gpio_rtos_irq          ) ? gpio_rtos_irq : 
                             DEFINED( gpio_irq               ) ? gpio_irq : 
                                                                 UnhandledInterrupt );
                                                             
EXTI2_irq            = 1 + ( DEFINED( gpio_rtos_irq          ) ? gpio_rtos_irq : 
                             DEFINED( gpio_irq               ) ? gpio_irq : 
                                                                 UnhandledInterrupt );
                                                             
EXTI3_irq            = 1 + ( DEFINED( gpio_rtos_irq          ) ? gpio_rtos_irq : 
                             DEFINED( gpio_irq               ) ? gpio_irq : 
                                                                 UnhandledInterrupt );
                                                             
EXTI4_irq            = 1 + ( DEFINED( gpio_rtos_irq          ) ? gpio_rtos_irq : 
                             DEFINED( gpio_irq               ) ? gpio_irq : 
                                                                 UnhandledInterrupt );
                                                                 
EXTI9_5_irq          = 1 + ( DEFINED( gpio_rtos_irq          ) ? gpio_rtos_irq : 
                             DEFINED( gpio_irq               ) ? gpio_irq : 
                                                                 UnhandledInterrupt );
                                                                 
EXTI15_10_irq        = 1 + ( DEFINED( gpio_rtos_irq          ) ? gpio_rtos_irq : 
                             DEFINED( gpio_irq               ) ? gpio_irq : 
                                                                 UnhandledInterrupt );   

DMA2_Stream3_irq     = 1 + ( DEFINED( dma_rtos_irq           ) ? dma_rtos_irq : 
                             DEFINED( dma_irq                ) ? dma_irq : 
                                                                 UnhandledInterrupt );
                                                                 
DMA1_Stream3_irq     = 1 + ( DEFINED( dma_rtos_irq           ) ? dma_rtos_irq : 
                             DEFINED( dma_irq                ) ? dma_irq : 
                                                                 UnhandledInterrupt );
                                                             
TIM7_irq             = 1 + ( DEFINED( dbg_watchdog_rtos_irq  ) ? dbg_watchdog_rtos_irq : 
                             DEFINED( dbg_watchdog_irq       ) ? dbg_watchdog_irq : 
                                                                 UnhandledInterrupt );
                                                                
USART1_irq           = 1 + ( DEFINED( usart1_rtos_irq        ) ? usart1_rtos_irq : 
                             DEFINED( usart1_irq             ) ? usart1_irq : 
                                                                 UnhandledInterrupt );                                                                                                                             
                                                             
USART2_irq           = 1 + ( DEFINED( usart2_rtos_irq        ) ? usart2_rtos_irq : 
                             DEFINED( usart2_irq             ) ? usart2_irq : 
                                                                 UnhandledInterrupt );
                                                                 
DMA2_Stream7_irq     = 1 + ( DEFINED( usart1_tx_dma_rtos_irq ) ? usart1_tx_dma_rtos_irq : 
                             DEFINED( usart1_tx_dma_irq      ) ? usart1_tx_dma_irq : 
                                                                 UnhandledInterrupt );
                                                                 
DMA1_Stream6_irq     = 1 + ( DEFINED( usart2_tx_dma_rtos_irq ) ? usart2_tx_dma_rtos_irq : 
                             DEFINED( usart2_tx_dma_irq      ) ? usart2_tx_dma_irq : 
                                                                 UnhandledInterrupt );                                                                                                                                                                                   
                                                                                                                                                                                                                                                
DMA2_Stream2_irq     = 1 + ( DEFINED( usart1_rx_dma_rtos_irq ) ? usart1_rx_dma_rtos_irq : 
                             DEFINED( usart1_rx_dma_irq      ) ? usart1_rx_dma_irq : 
                                                                 UnhandledInterrupt );
                                                                 
DMA1_Stream5_irq     = 1 + ( DEFINED( usart2_rx_dma_rtos_irq ) ? usart2_rx_dma_rtos_irq : 
                             DEFINED( usart2_rx_dma_irq      ) ? usart2_rx_dma_irq : 
                                                                 UnhandledInterrupt );                                                                  
ENTRY( _start );

MEMORY
{
    BTLDR_VECTORS    (rx)  : ORIGIN = 0x08000000, LENGTH = 512
    BTLDR_API        (rx)  : ORIGIN = 0x08000200, LENGTH = 512
    BTLDR_CODE       (rx)  : ORIGIN = 0x08000400, LENGTH = 15K
    DCT1_FLASH       (rx)  : ORIGIN = 0x08004000, LENGTH = 16K
    DCT2_FLASH       (rx)  : ORIGIN = 0x08008000, LENGTH = 16K
    APP_HDR          (rx)  : ORIGIN = 0x0800C000, LENGTH = 512
    APP_CODE         (rx)  : ORIGIN = 0x0800C200, LENGTH = 0xF3E00  /* 975.5K */
    SRAM             (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
    .text1 :
    {
        link_interrupt_vectors_location = .;
        KEEP(*(.vectors))
    } >BTLDR_VECTORS  AT> BTLDR_VECTORS
    
    .text2 :
    {
        bootloader_api_location = .;
        KEEP(*(.rodata.bootloader_api))
    } >BTLDR_API  AT> BTLDR_API

    .text3 :
    {
        link_code_location = .;

        *(.text .text.* .gnu.linkonce.t.*)

        KEEP(*(.text.vPortSVCHandler    .text.xPortPendSVHandler .text.xPortSysTickHandler ))
        KEEP(*(.text.__tx_SVCallHandler .text.__tx_PendSVHandler .text.__tx_SysTickHandler ))
        KEEP(*(.text.NoOS_systick_irq ))
        KEEP(*(.text.sdio_rtos_irq .text.sdio_irq ))
        KEEP(*(.text.uart_rtos_irq .text.uart_irq ))
        KEEP(*(.text.dma_rtos_irq  .text.dma_irq  ))
        KEEP(*(.text.gpio_rtos_irq .text.gpio_irq ))
        KEEP(*(.text.dbg_watchdog_rtos_irq  .text.dbg_watchdog_irq  ))
        KEEP(*(.text.usart1_rtos_irq .text.usart1_irq ))
        KEEP(*(.text.usart2_rtos_irq .text.usart2_irq ))
        KEEP(*(.text.usart1_tx_dma_rtos_irq .text.usart1_tx_dma_irq ))
        KEEP(*(.text.usart2_tx_dma_rtos_irq .text.usart2_tx_dma_irq ))
        KEEP(*(.text.usart1_rx_dma_rtos_irq .text.usart1_rx_dma_irq ))
        KEEP(*(.text.usart2_rx_dma_rtos_irq .text.usart2_rx_dma_irq ))
        link_code_end = .;
        
        . = ALIGN(0x4);
        
        link_const_variable_data_location = .;
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

    }>BTLDR_CODE  AT> BTLDR_CODE

	/*
	 * The .ARM.exidx and .ARM.extab sections are used for C++ exception handling.
	 * It is located here for completeness. Bare-metal ARM projects 
	 * typically cannot afford the overhead associated with C++ 
	 * exceptions handling.
	 */
    .ARM.exidx :
    {
    	__exidx_start = ALIGN(4);
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > BTLDR_CODE  AT> BTLDR_CODE
  	
    .ARM.extab :
    {
    	__extab_start = ALIGN(4);
        *(.ARM.extab*)
        __extab_end = .;
    } > BTLDR_CODE  AT> BTLDR_CODE

    .fast : /* This section contains code that is run from RAM after being loaded from flash - functions can be put in this section with the C attribute: __attribute__ ((section (".fast"))) */
    {
        link_run_from_ram_code_flash_location = LOADADDR( .fast ); /* This is the location in flash of the code */
        link_run_from_ram_code_ram_location = .;
        *(.fast .fast.* .text.fastcode)
        link_run_from_ram_code_ram_end = .;
    }> SRAM AT> BTLDR_CODE

    .data : /* Contains the non-zero initialised global variables */
    {
        link_global_data_initial_values = LOADADDR( .data ); /* This is the location in flash of the initial values of global variables */
        link_global_data_start = .;
        *(.data*)
        link_global_data_end = .;
        . = ALIGN(., 4);
    }> SRAM AT> BTLDR_CODE


    .bss : /* Zero initialised memory used for zero initialised variables */
    {
        link_bss_location = ALIGN(., 4);
        *(.bss*)
        *(COMMON)
        link_bss_end = .;
        . = ALIGN(., 4);
    }> SRAM AT> SRAM

    .stack : /* Contains the initial stack */
    {
        link_stack_location = ALIGN(., 4);
        *(.stack)
        . = ALIGN(MAX(link_stack_location + __STACKSIZE__ , .), 4);
        link_stack_end = .;
    }> SRAM AT> SRAM

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

PROVIDE( dct1_start_addr_loc     = ORIGIN( DCT1_FLASH ));
PROVIDE( dct1_size_loc           = LENGTH( DCT1_FLASH ));
PROVIDE( dct2_start_addr_loc     = ORIGIN( DCT2_FLASH ));
PROVIDE( dct2_size_loc           = LENGTH( DCT2_FLASH ));
PROVIDE( app_hdr_start_addr_loc  = ORIGIN( APP_HDR ));
PROVIDE( sram_start_addr_loc     = ORIGIN( SRAM ));
PROVIDE( sram_size_loc           = LENGTH( SRAM ));


/* ThreadX aliases */
PROVIDE( __RAM_segment_used_end__ = link_stack_end );
PROVIDE( __tx_free_memory_start   = link_stack_end );
PROVIDE( __tx_vectors             = link_interrupt_vectors_location );
PROVIDE( bootloader_api_addr_loc  = ORIGIN( BTLDR_API ));




