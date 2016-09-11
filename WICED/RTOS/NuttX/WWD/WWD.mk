#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := WWD_NuttX_Interface

GLOBAL_INCLUDES := .
$(NAME)_SOURCES  := wwd_rtos.c

ifneq ($(filter $(HOST_ARCH), ARM_CR4),)
GLOBAL_INCLUDES    += CR4
$(NAME)_SOURCES    += CR4/timer_isr.c
$(NAME)_LINK_FILES += CR4/timer_isr.o
else
$(error No NuttX low_level_init function for architecture $(HOST_ARCH))
endif

$(NAME)_CFLAGS = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)

$(NAME)_CHECK_HEADERS := \
                         wwd_rtos.h