#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_FLAC

OGG_SRCS_C = \
	libFLAC/ogg_helper.c \
	libFLAC/ogg_mapping.c	\
	libFLAC/ogg_decoder_aspect.c

LIBFLAC_SRC_C = \
	libFLAC/bitmath.c \
	libFLAC/bitreader.c \
	libFLAC/bitwriter.c \
	libFLAC/cpu.c \
	libFLAC/crc.c \
	libFLAC/fixed.c \
	libFLAC/fixed_intrin_sse2.c \
	libFLAC/fixed_intrin_ssse3.c \
	libFLAC/float.c \
	libFLAC/format.c \
	libFLAC/lpc.c \
	libFLAC/lpc_intrin_sse.c \
	libFLAC/lpc_intrin_sse2.c \
	libFLAC/lpc_intrin_sse41.c \
	libFLAC/lpc_intrin_avx2.c \
	libFLAC/md5.c \
	libFLAC/memory.c \
	libFLAC/metadata_iterators.c \
	libFLAC/metadata_object.c \
	libFLAC/window.c

LIBFLAC_DECODER_SRC_C = \
	libFLAC/stream_decoder.c

LIBFLAC_ENCODER_SRC_C = \
	libFLAC/stream_encoder.c \
	libFLAC/stream_encoder_framing.c \
	libFLAC/stream_encoder_intrin_avx2.c \
	libFLAC/stream_encoder_intrin_sse2.c \
	libFLAC/stream_encoder_intrin_ssse3.c

SRCS_C = \
	$(LIBFLAC_SRC_C)		\
	$(LIBFLAC_DECODER_SRC_C)		\
	$(LIBFLAC_ENCODER_SRC_C)		\
	$(OGG_SRCS)

$(NAME)_SOURCES := 	$(SRCS_C)


$(NAME)_INCLUDES :=  .

GLOBAL_INCLUDES += include
GLOBAL_INCLUDES += include/FLAC/
GLOBAL_INCLUDES += include/private
GLOBAL_INCLUDES += include/protected

