#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_Bluetooth_SmartBridge

GLOBAL_DEFINES  += BUILDCFG
GLOBAL_INCLUDES := .

GLOBAL_INCLUDES += ./include \
                   ./internal\

$(NAME)_SOURCES := wiced_bt_smartbridge.c \
                   wiced_bt_management.c \
                   wiced_bt_smartbridge_gatt.c \
                   wiced_bt_smartbridge_cfg.c \
                   internal/bt_smartbridge_stack_interface.c \
                   internal/bt_smartbridge_helper.c \
                   internal/bt_smartbridge_socket_manager.c \
                   internal/bt_smartbridge_att_cache_manager.c \
                   internal/bt_smart_attribute.c
