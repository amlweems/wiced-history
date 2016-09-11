#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME :=                 Lib_sbc_if

GLOBAL_INCLUDES :=      .

SBCINCLUDES :=          ../../../drivers/bluetooth/sbc/decoder/include \
                        ../../../drivers/bluetooth/BTE/Components/stack/include \
                        ../../../drivers/bluetooth/BTE/WICED \
                        ../../../drivers/bluetooth/BTE/Projects/bte/main \
                        ../../../drivers/bluetooth/BTE \
                        ../codec_framework

$(NAME)_SOURCES :=      sbc_if.c

$(NAME)_INCLUDES :=     . \
                        $(SBCINCLUDES)

$(NAME)_COMPONENTS :=   libraries/drivers/bluetooth/sbc
