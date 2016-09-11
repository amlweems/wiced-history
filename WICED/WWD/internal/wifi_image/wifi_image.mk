#
# Copyright 2013, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Wiced_Wifi_image

GLOBAL_INCLUDES := ../chips/$(CHIP)

$(NAME)_SOURCES :=  ../chips/$(CHIP)/wifi_image.c