#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME :=                 Lib_sbc

GLOBAL_INCLUDES +=      ../include \
                        ../BTE/WICED \
                        ../BTE/Components/stack/include \
                        ../BTE/Projects/bte/main


ifneq ($(wildcard $(CURDIR)sbc.$(RTOS).$(NETWORK).$(HOST_ARCH).release.a),)
$(NAME)_PREBUILT_LIBRARY := sbc.$(RTOS).$(NETWORK).$(HOST_ARCH).release.a
else
# Build from source (Broadcom internal)
include $(CURDIR)sbc_src.mk
endif
