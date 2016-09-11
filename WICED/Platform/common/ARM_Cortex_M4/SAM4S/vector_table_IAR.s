;
;  Copyright 2013, Broadcom Corporation
; All Rights Reserved.
;
; This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
; the contents of this file may not be disclosed to third parties, copied
; or duplicated in any form, in whole or in part, without the prior
; written permission of Broadcom Corporation.
;

		EXTERN wiced_program_start
		SECTION CSTACK:DATA:NOROOT(3)

		SECTION .intvec:CODE:NOROOT(2)

		EXTERN __iar_program_start
		EXTERN HardFaultException_handler

		PUBWEAK NMIException
		PUBWEAK MemManageException
		PUBWEAK BusFaultException
		PUBWEAK UsageFaultException
  /*---------------------------------------------------------*/
  #ifdef FreeRTOS
  EXTERN vPortSVCHandler					; SVC interupt
  #else /* #ifdef FreeRTOS */
  #ifdef ThreadX
  EXTERN __tx_SVCallHandler
  #else /* #ifdef ThreadX */
  PUBWEAK SVC_irq
  #endif /* #ifdef THREADX */
  #endif /* #ifdef FreeRTOS */
  /*---------------------------------------------------------*/
  PUBWEAK DebugMonitor
  /*---------------------------------------------------------*/
  #ifdef FreeRTOS
  EXTERN xPortPendSVHandler				; Pend-SV interrupt
  #else /* FreeRTOS */
  #ifdef ThreadX
  EXTERN __tx_PendSVHandler
  #else /* ThreadX */
  PUBWEAK PENDSV_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  #ifdef FreeRTOS
  EXTERN xPortSysTickHandler				; Sys-tick interrupt
  #else /* FreeRTOS */
  #ifdef ThreadX
  EXTERN __tx_SysTickHandler
  #else /* ThreadX */
  PUBWEAK SYSTICK_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  PUBWEAK SUPPLY_CTRL_irq
  PUBWEAK RESET_CTRL_irq
  PUBWEAK RTC_irq
  PUBWEAK RTT_irq
  PUBWEAK WDT_irq
  PUBWEAK PMC_irq
  PUBWEAK EEFC_irq
  /*---------------------------------------------------------*/
  /* previously it was UART0_irq */
  #ifdef FreeRTOS
  PUBWEAK uart_irq				  ; ; UART0_irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  EXTERN uart_rtos_irq
  #else /* ThreadX */
  PUBWEAK uart_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  PUBWEAK UART1_irq
  PUBWEAK SMC_irq
  /*---------------------------------------------------------*/
  /* previously it was PIO_CTRL_A_irq */
  /* previously it was PIO_CTRL_B_irq */
  /* previously it was PIO_CTRL_C_irq */
  #ifdef FreeRTOS
  PUBWEAK gpio_irq						; PIO_CTRL_B_irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  EXTERN gpio_rtos_irq
  #else /* ThreadX */
  PUBWEAK gpio_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously was MCI_irq */
  #ifdef FreeRTOS
  PUBWEAK sdio_irq						; MCI_irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  #ifdef SDIO
  /* build for SDIO bus, have to use sdio_rto_irq handler from threadx handlers */
  EXTERN sdio_rtos_irq
  #else /* #ifdef SDIO */
  /* build for spi, fill with default handler */
  PUBWEAK sdio_irq
  #endif /* #ifdef SDIO */
  #else /* ThreadX */
  PUBWEAK sdio_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  PUBWEAK TWI0_irq
  PUBWEAK TWI1_irq
  PUBWEAK SPI_irq
  PUBWEAK SSC_irq
  PUBWEAK TC0_irq
  PUBWEAK TC1_irq
  PUBWEAK TC2_irq
  PUBWEAK TC3_irq
  PUBWEAK TC4_irq
  PUBWEAK TC5_irq
  PUBWEAK ADC_irq
  PUBWEAK DAC_irq
  PUBWEAK PWM_irq
  PUBWEAK CRCCU_ir
  PUBWEAK AC_irq
  PUBWEAK USB_irq




  PUBLIC _vectors
  PUBLIC __vector_table
  DATA
_vectors
__vector_table
  DC32 sfe(CSTACK)                        ; Initial stack location
  DC32 wiced_program_start             ; Reset vector
  DC32 NMIException                       ; Non Maskable Interrupt
  DC32 HardFaultException                 ; Hard Fault interrupt
  DC32 MemManageException                 ; Memory Management Fault interrupt
  DC32 BusFaultException                  ; Bus Fault interrupt
  DC32 UsageFaultException                ; Usage Fault interrupt
  DC32 0 ; Reserved
  DC32 0 ; Reserved
  DC32 0 ; Reserved
  DC32 0 ; Reserved
  /*---------------------------------------------------------*/
  /* previously was SVC_irq */
  #ifdef FreeRTOS
  DC32 vPortSVCHandler			   ; SVC_irq interrupt
  #else /* #ifdef FreeRTOS */
  #ifdef ThreadX
  DC32 __tx_SVCallHandler
  #else /* #ifdef ThreadX */
  DC32 SVC_irq
  #endif /* #ifdef ThreadX */
  #endif /* #ifdef FreeRTOS */
  /*---------------------------------------------------------*/
  DC32 DebugMonitor                       ; Debug Monitor interrupt
  DC32 0 ; Reserved
  /*---------------------------------------------------------*/
  /* previously was PENDSV_irq */
  #ifdef FreeRTOS
  DC32 xPortPendSVHandler		   ; Pend-SV interrupt
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 __tx_PendSVHandler
  #else /* ThreadX */
  DC32 PENDSV_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously was SYSTICK_irq */
  #ifdef FreeRTOS
  DC32 xPortSysTickHandler		   ; SYSTICK_irq interrupt
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 __tx_SysTickHandler
  #else /* ThreadX */
  DC32 SYSTICK_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  DC32 SUPPLY_CTRL_irq                    ; 0  SUPPLY CONTROLLER
  DC32 RESET_CTRL_irq                     ; 1  RESET CONTROLLER
  DC32 RTC_irq                            ; 2  REAL TIME CLOCK
  DC32 RTT_irq                            ; 3  REAL TIME TIMER
  DC32 WDT_irq                            ; 4  WATCHDOG TIMER
  DC32 PMC_irq                            ; 5  PMC
  DC32 EEFC_irq                           ; 6  EEFC
  DC32 0                                  ; 7  Reserved
  /*---------------------------------------------------------*/
  /* previously was UART0_irq */
  #ifdef FreeRTOS
  DC32 uart_irq			   ; 14 UART 0
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 uart_irq
  #else /* ThreadX */
  DC32 uart_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  DC32 UART1_irq                          ; 9  UART1
  DC32 SMC_irq                            ; 10 SMC
  /*---------------------------------------------------------*/
  /* previously this vector was named PIO_CTRL_A_irq */
  #ifdef FreeRTOS
  DC32 gpio_irq		   ; 11 Parallel IO Controller A
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 gpio_rtos_irq
  #else /* ThreadX */
  DC32 gpio_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously this vector was named PIO_CTRL_B_irq */
  #ifdef FreeRTOS
  DC32 gpio_irq		   ; 12 Parallel IO Controller B
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 gpio_rtos_irq
  #else /* ThreadX */
  DC32 gpio_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously this vector was named PIO_CTRL_C_irq */
  #ifdef FreeRTOS
  DC32 gpio_irq		   ; 13 Parallel IO Controller C
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 gpio_rtos_irq
  #else /* ThreadX */
  DC32 gpio_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  DC32 USART0_irq
  DC32 USART1_irq                         ; 15 USART 1
  DC32 0                                  ; 16 Reserved
  DC32 0                                  ; 17 Reserved
  /*---------------------------------------------------------*/
  /* previously was MCI_irq */
  #ifdef FreeRTOS
  DC32 sdio_irq			   ; 18 MCI
  #else /* FreeRTOS */
  #ifdef ThreadX
  #ifdef SDIO
  /* build for SDIO bus, have to use sdio_rto_irq handler from threadx handlers */
  DC32 sdio_rtos_irq
  #else /* #ifdef SDIO */
  /* build for spi, fill with defualt handler */
  DC32 sdio_irq
  #endif /* #ifdef SDIO */
  #else /* ThreadX */
  DC32 sdio_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  DC32 TWI0_irq                           ; 19 TWI 0
  DC32 TWI1_irq                           ; 20 TWI 1
  DC32 SPI_irq                            ; 21 SPI
  DC32 SSC_irq                            ; 22 SSC
  DC32 TC0_irq                            ; 23 Timer Counter 0
  DC32 TC1_irq                            ; 24 Timer Counter 1
  DC32 TC2_irq                            ; 25 Timer Counter 2
  DC32 TC3_irq                            ; 26 Timer Counter 3
  DC32 TC4_irq                            ; 27 Timer Counter 4
  DC32 TC5_irq                            ; 28 Timer Counter 5
  DC32 ADC_irq                            ; 29 ADC controller
  DC32 DAC_irq                            ; 30 DAC controller
  DC32 PWM_irq                            ; 31 PWM
  DC32 CRCCU_ir                           ; 32 CRC Calculation Unit
  DC32 AC_irq                             ; 33 Analog Comparator
  DC32 USB_irq                            ; 34 USB Device Port
  DC32 0                                  ; 35 not used

  THUMB

  PUBWEAK UnhandledInterrupt
  SECTION .text:CODE:REORDER(1)
/*---------------------------------------------------------*/
UnhandledInterrupt:
NMIException:
MemManageException:
BusFaultException:
UsageFaultException:
SVC_irq:
DebugMonitor:
PENDSV_irq:
SYSTICK_irq:
SUPPLY_CTRL_irq:
RESET_CTRL_irq:
RTC_irq:
RTT_irq:
WDT_irq:
PMC_irq:
EEFC_irq:
uart_irq:
UART1_irq:
SMC_irq:
/* previously was PIO_CTRL_A_irq, PIO_CTRL_B_irq, PIO_CTRL_C_irq, replaced all */
gpio_irq:
USART0_irq:
USART1_irq:
/* previously was MCI_irq */
sdio_irq:
TWI0_irq:
TWI1_irq:
SPI_irq:
SSC_irq:
TC0_irq:
TC1_irq:
TC2_irq:
TC3_irq:
TC4_irq:
TC5_irq:
ADC_irq:
DAC_irq:
PWM_irq:
CRCCU_ir:
AC_irq:
USB_irq:
        BKPT 0 ; Unhandled interrupt!
DEFAULT_HANDLER_LOOP
;        BX LR ;  step here to return to program location - don't expect things to run though
        B DEFAULT_HANDLER_LOOP ; Loop forever



        PUBLIC  HardFaultException
        THUMB
        SECTION .text:CODE:REORDER(1)
HardFaultException:
; Disable interrupts, clobbering R5 - this is so that when debugger continues, it will go to caller, not interrupt routine
         MOV R5, #0x01
         MSR PRIMASK, R5

; These may be necessary if the return below does not work
         ;MRS     R5,PSP                  ; Read PSP
         ;LDR     R6,[R5,#24]             ; Read Saved PC from Stack
         ;LDR     R7,[R5,#20]             ; Read Saved LR from Stack


	    ; C handler cannot get the stack pointer before modifying it, so pass it to the handler. Call cannot be made without destroying LR, so pass it too.
		 MRS R0, MSP
		 MRS R1, PSP
		 MOV R2, LR
		 B HardFaultException_handler


; TODO: Break here if debug image or loop infinitely if not?
;       Break if debugging
         BKPT  0; Hard Fault!!! - probably bad pointers
         BX LR ; step here to return to program location - don't expect things to run though

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  		PUBLIC iar_set_msp
		THUMB
        SECTION .text:CODE:REORDER(1)

iar_set_msp:
		MOV SP, R0
		BX LR

		END
  END
