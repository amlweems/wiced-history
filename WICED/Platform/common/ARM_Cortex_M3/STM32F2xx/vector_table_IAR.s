;
;  Copyright 2014, Broadcom Corporation
; All Rights Reserved.
;
; This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
; the contents of this file may not be disclosed to third parties, copied
; or duplicated in any form, in whole or in part, without the prior
; written permission of Broadcom Corporation.
;

		MODULE  ?STM32F2xx_vector_table_IAR
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
  /*----------------------------------------------------------*/
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
  PUBWEAK WWDG_irq
  /*---------------------------------------------------------*/
  PUBWEAK PVD_irq
  /*---------------------------------------------------------*/
  PUBWEAK TAMP_STAMP_irq
  /*---------------------------------------------------------*/
  PUBWEAK RTC_WKUP_irq
  /*---------------------------------------------------------*/
  PUBWEAK FLASH_irq
  /*---------------------------------------------------------*/
  PUBWEAK RCC_irq
  /*---------------------------------------------------------*/
  /* previously it was EXTI0_irq - EXTI4_irq, EXTI9_5_irq and EXTI15_10_irq */
  #ifdef FreeRTOS
  PUBWEAK gpio_irq						; Ext0-4 interrupts use gpio_irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  EXTERN gpio_rtos_irq
  #else /* ThreadX */
  PUBWEAK gpio_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  PUBWEAK DMA1_Stream0_irq
  /*---------------------------------------------------------*/
  PUBWEAK DMA1_Stream1_irq
  /*---------------------------------------------------------*/
  PUBWEAK DMA1_Stream2_irq
  /*---------------------------------------------------------*/
  /* previously was DMA1_Stream3_irq */
  //#ifdef FreeRTOS
  //PUBWEAK dma_irq
  //#else /* FreeRTOS */
  //#ifdef ThreadX
  //EXTERN dma_rtos_irq
  //#else /* ThreadX */
  //PUBWEAK dma_irq
  //#endif /* ThreadX */
  //#endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  PUBWEAK DMA1_Stream4_irq
  /*---------------------------------------------------------*/
  /* previously was DMA1_Stream5_irq */
  #ifdef FreeRTOS
  PUBWEAK usart2_rx_dma_irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  EXTERN usart2_rx_dma_rtos_irq
  #else /* ThreadX */
  PUBWEAK usart2_rx_dma_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously was DMA1_Stream6_irq */
  #ifdef FreeRTOS
  PUBWEAK usart2_tx_dma_irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  EXTERN usart2_tx_dma_rtos_irq
  #else /* ThreadX */
  PUBWEAK usart2_tx_dma_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  PUBWEAK ADC_irq
  /*---------------------------------------------------------*/
  PUBWEAK CAN1_TX_irq
  /*---------------------------------------------------------*/
  PUBWEAK CAN1_RX0_irq
  /*---------------------------------------------------------*/
  PUBWEAK CAN1_RX1_irq
  /*---------------------------------------------------------*/
  PUBWEAK CAN1_SCE_irq
  /*---------------------------------------------------------*/
  PUBWEAK TIM1_BRK_TIM9_irq
  /*---------------------------------------------------------*/
  PUBWEAK TIM1_UP_TIM10_irq
  /*---------------------------------------------------------*/
  PUBWEAK TIM1_TRG_COM_TIM11_irq
  /*---------------------------------------------------------*/
  PUBWEAK TIM1_CC_irq
  /*---------------------------------------------------------*/
  PUBWEAK TIM2_irq
  /*---------------------------------------------------------*/
  PUBWEAK TIM3_irq
  /*---------------------------------------------------------*/
  PUBWEAK TIM4_irq
  /*---------------------------------------------------------*/
  PUBWEAK I2C1_EV_irq
  /*---------------------------------------------------------*/
  PUBWEAK I2C1_ER_irq
  /*---------------------------------------------------------*/
  PUBWEAK I2C2_EV_irq
  /*---------------------------------------------------------*/
  PUBWEAK I2C2_ER_irq
  /*---------------------------------------------------------*/
  PUBWEAK SPI1_irq
  /*---------------------------------------------------------*/
  PUBWEAK SPI2_irq
  /*---------------------------------------------------------*/
  /* previously it was USART1_irq */
  #ifdef FreeRTOS
  PUBWEAK usart1_irq				  ; ; USART1 irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  EXTERN usart1_rtos_irq
  #else /* ThreadX */
  PUBWEAK usart1_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously it was USART2_irq */
  #ifdef FreeRTOS
  PUBWEAK usart2_irq				   ; USART2 irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  EXTERN usart2_rtos_irq
  #else /* FreeRTOS */
  PUBWEAK usart2_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  PUBWEAK USART3_irq
  /*---------------------------------------------------------*/
  PUBWEAK RTC_Alarm_irq
  /*---------------------------------------------------------*/
  PUBWEAK OTG_FS_WKUP_irq
  /*---------------------------------------------------------*/
  PUBWEAK TIM8_BRK_TIM12_irq
  /*---------------------------------------------------------*/
  PUBWEAK TIM8_UP_TIM13_irq
  /*---------------------------------------------------------*/
  PUBWEAK TIM8_TRG_COM_TIM14_irq
  /*---------------------------------------------------------*/
  PUBWEAK TIM8_CC_irq
  /*---------------------------------------------------------*/
  PUBWEAK DMA1_Stream7_irq
  /*---------------------------------------------------------*/
  PUBWEAK FSMC_irq
  /*---------------------------------------------------------*/
  #ifdef FreeRTOS
  PUBWEAK sdio_irq						; SDIO interrupt
  #else /* FreeRTOS */
  #ifdef ThreadX
  #ifdef SDIO
  /* build for SDIO bus, have to use sdio_rto_irq handler from threadx handlers */
  EXTERN sdio_rtos_irq
  #else /* #ifdef SDIO */
  /* build for spi, fill with defualt handler */
  PUBWEAK sdio_irq
  #endif /* #ifdef SDIO */
  #else /* ThreadX */
  PUBWEAK sdio_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  PUBWEAK TIM5_irq
  /*---------------------------------------------------------*/
  PUBWEAK SPI3_irq
  /*---------------------------------------------------------*/
  PUBWEAK UART4_irq
  /*---------------------------------------------------------*/
  PUBWEAK UART5_irq
  /*---------------------------------------------------------*/
  PUBWEAK TIM6_DAC_irq
  /*---------------------------------------------------------*/
  #ifdef FreeRTOS
  PUBWEAK dbg_watchdog_irq				 ; TIM7 interrupt
  #else /* FreeRTOS */
  #ifdef ThreadX
  EXTERN dbg_watchdog_rtos_irq
  #else /* ThreadX */
  PUBWEAK dbg_watchdog_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  PUBWEAK DMA2_Stream0_irq
  /*---------------------------------------------------------*/
  /* previously was DMA2_Stream1_irq */
  #ifdef FreeRTOS
  PUBWEAK usart6_rx_dma_irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  EXTERN usart6_rx_dma_rtos_irq
  #else /* ThreadX */
  PUBWEAK usart6_rx_dma_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously was DMA2_Stream2_irq */
  #ifdef FreeRTOS
  PUBWEAK usart1_rx_dma_irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  EXTERN usart1_rx_dma_rtos_irq
  #else /* ThreadX */
  PUBWEAK usart1_rx_dma_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  #ifdef FreeRTOS
  PUBWEAK dma_irq						; DMA2_stream3 interrupt
  #else /* FreeRTOS */
  #ifdef ThreadX
  EXTERN dma_rtos_irq
  #else /* ThreadX */
  PUBWEAK dma_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  PUBWEAK DMA2_Stream4_irq
  /*---------------------------------------------------------*/
  PUBWEAK ETH_irq
  /*---------------------------------------------------------*/
  PUBWEAK ETH_WKUP_irq
  /*---------------------------------------------------------*/
  PUBWEAK CAN2_TX_irq
  /*---------------------------------------------------------*/
  PUBWEAK CAN2_RX0_irq
  /*---------------------------------------------------------*/
  PUBWEAK CAN2_RX1_irq
  /*---------------------------------------------------------*/
  PUBWEAK CAN2_SCE_irq
  /*---------------------------------------------------------*/
  PUBWEAK OTG_FS_irq
  /*---------------------------------------------------------*/
  PUBWEAK DMA2_Stream5_irq
  /*---------------------------------------------------------*/
  /* previously was DMA2_Stream6_irq */
  #ifdef FreeRTOS
  PUBWEAK usart6_tx_dma_irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  EXTERN usart6_tx_dma_rtos_irq
  #else /* ThreadX */
  PUBWEAK usart6_tx_dma_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously was  DMA2_Stream7_irq */
  #ifdef FreeRTOS
  PUBWEAK usart1_tx_dma_irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  EXTERN usart1_tx_dma_rtos_irq
  #else /* ThreadX */
  PUBWEAK usart1_tx_dma_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously was USART6_irq */
  #ifdef FreeRTOS
  PUBWEAK usart6_irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  EXTERN usart6_rtos_irq
  #else /* ThreadX */
  PUBWEAK usart6_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  PUBWEAK I2C3_EV_irq
  /*---------------------------------------------------------*/
  PUBWEAK I2C3_ER_irq
  /*---------------------------------------------------------*/
  PUBWEAK OTG_HS_EP1_OUT_irq
  /*---------------------------------------------------------*/
  PUBWEAK OTG_HS_EP1_IN_irq
  /*---------------------------------------------------------*/
  PUBWEAK OTG_HS_WKUP_irq
  /*---------------------------------------------------------*/
  PUBWEAK OTG_HS_irq
  /*---------------------------------------------------------*/
  PUBWEAK DCMI_irq
  /*---------------------------------------------------------*/
  PUBWEAK CRYP_irq
  /*---------------------------------------------------------*/
  PUBWEAK HASH_RNG_irq



  PUBLIC _vectors
  PUBLIC __vector_table
  DATA
_vectors
__vector_table
  DC32 sfe(CSTACK)                     ; Initial stack location
  DC32 wiced_program_start             ; Reset vector
  DC32 NMIException                    ; Non Maskable Interrupt
  DC32 HardFaultException              ; Hard Fault interrupt
  DC32 MemManageException              ; Memory Management Fault interrupt
  DC32 BusFaultException               ; Bus Fault interrupt
  DC32 UsageFaultException             ; Usage Fault interrupt
  DC32 0 ; Reserved
  DC32 0 ; Reserved
  DC32 0 ; Reserved
  DC32 0 ; Reserved
  /*---------------------------------------------------------*/
  #ifdef FreeRTOS
  DC32 vPortSVCHandler			   ; SVC interrupt
  #else /* #ifdef FreeRTOS */
  #ifdef ThreadX
  DC32 __tx_SVCallHandler
  #else /* #ifdef ThreadX */
  DC32 SVC_irq
  #endif /* #ifdef ThreadX */
  #endif /* #ifdef FreeRTOS */
  /*---------------------------------------------------------*/
  DC32 DebugMonitor                ; Debug Monitor interrupt
  DC32 0 						   ; Reserved
  /*---------------------------------------------------------*/
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
  #ifdef FreeRTOS
  DC32 xPortSysTickHandler		   ; Sys-tick interrupt
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 __tx_SysTickHandler
  #else /* ThreadX */
  DC32 SYSTICK_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  DC32 WWDG_irq                    ; Window WatchDog
  DC32 PVD_irq                     ; PVD through EXTI Line detection
  DC32 TAMP_STAMP_irq              ; Tamper and TimeStamps through the EXTI line
  DC32 RTC_WKUP_irq                ; RTC Wakeup through the EXTI line
  DC32 FLASH_irq                   ; FLASH
  DC32 RCC_irq                     ; RCC
  /*---------------------------------------------------------*/
  /* previously this vector was named EXT0_irq */
  #ifdef FreeRTOS
  DC32 gpio_irq		   ; Ext0 interrupt
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 gpio_rtos_irq
  #else /* ThreadX */
  DC32 gpio_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously this vector was named EXT1_irq */
  #ifdef FreeRTOS
  DC32 gpio_irq		   ; Ext1 interrupt
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 gpio_rtos_irq
  #else /* ThreadX */
  DC32 gpio_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously this vector was named EXT2_irq */
  #ifdef FreeRTOS
  DC32 gpio_irq		   ; Ext2 interrupt
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 gpio_rtos_irq
  #else /* ThreadX */
  DC32 gpio_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously this vector was named EXT3_irq */
  #ifdef FreeRTOS
  DC32 gpio_irq		   ; Ext3 interrupt
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 gpio_rtos_irq
  #else /* ThreadX */
  DC32 gpio_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously this vector was named EXT4_irq */
  #ifdef FreeRTOS
  DC32 gpio_irq		   ; Ext4 interrupt
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 gpio_rtos_irq
  #else /* ThreadX */
  DC32 gpio_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  DC32 DMA1_Stream0_irq            ; DMA1 Stream 0
  DC32 DMA1_Stream1_irq            ; DMA1 Stream 1
  DC32 DMA1_Stream2_irq            ; DMA1 Stream 2
  /*---------------------------------------------------------*/
  /* previoulsly was DMA1_Stream3_irq */
  #ifdef FreeRTOS
  DC32 dma_irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 dma_rtos_irq
  #else /* ThreadX */
  DC32 dma_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  DC32 DMA1_Stream4_irq            ; DMA1 Stream 4
  /*---------------------------------------------------------*/
  /* previously was DMA1_Stream5_irq */
  #ifdef FreeRTOS
  DC32 usart2_rx_dma_irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 usart2_rx_dma_rtos_irq
  #else /* FreeRTOS */
  DC32 usart2_rx_dma_irq
  #endif /* FreeRTOS */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously was DMA1_Stream6_irq */
  #ifdef FreeRTOS
  DC32 usart2_tx_dma_irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 usart2_tx_dma_rtos_irq
  #else /* ThreadX */
  DC32 usart2_tx_dma_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  DC32 ADC_irq                     ; ADC1, ADC2 and ADC3s
  DC32 CAN1_TX_irq                 ; CAN1 TX
  DC32 CAN1_RX0_irq                ; CAN1 RX0
  DC32 CAN1_RX1_irq                ; CAN1 RX1
  DC32 CAN1_SCE_irq                ; CAN1 SCE
  /*---------------------------------------------------------*/
  /* previously this vector was named EXT9_5_irq */
  #ifdef FreeRTOS
  DC32 gpio_irq		   ; Ext9-5 interrupt
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 gpio_rtos_irq
  #else /* ThreadX */
  DC32 gpio_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
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
  /*---------------------------------------------------------*/
  #ifdef FreeRTOS
  DC32 usart1_irq			   ; USART1
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 usart1_rtos_irq
  #else /* ThreadX */
  DC32 usart1_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  #ifdef FreeRTOS
  DC32 usart2_irq			   ; USART2
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 usart2_rtos_irq
  #else /* ThreadX */
  DC32 usart2_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  DC32 USART3_irq                  ; USART3
  /*---------------------------------------------------------*/
  /* previously this vector was named EXT15_10_irq */
  #ifdef FreeRTOS
  DC32 gpio_irq		   ; Ext15_10 interrupt
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 gpio_rtos_irq
  #else /* FreeRTOS */
  DC32 gpio_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  DC32 RTC_Alarm_irq               ; RTC Alarm (A and B) through EXTI Line
  DC32 OTG_FS_WKUP_irq             ; USB OTG FS Wakeup through EXTI line
  DC32 TIM8_BRK_TIM12_irq          ; TIM8 Break and TIM12
  DC32 TIM8_UP_TIM13_irq           ; TIM8 Update and TIM13
  DC32 TIM8_TRG_COM_TIM14_irq      ; TIM8 Trigger and Commutation and TIM14
  DC32 TIM8_CC_irq                 ; TIM8 Capture Compare
  DC32 DMA1_Stream7_irq            ; DMA1 Stream7
  DC32 FSMC_irq                    ; FSMC
  /*---------------------------------------------------------*/
  #ifdef FreeRTOS
  DC32 sdio_irq			   ; SDIO interrupt
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
  DC32 TIM5_irq                    ; TIM5
  DC32 SPI3_irq                    ; SPI3
  DC32 UART4_irq                   ; UART4
  DC32 UART5_irq                   ; UART5
  DC32 TIM6_DAC_irq                ; TIM6 and DAC1&2 underrun errors
  /*---------------------------------------------------------*/
  #ifdef FreeRTOS
  DC32 dbg_watchdog_irq	   ; TIM7 interrupt
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 dbg_watchdog_rtos_irq
  #else /* ThreadX */
  DC32 dbg_watchdog_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  DC32 DMA2_Stream0_irq            ; DMA2 Stream 0
  /*---------------------------------------------------------*/
  /* previously was DMA2_Stream1_irq */
  #ifdef FreeRTOS
  DC32 usart6_rx_dma_irq   ; DMA2 Stream 1
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 usart6_rx_dma_rtos_irq
  #else /* ThreadX */
  DC32 usart6_rx_dma_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously was DMA2_Stream2_irq */
  #ifdef FreeRTOS
  DC32 usart1_rx_dma_irq      ; DMA2_Stream 2
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 usart1_rx_dma_rtos_irq
  #else /* ThreadX */
  DC32 usart1_rx_dma_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously was DMA2_Stream3_irq */
  #ifdef FreeRTOS
  DC32 dma_irq				   ; DMA2_stream3
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 dma_rtos_irq
  #else /* ThreadX */
  DC32 dma_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  DC32 DMA2_Stream4_irq            ; DMA2 Stream 4
  DC32 ETH_irq                     ; Ethernet
  DC32 ETH_WKUP_irq                ; Ethernet Wakeup through EXTI line
  DC32 CAN2_TX_irq                 ; CAN2 TX
  DC32 CAN2_RX0_irq                ; CAN2 RX0
  DC32 CAN2_RX1_irq                ; CAN2 RX1
  DC32 CAN2_SCE_irq                ; CAN2 SCE
  DC32 OTG_FS_irq                  ; USB OTG FS
  DC32 DMA2_Stream5_irq            ; DMA2 Stream 5
  /*---------------------------------------------------------*/
  /* previously was DMA2_Stream6_irq */
  #ifdef FreeRTOS
  DC32 usart6_tx_dma_irq   ; DMA2 stream 6
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 usart6_tx_dma_rtos_irq
  #else /* ThreadX */
  DC32 usart6_tx_dma_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /*---------------------------------------------------------*/
  /* previously was  DMA2_Stream7_irq */
  #ifdef FreeRTOS
  DC32 usart1_tx_dma_irq
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 usart1_tx_dma_rtos_irq
  #else /* ThreadX */
  DC32 usart1_tx_dma_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
  /* previously was USART6_irq */
  #ifdef FreeRTOS
  DC32 usart6_irq		   ; USART6
  #else /* FreeRTOS */
  #ifdef ThreadX
  DC32 usart6_rtos_irq
  #else /* ThreadX */
  DC32 usart6_irq
  #endif /* ThreadX */
  #endif /* FreeRTOS */
  /*---------------------------------------------------------*/
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
/*---------------------------------------------------------*/
UnhandledInterrupt
/*---------------------------------------------------------*/
reset_handler
/*---------------------------------------------------------*/
NMIException
/*---------------------------------------------------------*/
MemManageException
/*---------------------------------------------------------*/
BusFaultException
/*---------------------------------------------------------*/
UsageFaultException
/*---------------------------------------------------------*/
SVC_irq
/*---------------------------------------------------------*/
DebugMonitor
/*---------------------------------------------------------*/
PENDSV_irq
/*---------------------------------------------------------*/
SYSTICK_irq
/*---------------------------------------------------------*/
WWDG_irq
/*---------------------------------------------------------*/
PVD_irq
/*---------------------------------------------------------*/
TAMP_STAMP_irq
/*---------------------------------------------------------*/
RTC_WKUP_irq
/*---------------------------------------------------------*/
FLASH_irq
/*---------------------------------------------------------*/
RCC_irq
/*---------------------------------------------------------*/
/* previously was EXT0_irq - Ext4_irq, replaced all */
/* ext0-ext4 interrupt handlers with gpio_irq */
gpio_irq
/*---------------------------------------------------------*/
DMA1_Stream0_irq
/*---------------------------------------------------------*/
DMA1_Stream1_irq
/*---------------------------------------------------------*/
DMA1_Stream2_irq
/*---------------------------------------------------------*/
DMA1_Stream3_irq
/*---------------------------------------------------------*/
DMA1_Stream4_irq
/*---------------------------------------------------------*/
/* previoulsly was DMA1_Stream5_irq */
usart2_rx_dma_irq
/*---------------------------------------------------------*/
/* previously was DMA1_Stream6_irq */
usart2_tx_dma_irq
/*---------------------------------------------------------*/
ADC_irq
/*---------------------------------------------------------*/
CAN1_TX_irq
/*---------------------------------------------------------*/
CAN1_RX0_irq
/*---------------------------------------------------------*/
CAN1_RX1_irq
/*---------------------------------------------------------*/
CAN1_SCE_irq
/*---------------------------------------------------------*/
EXTI9_5_irq
/*---------------------------------------------------------*/
TIM1_BRK_TIM9_irq
/*---------------------------------------------------------*/
TIM1_UP_TIM10_irq
/*---------------------------------------------------------*/
TIM1_TRG_COM_TIM11_irq
/*---------------------------------------------------------*/
TIM1_CC_irq
/*---------------------------------------------------------*/
TIM2_irq
/*---------------------------------------------------------*/
TIM3_irq
/*---------------------------------------------------------*/
TIM4_irq
/*---------------------------------------------------------*/
I2C1_EV_irq
/*---------------------------------------------------------*/
I2C1_ER_irq
/*---------------------------------------------------------*/
I2C2_EV_irq
/*---------------------------------------------------------*/
I2C2_ER_irq
/*---------------------------------------------------------*/
SPI1_irq
/*---------------------------------------------------------*/
SPI2_irq
/*---------------------------------------------------------*/
/* previously was USART1_irq */
usart1_irq
/*---------------------------------------------------------*/
/* previously was USART2_irq */
usart2_irq
/*---------------------------------------------------------*/
USART3_irq
/*---------------------------------------------------------*/
EXTI15_10_irq
/*---------------------------------------------------------*/
RTC_Alarm_irq
/*---------------------------------------------------------*/
OTG_FS_WKUP_irq
/*---------------------------------------------------------*/
TIM8_BRK_TIM12_irq
/*---------------------------------------------------------*/
TIM8_UP_TIM13_irq
/*---------------------------------------------------------*/
TIM8_TRG_COM_TIM14_irq
/*---------------------------------------------------------*/
TIM8_CC_irq
/*---------------------------------------------------------*/
DMA1_Stream7_irq
/*---------------------------------------------------------*/
FSMC_irq
/*---------------------------------------------------------*/
sdio_irq
/*---------------------------------------------------------*/
TIM5_irq
/*---------------------------------------------------------*/
SPI3_irq
/*---------------------------------------------------------*/
UART4_irq
/*---------------------------------------------------------*/
UART5_irq
/*---------------------------------------------------------*/
TIM6_DAC_irq
/*---------------------------------------------------------*/
/* previously was TIM7_irq */
dbg_watchdog_irq
/*---------------------------------------------------------*/
DMA2_Stream0_irq
/*---------------------------------------------------------*/
/* previously was DMA2_Stream1_irq */
usart6_rx_dma_irq
/*---------------------------------------------------------*/
/* previoulsy was DMA2_Stream2_irq */
usart1_rx_dma_irq
/*---------------------------------------------------------*/
/* previously was DMA2_Stream3_irq */
dma_irq
/*---------------------------------------------------------*/
DMA2_Stream4_irq
/*---------------------------------------------------------*/
ETH_irq
/*---------------------------------------------------------*/
ETH_WKUP_irq
/*---------------------------------------------------------*/
CAN2_TX_irq
/*---------------------------------------------------------*/
CAN2_RX0_irq
/*---------------------------------------------------------*/
CAN2_RX1_irq
/*---------------------------------------------------------*/
CAN2_SCE_irq
/*---------------------------------------------------------*/
OTG_FS_irq
/*---------------------------------------------------------*/
DMA2_Stream5_irq
/*---------------------------------------------------------*/
/* previously was DMA2_Stream6_irq */
usart6_tx_dma_irq
/*---------------------------------------------------------*/
/* previously was DMA2_Stream7_irq */
usart1_tx_dma_irq
/*---------------------------------------------------------*/
/* previously was USART6_irq */
usart6_irq
/*---------------------------------------------------------*/
I2C3_EV_irq
/*---------------------------------------------------------*/
I2C3_ER_irq
/*---------------------------------------------------------*/
OTG_HS_EP1_OUT_irq
/*---------------------------------------------------------*/
OTG_HS_EP1_IN_irq
/*---------------------------------------------------------*/
OTG_HS_WKUP_irq
/*---------------------------------------------------------*/
OTG_HS_irq
/*---------------------------------------------------------*/
DCMI_irq
/*---------------------------------------------------------*/
CRYP_irq
/*---------------------------------------------------------*/
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

