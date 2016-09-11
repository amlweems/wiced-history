#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_802_dot_1as_avb

AVB_LIBRARY_NAME:=802_dot_1as_avb.$(RTOS).$(NETWORK).$(HOST_ARCH).$(BUILD_TYPE).a

ifneq ($(wildcard $(CURDIR)$(AVB_LIBRARY_NAME)),)
$(info Using PREBUILT:  $(AVB_LIBRARY_NAME))
$(NAME)_PREBUILT_LIBRARY :=$(AVB_LIBRARY_NAME)
else
# Build from source (Broadcom internal)
$(info Building SRC:  $(AVB_LIBRARY_NAME))
include $(CURDIR)802_dot_1as_avb_src.mk
endif # ifneq ($(wildcard $(CURDIR)$(AVB_LIBRARY_NAME)),)

GLOBAL_DEFINES  += IBSS_RMC
GLOBAL_DEFINES  += WICED_APP_BUILD
GLOBAL_DEFINES  += STANDALONE

GLOBAL_INCLUDES += .
