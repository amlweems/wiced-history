#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME = STM32F2xx_Drv

GLOBAL_INCLUDES :=  . \
                    STM32F2xx_StdPeriph_Driver/inc \
                    ../../CMSIS

$(NAME)_SOURCES := \
                   STM32F2xx_StdPeriph_Driver/src/misc.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_adc.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_can.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_crc.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_dac.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_dbgmcu.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_dma.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_exti.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_flash.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_fsmc.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_gpio.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_rng.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_i2c.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_iwdg.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_pwr.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_rcc.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_rtc.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_sdio.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_spi.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_syscfg.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_tim.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_usart.c \
                   STM32F2xx_StdPeriph_Driver/src/stm32f2xx_wwdg.c
