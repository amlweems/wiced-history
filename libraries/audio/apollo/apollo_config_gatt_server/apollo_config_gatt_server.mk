#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_apollo_config_GATT_server

GLOBAL_INCLUDES  += .

$(NAME)_HEADERS := apollo_config_gatt_server.h

$(NAME)_SOURCES := \
					apollo_config_gatt_server.c \
					wiced_bt_cfg.c

$(NAME)_COMPONENTS := libraries/drivers/bluetooth
$(NAME)_COMPONENTS += audio/apollo/apollocore

$(NAME)_INCLUDES   := \
					. \
					../apollocore \
					../apollo_streamer