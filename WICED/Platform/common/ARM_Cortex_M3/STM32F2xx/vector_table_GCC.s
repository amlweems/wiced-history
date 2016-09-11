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
  .align 4
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
  .word WWDG_irq                    @ Window WatchDog
  .word PVD_irq                     @ PVD through EXTI Line detection
  .word TAMP_STAMP_irq              @ Tamper and TimeStamps through the EXTI line
  .word RTC_WKUP_irq                @ RTC Wakeup through the EXTI line
  .word FLASH_irq                   @ FLASH
  .word RCC_irq                     @ RCC
  .word EXTI0_irq                   @ EXTI Line0
  .word EXTI1_irq                   @ EXTI Line1
  .word EXTI2_irq                   @ EXTI Line2
  .word EXTI3_irq                   @ EXTI Line3
  .word EXTI4_irq                   @ EXTI Line4
  .word DMA1_Stream0_irq            @ DMA1 Stream 0
  .word DMA1_Stream1_irq            @ DMA1 Stream 1
  .word DMA1_Stream2_irq            @ DMA1 Stream 2
  .word DMA1_Stream3_irq            @ DMA1 Stream 3
  .word DMA1_Stream4_irq            @ DMA1 Stream 4
  .word DMA1_Stream5_irq            @ DMA1 Stream 5
  .word DMA1_Stream6_irq            @ DMA1 Stream 6
  .word ADC_irq                     @ ADC1, ADC2 and ADC3s
  .word CAN1_TX_irq                 @ CAN1 TX
  .word CAN1_RX0_irq                @ CAN1 RX0
  .word CAN1_RX1_irq                @ CAN1 RX1
  .word CAN1_SCE_irq                @ CAN1 SCE
  .word EXTI9_5_irq                 @ External Line[9:5]s
  .word TIM1_BRK_TIM9_irq           @ TIM1 Break and TIM9
  .word TIM1_UP_TIM10_irq           @ TIM1 Update and TIM10
  .word TIM1_TRG_COM_TIM11_irq      @ TIM1 Trigger and Commutation and TIM11
  .word TIM1_CC_irq                 @ TIM1 Capture Compare
  .word TIM2_irq                    @ TIM2
  .word TIM3_irq                    @ TIM3
  .word TIM4_irq                    @ TIM4
  .word I2C1_EV_irq                 @ I2C1 Event
  .word I2C1_ER_irq                 @ I2C1 Error
  .word I2C2_EV_irq                 @ I2C2 Event
  .word I2C2_ER_irq                 @ I2C2 Error
  .word SPI1_irq                    @ SPI1
  .word SPI2_irq                    @ SPI2
  .word USART1_irq                  @ USART1
  .word USART2_irq                  @ USART2
  .word USART3_irq                  @ USART3
  .word EXTI15_10_irq               @ External Line[15:10]s
  .word RTC_Alarm_irq               @ RTC Alarm (A and B) through EXTI Line
  .word OTG_FS_WKUP_irq             @ USB OTG FS Wakeup through EXTI line
  .word TIM8_BRK_TIM12_irq          @ TIM8 Break and TIM12
  .word TIM8_UP_TIM13_irq           @ TIM8 Update and TIM13
  .word TIM8_TRG_COM_TIM14_irq      @ TIM8 Trigger and Commutation and TIM14
  .word TIM8_CC_irq                 @ TIM8 Capture Compare
  .word DMA1_Stream7_irq            @ DMA1 Stream7
  .word FSMC_irq                    @ FSMC
  .word SDIO_irq                    @ SDIO
  .word TIM5_irq                    @ TIM5
  .word SPI3_irq                    @ SPI3
  .word UART4_irq                   @ UART4
  .word UART5_irq                   @ UART5
  .word TIM6_DAC_irq                @ TIM6 and DAC1&2 underrun errors
  .word TIM7_irq                    @ TIM7
  .word DMA2_Stream0_irq            @ DMA2 Stream 0
  .word DMA2_Stream1_irq            @ DMA2 Stream 1
  .word DMA2_Stream2_irq            @ DMA2 Stream 2
  .word DMA2_Stream3_irq            @ DMA2 Stream 3
  .word DMA2_Stream4_irq            @ DMA2 Stream 4
  .word ETH_irq                     @ Ethernet
  .word ETH_WKUP_irq                @ Ethernet Wakeup through EXTI line
  .word CAN2_TX_irq                 @ CAN2 TX
  .word CAN2_RX0_irq                @ CAN2 RX0
  .word CAN2_RX1_irq                @ CAN2 RX1
  .word CAN2_SCE_irq                @ CAN2 SCE
  .word OTG_FS_irq                  @ USB OTG FS
  .word DMA2_Stream5_irq            @ DMA2 Stream 5
  .word DMA2_Stream6_irq            @ DMA2 Stream 6
  .word DMA2_Stream7_irq            @ DMA2 Stream 7
  .word USART6_irq                  @ USART6
  .word I2C3_EV_irq                 @ I2C3 event
  .word I2C3_ER_irq                 @ I2C3 error
  .word OTG_HS_EP1_OUT_irq          @ USB OTG HS End Point 1 Out
  .word OTG_HS_EP1_IN_irq           @ USB OTG HS End Point 1 In
  .word OTG_HS_WKUP_irq             @ USB OTG HS Wakeup through EXTI
  .word OTG_HS_irq                  @ USB OTG HS
  .word DCMI_irq                    @ DCMI
  .word CRYP_irq                    @ CRYP crypto
  .word HASH_RNG_irq                @ Hash and Rng

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
        .weak TAMP_STAMP_irq
        .weak RTC_WKUP_irq
        .weak FLASH_irq
        .weak RCC_irq
        .weak EXTI0_irq
        .weak EXTI1_irq
        .weak EXTI2_irq
        .weak EXTI3_irq
        .weak EXTI4_irq
        .weak DMA1_Stream0_irq
        .weak DMA1_Stream1_irq
        .weak DMA1_Stream2_irq
        .weak DMA1_Stream3_irq
        .weak DMA1_Stream4_irq
        .weak DMA1_Stream5_irq
        .weak DMA1_Stream6_irq
        .weak ADC_irq
        .weak CAN1_TX_irq
        .weak CAN1_RX0_irq
        .weak CAN1_RX1_irq
        .weak CAN1_SCE_irq
        .weak EXTI9_5_irq
        .weak TIM1_BRK_TIM9_irq
        .weak TIM1_UP_TIM10_irq
        .weak TIM1_TRG_COM_TIM11_irq
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
        .weak RTC_Alarm_irq
        .weak OTG_FS_WKUP_irq
        .weak TIM8_BRK_TIM12_irq
        .weak TIM8_UP_TIM13_irq
        .weak TIM8_TRG_COM_TIM14_irq
        .weak TIM8_CC_irq
        .weak DMA1_Stream7_irq
        .weak FSMC_irq
        .weak SDIO_irq
        .weak TIM5_irq
        .weak SPI3_irq
        .weak UART4_irq
        .weak UART5_irq
        .weak TIM6_DAC_irq
        .weak TIM7_irq
        .weak DMA2_Stream0_irq
        .weak DMA2_Stream1_irq
        .weak DMA2_Stream2_irq
        .weak DMA2_Stream3_irq
        .weak DMA2_Stream4_irq
        .weak ETH_irq
        .weak ETH_WKUP_irq
        .weak CAN2_TX_irq
        .weak CAN2_RX0_irq
        .weak CAN2_RX1_irq
        .weak CAN2_SCE_irq
        .weak OTG_FS_irq
        .weak DMA2_Stream5_irq
        .weak DMA2_Stream6_irq
        .weak DMA2_Stream7_irq
        .weak USART6_irq
        .weak I2C3_EV_irq
        .weak I2C3_ER_irq
        .weak OTG_HS_EP1_OUT_irq
        .weak OTG_HS_EP1_IN_irq
        .weak OTG_HS_WKUP_irq
        .weak OTG_HS_irq
        .weak DCMI_irq
        .weak CRYP_irq
        .weak HASH_RNG_irq
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
TAMP_STAMP_irq:
RTC_WKUP_irq:
FLASH_irq:
RCC_irq:
EXTI0_irq:
EXTI1_irq:
EXTI2_irq:
EXTI3_irq:
EXTI4_irq:
DMA1_Stream0_irq:
DMA1_Stream1_irq:
DMA1_Stream2_irq:
DMA1_Stream3_irq:
DMA1_Stream4_irq:
DMA1_Stream5_irq:
DMA1_Stream6_irq:
ADC_irq:
CAN1_TX_irq:
CAN1_RX0_irq:
CAN1_RX1_irq:
CAN1_SCE_irq:
EXTI9_5_irq:
TIM1_BRK_TIM9_irq:
TIM1_UP_TIM10_irq:
TIM1_TRG_COM_TIM11_irq:
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
RTC_Alarm_irq:
OTG_FS_WKUP_irq:
TIM8_BRK_TIM12_irq:
TIM8_UP_TIM13_irq:
TIM8_TRG_COM_TIM14_irq:
TIM8_CC_irq:
DMA1_Stream7_irq:
FSMC_irq:
SDIO_irq:
TIM5_irq:
SPI3_irq:
UART4_irq:
UART5_irq:
TIM6_DAC_irq:
TIM7_irq:
DMA2_Stream0_irq:
DMA2_Stream1_irq:
DMA2_Stream2_irq:
DMA2_Stream3_irq:
DMA2_Stream4_irq:
ETH_irq:
ETH_WKUP_irq:
CAN2_TX_irq:
CAN2_RX0_irq:
CAN2_RX1_irq:
CAN2_SCE_irq:
OTG_FS_irq:
DMA2_Stream5_irq:
DMA2_Stream6_irq:
DMA2_Stream7_irq:
USART6_irq:
I2C3_EV_irq:
I2C3_ER_irq:
OTG_HS_EP1_OUT_irq:
OTG_HS_EP1_IN_irq:
OTG_HS_WKUP_irq:
OTG_HS_irq:
DCMI_irq:
CRYP_irq:
HASH_RNG_irq:
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


