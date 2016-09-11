#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_codec_if

GLOBAL_INCLUDES := . \
                   ./include

$(NAME)_SOURCES := wiced_codec_interface.c

$(NAME)_COMPONENTS := libraries/audio/codec/sbc_if
