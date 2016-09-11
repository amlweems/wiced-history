#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := NuttX

include $(CURDIR)NuttX_targets.mk
include $(CURDIR)NuttX_prepare.mk

$(NAME)_COMPONENTS := WICED/RTOS/NuttX/WWD
ifeq (,$(APP_WWD_ONLY)$(NS_WWD_ONLY)$(RTOS_WWD_ONLY))
$(NAME)_COMPONENTS += WICED/RTOS/NuttX/WICED
endif

# Enable some conditional compilation
GLOBAL_DEFINES += __NUTTX__
# NuttX has own way of registering ISR. Use hook for this.
GLOBAL_DEFINES += PLATFORM_IRQ_DEMUXER_HOOK=1
# SOCSRAM powerdown has to be hooked into heap allocation and not yet implemented for NuttX
GLOBAL_DEFINES += PLATFORM_NO_SOCSRAM_POWERDOWN=1
# Define some macros to allow for some network-specific checks
GLOBAL_DEFINES += RTOS_$(NAME)=1
# NuttX's version of offsetof doesn't return an constant integer and messes up STRUCTURE_CHECK.
GLOBAL_DEFINES += STRUCTURE_CHECK_DISABLE
# OSL needs to be aware of types defined or not defined by NuttX.
GLOBAL_DEFINES += TARGETOS_nuttx
# NuttX does not have malloc.h.
GLOBAL_DEFINES += NO_MALLOC_H
# NuttX has own start and exit function implementation.
GLOBAL_DEFINES += NO_CRT_START_AND_EXIT
# NuttX has own CRC implementation incompatible with WICED
GLOBAL_DEFINES += NO_CRC_FUNCTIONS

# NuttX uses non-standard format strings, so ignore warnings.
GLOBAL_CFLAGS += -Wno-format
# NuttX use warning pragmas, it should not stop compiling
GLOBAL_CFLAGS += -Wno-error=cpp
# NuttX has some unused functions
GLOBAL_CFLAGS += -Wno-error=unused-function
# NuttX does not support strict aliasing
GLOBAL_CFLAGS += -fno-strict-aliasing
# NuttX uses non-standard conforming variadic macros, so ignore warnings.
GLOBAL_CFLAGS += -Wno-variadic-macros
# Tell NuttX that C++ compiler has builtin wchar_t type
GLOBAL_CXXFLAGS += -DCONFIG_WCHAR_BUILTIN
# NuttX has own libc
GLOBAL_LDFLAGS  += -nodefaultlibs -lgcc

# NuttX has its own C library, so disable newlibc.
NO_NEWLIBC := y

# Extra variables/targets to help build NuttX.
EXTRA_TARGET_MAKEFILES += $(SOURCE_ROOT)WICED/RTOS/NuttX/NuttX_targets.mk

include $(CURDIR)NuttX_src.mk
