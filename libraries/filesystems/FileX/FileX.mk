#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_FileX

FILEX_VERSION := 5.2

$(NAME)_COMPONENTS := libraries/filesystems/FileX/WICED

VALID_RTOS_LIST:= ThreadX

GLOBAL_INCLUDES := ver$(FILEX_VERSION)
GLOBAL_INCLUDES += ver$(FILEX_VERSION)/filex_utilities

GLOBAL_DEFINES += FX_INCLUDE_USER_DEFINE_FILE
GLOBAL_DEFINES += TX_INCLUDE_USER_DEFINE_FILE

ifdef WICED_ENABLE_TRACEX
# Precompiled library with TraceX
FILEX_LIBRARY_NAME :=FileX.TraceX.$(RTOS).$(HOST_ARCH).release.a
else
# Precompiled library
FILEX_LIBRARY_NAME :=FileX.$(RTOS).$(HOST_ARCH).release.a
endif

ifneq ($(wildcard $(CURDIR)$(FILEX_LIBRARY_NAME)),)
# Using a precompiled Library
$(NAME)_PREBUILT_LIBRARY := $(FILEX_LIBRARY_NAME)
else
# Build from source (Broadcom internal)
include $(CURDIR)FileX_src.mk
endif #ifneq ($(wildcard $(CURDIR)FileX.$(HOST_ARCH).$(BUILD_TYPE).a),)
