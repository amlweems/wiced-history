#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_FATFS

FATFS_VERSION := ver0.11

$(NAME)_SOURCES := \
				   fatfs_to_block_device_driver.c \
				   fatfs_user_api_driver.c \
				   $(FATFS_VERSION)/src/ff.c \
				   $(FATFS_VERSION)/src/option/unicode.c \
				   $(FATFS_VERSION)/src/option/syscall.c

GLOBAL_INCLUDES := $(FATFS_VERSION)/src \
                   $(FATFS_VERSION)/src/option

GLOBAL_DEFINES := USING_FATFS

$(NAME)_CFLAGS  :=
