#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_Bluetooth_Internet_Gateway

GLOBAL_DEFINES     += BUILDCFG
# enable bluetooth traces/logging - you can define this flag either from the application or from this daemon
# but not from both. This daemon is used by multiple apps in apps/demo/bt_internet_gateway
#GLOBAL_DEFINES     += ENABLE_BT_PROTOCOL_TRACES

GLOBAL_INCLUDES    += . \
                      internal

$(NAME)_COMPONENTS += utilities/linked_list \
                      drivers/bluetooth/low_energy \
                      daemons/HTTP_server

$(NAME)_SOURCES    := internal/bt_internet_gateway.c\
                      internal/big_stack_interface.c \
                      internal/big_http_server.c
