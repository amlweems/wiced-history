#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_audio_pll_tuner

AUDIO_PLL_TUNER_LIBRARY_NAME :=audio_pll_tuner.$(RTOS).$(HOST_ARCH).$(BUILD_TYPE).a

ifneq ($(wildcard $(CURDIR)$(AUDIO_PLL_TUNER_LIBRARY_NAME)),)
$(info Using PREBUILT:  $(AUDIO_PLL_TUNER_LIBRARY_NAME))
$(NAME)_PREBUILT_LIBRARY :=$(AUDIO_PLL_TUNER_LIBRARY_NAME)
else
# Build from source (Broadcom internal)
$(info Building SRC:  $(AUDIO_PLL_TUNER_LIBRARY_NAME))
include $(CURDIR)audio_pll_tuner_src.mk
endif # ifneq ($(wildcard $(CURDIR)$(AUDIO_PLL_TUNER_LIBRARY_NAME)),)

GLOBAL_INCLUDES += .

$(NAME)_COMPONENTS += audio/apollo/apollocore

$(NAME)_CFLAGS  :=
