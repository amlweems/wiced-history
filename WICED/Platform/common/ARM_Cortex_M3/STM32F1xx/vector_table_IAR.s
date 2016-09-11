;
;  Copyright 2013, Broadcom Corporation
; All Rights Reserved.
;
; This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
; the contents of this file may not be disclosed to third parties, copied
; or duplicated in any form, in whole or in part, without the prior
; written permission of Broadcom Corporation.
;

  MODULE  ?STM32F1xx_vector_table_IAR

  SECTION CSTACK:DATA:NOROOT(3)

  SECTION .intvec:CODE:NOROOT(2)

  EXTERN __iar_program_start

  PUBWEAK NMIException
  PUBWEAK MemManageException
  PUBWEAK BusFaultException
  PUBWEAK UsageFaultException
  PUBWEAK SVC_irq
  PUBWEAK DebugMonitor
  PUBWEAK PENDSV_irq
  PUBWEAK SYSTICK_irq
  PUBWEAK WWDG_irq
  PUBWEAK PVD_irq
  PUBWEAK TAMPER_irq
  PUBWEAK RTC_irq
  PUBWEAK FLASH_irq
  PUBWEAK RCC_irq
  PUBWEAK EXTI0_irq
  PUBWEAK EXTI1_irq
  PUBWEAK EXTI2_irq
  PUBWEAK EXTI3_irq
  PUBWEAK EXTI4_irq
  PUBWEAK DMAChannel1_irq
  PUBWEAK DMAChannel2_irq
  PUBWEAK DMAChannel3_irq
  PUBWEAK DMAChannel4_irq
  PUBWEAK DMAChannel5_irq
  PUBWEAK DMAChannel6_irq
  PUBWEAK DMAChannel7_irq
  PUBWEAK ADC_irq
  PUBWEAK USB_HP_CAN_TX_irq
  PUBWEAK USB_LP_CAN_RX0_irq
  PUBWEAK CAN_RX1_irq
  PUBWEAK CAN_SCE_irq
  PUBWEAK EXTI9_5_irq
  PUBWEAK TIM1_BRK_irq
  PUBWEAK TIM1_UP_irq
  PUBWEAK TIM1_TRG_COM_irq
  PUBWEAK TIM1_CC_irq
  PUBWEAK TIM2_irq
  PUBWEAK TIM3_irq
  PUBWEAK TIM4_irq
  PUBWEAK I2C1_EV_irq
  PUBWEAK I2C1_ER_irq
  PUBWEAK I2C2_EV_irq
  PUBWEAK I2C2_ER_irq
  PUBWEAK SPI1_irq
  PUBWEAK SPI2_irq
  PUBWEAK USART1_irq
  PUBWEAK USART2_irq
  PUBWEAK USART3_irq
  PUBWEAK EXTI15_10_irq
  PUBWEAK RTCAlarm_irq
  PUBWEAK USBWakeUp_irq
  PUBWEAK TIM8_BRK_irq
  PUBWEAK TIM8_UP_irq
  PUBWEAK TIM8_TRG_COM_irq
  PUBWEAK TIM8_CC_irq
  PUBWEAK ADC3_irq
  PUBWEAK FSMC_irq
  PUBWEAK SDIO_irq
  PUBWEAK TIM5_irq
  PUBWEAK SPI3_irq
  PUBWEAK UART4_irq
  PUBWEAK UART5_irq
  PUBWEAK TIM6_irq
  PUBWEAK TIM7_irq
  PUBWEAK DMA2_Channel1_irq
  PUBWEAK DMA2_Channel2_irq
  PUBWEAK DMA2_Channel3_irq
  PUBWEAK DMA2_Channel4_5_irq
  PUBWEAK ETH_irq
  PUBWEAK ETH_WKUP_irq
  PUBWEAK CAN2_TX_irq
  PUBWEAK CAN2_RX0_irq
  PUBWEAK CAN2_RX1_irq
  PUBWEAK CAN2_SCE_irq
  PUBWEAK OTG_FS_irq



  PUBLIC _vectors
  PUBLIC __vector_table
  DATA
_vectors
__vector_table
  DC32 sfe(CSTACK)                     ; Initial stack location
  DC32 __iar_program_start             ; Reset vector
  DC32 NMIException                    ; Non Maskable Interrupt
  DC32 HardFaultException              ; Hard Fault interrupt
  DC32 MemManageException              ; Memory Management Fault interrupt
  DC32 BusFaultException               ; Bus Fault interrupt
  DC32 UsageFaultException             ; Usage Fault interrupt
  DC32 0 ; Reserved
  DC32 0 ; Reserved
  DC32 0 ; Reserved
  DC32 0 ; Reserved
  DC32 SVC_irq                         ; SVC interrupt
  DC32 DebugMonitor                    ; Debug Monitor interrupt
  DC32 0 ; Reserved
  DC32 PENDSV_irq                      ; PendSV interrupt
  DC32 SYSTICK_irq                     ; Sys Tick Interrupt
  DC32 WWDG_irq                        ; Window Watchdog interrupt
  DC32 PVD_irq                         ; PVD through EXTI Line detection
  DC32 TAMPER_irq                      ; Tamper interrupt
  DC32 RTC_irq                         ; RTC global interrupt
  DC32 FLASH_irq                       ; Flash global interrupt
  DC32 RCC_irq                         ; RCC global interrupt
  DC32 EXTI0_irq                       ; EXTI Line0 interrupt
  DC32 EXTI1_irq                       ; EXTI Line1 interrupt
  DC32 EXTI2_irq                       ; EXTI Line2 interrupt
  DC32 EXTI3_irq                       ; EXTI Line3 interrupt
  DC32 EXTI4_irq                       ; EXTI Line4 interrupt
  DC32 DMAChannel1_irq                 ; DMA Channel 1 interrupt
  DC32 DMAChannel2_irq                 ; DMA Channel 2 interrupt
  DC32 DMAChannel3_irq                 ; DMA Channel 3 interrupt
  DC32 DMAChannel4_irq                 ; DMA Channel 4 interrupt
  DC32 DMAChannel5_irq                 ; DMA Channel 5 interrupt
  DC32 DMAChannel6_irq                 ; DMA Channel 6 interrupt
  DC32 DMAChannel7_irq                 ; DMA Channel 7 interrupt
  DC32 ADC_irq                         ; ADC interrupt
  DC32 USB_HP_CAN_TX_irq               ; USB HP CAN TX interrupt
  DC32 USB_LP_CAN_RX0_irq              ; USB LP CAN RX0 interrupt
  DC32 CAN_RX1_irq                     ; CAN RX1 interrupt
  DC32 CAN_SCE_irq                     ; CAN SCE interrupt
  DC32 EXTI9_5_irq                     ; EXTI Lines 5-9
  DC32 TIM1_BRK_irq                    ; TIMER 1 BRK interrupt
  DC32 TIM1_UP_irq                     ; TIMER 1 UP interrupt
  DC32 TIM1_TRG_COM_irq                ; TIMER 1 TRG COM interrupt
  DC32 TIM1_CC_irq                     ; TIMER 1 CC interrupt
  DC32 TIM2_irq                        ; TIMER 2 interrupt
  DC32 TIM3_irq                        ; TIMER 3 interrupt
  DC32 TIM4_irq                        ; TIMER 4 interrupt
  DC32 I2C1_EV_irq                     ; I2C 1 EV interrupt
  DC32 I2C1_ER_irq                     ; I2C 1 ER interrupt
  DC32 I2C2_EV_irq                     ; I2C 2 EV interrupt
  DC32 I2C2_ER_irq                     ; I2C 2 ER interrupt
  DC32 SPI1_irq                        ; SPI 1 interrupt
  DC32 SPI2_irq                        ; SPI 2 interrupt
  DC32 USART1_irq                      ; USART 1 interrupt
  DC32 USART2_irq                      ; USART 2 interrupt
  DC32 USART3_irq                      ; USART 3 interrupt
  DC32 EXTI15_10_irq                   ; EXTI Lines 10-15 interrupt
  DC32 RTCAlarm_irq                    ; RTC Alarm interrupt
  DC32 USBWakeUp_irq                   ; USB Wake Up interrupt
  DC32 TIM8_BRK_irq                    ; TIMER 8 BRK interrupt
  DC32 TIM8_UP_irq                     ; TIMER 8 UP interrupt
  DC32 TIM8_TRG_COM_irq                ; TIMER 8 TRG COM interrupt
  DC32 TIM8_CC_irq                     ; TIMER 8 CC interrupt
  DC32 ADC3_irq                        ; ADC 3 interrupt
  DC32 FSMC_irq                        ; FSMC interrupt
  DC32 SDIO_irq                        ; SDIO interrupt
  DC32 TIM5_irq                        ; TIMER 5 interrupt
  DC32 SPI3_irq                        ; SPI 3 interrupt
  DC32 UART4_irq                       ; UART 4 interrupt
  DC32 UART5_irq                       ; UART 5 interrupt
  DC32 TIM6_irq                        ; TIMER 6 interrupt
  DC32 TIM7_irq                        ; TIMER 7 interrupt
  DC32 DMA2_Channel1_irq               ; DMA 2 Channel 1 interrupt
  DC32 DMA2_Channel2_irq               ; DMA 2 Channel 2 interrupt
  DC32 DMA2_Channel3_irq               ; DMA 2 Channel 3 interrupt
  DC32 DMA2_Channel4_5_irq             ; DMA 2 Channel 4 & 5 interrupt
  DC32 ETH_irq
  DC32 ETH_WKUP_irq
  DC32 CAN2_TX_irq
  DC32 CAN2_RX0_irq
  DC32 CAN2_RX1_irq
  DC32 CAN2_SCE_irq
  DC32 OTG_FS_irq

  THUMB

  PUBWEAK UnHandledInterrupt
  SECTION .text:CODE:REORDER(1)
UnHandledInterrupt
NMIException
MemManageException
BusFaultException
UsageFaultException
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
;         MRS     R5,PSP                  ; Read PSP
;         LDR     R6,[R5,#24]             ; Read Saved PC from Stack
;         LDR     R7,[R5,#20]             ; Read Saved LR from Stack


; TODO: Break here if debug image or loop infinitely if not?
;       Break if debugging
         BKPT  0; Hard Fault!!! - probably bad pointers
         BX LR ; step here to return to program location - don't expect things to run though


  END
