#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_apollo_streamer

APOLLO_STREAMER_LIBRARY_NAME :=apollo_streamer.$(RTOS).$(NETWORK).$(HOST_ARCH).$(BUILD_TYPE).a

ifneq ($(wildcard $(CURDIR)$(APOLLO_STREAMER_LIBRARY_NAME)),)
$(info Using PREBUILT:  $(APOLLO_STREAMER_LIBRARY_NAME))
$(NAME)_PREBUILT_LIBRARY :=$(APOLLO_STREAMER_LIBRARY_NAME)
else
# Build from source (Broadcom internal)
include $(CURDIR)apollo_streamer_src.mk
endif # ifneq ($(wildcard $(CURDIR)$(APOLLO_STREAMER_LIBRARY_NAME)),)

GLOBAL_INCLUDES += .

GLOBAL_DEFINES  += WICED_USE_AUDIO
GLOBAL_DEFINES  += BUILDCFG
GLOBAL_DEFINES  += WICED_DCT_INCLUDE_BT_CONFIG

$(NAME)_COMPONENTS += audio/apollo/apollocore
$(NAME)_COMPONENTS += audio/apollo/apollo_bt
$(NAME)_COMPONENTS += bt_audio

$(NAME)_CFLAGS  :=
