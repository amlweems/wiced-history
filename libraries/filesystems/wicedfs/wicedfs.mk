#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_Wiced_RO_FS

$(NAME)_SOURCES := \
                   src/wicedfs.c \
                   wicedfs_drivers.c

GLOBAL_INCLUDES := src

GLOBAL_DEFINES := USING_WICEDFS

$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)


$(NAME)_UNIT_TEST_SOURCES := src/unit/wicedfs_unit_images.c src/unit/wicedfs_unit.cpp