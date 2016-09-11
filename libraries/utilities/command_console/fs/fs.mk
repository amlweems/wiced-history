#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_command_console_FS

$(NAME)_SOURCES := command_console_fs.c fs_test.c
GLOBAL_INCLUDES := .
GLOBAL_DEFINES  := COMMAND_CONSOLE_FS_ENABLED