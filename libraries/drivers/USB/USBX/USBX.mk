#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
NAME := Lib_USBX

USBX_VERSION := 5.7

VALID_RTOS_LIST:= ThreadX

$(NAME)_COMPONENTS += libraries/drivers/USB/USBX/WICED

ifeq ($(RTOS),)
$(error Must define RTOS)
endif

# Define some macros to allow for some usb-specific checks
GLOBAL_DEFINES += USB_$(NAME)=1
GLOBAL_DEFINES += $(NAME)_VERSION=$$(SLASH_QUOTE_START)v$(USBX_VERSION)$$(SLASH_QUOTE_END)
GLOBAL_DEFINES += UX_INCLUDE_USER_DEFINE_FILE UX_ENABLE_DEBUG_LOG

# Check if we had FileX compiled in
#ifneq ($(FILESYSTEM),FileX)
#GLOBAL_DEFINES += UX_NO_FILEX
#endif

$(NAME)_COMPONENTS += filesystems/FileX
USING_FILEX_USBX := yes


GLOBAL_INCLUDES := ver$(USBX_VERSION)
GLOBAL_INCLUDES += ver$(USBX_VERSION)/usbx_host_classes
GLOBAL_INCLUDES += ver$(USBX_VERSION)/usbx_host_controllers
GLOBAL_INCLUDES += ver$(USBX_VERSION)/usbx_device_classes
GLOBAL_INCLUDES += ver$(USBX_VERSION)/usbx_device_controllers


ifdef WICED_ENABLE_TRACEX
# Precompiled library with TraceX
USBX_LIBRARY_NAME :=USBX.TraceX.$(RTOS).$(HOST_ARCH).$(BUILD_TYPE).a
else
# Precompiled library
USBX_LIBRARY_NAME :=USBX.$(RTOS).$(HOST_ARCH).$(BUILD_TYPE).a
endif

ifneq ($(wildcard $(CURDIR)$(USBX_LIBRARY_NAME)),)
# Using a precompiled Library
$(NAME)_PREBUILT_LIBRARY := $(USBX_LIBRARY_NAME)
else
# Build from source (Broadcom internal)
include $(CURDIR)USBX_src.mk
endif #ifneq ($(wildcard $(CURDIR)USBX.$(HOST_ARCH).$(BUILD_TYPE).a),)

$(NAME)_SOURCES +=  ver$(USBX_VERSION)/ux_utility_wiced_all.c
