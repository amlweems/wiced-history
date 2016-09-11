#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

$(NAME)_HEADERS := \
		src/audioPLCerrs.h\
		src/AudioStreamingConcealment.h\
		src/AudioStreamingConcealmentDefs.h\
		src/lcplc/autocor.h\
		src/lcplc/estpitch.h\
		src/lcplc/lcplc.h\
		src/lcplc/levi.h\
		src/lcplc/smdpitch.h\
		src/music/audioPLC.h\
		src/music/audioProcessing.h\
		src/samplePLC/aswin192.h\
		src/samplePLC/aswin192_fix.h\
		src/samplePLC/aswin44.h\
		src/samplePLC/aswin44_fix.h\
		src/samplePLC/aswin48.h\
		src/samplePLC/aswin48_fix.h\
		src/samplePLC/aswin96.h\
		src/samplePLC/aswin96_fix.h\
		src/samplePLC/Fautocor.h\
		src/samplePLC/levinson.h\
		src/samplePLC/matrix.h\
		src/samplePLC/samplePLC.h\
		src/samplePLC/samplePLCdefs.h\
		src/samplePLC/samplePLCtables.h\
		src/splib/Bopslib.h\
		src/splib/fixmath.h\
		src/typedef.h

$(NAME)_SOURCES := \
		src/splib/Bopslib.c\
		src/splib/fixmath.c\
		src/lcplc/autocor.c\
		src/lcplc/configs.c\
		src/lcplc/estpitch.c\
		src/lcplc/lcplc.c\
		src/lcplc/levi.c\
		src/lcplc/smdpitch.c\
		src/lcplc/tables.c\
		src/music/audioPLC.c\
		src/music/audioProcessing.c\
		src/samplePLC/Fautocor.c\
		src/samplePLC/levinson.c\
		src/samplePLC/matrix.c\
		src/samplePLC/samplePLC.c\
		src/samplePLC/samplePLCtables.c\
		src/AudioStreamingConcealment.c

$(NAME)_INCLUDES := src\
                src/splib\
                src/samplePLC\
                src/lcplc\
                src/music

KEEP_LIST:= \
		audioplc*.a \
		audioplc*.mk \
		src/audioPLCerrs.h\
		src/AudioStreamingConcealment.h\
		src/AudioStreamingConcealmentDefs.h\
		src/music/audioPLC.h\
		src/music/audioProcessing.h\
                src/lcplc/lcplc.h\
		src/music/audioPLC.h \
		src/samplePLC/samplePLC.h\
		src/samplePLC/samplePLCdefs.h\
		src/samplePLC/samplePLCtables.h \
		src/splib/Bopslib.h\
		src/typedef.h

$(NAME)_CFLAGS += \
		-std=gnu99 \
		-D_FILE_OFFSET_BITS=64 \
		-D_LARGEFILE_SOURCE=1 \
		-D_LARGEFILE64_SOURCE=1 \
		-D ARM_MATH=1

$(NAME)_DEFINES := WICED
$(NAME)_DEFINES += DEBUG_FPRINTF_ENABLED=0

GLOBAL_DEFINES  += USE_LINEAR_INTERPOLATION
