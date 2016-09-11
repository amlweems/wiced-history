#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := NuttX_NS

$(NAME)_COMPONENTS += WICED/network/NuttX_NS/WWD

ifeq (,$(APP_WWD_ONLY)$(NS_WWD_ONLY)$(RTOS_WWD_ONLY))
$(NAME)_COMPONENTS += WICED/network/NuttX_NS/WICED
endif

# Define some macros to allow for some network-specific checks
GLOBAL_DEFINES += NETWORK_$(NAME)=1

$(NAME)_INCLUDES :=
$(NAME)_SOURCES  :=
$(NAME)_DEFINES  :=

VALID_RTOS_LIST:= NuttX
