#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := common_IAR

$(NAME)_SOURCES = stdio_IAR.c \
                  mem_IAR.c \
                  gnu_compat/iar_strlcpy.c \
                  gnu_compat/iar_getopt_long.c

# These need to be forced into the final ELF since they are not referenced otherwise
# The other obj files will be in the .a library and linked when refrenced.
$(NAME)_LINK_FILES := stdio_IAR.o

GLOBAL_INCLUDES += . \
                   gnu_compat

$(NAME)_ESC_DOLLAR:=$$$$$$$$

# Turn off warning about non-standard line endings
IAR_DIAG_SUPPRESSIONS += --diag_suppress Pa050

# Unreachable code is intentional
IAR_DIAG_SUPPRESSIONS += --diag_suppress Pe111

# Dynamic initialization may occur in intentionally unreachable code
IAR_DIAG_SUPPRESSIONS += --diag_suppress Pe185

GLOBAL_CFLAGS         += $(IAR_DIAG_SUPPRESSIONS)
GLOBAL_CXXFLAGS       += $(IAR_DIAG_SUPPRESSIONS)

# Set program entry point.
GLOBAL_LDFLAGS += --entry _start

# Suppress mixed section warnings.
GLOBAL_LDFLAGS += --diag_suppress Lp005,Lp006

# Symbol aliases.
GLOBAL_LDFLAGS += \
                  --redirect link_stack_end=CSTACK$($(NAME)_ESC_DOLLAR)$($(NAME)_ESC_DOLLAR)Limit \
                  --redirect reset_handler=_start \
                  --redirect __vector_table=interrupt_vector_table
