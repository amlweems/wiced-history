#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := common_GCC

$(NAME)_SOURCES  =

ifeq ($(NO_NEWLIBC),)
$(NAME)_SOURCES += mem_newlib.c \
                   math_newlib.c
endif

$(NAME)_SOURCES += cxx_funcs.c


GLOBAL_INCLUDES :=  .

# These need to be forced into the final ELF since they are not referenced otherwise
ifeq ($(NO_NEWLIBC),)
$(NAME)_LINK_FILES := mem_newlib.o
endif

ifeq ($(NO_NEWLIBC),)
$(NAME)_SOURCES += $(if $(WICED_DISABLE_STDIO),,stdio_newlib.c)
$(NAME)_LINK_FILES += $(if $(WICED_DISABLE_STDIO),,stdio_newlib.o)
endif

GLOBAL_INCLUDES += .
