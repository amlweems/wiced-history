#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_MQTT_Client

$(NAME)_SOURCES :=  mqtt_network.c  \
                    mqtt_frame.c    \
                    mqtt_connection.c \
                    mqtt_manager.c  \
                    mqtt_session.c \
                    mqtt_api.c
#make it visible for the applications which take advantage of this lib
GLOBAL_INCLUDES := .


$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)