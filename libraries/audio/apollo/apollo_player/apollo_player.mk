#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_apollo_player

APOLLO_PLAYER_LIBRARY_NAME :=apollo_player.$(RTOS).$(NETWORK).$(HOST_ARCH).$(BUILD_TYPE).a

ifneq ($(wildcard $(CURDIR)$(APOLLO_PLAYER_LIBRARY_NAME)),)
$(info Using PREBUILT:  $(APOLLO_PLAYER_LIBRARY_NAME))
$(NAME)_PREBUILT_LIBRARY :=$(APOLLO_PLAYER_LIBRARY_NAME)
else
# Build from source (Broadcom internal)
include $(CURDIR)apollo_player_src.mk
endif # ifneq ($(wildcard $(CURDIR)$(APOLLO_PLAYER_LIBRARY_NAME)),)

GLOBAL_INCLUDES += .

GLOBAL_DEFINES  += AUDIOPCM_ENABLE
GLOBAL_DEFINES  += WICED_USE_AUDIO

$(NAME)_COMPONENTS += audio/apollo/apollocore
$(NAME)_COMPONENTS += audio/apollo/audio_render

ifneq (,$(findstring AUDIOPCM_ENABLE,$(GLOBAL_DEFINES)))
$(info Enabling audiopcm libraries...)
$(NAME)_COMPONENTS += audio/apollo/audiopcm
$(NAME)_COMPONENTS += audio/apollo/audioplc
endif

$(NAME)_CFLAGS  :=
