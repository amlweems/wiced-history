#
# Copyright 2013, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := FreeRTOS

VERSION := 7.1.0

$(NAME)_COMPONENTS := Wiced/RTOS/FreeRTOS/wwd
ifeq (,$(APP_WWD_ONLY)$(NS_WWD_ONLY)$(RTOS_WWD_ONLY))
$(NAME)_COMPONENTS += Wiced/RTOS/FreeRTOS/wiced
endif

# Define some macros to allow for some network-specific checks
GLOBAL_DEFINES += RTOS_$(NAME)=1
GLOBAL_DEFINES += configUSE_MUTEXES
GLOBAL_DEFINES += configUSE_RECURSIVE_MUTEXES
GLOBAL_DEFINES += $(NAME)_VERSION=$$(SLASH_QUOTE_START)v$(VERSION)$$(SLASH_QUOTE_END)

GLOBAL_INCLUDES := ver$(VERSION)/Source/include


$(NAME)_SOURCES :=  ver$(VERSION)/Source/croutine.c \
                    ver$(VERSION)/Source/list.c \
                    ver$(VERSION)/Source/queue.c \
                    ver$(VERSION)/Source/tasks.c \
                    ver$(VERSION)/Source/timers.c \
                    ver$(VERSION)/Source/portable/MemMang/heap_3.c

# Win32_x86 specific sources and includes
Win32_x86_SOURCES  := ver$(VERSION)/Source/portable/MSVC-MingW/port.c
Win32_x86_INCLUDES := ver$(VERSION)/Source/portable/MSVC-MingW

# ARM Cortex M3/4 specific sources and includes
ifeq ($(TOOLCHAIN_NAME),IAR)
ARM_Cortex_M3_SOURCES  := ver$(VERSION)/Source/portable/IAR/ARM_CM3/port.c \
                          ver$(VERSION)/Source/portable/IAR/ARM_CM3/portasm.S
ARM_Cortex_M3_INCLUDES := ver$(VERSION)/Source/portable/IAR/ARM_CM3 \
                          wwd/ARM_CM3  
else
ARM_Cortex_M3_SOURCES  := ver$(VERSION)/Source/portable/GCC/ARM_CM3/port.c 
ARM_Cortex_M3_INCLUDES := ver$(VERSION)/Source/portable/GCC/ARM_CM3
endif
ARM_Cortex_M4_SOURCES  := $(ARM_Cortex_M3_SOURCES)
ARM_Cortex_M4_INCLUDES := $(ARM_Cortex_M3_INCLUDES)

$(NAME)_SOURCES += $($(HOST_ARCH)_SOURCES)
GLOBAL_INCLUDES += $($(HOST_ARCH)_INCLUDES)


