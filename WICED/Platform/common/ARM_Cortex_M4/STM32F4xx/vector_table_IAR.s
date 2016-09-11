;
;  Copyright 2012, Broadcom Corporation
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
  PUBWEAK TAMP_STAMP_irq
  PUBWEAK RTC_WKUP_irq
  PUBWEAK FLASH_irq
  PUBWEAK RCC_irq
  PUBWEAK EXTI0_irq
  PUBWEAK EXTI1_irq
  PUBWEAK EXTI2_irq
  PUBWEAK EXTI3_irq
  PUBWEAK EXTI4_irq
  PUBWEAK DMA1_Stream0_irq
  PUBWEAK DMA1_Stream1_irq
  PUBWEAK DMA1_Stream2_irq
  PUBWEAK DMA1_Stream3_irq
  PUBWEAK DMA1_Stream4_irq
  PUBWEAK DMA1_Stream5_irq
  PUBWEAK DMA1_Stream6_irq
  PUBWEAK ADC_irq
  PUBWEAK CAN1_TX_irq
  PUBWEAK CAN1_RX0_irq
  PUBWEAK CAN1_RX1_irq
  PUBWEAK CAN1_SCE_irq
  PUBWEAK EXTI9_5_irq
  PUBWEAK TIM1_BRK_TIM9_irq
  PUBWEAK TIM1_UP_TIM10_irq
  PUBWEAK TIM1_TRG_COM_TIM11_irq
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
  PUBWEAK RTC_Alarm_irq
  PUBWEAK OTG_FS_WKUP_irq
  PUBWEAK TIM8_BRK_TIM12_irq
  PUBWEAK TIM8_UP_TIM13_irq
  PUBWEAK TIM8_TRG_COM_TIM14_irq
  PUBWEAK TIM8_CC_irq
  PUBWEAK DMA1_Stream7_irq
  PUBWEAK FSMC_irq
  PUBWEAK SDIO_irq
  PUBWEAK TIM5_irq
  PUBWEAK SPI3_irq
  PUBWEAK UART4_irq
  PUBWEAK UART5_irq
  PUBWEAK TIM6_DAC_irq
  PUBWEAK TIM7_irq
  PUBWEAK DMA2_Stream0_irq
  PUBWEAK DMA2_Stream1_irq
  PUBWEAK DMA2_Stream2_irq
  PUBWEAK DMA2_Stream3_irq
  PUBWEAK DMA2_Stream4_irq
  PUBWEAK ETH_irq
  PUBWEAK ETH_WKUP_irq
  PUBWEAK CAN2_TX_irq
  PUBWEAK CAN2_RX0_irq
  PUBWEAK CAN2_RX1_irq
  PUBWEAK CAN2_SCE_irq
  PUBWEAK OTG_FS_irq
  PUBWEAK DMA2_Stream5_irq
  PUBWEAK DMA2_Stream6_irq
  PUBWEAK DMA2_Stream7_irq
  PUBWEAK USART6_irq
  PUBWEAK I2C3_EV_irq
  PUBWEAK I2C3_ER_irq
  PUBWEAK OTG_HS_EP1_OUT_irq
  PUBWEAK OTG_HS_EP1_IN_irq
  PUBWEAK OTG_HS_WKUP_irq
  PUBWEAK OTG_HS_irq
  PUBWEAK DCMI_irq
  PUBWEAK CRYP_irq
  PUBWEAK HASH_RNG_irq



  PUBLIC _vectors
  PUBLIC __vector_table
  DATA
_vectors
__vector_table
  DC32 sfe(CSTACK)                     ; Initial stack location
  DC32 __iar_program_start             ; Reset vector
  DC32 NMIException                       ; Non Maskable Interrupt
  DC32 HardFaultException                 ; Hard Fault interrupt
  DC32 MemManageException                 ; Memory Management Fault interrupt
  DC32 BusFaultException                  ; Bus Fault interrupt
  DC32 UsageFaultException                ; Usage Fault interrupt
  DC32 0 ; Reserved
  DC32 0 ; Reserved
  DC32 0 ; Reserved
  DC32 0 ; Reserved
  DC32 SVC_irq                            ; SVC interrupt
  DC32 DebugMonitor                       ; Debug Monitor interrupt
  DC32 0 ; Reserved
  DC32 PENDSV_irq                         ; PendSV interrupt
  DC32 SYSTICK_irq                        ; Sys Tick Interrupt
  DC32 WWDG_irq                    ; Window WatchDog
  DC32 PVD_irq                     ; PVD through EXTI Line detection
  DC32 TAMP_STAMP_irq              ; Tamper and TimeStamps through the EXTI line
  DC32 RTC_WKUP_irq                ; RTC Wakeup through the EXTI line
  DC32 FLASH_irq                   ; FLASH
  DC32 RCC_irq                     ; RCC
  DC32 EXTI0_irq                   ; EXTI Line0
  DC32 EXTI1_irq                   ; EXTI Line1
  DC32 EXTI2_irq                   ; EXTI Line2
  DC32 EXTI3_irq                   ; EXTI Line3
  DC32 EXTI4_irq                   ; EXTI Line4
  DC32 DMA1_Stream0_irq            ; DMA1 Stream 0
  DC32 DMA1_Stream1_irq            ; DMA1 Stream 1
  DC32 DMA1_Stream2_irq            ; DMA1 Stream 2
  DC32 DMA1_Stream3_irq            ; DMA1 Stream 3
  DC32 DMA1_Stream4_irq            ; DMA1 Stream 4
  DC32 DMA1_Stream5_irq            ; DMA1 Stream 5
  DC32 DMA1_Stream6_irq            ; DMA1 Stream 6
  DC32 ADC_irq                     ; ADC1, ADC2 and ADC3s
  DC32 CAN1_TX_irq                 ; CAN1 TX
  DC32 CAN1_RX0_irq                ; CAN1 RX0
  DC32 CAN1_RX1_irq                ; CAN1 RX1
  DC32 CAN1_SCE_irq                ; CAN1 SCE
  DC32 EXTI9_5_irq                 ; External Line[9:5]s
  DC32 TIM1_BRK_TIM9_irq           ; TIM1 Break and TIM9
  DC32 TIM1_UP_TIM10_irq           ; TIM1 Update and TIM10
  DC32 TIM1_TRG_COM_TIM11_irq      ; TIM1 Trigger and Commutation and TIM11
  DC32 TIM1_CC_irq                 ; TIM1 Capture Compare
  DC32 TIM2_irq                    ; TIM2
  DC32 TIM3_irq                    ; TIM3
  DC32 TIM4_irq                    ; TIM4
  DC32 I2C1_EV_irq                 ; I2C1 Event
  DC32 I2C1_ER_irq                 ; I2C1 Error
  DC32 I2C2_EV_irq                 ; I2C2 Event
  DC32 I2C2_ER_irq                 ; I2C2 Error
  DC32 SPI1_irq                    ; SPI1
  DC32 SPI2_irq                    ; SPI2
  DC32 USART1_irq                  ; USART1
  DC32 USART2_irq                  ; USART2
  DC32 USART3_irq                  ; USART3
  DC32 EXTI15_10_irq               ; External Line[15:10]s
  DC32 RTC_Alarm_irq               ; RTC Alarm (A and B) through EXTI Line
  DC32 OTG_FS_WKUP_irq             ; USB OTG FS Wakeup through EXTI line
  DC32 TIM8_BRK_TIM12_irq          ; TIM8 Break and TIM12
  DC32 TIM8_UP_TIM13_irq           ; TIM8 Update and TIM13
  DC32 TIM8_TRG_COM_TIM14_irq      ; TIM8 Trigger and Commutation and TIM14
  DC32 TIM8_CC_irq                 ; TIM8 Capture Compare
  DC32 DMA1_Stream7_irq            ; DMA1 Stream7
  DC32 FSMC_irq                    ; FSMC
  DC32 SDIO_irq                    ; SDIO
  DC32 TIM5_irq                    ; TIM5
  DC32 SPI3_irq                    ; SPI3
  DC32 UART4_irq                   ; UART4
  DC32 UART5_irq                   ; UART5
  DC32 TIM6_DAC_irq                ; TIM6 and DAC1&2 underrun errors
  DC32 TIM7_irq                    ; TIM7
  DC32 DMA2_Stream0_irq            ; DMA2 Stream 0
  DC32 DMA2_Stream1_irq            ; DMA2 Stream 1
  DC32 DMA2_Stream2_irq            ; DMA2 Stream 2
  DC32 DMA2_Stream3_irq            ; DMA2 Stream 3
  DC32 DMA2_Stream4_irq            ; DMA2 Stream 4
  DC32 ETH_irq                     ; Ethernet
  DC32 ETH_WKUP_irq                ; Ethernet Wakeup through EXTI line
  DC32 CAN2_TX_irq                 ; CAN2 TX
  DC32 CAN2_RX0_irq                ; CAN2 RX0
  DC32 CAN2_RX1_irq                ; CAN2 RX1
  DC32 CAN2_SCE_irq                ; CAN2 SCE
  DC32 OTG_FS_irq                  ; USB OTG FS
  DC32 DMA2_Stream5_irq            ; DMA2 Stream 5
  DC32 DMA2_Stream6_irq            ; DMA2 Stream 6
  DC32 DMA2_Stream7_irq            ; DMA2 Stream 7
  DC32 USART6_irq                  ; USART6
  DC32 I2C3_EV_irq                 ; I2C3 event
  DC32 I2C3_ER_irq                 ; I2C3 error
  DC32 OTG_HS_EP1_OUT_irq          ; USB OTG HS End Point 1 Out
  DC32 OTG_HS_EP1_IN_irq           ; USB OTG HS End Point 1 In
  DC32 OTG_HS_WKUP_irq             ; USB OTG HS Wakeup through EXTI
  DC32 OTG_HS_irq                  ; USB OTG HS
  DC32 DCMI_irq                    ; DCMI
  DC32 CRYP_irq                    ; CRYP crypto
  DC32 HASH_RNG_irq                ; Hash and Rng


  THUMB

  PUBWEAK UnhandledInterrupt
  SECTION .text:CODE:REORDER(1)
UnhandledInterrupt
reset_handler
NMIException
MemManageException
BusFaultException
UsageFaultException
SVC_irq
DebugMonitor
PENDSV_irq
SYSTICK_irq
WWDG_irq
PVD_irq
TAMP_STAMP_irq
RTC_WKUP_irq
FLASH_irq
RCC_irq
EXTI0_irq
EXTI1_irq
EXTI2_irq
EXTI3_irq
EXTI4_irq
DMA1_Stream0_irq
DMA1_Stream1_irq
DMA1_Stream2_irq
DMA1_Stream3_irq
DMA1_Stream4_irq
DMA1_Stream5_irq
DMA1_Stream6_irq
ADC_irq
CAN1_TX_irq
CAN1_RX0_irq
CAN1_RX1_irq
CAN1_SCE_irq
EXTI9_5_irq
TIM1_BRK_TIM9_irq
TIM1_UP_TIM10_irq
TIM1_TRG_COM_TIM11_irq
TIM1_CC_irq
TIM2_irq
TIM3_irq
TIM4_irq
I2C1_EV_irq
I2C1_ER_irq
I2C2_EV_irq
I2C2_ER_irq
SPI1_irq
SPI2_irq
USART1_irq
USART2_irq
USART3_irq
EXTI15_10_irq
RTC_Alarm_irq
OTG_FS_WKUP_irq
TIM8_BRK_TIM12_irq
TIM8_UP_TIM13_irq
TIM8_TRG_COM_TIM14_irq
TIM8_CC_irq
DMA1_Stream7_irq
FSMC_irq
SDIO_irq
TIM5_irq
SPI3_irq
UART4_irq
UART5_irq
TIM6_DAC_irq
TIM7_irq
DMA2_Stream0_irq
DMA2_Stream1_irq
DMA2_Stream2_irq
DMA2_Stream3_irq
DMA2_Stream4_irq
ETH_irq
ETH_WKUP_irq
CAN2_TX_irq
CAN2_RX0_irq
CAN2_RX1_irq
CAN2_SCE_irq
OTG_FS_irq
DMA2_Stream5_irq
DMA2_Stream6_irq
DMA2_Stream7_irq
USART6_irq
I2C3_EV_irq
I2C3_ER_irq
OTG_HS_EP1_OUT_irq
OTG_HS_EP1_IN_irq
OTG_HS_WKUP_irq
OTG_HS_irq
DCMI_irq
CRYP_irq
HASH_RNG_irq
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
