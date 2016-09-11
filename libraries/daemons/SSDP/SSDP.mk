#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_SSDP

$(NAME)_SOURCES := wiced_ssdp.c \
				   wiced_ssdp_multicast.c

$(NAME)_COMPONENTS += protocols/HTTP

GLOBAL_INCLUDES    += protocols/HTTP

VALID_OSNS_COMBOS  := ThreadX-NetX ThreadX-NetX_Duo
VALID_PLATFORMS    := BCM943909WCD*