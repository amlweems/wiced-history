#
# Copyright 2013, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

TOOLCHAIN_PATH    := /mingw/bin/
TOOLS_PATH    := /bin/
TOOLCHAIN_PREFIX  :=
#PATH :=

# MinGW settings
TOOLCHAIN_SUFFIX  := .exe
OPENOCD_FULL_NAME := $(OPENOCD_PATH)openocd-all-brcm-libftdi
PRINT_SLASH:=\\\\
SLASH_QUOTE:=\\\"
ECHO_BLANK_LINE   := echo
PERL    := "$(TOOLS_PATH)perl"
CLEAN_COMMAND := $(TOOLS_PATH)rm -rf build
MKDIR   = "$(TOOLS_PATH)mkdir$(TOOLCHAIN_SUFFIX)" -p $1
CONV_SLASHES = $1

# $(1) is the content, $(2) is the file to print to.
define PRINT
@$(ECHO) '$(1)'>>$(2)

endef


# Set shortcuts to the compiler and other tools
CC      := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)gcc$(TOOLCHAIN_SUFFIX)"
CXX     := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)g++$(TOOLCHAIN_SUFFIX)"
AS      := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)as$(TOOLCHAIN_SUFFIX)"
AR      := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)ar$(TOOLCHAIN_SUFFIX)"
OBJDUMP := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)objdump$(TOOLCHAIN_SUFFIX)"
OBJCOPY := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)objcopy$(TOOLCHAIN_SUFFIX)"
RM      := "$(TOOLS_PATH)rm$(TOOLCHAIN_SUFFIX)" -f
CP      := "$(TOOLS_PATH)cp$(TOOLCHAIN_SUFFIX)" -f
STRIP   := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)strip$(TOOLCHAIN_SUFFIX)"
ECHO    := "$(TOOLS_PATH)echo"
MAKE    := "$(TOOLS_PATH)make$(TOOLCHAIN_SUFFIX)"

LINK_OUTPUT_SUFFIX :=.exe
FINAL_OUTPUT_SUFFIX :=.exe

BUILD_STRING := $(strip $(filter-out clean debug download download_only run terminal, $(MAKECMDGOALS)))
BUILD_DIR    :=  build
OUTPUT_DIR   := $(BUILD_DIR)/$(BUILD_STRING)

