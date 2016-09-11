;
;  Copyright 2014, Broadcom Corporation
; All Rights Reserved.
;
; This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
; the contents of this file may not be disclosed to third parties, copied
; or duplicated in any form, in whole or in part, without the prior
; written permission of Broadcom Corporation.
;

  MODULE  ?ThreadX_interrupt_handlers_IAR

  EXTERN _tx_timer_interrupt
  EXTERN _tx_thread_context_save
  EXTERN _tx_thread_context_restore




        PUBWEAK sdio_irq
        PUBWEAK uart_irq
        PUBWEAK dma_irq
        PUBWEAK gpio_irq
		PUBWEAK dbg_watchdog_irq
		PUBWEAK usart1_irq
		PUBWEAK usart2_irq
		PUBWEAK usart6_irq
		PUBWEAK usart1_tx_dma_irq
		PUBWEAK usart2_tx_dma_irq
		PUBWEAK usart6_tx_dma_irq
		PUBWEAK usart1_rx_dma_irq
		PUBWEAK usart2_rx_dma_irq
		PUBWEAK usart6_rx_dma_irq



; SPI External Interrupt handler
; saves context and calls exti_irq
        PUBLIC gpio_rtos_irq
        SECTION .text:CODE:REORDER(1)
        THUMB
gpio_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL gpio_irq
        B       _tx_thread_context_restore



; SDIO Interrupt handler
; saves context and calls sdio_irq
        PUBLIC sdio_rtos_irq
        SECTION .text:CODE:REORDER(1)
        THUMB
sdio_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL sdio_irq
        B       _tx_thread_context_restore

; DMA Interrupt handler
; saves context and calls dma_irq
        PUBLIC dma_rtos_irq
        SECTION .text:CODE:REORDER(1)
        THUMB
dma_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL dma_irq
        B       _tx_thread_context_restore

; UART Interrupt handler
; saves context and calls uart_irq
;        PUBLIC uart_rtos_irq
;        SECTION .text:CODE:REORDER(1)
;        THUMB
;uart_rtos_irq:
;        PUSH    {lr}
;        BL  _tx_thread_context_save
;        BL uart_irq
;        B   _tx_thread_context_restore



;uart2 rx dma interrupt handler
        PUBLIC usart2_rx_dma_rtos_irq
        SECTION .text:CODE:REORDER(1)
        THUMB
usart2_rx_dma_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart2_rx_dma_irq
        B   _tx_thread_context_restore

usart1_irq

;uart6 rx dma interrupt handler
        PUBLIC usart6_rx_dma_rtos_irq
        SECTION .text:CODE:REORDER(1)
        THUMB
usart6_rx_dma_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart6_rx_dma_irq
        B   _tx_thread_context_restore


;uart1 rx dma interrupt handler
        PUBLIC usart1_rx_dma_rtos_irq
        SECTION .text:CODE:REORDER(1)
        THUMB
usart1_rx_dma_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart1_rx_dma_irq
        B   _tx_thread_context_restore




;uart2 tx dma interrupt handler
        PUBLIC usart2_tx_dma_rtos_irq
        SECTION .text:CODE:REORDER(1)
        THUMB
usart2_tx_dma_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart2_tx_dma_irq
        B   _tx_thread_context_restore



;uart6 tx dma interrupt handler
        PUBLIC usart6_tx_dma_rtos_irq
        SECTION .text:CODE:REORDER(1)
        THUMB
usart6_tx_dma_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart6_tx_dma_irq
        B   _tx_thread_context_restore


;uart1 tx dma interrupt handler
        PUBLIC usart1_tx_dma_rtos_irq
        SECTION .text:CODE:REORDER(1)
        THUMB
usart1_tx_dma_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart1_tx_dma_irq
        B   _tx_thread_context_restore






;usart1 interrupt handler
        PUBLIC usart1_rtos_irq
        SECTION .text:CODE:REORDER(1)
        THUMB
usart1_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart1_irq
        B   _tx_thread_context_restore


;usart2 interrupt handler
        PUBLIC usart2_rtos_irq
        SECTION .text:CODE:REORDER(1)
        THUMB
usart2_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart2_irq
        B   _tx_thread_context_restore


;usart6 interrupt handler
        PUBLIC usart6_rtos_irq
        SECTION .text:CODE:REORDER(1)
        THUMB
usart6_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL usart6_irq
        B   _tx_thread_context_restore


;debug watdhdog interrupt handler
        PUBLIC dbg_watchdog_rtos_irq
        SECTION .text:CODE:REORDER(1)
        THUMB
dbg_watchdog_rtos_irq:
        PUSH    {lr}
        BL  _tx_thread_context_save
        BL dbg_watchdog_irq
        B   _tx_thread_context_restore



; System Tick Interrupt handler
; saves context and calls _tx_timer_interrupt
        PUBLIC __tx_SysTickHandler
        SECTION .text:CODE:REORDER(1)
        THUMB
__tx_SysTickHandler:
       PUSH {lr}
       BL   _tx_thread_context_save
       BL   _tx_timer_interrupt
       B    _tx_thread_context_restore


; Weak function to provide stub for sdio_irq, uart_irq, dma_irq, exti_irq when not in use
        SECTION .text:CODE:REORDER(1)
        THUMB
sdio_irq:
uart_irq:
dma_irq:
gpio_irq:
dbg_watchdog_irq:
usart1_irq:
usart2_irq:
usart6_irq:
usart1_tx_dma_irq:
usart2_tx_dma_irq:
usart6_tx_dma_irq:
usart1_rx_dma_irq:
usart2_rx_dma_irq:
usart6_rx_dma_irq:
        BKPT 0
sdio_irq_loop:
        B sdio_irq_loop ; endless loop

 END
