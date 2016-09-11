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
.word link_stack_end           @                          ARM core         Initial Supervisor SP
.word reset_handler            @ 0x0000_0004 1 -          ARM core         Initial Program Counter
.word NMIException     	       @ 0x0000_0008 2 -          ARM core         Non-maskable Interrupt (NMI)
.word HardFaultException       @ 0x0000_000C 3 -          ARM core         Hard Fault
.word MemManageException       @ 0x0000_0010 4 -		  ARM core        MemManage Fault
.word BusFaultException        @ 0x0000_0014 5 -          ARM core         Bus Fault
.word UsageFaultException      @ 0x0000_0018 6 -          ARM core         Usage Fault
.word UnhandledInterrupt       @ 0x0000_001C 7 -						   Reserved
.word UnhandledInterrupt       @ 0x0000_0020 8 -						   Reserved
.word UnhandledInterrupt       @ 0x0000_0024 9 -						   Reserved
.word UnhandledInterrupt       @ 0x0000_0028 10 -						   Reserved
.word SVC_irq     			   @ 0x0000_002C 11 -         ARM core         Supervisor call (SVCall)
.word DebugMonitor     		   @ 0x0000_0030 12 -         ARM core         Debug Monitor
.word UnhandledInterrupt       @ 0x0000_0034 13 -						   Reserved
.word PENDSV_irq  			   @ 0x0000_0038 14 -         ARM core         Pendable request for system service (PendableSrvReq)
.word SYSTICK_irq 			   @ 0x0000_003C 15 -         ARM core         System tick timer (SysTick)
.word DMA0_irq                 @ 0x0000_0040 16     0     DMA              DMA Channel 0 transfer complete
.word DMA1_irq                 @ 0x0000_0044 17     1     DMA              DMA Channel 1 transfer complete
.word DMA2_irq                 @ 0x0000_0048 18     2     DMA              DMA Channel 2 transfer complete
.word DMA3_irq                 @ 0x0000_004C 19     3     DMA              DMA Channel 3 transfer complete
.word DMA4_irq                 @ 0x0000_0050 20     4     DMA              DMA Channel 4 transfer complete
.word DMA5_irq                 @ 0x0000_0054 21     5     DMA              DMA Channel 5 transfer complete
.word DMA6_irq                 @ 0x0000_0058 22     6     DMA              DMA Channel 6 transfer complete
.word DMA7_irq                 @ 0x0000_005C 23     7     DMA              DMA Channel 7 transfer complete
.word DMA8_irq                 @ 0x0000_0060 24     8     DMA              DMA Channel 8 transfer complete
.word DMA9_irq                 @ 0x0000_0064 25     9     DMA              DMA Channel 9 transfer complete
.word DMA10_irq                @ 0x0000_0068 26    10     DMA              DMA Channel 10 transfer complete
.word DMA11_irq                @ 0x0000_006C 27    11     DMA              DMA Channel 11 transfer complete
.word DMA12_irq                @ 0x0000_0070 28    12     DMA              DMA Channel 12 transfer complete
.word DMA13_irq                @ 0x0000_0074 29    13     DMA              DMA Channel 13 transfer complete
.word DMA14_irq                @ 0x0000_0078 30    14     DMA              DMA Channel 14 transfer complete
.word DMA15_irq                @ 0x0000_007C 31    15     DMA              DMA Channel 15 transfer complete
.word DMA_Error_irq            @ 0x0000_0080 32    16     DMA              DMA Error Interrupt Channels 0-15
.word MCM_irq                  @ 0x0000_0084 33    17     MCM              Normal interrupt
.word FLASH_Cmd_Complete_irq   @ 0x0000_0088 34    18     Flash memory     Command Complete
.word FLASH_Read_Collision_irq @ 0x0000_008C 35    19     Flash memory     Read Collision
.word SMC_irq                  @ 0x0000_0090 36    20     Mode Controller  Low Voltage Detect,Low Voltage Warning, Low Leakage Wakeup
.word LLWU_irq                 @ 0x0000_0094 37    21     LLWU
.word WDOG_irq                 @ 0x0000_0098 38    22     WDOG
.word RNGB_irq                 @ 0x0000_009C 39    23	  RNGB
.word I2C0_irq                 @ 0x0000_00A0 40    24     I2C0
.word I2C1_irq                 @ 0x0000_00A4 41    25     I2C1
.word SPI0_irq                 @ 0x0000_00A8 42    26     SPI0             Single interrupt vector for all sources
.word SPI1_irq                 @ 0x0000_00AC 43    27     SPI1             Single interrupt vector for all sources
.word SPI2_irq                 @ 0x0000_00B0 44    28     SPI2             Single interrupt vector for all sources
.word CAN0_Message_Buffer_irq  @ 0x0000_00B4 45    29     CAN0             OR'ed Message buffer (0-15)
.word CAN0_Bus_Off_irq         @ 0x0000_00B8 46    30     CAN0             Bus Off
.word CAN0_Error_irq           @ 0x0000_00BC 47    31     CAN0             Error
.word CAN0_Transmit_irq        @ 0x0000_00C0 48    32     CAN0             Transmit Warning
.word CAN0_Receive_irq         @ 0x0000_00C4 49    33     CAN0             Receive Warning
.word CAN0_Wake_Up_irq         @ 0x0000_00C8 50    34     CAN0             Wake Up
.word CAN0_IMEU_irq            @ 0x0000_00CC 51    35     CAN0             Individual Matching Elements Update (IMEU)
.word CAN0_Lost_Receive_irq    @ 0x0000_00D0 52    36     CAN0             Lost receive
.word CAN1_Message_Buffer_irq  @ 0x0000_00D4 53    37     CAN1             OR'ed Message buffer (0-15)
.word CAN1_Bus_Off_irq         @ 0x0000_00D8 54    38     CAN1             Bus off
.word CAN1_Error_irq           @ 0x0000_00DC 55    39     CAN1             Error
.word CAN1_Transmit_irq        @ 0x0000_00E0 56    40     CAN1             Transmit Warning
.word CAN1_Receive_irq         @ 0x0000_00E4 57    41     CAN1             Receive Warning
.word CAN1_Wake_Up_irq         @ 0x0000_00E8 58    42     CAN1             Wake Up
.word CAN1_IMEU_irq            @ 0x0000_00EC 59    43     CAN1             Individual Matching Elements Update (IMEU)
.word CAN1_Lost_Receive_irq    @ 0x0000_00F0 60    44     CAN1             Lost receive
.word UART0_Status_irq         @ 0x0000_00F4 61    45     UART0            Single interrupt vector for UART status sources
.word UART0_Error_irq          @ 0x0000_00F8 62    46     UART0            Single interrupt vector for UART error sources
.word UART1_Status_irq         @ 0x0000_00FC 63    47     UART1            Single interrupt vector for UART status sources
.word UART1_Error_irq          @ 0x0000_0100 64    48     UART1            Single interrupt vector for UART error sources
.word UART2_Status_irq         @ 0x0000_0104 65    49     UART2            Single interrupt vector for UART status sources
.word UART2_Error_irq          @ 0x0000_0108 66    50     UART2            Single interrupt vector for UART error sources
.word UART3_Status_irq         @ 0x0000_010C 67    51     UART3            Single interrupt vector for UART status sources
.word UART3_Error_irq          @ 0x0000_0110 68    52     UART3            Single interrupt vector for UART error sources
.word UART4_Status_irq         @ 0x0000_0114 69    53     UART4            Single interrupt vector for UART status sources
.word UART4_Error_irq          @ 0x0000_0118 70    54     UART4            Single interrupt vector for UART error sources
.word UART5_Status_irq         @ 0x0000_011C 71    55     UART5            Single interrupt vector for UART status sources
.word UART5_Error_irq          @ 0x0000_0120 72    56     UART5            Single interrupt vector for UART error sources
.word ADC0_irq                 @ 0x0000_0124 73    57     ADC0
.word ADC1_irq                 @ 0x0000_0128 74    58     ADC1
.word CMP0_irq                 @ 0x0000_012C 75    59     CMP0             High-speed comparator
.word CMP1_irq                 @ 0x0000_0130 76    60     CMP1
.word CMP2_irq                 @ 0x0000_0134 77    61     CMP2
.word FTM0_irq                 @ 0x0000_0138 78    62     FTM0 			   Single interrupt vector for all sources
.word FTM1_irq                 @ 0x0000_013C 79    63     FTM1 			   Single interrupt vector for all sources
.word FTM2_irq                 @ 0x0000_0140 80    64     FTM2 			   Single interrupt vector for all sources
.word CMT_irq                  @ 0x0000_0144 81    65     CMT
.word RTC_irq                  @ 0x0000_0148 82    66     RTC Timer interrupt
.word UnhandledInterrupt       @ 0x0000_014C 83    67
.word PIT_Channel0_irq         @ 0x0000_0150 84    68     PIT Channel 0
.word PIT_Channel1_irq         @ 0x0000_0154 85    69     PIT Channel 1
.word PIT_Channel2_irq         @ 0x0000_0158 86    70     PIT Channel 2
.word PIT_Channel3_irq         @ 0x0000_015C 87    71     PIT Channel 3
.word PDB_irq                  @ 0x0000_0160 88    72     PDB
.word USB_OTG_irq              @ 0x0000_0164 89    73     USB OTG
.word USB_Charger_Detect_irq   @ 0x0000_0168 90    74     USB Charger Detect
.word ENET_Timer_irq           @ 0x0000_016C 91    75	  ENET			   IEEE 1588 Timer interrupt
.word ENET_Transmit_irq        @ 0x0000_0170 92    76	  ENET			   Transmit interrupt
.word ENET_Receive_irq         @ 0x0000_0174 93    77	  ENET			   Receive interrupt
.word ENET_Error_irq           @ 0x0000_0178 94    78	  ENET			   Error and miscellaneous interrupt
.word I2S_irq                  @ 0x0000_017C 95    79     I2S
.word SDHC_irq                 @ 0x0000_0180 96    80     SDHC
.word DAC0_irq                 @ 0x0000_0184 97    81     DAC0
.word DAC1_irq                 @ 0x0000_0188 98    82     DAC1
.word TSI_irq                  @ 0x0000_018C 99    83     TSI 			   Single interrupt vector for all sources
.word MCG_irq                  @ 0x0000_0190 100   84     MCG
.word LPTMR_irq                @ 0x0000_0194 101   85     Low Power Timer
.word UnhandledInterrupt       @ 0x0000_0198 102   86     Segment LCD 	   Single interrupt vector for all sources
.word PortA_irq                @ 0x0000_019C 103   87     Port control module Pin Detect (Port A)
.word PortB_irq                @ 0x0000_01A0 104   88     Port control module Pin Detect (Port B)
.word PortC_irq                @ 0x0000_01A4 105   89     Port control module Pin Detect (Port C)
.word PortD_irq                @ 0x0000_01A8 106   90     Port control module Pin Detect (Port D)
.word PortE_irq                @ 0x0000_01AC 107   91     Port control module Pin Detect (Port E)
.word UnhandledInterrupt       @ 0x0000_01B0 108   92
.word UnhandledInterrupt       @ 0x0000_01B4 109   93
.word SWI_irq                  @ 0x0000_01B8 110   94     Software Interrupt
.word UnhandledInterrupt       @ 0x0000_01BC 111   95
.word UnhandledInterrupt       @ 0x0000_01C0 112   96
.word UnhandledInterrupt       @ 0x0000_01C4 113   97
.word UnhandledInterrupt       @ 0x0000_01C8 114   98
.word UnhandledInterrupt       @ 0x0000_01CC 115   99
.word UnhandledInterrupt       @ 0x0000_01D0 116   100
.word UnhandledInterrupt       @ 0x0000_01D4 117   101
.word UnhandledInterrupt       @ 0x0000_01D8 118   102
.word UnhandledInterrupt       @ 0x0000_01DC 119   103
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word UnhandledInterrupt       @
.word 0xffffffff
.word 0xffffffff
.word 0xffffffff
.word 0xfffffffe

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
		.weak DMA0_irq
		.weak DMA1_irq
		.weak DMA2_irq
		.weak DMA3_irq
		.weak DMA4_irq
		.weak DMA5_irq
		.weak DMA6_irq
		.weak DMA7_irq
		.weak DMA8_irq
		.weak DMA9_irq
		.weak DMA10_irq
		.weak DMA11_irq
		.weak DMA12_irq
		.weak DMA13_irq
		.weak DMA14_irq
		.weak DMA15_irq
		.weak DMA_Error_irq
		.weak MCM_irq
		.weak FLASH_Cmd_Complete_irq
		.weak FLASH_Read_Collision_irq
		.weak SMC_irq
		.weak LLWU_irq
		.weak WDOG_irq
		.weak RNGB_irq
		.weak I2C0_irq
		.weak I2C1_irq
		.weak SPI0_irq
		.weak SPI1_irq
		.weak SPI2_irq
		.weak CAN0_Message_Buffer_irq
		.weak CAN0_Bus_Off_irq
		.weak CAN0_Error_irq
		.weak CAN0_Transmit_irq
		.weak CAN0_Receive_irq
		.weak CAN0_Wake_Up_irq
		.weak CAN0_IMEU_irq
		.weak CAN0_Lost_Receive_irq
		.weak CAN1_Message_Buffer_irq
		.weak CAN1_Bus_Off_irq
		.weak CAN1_Error_irq
		.weak CAN1_Transmit_irq
		.weak CAN1_Receive_irq
		.weak CAN1_Wake_Up_irq
		.weak CAN1_IMEU_irq
		.weak CAN1_Lost_Receive_irq
		.weak UART0_Status_irq
		.weak UART0_Error_irq
		.weak UART1_Status_irq
		.weak UART1_Error_irq
		.weak UART2_Status_irq
		.weak UART2_Error_irq
		.weak UART3_Status_irq
		.weak UART3_Error_irq
		.weak UART4_Status_irq
		.weak UART4_Error_irq
		.weak UART5_Status_irq
		.weak UART5_Error_irq
		.weak ADC0_irq
		.weak ADC1_irq
		.weak CMP0_irq
		.weak CMP1_irq
		.weak CMP2_irq
		.weak FTM0_irq
		.weak FTM1_irq
		.weak FTM2_irq
		.weak CMT_irq
		.weak RTC_irq
		.weak PIT_Channel0_irq
		.weak PIT_Channel1_irq
		.weak PIT_Channel2_irq
		.weak PIT_Channel3_irq
		.weak PDB_irq
		.weak USB_OTG_irq
		.weak USB_Charger_Detect_irq
		.weak ENET_Timer_irq
		.weak ENET_Transmit_irq
		.weak ENET_Receive_irq
		.weak ENET_Error_irq
		.weak I2S_irq
		.weak SDHC_irq
		.weak DAC0_irq
		.weak DAC1_irq
		.weak TSI_irq
		.weak MCG_irq
		.weak LPTMR_irq
		.weak PortA_irq
		.weak PortB_irq
		.weak PortC_irq
		.weak PortD_irq
		.weak PortE_irq
		.weak SWI_irq
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
DMA0_irq:
DMA1_irq:
DMA2_irq:
DMA3_irq:
DMA4_irq:
DMA5_irq:
DMA6_irq:
DMA7_irq:
DMA8_irq:
DMA9_irq:
DMA10_irq:
DMA11_irq:
DMA12_irq:
DMA13_irq:
DMA14_irq:
DMA15_irq:
DMA_Error_irq:
MCM_irq:
FLASH_Cmd_Complete_irq:
FLASH_Read_Collision_irq:
SMC_irq:
LLWU_irq:
WDOG_irq:
RNGB_irq:
I2C0_irq:
I2C1_irq:
SPI0_irq:
SPI1_irq:
SPI2_irq:
CAN0_Message_Buffer_irq:
CAN0_Bus_Off_irq:
CAN0_Error_irq:
CAN0_Transmit_irq:
CAN0_Receive_irq:
CAN0_Wake_Up_irq:
CAN0_IMEU_irq:
CAN0_Lost_Receive_irq:
CAN1_Message_Buffer_irq:
CAN1_Bus_Off_irq:
CAN1_Error_irq:
CAN1_Transmit_irq:
CAN1_Receive_irq:
CAN1_Wake_Up_irq:
CAN1_IMEU_irq:
CAN1_Lost_Receive_irq:
UART0_Status_irq:
UART0_Error_irq:
UART1_Status_irq:
UART1_Error_irq:
UART2_Status_irq:
UART2_Error_irq:
UART3_Status_irq:
UART3_Error_irq:
UART4_Status_irq:
UART4_Error_irq:
UART5_Status_irq:
UART5_Error_irq:
ADC0_irq:
ADC1_irq:
CMP0_irq:
CMP1_irq:
CMP2_irq:
FTM0_irq:
FTM1_irq:
FTM2_irq:
CMT_irq:
RTC_irq:
PIT_Channel0_irq:
PIT_Channel1_irq:
PIT_Channel2_irq:
PIT_Channel3_irq:
PDB_irq:
USB_OTG_irq:
USB_Charger_Detect_irq:
ENET_Timer_irq:
ENET_Transmit_irq:
ENET_Receive_irq:
ENET_Error_irq:
I2S_irq:
SDHC_irq:
DAC0_irq:
DAC1_irq:
TSI_irq:
MCG_irq:
LPTMR_irq:
PortA_irq:
PortB_irq:
PortC_irq:
PortD_irq:
PortE_irq:
SWI_irq:
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
