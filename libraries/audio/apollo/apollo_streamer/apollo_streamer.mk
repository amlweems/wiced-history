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
$(info Building SRC:  $(APOLLO_STREAMER_LIBRARY_NAME))
include $(CURDIR)apollo_streamer_src.mk
endif # ifneq ($(wildcard $(CURDIR)$(APOLLO_STREAMER_LIBRARY_NAME)),)

GLOBAL_INCLUDES += .

GLOBAL_DEFINES  += WICED_USE_AUDIO
GLOBAL_DEFINES  += BUILDCFG

# this needs to be FALSE to allow app to override the BTEWICED
# linkkey management
GLOBAL_DEFINES  += BTM_INTERNAL_LINKKEY_STORAGE_INCLUDED=FALSE

$(NAME)_COMPONENTS += audio/apollo/apollocore
$(NAME)_COMPONENTS += audio/apollo/audio_render
$(NAME)_COMPONENTS += audio/apollo/802_dot_1as_avb
$(NAME)_COMPONENTS += audio/apollo/audio_pll_tuner
$(NAME)_COMPONENTS += libraries/drivers/bluetooth
$(NAME)_COMPONENTS += libraries/audio/codec/codec_framework

$(NAME)_CFLAGS  :=
