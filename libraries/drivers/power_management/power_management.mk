#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME                := power_management

GLOBAL_INCLUDES     := .

$(NAME)_SOURCES     := power_management.c

$(NAME)_COMPONENTS  := drivers/power_management/max17040 \
                       drivers/power_management/max8971

ifneq (,$(findstring BCM943907WAE_1,$(PLATFORM)))
$(info enabling power management on $(PLATFORM))
GLOBAL_DEFINES  += POWER_MANAGEMENT_ON_BCM943907WAE_1
endif

KEEP_LIST           := power_management.h
