#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_Bluetooth_Embedded_Low_energy_Stack_for_WICED

GLOBAL_INCLUDES += . \
                   ../include \
                   ../BTE/WICED \
                   ../BTE/Components/stack/include \
                   ../BTE/Projects/bte/main \
                   ../BTE/Components/gki/common

BLUETOOTH_LIB_TYPE := low_energy

ifneq ($(wildcard $(CURDIR)BTE_$(BLUETOOTH_LIB_TYPE).$(RTOS).$(NETWORK).$(HOST_ARCH).release.a),)
$(NAME)_PREBUILT_LIBRARY := BTE_$(BLUETOOTH_LIB_TYPE).$(RTOS).$(NETWORK).$(HOST_ARCH).release.a
else
# Build from source (Broadcom internal)
include $(CURDIR)$(BLUETOOTH_LIB_TYPE)_src.mk
endif

# Include appropriate firmware as component
$(NAME)_COMPONENTS := libraries/drivers/bluetooth/firmware
