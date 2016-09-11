#
# Copyright 2013, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := WWD_FreeRTOS_Interface_$(PLATFORM)

GLOBAL_INCLUDES := . \
                   ./$(PLATFORM_FULL) \
                   ./ARM_CM3

$(NAME)_SOURCES := wwd_rtos.c

$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)

