#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_Bluetooth_Low_Energy_Mesh

BLEMESH_LIBRARY_NAME := blemesh.$(RTOS).$(NETWORK).$(HOST_ARCH).release.a

ifneq ($(wildcard $(CURDIR)$(BLEMESH_LIBRARY_NAME)),)
$(NAME)_PREBUILT_LIBRARY := $(BLEMESH_LIBRARY_NAME)
else
# Build from source (Broadcom internal)
include $(CURDIR)blemesh_src.mk
endif # ifneq ($(wildcard $(CURDIR)$(BLEMESH_LIBRARY_NAME)),)

$(NAME)_COMPONENTS := daemons/bt_internet_gateway

$(NAME)_SOURCES    += blemesh_uri.c

GLOBAL_INCLUDES    += .

GLOBAL_DEFINES     += BIG_INCLUDES_BLE_MESH
