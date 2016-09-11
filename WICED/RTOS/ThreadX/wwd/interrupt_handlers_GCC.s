@
@  Copyright 2013, Broadcom Corporation
@ All Rights Reserved.
@
@ This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
@ the contents of this file may not be disclosed to third parties, copied
@ or duplicated in any form, in whole or in part, without the prior
@ written permission of Broadcom Corporation.
@

.extern _tx_timer_interrupt
.extern _tx_thread_context_save
.extern _tx_thread_context_restore


  .syntax unified

@ Combined GPIO interrupt handler
@ saves context and calls gpio_irq
        .text 32
        .section .text.gpio_rtos_irq, "ax"
        .align 4
        .global  gpio_rtos_irq
        .thumb_func
gpio_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL gpio_irq
        B       _tx_thread_context_restore

@ SDIO Interrupt handler
@ saves context and calls sdio_irq
        .text 32
        .section .text.sdio_rtos_irq, "ax"
        .align 4
        .global  sdio_rtos_irq
        .thumb_func
sdio_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL sdio_irq
        B       _tx_thread_context_restore

@ DMA Interrupt handler
@ saves context and calls dma_irq
        .text 32
        .section .text.dma_rtos_irq, "ax"
        .align 4
        .global  dma_rtos_irq
        .thumb_func
dma_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL dma_irq
        B       _tx_thread_context_restore

@ System Tick Interrupt handler
@ saves context and calls _tx_timer_interrupt
        .text 32
        .section .text.__tx_SysTickHandler, "ax"
        .align 4
        .global  __tx_SysTickHandler
        .thumb_func
__tx_SysTickHandler:
       PUSH {lr}
       BL   _tx_thread_context_save
       BL   _tx_timer_interrupt
       B    _tx_thread_context_restore


@ Debug Watchdog Timer Interrupt Handler
@ saves context and calls _tx_timer_interrupt
        .text 32
        .section .text.dbg_watchdog_rtos_irq, "ax"
        .align 4
        .global  dbg_watchdog_rtos_irq
        .thumb_func
dbg_watchdog_rtos_irq:
       PUSH {lr}
       BL   _tx_thread_context_save
       BL   dbg_watchdog_irq
       B    _tx_thread_context_restore


@ USART1 Interrupt handler
@ saves context and calls usart1_irq
        .text 32
        .section .text.usart1_rtos_irq, "ax"
        .align 4
        .global  usart1_rtos_irq
        .thumb_func
usart1_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart1_irq
        B  _tx_thread_context_restore


@ USART2 Interrupt handler
@ saves context and calls usart2_irq
        .text 32
        .section .text.usart2_rtos_irq, "ax"
        .align 4
        .global  usart2_rtos_irq
        .thumb_func
usart2_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart2_irq
        B  _tx_thread_context_restore

@ USART6 Interrupt handler
@ saves context and calls usart2_irq
        .text 32
        .section .text.usart6_rtos_irq, "ax"
        .align 4
        .global  usart6_rtos_irq
        .thumb_func
usart6_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart6_irq
        B  _tx_thread_context_restore


@ USART1 TX DMA Interrupt handler
@ saves context and calls usart1_tx_dma_irq
        .text 32
        .section .text.usart1_tx_dma_rtos_irq, "ax"
        .align 4
        .global  usart1_tx_dma_rtos_irq
        .thumb_func
usart1_tx_dma_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart1_tx_dma_irq
        B  _tx_thread_context_restore


@ USART2 TX DMA Interrupt handler
@ saves context and calls usart2_tx_dma_irq
        .text 32
        .section .text.usart2_tx_dma_rtos_irq, "ax"
        .align 4
        .global  usart2_tx_dma_rtos_irq
        .thumb_func
usart2_tx_dma_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart2_tx_dma_irq
        B  _tx_thread_context_restore

@ USART6 TX DMA Interrupt handler
@ saves context and calls usart2_tx_dma_irq
        .text 32
        .section .text.usart6_tx_dma_rtos_irq, "ax"
        .align 4
        .global  usart6_tx_dma_rtos_irq
        .thumb_func
usart6_tx_dma_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart6_tx_dma_irq
        B  _tx_thread_context_restore

@ USART1 RX DMA Interrupt handler
@ saves context and calls usart1_rx_dma_irq
        .text 32
        .section .text.usart1_rx_dma_rtos_irq, "ax"
        .align 4
        .global  usart1_rx_dma_rtos_irq
        .thumb_func
usart1_rx_dma_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart1_rx_dma_irq
        B  _tx_thread_context_restore


@ USART2 RX DMA Interrupt handler
@ saves context and calls usart2_rx_dma_irq
        .text 32
        .section .text.usart2_rx_dma_rtos_irq, "ax"
        .align 4
        .global  usart2_rx_dma_rtos_irq
        .thumb_func
usart2_rx_dma_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart2_rx_dma_irq
        B  _tx_thread_context_restore

@ USART6 RX DMA Interrupt handler
@ saves context and calls usart6_rx_dma_irq
        .text 32
        .section .text.usart6_rx_dma_rtos_irq, "ax"
        .align 4
        .global  usart6_rx_dma_rtos_irq
        .thumb_func
usart6_rx_dma_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart6_rx_dma_irq
        B  _tx_thread_context_restore


@ Weak function to provide stub for sdio_irq, uart_irq, dma_irq, exti_irq when not in use
        .text 32
        .section .text.weak_interrupt_handlers, "ax"
        .align 4
        .thumb_func
        .weak sdio_irq
        .weak uart_irq
        .weak dma_irq
        .weak gpio_irq
        .weak dbg_watchdog_irq
        .weak usart1_irq
        .weak usart2_irq
        .weak usart1_tx_dma_irq
        .weak usart2_tx_dma_irq
        .weak usart1_rx_dma_irq
        .weak usart2_rx_dma_irq
sdio_irq:
uart_irq:
dma_irq:
gpio_irq:
dbg_watchdog_irq:
usart1_irq:
usart2_irq:
usart1_tx_dma_irq:
usart2_tx_dma_irq:
usart1_rx_dma_irq:
usart2_rx_dma_irq:
        BKPT 0
sdio_irq_loop:
        B sdio_irq_loop @ endless loop
