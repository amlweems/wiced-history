#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

SOURCE_ROOT :=../../

ifneq ($(MAKECMDGOALS),wipe_source_for_test)
ifeq ($(RTOS),)
$(error Must define RTOS)
endif

ifeq ($(NETWORK),)
$(error Must define NETWORK)
endif
endif

#include $(SOURCE_ROOT)WICED/RTOS/$(RTOS)/WWD/WWD.mk

CFLAGS += $(addprefix -I$(SOURCE_ROOT)WICED/RTOS/$(RTOS)/WWD/, $(GLOBAL_INCLUDES))



NAME    := FLAC
ALWAYS_OPTIMISE := 1

include FLAC.mk


CFLAGS += -I$(SOURCE_ROOT)WICED/WWD/include
CFLAGS += -I$(SOURCE_ROOT)WICED/WWD/include/network
CFLAGS += -I$(SOURCE_ROOT)WICED/platform/include
CFLAGS += -I$(SOURCE_ROOT)WICED/platform/MCU
CFLAGS += -I$(SOURCE_ROOT)WICED/platform/GCC
CFLAGS += -I$(SOURCE_ROOT)WICED
CFLAGS += -I$(SOURCE_ROOT)include
CFLAGS += -I$(SOURCE_ROOT)WICED/RTOS/$(RTOS)/WICED
CFLAGS += -I$(SOURCE_ROOT)WICED/RTOS/$(RTOS)/WWD

CFLAGS += -I$(SOURCE_ROOT)WICED/network/$(NETWORK)/WICED
CFLAGS += -I$(SOURCE_ROOT)WICED/network/$(NETWORK)/WWD
CFLAGS += -I$(SOURCE_ROOT)WICED/WWD/internal/chips/43362
CFLAGS += -I$(SOURCE_ROOT)tools/prebuild_stubs
CFLAGS += -I$(SOURCE_ROOT)WICED/security/BESL/crypto
CFLAGS += -I$(SOURCE_ROOT)WICED/security/BESL/include
CFLAGS += -I$(SOURCE_ROOT)libraries/utilities/ring_buffer
CFLAGS += -I$(SOURCE_ROOT)libraries/utilities/linked_list
CFLAGS += -I$(SOURCE_ROOT)libraries/bluetooth/include

ifeq ($(HOST_ARCH),ARM_CR4)
CFLAGS += -DPLATFORM_L1_CACHE_SHIFT=5
endif

# FLAC__HAS_OGG


# FLAC compile flags
CFLAGS += VERSION="1.3.1"
CFLAGS += -DHAVE_LROUND=1
CFLAGS += -DHAVE_STDINT_H=1
CFLAGS += -DHAVE_CONFIG_H=1

SOURCES := $($(NAME)_SOURCES)
CFLAGS += $(addprefix -D,$(GLOBAL_DEFINES)) $(addprefix -I,$($(NAME)_INCLUDES)) $(addprefix -D,$($(NAME)_DEFINES)) $($(NAME)_CFLAGS)


$(NAME)_CFLAGS += $(CFLAGS)

KEEP_LIST_DIR := $(SOURCE_ROOT)libraries/flac



include $(SOURCE_ROOT)tools/release/wiced_library.mk




