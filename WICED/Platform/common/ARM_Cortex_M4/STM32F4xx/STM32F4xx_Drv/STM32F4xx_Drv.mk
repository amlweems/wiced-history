#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME = STM32F4xx_Drv

GLOBAL_INCLUDES :=  . \
                    STM32F4xx_StdPeriph_Driver/inc \
                    ../../CMSIS

$(NAME)_SOURCES := \
                   STM32F4xx_StdPeriph_Driver/src/misc.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_adc.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_can.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_crc.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_dac.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_dbgmcu.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_dma.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_exti.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_flash.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_fsmc.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_gpio.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rng.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_i2c.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_iwdg.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_pwr.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rcc.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rtc.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_sdio.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_spi.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_syscfg.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_tim.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_usart.c \
                   STM32F4xx_StdPeriph_Driver/src/stm32f4xx_wwdg.c
