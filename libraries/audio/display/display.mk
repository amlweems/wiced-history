#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Audio_Display_Library

GLOBAL_INCLUDES    := .

$(NAME)_SOURCES    := audio_display.c

$(NAME)_COMPONENTS := graphics/u8g drivers/power_management

KEEP_LIST := audio_display.h
