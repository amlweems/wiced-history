#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
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
$(info Building SRC:  $(APOLLO_PLAYER_LIBRARY_NAME))
include $(CURDIR)apollo_player_src.mk
endif # ifneq ($(wildcard $(CURDIR)$(APOLLO_PLAYER_LIBRARY_NAME)),)

GLOBAL_INCLUDES += .

GLOBAL_DEFINES  += WICED_USE_AUDIO

$(NAME)_COMPONENTS += audio/apollo/apollocore
$(NAME)_COMPONENTS += audio/apollo/audio_render
$(NAME)_COMPONENTS += audio/apollo/audio_pll_tuner

$(NAME)_COMPONENTS += audio/apollo/audiopcm2
$(NAME)_COMPONENTS += audio/apollo/audioplc2

$(NAME)_CFLAGS  :=
