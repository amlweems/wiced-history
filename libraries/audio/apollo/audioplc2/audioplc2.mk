#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_audioplc2

GLOBAL_INCLUDES += .
GLOBAL_INCLUDES += src/music/
GLOBAL_INCLUDES += src/samplePLC/
GLOBAL_INCLUDES += src/lcplc/
GLOBAL_INCLUDES += src/splib/


#AUDIOPLC_LIBRARY_NAME :=audioplc2.$(RTOS).$(NETWORK).$(HOST_ARCH).$(BUILD_TYPE).a
AUDIOPLC_LIBRARY_NAME :=audioplc2.$(RTOS).$(HOST_ARCH).$(BUILD_TYPE).a


ifneq ($(wildcard $(CURDIR)$(AUDIOPLC_LIBRARY_NAME)),)

# Use the available prebuilt
$(info Using PREBUILT:  $(AUDIOPLC_LIBRARY_NAME))
$(NAME)_PREBUILT_LIBRARY := $(AUDIOPLC_LIBRARY_NAME)

else

# Build from source (Broadcom internal)
$(info Building SRC:  $(AUDIOPLC_LIBRARY_NAME))
include $(CURDIR)audioplc2_src.mk

endif
