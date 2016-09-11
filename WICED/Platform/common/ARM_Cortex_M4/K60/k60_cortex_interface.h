/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/**
 * \brief Configuration of the Cortex-M4 Processor and Core Peripherals
 */

#define __CM4_REV              0x0000 /**< K60 core revision number ([15:8] revision number, [7:0] patch number) */
#define __MPU_PRESENT          0      /**< K60 does provide a MPU */
#define __FPU_PRESENT          0      /**< K60 does not provide a FPU */
#define __NVIC_PRIO_BITS       4      /**< K60 uses 4 Bits for the Priority Levels */
#define __Vendor_SysTickConfig 0      /**< Set to 1 if different SysTick Config is used */
/*! \} */ /* end of group Interrupt_vector_numbers */

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum IRQn
{
/******  Cortex-M4 Processor Exceptions Numbers ******************************/
  NonMaskableInt_IRQn       = -14, /**<  2 Non Maskable Interrupt                */
  MemoryManagement_IRQn     = -12, /**<  4 Cortex-M4 Memory Management Interrupt */
  BusFault_IRQn             = -11, /**<  5 Cortex-M4 Bus Fault Interrupt         */
  UsageFault_IRQn           = -10, /**<  6 Cortex-M4 Usage Fault Interrupt       */
  SVCall_IRQn               = -5,  /**< 11 Cortex-M4 SV Call Interrupt           */
  DebugMonitor_IRQn         = -4,  /**< 12 Cortex-M4 Debug Monitor Interrupt     */
  PendSV_IRQn               = -2,  /**< 14 Cortex-M4 Pend SV Interrupt           */
  SysTick_IRQn              = -1,  /**< 15 Cortex-M4 System Tick Interrupt       */
/******  K60 specific Interrupt Numbers *********************************/
  DMA0_IRQn                 =  0,  // 0x0000_0040 16     0     DMA              DMA Channel 0 transfer complete
  DMA1_IRQn                 =  1,  // 0x0000_0044 17     1     DMA              DMA Channel 1 transfer complete
  DMA2_IRQn                 =  2,  // 0x0000_0048 18     2     DMA              DMA Channel 2 transfer complete
  DMA3_IRQn                 =  3,  // 0x0000_004C 19     3     DMA              DMA Channel 3 transfer complete
  DMA4_IRQn                 =  4,  // 0x0000_0050 20     4     DMA              DMA Channel 4 transfer complete
  DMA5_IRQn                 =  5,  // 0x0000_0054 21     5     DMA              DMA Channel 5 transfer complete
  DMA6_IRQn                 =  6,  // 0x0000_0058 22     6     DMA              DMA Channel 6 transfer complete
  DMA7_IRQn                 =  7,  // 0x0000_005C 23     7     DMA              DMA Channel 7 transfer complete
  DMA8_IRQn                 =  8,  // 0x0000_0060 24     8     DMA              DMA Channel 8 transfer complete
  DMA9_IRQn                 =  9,  // 0x0000_0064 25     9     DMA              DMA Channel 9 transfer complete
  DMA10_IRQn                = 10,  // 0x0000_0068 26    10     DMA              DMA Channel 10 transfer complete
  DMA11_IRQn                = 11,  // 0x0000_006C 27    11     DMA              DMA Channel 11 transfer complete
  DMA12_IRQn                = 12,  // 0x0000_0070 28    12     DMA              DMA Channel 12 transfer complete
  DMA13_IRQn                = 13,  // 0x0000_0074 29    13     DMA              DMA Channel 13 transfer complete
  DMA14_IRQn                = 14,  // 0x0000_0078 30    14     DMA              DMA Channel 14 transfer complete
  DMA15_IRQn                = 15,  // 0x0000_007C 31    15     DMA              DMA Channel 15 transfer complete
  DMA_Error_IRQn            = 16,  // 0x0000_0080 32    16     DMA              DMA Error Interrupt Channels 0-15
  MCM_IRQn                  = 17,  // 0x0000_0084 33    17     MCM              Normal interrupt
  FLASH_Cmd_Complete_IRQn   = 18,  // 0x0000_0088 34    18     Flash memory     Command Complete
  FLASH_Read_Collision_IRQn = 19,  // 0x0000_008C 35    19     Flash memory     Read Collision
  SMC_IRQn                  = 20,  // 0x0000_0090 36    20     Mode Controller  Low Voltage Detect,Low Voltage Warning, Low Leakage Wakeup
  LLWU_IRQn                 = 21,  // 0x0000_0094 37    21     LLWU
  WDOG_IRQn                 = 22,  // 0x0000_0098 38    22     WDOG
  RNGB_IRQn                 = 23,  // 0x0000_009C 39    23     RNGB
  I2C0_IRQn                 = 24,  // 0x0000_00A0 40    24     I2C0
  I2C1_IRQn                 = 25,  // 0x0000_00A4 41    25     I2C1
  SPI0_IRQn                 = 26,  // 0x0000_00A8 42    26     SPI0             Single interrupt vector for all sources
  SPI1_IRQn                 = 27,  // 0x0000_00AC 43    27     SPI1             Single interrupt vector for all sources
  SPI2_IRQn                 = 28,  // 0x0000_00B0 44    28     SPI2             Single interrupt vector for all sources
  CAN0_Message_Buffer_IRQn  = 29,  // 0x0000_00B4 45    29     CAN0             OR'ed Message buffer (0-15)
  CAN0_Bus_Off_IRQn         = 30,  // 0x0000_00B8 46    30     CAN0             Bus Off
  CAN0_Error_IRQn           = 31,  // 0x0000_00BC 47    31     CAN0             Error
  CAN0_Transmit_IRQn        = 32,  // 0x0000_00C0 48    32     CAN0             Transmit Warning
  CAN0_Receive_IRQn         = 33,  // 0x0000_00C4 49    33     CAN0             Receive Warning
  CAN0_Wake_Up_IRQn         = 34,  // 0x0000_00C8 50    34     CAN0             Wake Up
  CAN0_IMEU_IRQn            = 35,  // 0x0000_00CC 51    35     CAN0             Individual Matching Elements Update (IMEU)
  CAN0_Lost_Receive_IRQn    = 36,  // 0x0000_00D0 52    36     CAN0             Lost receive
  CAN1_Message_Buffer_IRQn  = 37,  // 0x0000_00D4 53    37     CAN1             OR'ed Message buffer (0-15)
  CAN1_Bus_Off_IRQn         = 38,  // 0x0000_00D8 54    38     CAN1             Bus off
  CAN1_Error_IRQn           = 39,  // 0x0000_00DC 55    39     CAN1             Error
  CAN1_Transmit_IRQn        = 40,  // 0x0000_00E0 56    40     CAN1             Transmit Warning
  CAN1_Receive_IRQn         = 41,  // 0x0000_00E4 57    41     CAN1             Receive Warning
  CAN1_Wake_Up_IRQn         = 42,  // 0x0000_00E8 58    42     CAN1             Wake Up
  CAN1_IMEU_IRQn            = 43,  // 0x0000_00EC 59    43     CAN1             Individual Matching Elements Update (IMEU)
  CAN1_Lost_Receive_IRQn    = 44,  // 0x0000_00F0 60    44     CAN1             Lost receive
  UART0_Status_IRQn         = 45,  // 0x0000_00F4 61    45     UART0            Single interrupt vector for UART status sources
  UART0_Error_IRQn          = 46,  // 0x0000_00F8 62    46     UART0            Single interrupt vector for UART error sources
  UART1_Status_IRQn         = 47,  // 0x0000_00FC 63    47     UART1            Single interrupt vector for UART status sources
  UART1_Error_IRQn          = 48,  // 0x0000_0100 64    48     UART1            Single interrupt vector for UART error sources
  UART2_Status_IRQn         = 49,  // 0x0000_0104 65    49     UART2            Single interrupt vector for UART status sources
  UART2_Error_IRQn          = 50,  // 0x0000_0108 66    50     UART2            Single interrupt vector for UART error sources
  UART3_Status_IRQn         = 51,  // 0x0000_010C 67    51     UART3            Single interrupt vector for UART status sources
  UART3_Error_IRQn          = 52,  // 0x0000_0110 68    52     UART3            Single interrupt vector for UART error sources
  UART4_Status_IRQn         = 53,  // 0x0000_0114 69    53     UART4            Single interrupt vector for UART status sources
  UART4_Error_IRQn          = 54,  // 0x0000_0118 70    54     UART4            Single interrupt vector for UART error sources
  UART5_Status_IRQn         = 55,  // 0x0000_011C 71    55     UART5            Single interrupt vector for UART status sources
  UART5_Error_IRQn          = 56,  // 0x0000_0120 72    56     UART5            Single interrupt vector for UART error sources
  ADC0_IRQn                 = 57,  // 0x0000_0124 73    57     ADC0
  ADC1_IRQn                 = 58,  // 0x0000_0128 74    58     ADC1
  CMP0_IRQn                 = 59,  // 0x0000_012C 75    59     CMP0             High-speed comparator
  CMP1_IRQn                 = 60,  // 0x0000_0130 76    60     CMP1
  CMP2_IRQn                 = 61,  // 0x0000_0134 77    61     CMP2
  FTM0_IRQn                 = 62,  // 0x0000_0138 78    62     FTM0             Single interrupt vector for all sources
  FTM1_IRQn                 = 63,  // 0x0000_013C 79    63     FTM1             Single interrupt vector for all sources
  FTM2_IRQn                 = 64,  // 0x0000_0140 80    64     FTM2             Single interrupt vector for all sources
  CMT_IRQn                  = 65,  // 0x0000_0144 81    65     CMT
  RTC_IRQn                  = 66,  // 0x0000_0148 82    66     RTC Timer interrupt
  PIT_Channel0_IRQn         = 68,  // 0x0000_0150 84    68     PIT Channel 0
  PIT_Channel1_IRQn         = 69,  // 0x0000_0154 85    69     PIT Channel 1
  PIT_Channel2_IRQn         = 70,  // 0x0000_0158 86    70     PIT Channel 2
  PIT_Channel3_IRQn         = 71,  // 0x0000_015C 87    71     PIT Channel 3
  PDB_IRQn                  = 72,  // 0x0000_0160 88    72     PDB
  USB_OTG_IRQn              = 73,  // 0x0000_0164 89    73     USB OTG
  USB_Charger_Detect_IRQn   = 74,  // 0x0000_0168 90    74     USB Charger Detect
  ENET_Timer_IRQn           = 75,  // 0x0000_016C 91    75     ENET              IEEE 1588 Timer interrupt
  ENET_Transmit_IRQn        = 76,  // 0x0000_0170 92    76     ENET              Transmit interrupt
  ENET_Receive_IRQn         = 77,  // 0x0000_0174 93    77     ENET              Receive interrupt
  ENET_Error_IRQn           = 78,  // 0x0000_0178 94    78     ENET              Error and miscellaneous interrupt
  I2S_IRQn                  = 79,  // 0x0000_017C 95    79     I2S
  SDHC_IRQn                 = 80,  // 0x0000_0180 96    80     SDHC
  DAC0_IRQn                 = 81,  // 0x0000_0184 97    81     DAC0
  DAC1_IRQn                 = 82,  // 0x0000_0188 98    82     DAC1
  TSI_IRQn                  = 83,  // 0x0000_018C 99    83     TSI               Single interrupt vector for all sources
  MCG_IRQn                  = 84,  // 0x0000_0190 100   84     MCG
  LPTMR_IRQn                = 85,  // 0x0000_0194 101   85     Low Power Timer
  PortA_IRQn                = 87,  // 0x0000_019C 103   87     Port control module Pin Detect (Port A)
  PortB_IRQn                = 88,  // 0x0000_01A0 104   88     Port control module Pin Detect (Port B)
  PortC_IRQn                = 89,  // 0x0000_01A4 105   89     Port control module Pin Detect (Port C)
  PortD_IRQn                = 90,  // 0x0000_01A8 106   90     Port control module Pin Detect (Port D)
  PortE_IRQn                = 91,  // 0x0000_01AC 107   91     Port control module Pin Detect (Port E)
  SWI_IRQn                  = 94,  // 0x0000_01B8 110   94     Software Interrupt
} IRQn_Type;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
