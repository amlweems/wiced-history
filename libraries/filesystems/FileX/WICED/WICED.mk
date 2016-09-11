#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := WICED_Lib_FileX_Interface

GLOBAL_INCLUDES := .

$(NAME)_SOURCES := filex_to_block_device_driver.c \
                   filex_user_api_driver.c

$(NAME)_CFLAGS = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)

GLOBAL_DEFINES += USING_FILEX

ifneq ($(USING_FILEX_USBX),)
$(NAME)_SOURCES += filex_usbx_user_api_driver.c
GLOBAL_DEFINES += USING_FILEX_USBX
endif
