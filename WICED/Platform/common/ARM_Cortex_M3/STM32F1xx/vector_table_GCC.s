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
  .word WWDG_irq                           @ Window Watchdog interrupt
  .word PVD_irq                            @ PVD through EXTI Line detection
  .word TAMPER_irq                         @ Tamper interrupt
  .word RTC_irq                            @ RTC global interrupt
  .word FLASH_irq                          @ Flash global interrupt
  .word RCC_irq                            @ RCC global interrupt
  .word EXTI0_irq                          @ EXTI Line0 interrupt
  .word EXTI1_irq                          @ EXTI Line1 interrupt
  .word EXTI2_irq                          @ EXTI Line2 interrupt
  .word EXTI3_irq                          @ EXTI Line3 interrupt
  .word EXTI4_irq                          @ EXTI Line4 interrupt
  .word DMAChannel1_irq                    @ DMA Channel 1 interrupt
  .word DMAChannel2_irq                    @ DMA Channel 2 interrupt
  .word DMAChannel3_irq                    @ DMA Channel 3 interrupt
  .word DMAChannel4_irq                    @ DMA Channel 4 interrupt
  .word DMAChannel5_irq                    @ DMA Channel 5 interrupt
  .word DMAChannel6_irq                    @ DMA Channel 6 interrupt
  .word DMAChannel7_irq                    @ DMA Channel 7 interrupt
  .word ADC_irq                            @ ADC interrupt
  .word USB_HP_CAN_TX_irq                  @ USB HP CAN TX interrupt
  .word USB_LP_CAN_RX0_irq                 @ USB LP CAN RX0 interrupt
  .word CAN_RX1_irq                        @ CAN RX1 interrupt
  .word CAN_SCE_irq                        @ CAN SCE interrupt
  .word EXTI9_5_irq                        @ EXTI Lines 5-9
  .word TIM1_BRK_irq                       @ TIMER 1 BRK interrupt
  .word TIM1_UP_irq                        @ TIMER 1 UP interrupt
  .word TIM1_TRG_COM_irq                   @ TIMER 1 TRG COM interrupt
  .word TIM1_CC_irq                        @ TIMER 1 CC interrupt
  .word TIM2_irq                           @ TIMER 2 interrupt
  .word TIM3_irq                           @ TIMER 3 interrupt
  .word TIM4_irq                           @ TIMER 4 interrupt
  .word I2C1_EV_irq                        @ I2C 1 EV interrupt
  .word I2C1_ER_irq                        @ I2C 1 ER interrupt
  .word I2C2_EV_irq                        @ I2C 2 EV interrupt
  .word I2C2_ER_irq                        @ I2C 2 ER interrupt
  .word SPI1_irq                           @ SPI 1 interrupt
  .word SPI2_irq                           @ SPI 2 interrupt
  .word USART1_irq                         @ USART 1 interrupt
  .word USART2_irq                         @ USART 2 interrupt
  .word USART3_irq                         @ USART 3 interrupt
  .word EXTI15_10_irq                      @ EXTI Lines 10-15 interrupt
  .word RTCAlarm_irq                       @ RTC Alarm interrupt
  .word USBWakeUp_irq                      @ USB Wake Up interrupt
  .word TIM8_BRK_irq                       @ TIMER 8 BRK interrupt
  .word TIM8_UP_irq                        @ TIMER 8 UP interrupt
  .word TIM8_TRG_COM_irq                   @ TIMER 8 TRG COM interrupt
  .word TIM8_CC_irq                        @ TIMER 8 CC interrupt
  .word ADC3_irq                           @ ADC 3 interrupt
  .word FSMC_irq                           @ FSMC interrupt
  .word SDIO_irq                           @ SDIO interrupt
  .word TIM5_irq                           @ TIMER 5 interrupt
  .word SPI3_irq                           @ SPI 3 interrupt
  .word UART4_irq                          @ UART 4 interrupt
  .word UART5_irq                          @ UART 5 interrupt
  .word TIM6_irq                           @ TIMER 6 interrupt
  .word TIM7_irq                           @ TIMER 7 interrupt
  .word DMA2_Channel1_irq                  @ DMA 2 Channel 1 interrupt
  .word DMA2_Channel2_irq                  @ DMA 2 Channel 2 interrupt
  .word DMA2_Channel3_irq                  @ DMA 2 Channel 3 interrupt
  .word DMA2_Channel4_5_irq                @ DMA 2 Channel 4 & 5 interrupt
  .word ETH_irq							   @ Ethernet interrupt
  .word ETH_WKUP_irq					   @ Ethernet wake up interrupt
  .word CAN2_TX_irq						   @ CAN related interrupts
  .word CAN2_RX0_irq					   @ CAN related interrupts
  .word CAN2_RX1_irq                       @ CAN related interrupts
  .word CAN2_SCE_irq                       @ CAN related interrupts
  .word OTG_FS_irq						   @ USB on the go global interrupt


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
        .weak WWDG_irq
        .weak PVD_irq
        .weak TAMPER_irq
        .weak RTC_irq
        .weak FLASH_irq
        .weak RCC_irq
        .weak EXTI0_irq
        .weak EXTI1_irq
        .weak EXTI2_irq
        .weak EXTI3_irq
        .weak EXTI4_irq
        .weak DMAChannel1_irq
        .weak DMAChannel2_irq
        .weak DMAChannel3_irq
        .weak DMAChannel4_irq
        .weak DMAChannel5_irq
        .weak DMAChannel6_irq
        .weak DMAChannel7_irq
        .weak ADC_irq
        .weak USB_HP_CAN_TX_irq
        .weak USB_LP_CAN_RX0_irq
        .weak CAN_RX1_irq
        .weak CAN_SCE_irq
        .weak EXTI9_5_irq
        .weak TIM1_BRK_irq
        .weak TIM1_UP_irq
        .weak TIM1_TRG_COM_irq
        .weak TIM1_CC_irq
        .weak TIM2_irq
        .weak TIM3_irq
        .weak TIM4_irq
        .weak I2C1_EV_irq
        .weak I2C1_ER_irq
        .weak I2C2_EV_irq
        .weak I2C2_ER_irq
        .weak SPI1_irq
        .weak SPI2_irq
        .weak USART1_irq
        .weak USART2_irq
        .weak USART3_irq
        .weak EXTI15_10_irq
        .weak RTCAlarm_irq
        .weak USBWakeUp_irq
        .weak TIM8_BRK_irq
        .weak TIM8_UP_irq
        .weak TIM8_TRG_COM_irq
        .weak TIM8_CC_irq
        .weak ADC3_irq
        .weak FSMC_irq
        .weak SDIO_irq
        .weak TIM5_irq
        .weak SPI3_irq
        .weak UART4_irq
        .weak UART5_irq
        .weak TIM6_irq
        .weak TIM7_irq
        .weak DMA2_Channel1_irq
        .weak DMA2_Channel2_irq
        .weak DMA2_Channel3_irq
        .weak DMA2_Channel4_5_irq
		.weak ETH_irq
		.weak ETH_WKUP_irq
		.weak CAN2_TX_irq
		.weak CAN2_RX0_irq
		.weak CAN2_RX1_irq
		.weak CAN2_SCE_irq
		.weak OTG_FS_irq
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
WWDG_irq:
PVD_irq:
TAMPER_irq:
RTC_irq:
FLASH_irq:
RCC_irq:
EXTI0_irq:
EXTI1_irq:
EXTI2_irq:
EXTI3_irq:
EXTI4_irq:
DMAChannel1_irq:
DMAChannel2_irq:
DMAChannel3_irq:
DMAChannel4_irq:
DMAChannel5_irq:
DMAChannel6_irq:
DMAChannel7_irq:
ADC_irq:
USB_HP_CAN_TX_irq:
USB_LP_CAN_RX0_irq:
CAN_RX1_irq:
CAN_SCE_irq:
EXTI9_5_irq:
TIM1_BRK_irq:
TIM1_UP_irq:
TIM1_TRG_COM_irq:
TIM1_CC_irq:
TIM2_irq:
TIM3_irq:
TIM4_irq:
I2C1_EV_irq:
I2C1_ER_irq:
I2C2_EV_irq:
I2C2_ER_irq:
SPI1_irq:
SPI2_irq:
USART1_irq:
USART2_irq:
USART3_irq:
EXTI15_10_irq:
RTCAlarm_irq:
USBWakeUp_irq:
TIM8_BRK_irq:
TIM8_UP_irq:
TIM8_TRG_COM_irq:
TIM8_CC_irq:
ADC3_irq:
FSMC_irq:
SDIO_irq:
TIM5_irq:
SPI3_irq:
UART4_irq:
UART5_irq:
TIM6_irq:
TIM7_irq:
DMA2_Channel1_irq:
DMA2_Channel2_irq:
DMA2_Channel3_irq:
DMA2_Channel4_5_irq:
ETH_irq:
ETH_WKUP_irq:
CAN2_TX_irq:
CAN2_RX0_irq:
CAN2_RX1_irq:
CAN2_SCE_irq:
OTG_FS_irq:

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


