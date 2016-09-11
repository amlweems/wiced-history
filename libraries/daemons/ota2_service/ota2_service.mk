#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_OTA2_Service

GLOBAL_DEFINES     += WPRINT_ENABLE_LIB_DEBUG

$(NAME)_SOURCES := wiced_ota2_service.c

GLOBAL_INCLUDES := .

$(NAME)_COMPONENTS := filesystems/ota2
