#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_HTTP_Server

$(NAME)_SOURCES    := http_server.c

$(NAME)_COMPONENTS := utilities/linked_list

$(NAME)_CFLAGS      = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)

GLOBAL_INCLUDES    := .