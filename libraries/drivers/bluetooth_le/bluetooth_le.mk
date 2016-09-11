#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_WICED_Bluetooth_Low_Energy_Driver_for_BCM$(BT_CHIP)$(BT_CHIP_REVISION)

GLOBAL_INCLUDES += include

# Include BTE stack as component
$(NAME)_COMPONENTS := libraries/drivers/bluetooth_le/BTE

# Use Firmware images which are already present
ifeq ($(BT_CHIP_XTAL_FREQUENCY),)
$(NAME)_SOURCES := ../bluetooth/firmware/$(BT_CHIP)$(BT_CHIP_REVISION)/bt_firmware_image.c
else
$(NAME)_SOURCES := ../bluetooth/firmware/$(BT_CHIP)$(BT_CHIP_REVISION)/$(BT_CHIP_XTAL_FREQUENCY)/bt_firmware_image.c
endif
