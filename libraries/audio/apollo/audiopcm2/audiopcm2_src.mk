#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
$(NAME)_COMPONENTS += audio/apollo/audioplc2

$(NAME)_HEADERS := \
					globals.h \
					audiopcm_types.h \
					audiopcm_core.h \
					audiopcm_tables.h \
					audiopcm_api.h \
					clk.h \
					clk_core.h \
					dsp.h \
					dsp_core.h \
					dsp_tools.h \
					events.h \
					logger.h \
					output.h \
					output_core.h \
					rtp.h \
					rtp_core.h \
					sysclk.h \
					tsp.h \
					watchdog.h \
					watchdog_core.h \
					concealmet.h \
					concealment_core.h \
					version.h

$(NAME)_SOURCES := \
					audiopcm.c \
					clk.c \
					concealment.c \
					dsp.c \
					dsp_tools.c \
					events.c \
					output.c \
					rtp.c \
					watchdog.c \
					sysclk.c \
					logger.c

KEEP_LIST:= \
					audiopcm*.a \
					audiopcm*.mk \
					audiopcm_api.h \
					audiopcm_tables.h \
					events.h \
					globals.h \
					audiopcm_types.h


#$(NAME)_CFLAGS += $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)


$(NAME)_CFLAGS += \
					-std=gnu99 \
					-D_FILE_OFFSET_BITS=64 \
					-D_LARGEFILE_SOURCE=1 \
					-D_LARGEFILE64_SOURCE=1 \
					-D ARM_MATH=1 \
					-O2

$(NAME)_DEFINES := WICED

$(NAME)_INCLUDES := \
					. \
					../audioplc2/src \
					../audioplc2/src/lcplc \
					../audioplc2/src/music \
					../audioplc2/src/splib
