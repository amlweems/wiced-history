#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_COAP

$(NAME)_SOURCES := server/coap_server.c \
                   client/coap_client.c \
                   parser/coap_parser.c

GLOBAL_INCLUDES := .

$(NAME)_COMPONENTS := utilities/linked_list
