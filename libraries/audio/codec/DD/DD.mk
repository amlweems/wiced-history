#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_dolby_digital

DOLBY_DIGITAL_LIBRARY_NAME :=dolby_digital.$(RTOS).$(HOST_ARCH).$(BUILD_TYPE).a

ifneq ($(wildcard $(CURDIR)$(DOLBY_DIGITAL_LIBRARY_NAME)),)
$(info Using PREBUILT:  $(DOLBY_DIGITAL_LIBRARY_NAME))
$(NAME)_PREBUILT_LIBRARY := $(DOLBY_DIGITAL_LIBRARY_NAME)
else
# Build from source (Broadcom internal)
include $(CURDIR)DD_src.mk
endif # ifneq ($(wildcard $(CURDIR)$(DOLBY_DIGITAL_LIBRARY_NAME)),)

GLOBAL_INCLUDES += DD_Plus_Decoder/Source_Code/ddp_udc/include
GLOBAL_INCLUDES += DD_Plus_Decoder/Source_Code/dlb_buffer/include
