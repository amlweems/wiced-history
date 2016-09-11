#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_Bluetooth_HTTP_Proxy_Server

$(NAME)_SOURCES    := bt_http_proxy_server.c

$(NAME)_COMPONENTS := daemons/bt_internet_gateway

GLOBAL_INCLUDES    := .
