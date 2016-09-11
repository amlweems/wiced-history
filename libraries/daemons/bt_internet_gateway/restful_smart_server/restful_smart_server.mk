#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_Bluetooth_RESTful_Smart_Server

$(NAME)_COMPONENTS += daemons/bt_internet_gateway

$(NAME)_SOURCES    := restful_smart_constants.c \
                      restful_smart_uri.c \
                      restful_smart_response.c \
                      restful_smart_ble.c

$(NAME)_INCLUDES   := .

GLOBAL_INCLUDES    := .

GLOBAL_DEFINES     += BIG_INCLUDES_RESTFUL_SMART_SERVER
