@
@  Copyright 2013, Broadcom Corporation
@ All Rights Reserved.
@
@ This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
@ the contents of this file may not be disclosed to third parties, copied
@ or duplicated in any form, in whole or in part, without the prior
@ written permission of Broadcom Corporation.
@

.extern link_stack_end

  .global _start

  .syntax unified
  .section .vectors, "ax"
  .code 16
  .align 0
  .global _vectors

_vectors:
  .word link_stack_end                     @ Initial stack location
  .word reset_handler                      @ Reset vector
  .word NMIException                       @ Non Maskable Interrupt
  .word HardFaultException                 @ Hard Fault interrupt
  .word MemManageException                 @ Memory Management Fault interrupt
  .word BusFaultException                  @ Bus Fault interrupt
  .word UsageFaultException                @ Usage Fault interrupt
  .word 0 @ Reserved
  .word 0 @ Reserved
  .word 0 @ Reserved
  .word 0 @ Reserved
  .word SVC_irq                            @ SVC interrupt
  .word DebugMonitor                       @ Debug Monitor interrupt
  .word 0 @ Reserved
  .word PENDSV_irq                         @ PendSV interrupt
  .word SYSTICK_irq                        @ Sys Tick Interrupt
  .word SUPPLY_CTRL_irq                    @  0  SUPPLY CONTROLLER
  .word RESET_CTRL_irq                     @  1  RESET CONTROLLER
  .word RTC_irq                            @  2  REAL TIME CLOCK
  .word RTT_irq                            @  3  REAL TIME TIMER
  .word WDT_irq                            @  4  WATCHDOG TIMER
  .word PMC_irq                            @  5  PMC
  .word EEFC_irq                           @  6  EEFC
  .word 0                                  @  7  Reserved
  .word UART0_irq                          @  8  UART0
  .word UART1_irq                          @  9  UART1
  .word SMC_irq                            @  10 SMC
  .word PIO_CTRL_A_irq                     @  11 Parallel IO Controller A
  .word PIO_CTRL_B_irq                     @  12 Parallel IO Controller B
  .word PIO_CTRL_C_irq                     @  13 Parallel IO Controller C
  .word USART0_irq                         @  14 USART 0
  .word USART1_irq                         @  15 USART 1
  .word 0                                  @  16 Reserved
  .word 0                                  @  17 Reserved
  .word MCI_irq                            @  18 MCI
  .word TWI0_irq                           @  19 TWI 0
  .word TWI1_irq                           @  20 TWI 1
  .word SPI_irq                            @  21 SPI
  .word SSC_irq                            @  22 SSC
  .word TC0_irq                            @  23 Timer Counter 0
  .word TC1_irq                            @  24 Timer Counter 1
  .word TC2_irq                            @  25 Timer Counter 2
  .word TC3_irq                            @  26 Timer Counter 3
  .word TC4_irq                            @  27 Timer Counter 4
  .word TC5_irq                            @  28 Timer Counter 5
  .word ADC_irq                            @  29 ADC controller
  .word DAC_irq                            @  30 DAC controller
  .word PWM_irq                            @  31 PWM
  .word CRCCU_ir                           @  32 CRC Calculation Unit
  .word AC_irq                             @  33 Analog Comparator
  .word USB_irq                            @  34 USB Device Port
  .word 0                                  @  35 not used



@ Unused Interrupt handler
        .section .text.UnhandledInterrupt, "ax"
        .text 32
        .align 4
        .global  UnhandledInterrupt
        .weak reset_handler
        .weak NMIException
        .weak MemManageException
        .weak BusFaultException
        .weak UsageFaultException
        .weak SVC_irq
        .weak DebugMonitor
        .weak PENDSV_irq
        .weak SYSTICK_irq
        .weak SUPPLY_CTRL_irq
        .weak RESET_CTRL_irq
        .weak RTC_irq
        .weak RTT_irq
        .weak WDT_irq
        .weak PMC_irq
        .weak EEFC_irq
        .weak UART0_irq
        .weak UART1_irq
        .weak SMC_irq
        .weak PIO_CTRL_A_irq
        .weak PIO_CTRL_B_irq
        .weak PIO_CTRL_C_irq
        .weak USART0_irq
        .weak USART1_irq
        .weak MCI_irq
        .weak TWI0_irq
        .weak TWI1_irq
        .weak SPI_irq
        .weak SSC_irq
        .weak TC0_irq
        .weak TC1_irq
        .weak TC2_irq
        .weak TC3_irq
        .weak TC4_irq
        .weak TC5_irq
        .weak ADC_irq
        .weak DAC_irq
        .weak PWM_irq
        .weak CRCCU_ir
        .weak AC_irq
        .weak USB_irq
        .thumb_func
UnhandledInterrupt:
reset_handler:
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
UART0_irq:
UART1_irq:
SMC_irq:
PIO_CTRL_A_irq:
PIO_CTRL_B_irq:
PIO_CTRL_C_irq:
USART0_irq:
USART1_irq:
MCI_irq:
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
        BKPT 0  @ Unhandled interrupt!
@        BX LR   @ step here to return to program location - don't expect things to run though
        B UnhandledInterrupt  @ Loop forever


        .section .text.HardFaultException, "ax"
        .text 32
        .align 4
        .global  HardFaultException
        .thumb_func
HardFaultException:

    @ C handler cannot get the stack pointer before modifying it, so pass it to the handler. Call cannot be made without destroying LR, so pass it too.
	MRS R0, MSP
	MRS R1, PSP
	MOV R2, LR
	B HardFaultException_handler

