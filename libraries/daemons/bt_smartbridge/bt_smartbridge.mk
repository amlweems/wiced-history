#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_Bluetooth_SmartBridge

GLOBAL_DEFINES     += BT_TRACE_PROTOCOL=FALSE \
                      BT_USE_TRACES=FALSE
GLOBAL_INCLUDES := .

GLOBAL_INCLUDES +=  ./include \
		    ./internal\
		    ../../drivers/bluetooth/mfg_test/internal/transport/thread \
		    ../../drivers/bluetooth/mfg_test/internal/transport/driver \
                    ../../drivers/bluetooth/mfg_test/internal/packet \
		    ../../drivers/bluetooth/mfg_test/internal/bus


$(NAME)_SOURCES := wiced_bt_smartbridge.c \
                   wiced_bt_management.c \
		   smartbridge_stack_if.c \
		   smartbridge_helper.c \
                   wiced_bt_smartbridge_gatt.c \
                   internal/bt_smartbridge_socket_manager.c \
                   internal/bt_smartbridge_att_cache_manager.c \
		   ../../drivers/bluetooth/mfg_test/internal/transport/thread/bt_transport_thread.c \
		   ../../drivers/bluetooth/mfg_test/internal/packet/bt_packet.c \
		   ./attribute/bt_smart_attribute.c \
		   gatt/bt_smart_gatt.c \
		   wiced_bt_smartbridge_cfg.c
