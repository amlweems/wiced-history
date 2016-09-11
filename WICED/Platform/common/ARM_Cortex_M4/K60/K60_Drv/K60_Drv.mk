#
# Copyright 2013, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME = K60_Drv

GLOBAL_INCLUDES :=  common \
					cpu \
					drivers \
					drivers/mcg \
					drivers/wdog \
					drivers/uart \
					include \
					include/freescale \
					../../CMSIS

$(NAME)_SOURCES += \
                   drivers/uart/uart.c \
                   drivers/wdog/wdog.c
