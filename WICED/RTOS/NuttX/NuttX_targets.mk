#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

# Define current version
NUTTX_VERSION := 7.8

# Define NuttX sources directory
NUTTX_DIR := ver$(NUTTX_VERSION)

# Define full path to NuttX
NUTTX_FULL_PATH := WICED/RTOS/NuttX/$(NUTTX_DIR)/

# Override the toolchain system directory to point
# to the NuttX installation.
COMPILER_SPECIFIC_SYSTEM_DIR := -isystem $(SOURCE_ROOT)/$(NUTTX_FULL_PATH)/include -isystem $(OUTPUT_DIR)/$(NUTTX_FULL_PATH)/include/
