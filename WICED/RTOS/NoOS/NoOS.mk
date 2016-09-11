#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := NoOS

# Define some macros to allow for some network-specific checks
GLOBAL_DEFINES += RTOS_$(NAME)=1

$(NAME)_COMPONENTS := Wiced/RTOS/NoOS/wwd

#This makefile is a placeholder for cases when No Operating System is used
$(NAME)_INCLUDES :=
$(NAME)_SOURCES  := wiced/rtos.c

GLOBAL_INCLUDES := . \
                   wiced

RTOS_WWD_ONLY := TRUE