#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_Bluetooth_Internet_Gateway

GLOBAL_DEFINES     += BT_TRACE_PROTOCOL=FALSE \
                      BT_USE_TRACES=FALSE

GLOBAL_INCLUDES    += . \
                      internal

$(NAME)_COMPONENTS += utilities/linked_list \
                      drivers/bluetooth_le \
                      daemons/HTTP_server

$(NAME)_SOURCES    := internal/bt_internet_gateway.c\
                      internal/big_stack_interface.c \
                      internal/big_http_server.c
