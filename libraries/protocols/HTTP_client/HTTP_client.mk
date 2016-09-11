#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_HTTP_Client

$(NAME)_SOURCES    := http.c \
                      http_client.c

$(NAME)_COMPONENTS := utilities/linked_list

# make it visible for the applications which take advantage of this lib
GLOBAL_INCLUDES := .
