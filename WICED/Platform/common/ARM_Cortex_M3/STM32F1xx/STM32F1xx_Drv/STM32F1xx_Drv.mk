#
# Copyright 2013, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME = STM32F1xx_Drv

GLOBAL_INCLUDES :=  . \
                    STM32F10x_StdPeriph_Driver/inc \
                    ../../CMSIS


$(NAME)_SOURCES := \
                   STM32F10x_StdPeriph_Driver/src/misc.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_adc.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_bkp.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_can.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_cec.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_crc.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_dac.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_dbgmcu.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_exti.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_fsmc.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_i2c.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_iwdg.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_pwr.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_rtc.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_sdio.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c \
                   STM32F10x_StdPeriph_Driver/src/stm32f10x_wwdg.c
