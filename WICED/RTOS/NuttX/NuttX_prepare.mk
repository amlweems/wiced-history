#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

.PHONY: $(MAKECMDGOALS) configure_nuttx
$(MAKECMDGOALS): configure_nuttx

NUTTX_BOARD_DEFAULT := bcm4390x-wcd1_3
NUTTX_PLATFORM_DEFAULT := bcm4390x

NUTTX_COPY_COMMAND := "$(COMMON_TOOLS_PATH)cp" --remove-destination
NUTTX_RECURSIVE_COPY_COMMAND := $(NUTTX_COPY_COMMAND) --recursive
NUTTX_CREATE_DIR := "$(COMMON_TOOLS_PATH)mkdir" -p

NUTTX_DESTINATION_DIR := $(OUTPUT_DIR)/$(NUTTX_FULL_PATH)

ifeq (,$(NUTTX_BOARD))
ifneq (,$(wildcard $(NUTTX_FULL_PATH)/configs/$(PLATFORM)))
NUTTX_BOARD := $(PLATFORM)
else
NUTTX_BOARD := $(NUTTX_BOARD_DEFAULT)
endif
endif

ifeq (,$(NUTTX_PLATFORM))
NUTTX_PLATFORM := $(NUTTX_PLATFORM_DEFAULT)
endif

configure_nuttx:
	$(QUIET)$(ECHO) Configuring NuttX: NuttX board $(NUTTX_BOARD), NuttX platform $(NUTTX_PLATFORM), WICED platform $(PLATFORM)
	$(QUIET)$(ECHO) Create directories
	$(NUTTX_CREATE_DIR) $(NUTTX_DESTINATION_DIR)/include/nuttx
	$(NUTTX_CREATE_DIR) $(NUTTX_DESTINATION_DIR)/include/arch/chip
	$(NUTTX_CREATE_DIR) $(NUTTX_DESTINATION_DIR)/include/arch/board
	$(QUIET)$(ECHO) Copy math header
	$(NUTTX_COPY_COMMAND) $(NUTTX_FULL_PATH)/include/nuttx/math.h $(NUTTX_DESTINATION_DIR)/include/
	$(QUIET)$(ECHO) Copy arch headers
	$(NUTTX_RECURSIVE_COPY_COMMAND) $(NUTTX_FULL_PATH)/arch/arm/include/* $(NUTTX_DESTINATION_DIR)/include/arch/
	$(QUIET)$(ECHO) Copy chip headers
	$(NUTTX_RECURSIVE_COPY_COMMAND) $(NUTTX_FULL_PATH)/arch/arm/include/$(NUTTX_PLATFORM)/* $(NUTTX_DESTINATION_DIR)/include/arch/chip/
	$(QUIET)$(ECHO) Copy board headers
	$(NUTTX_RECURSIVE_COPY_COMMAND) $(NUTTX_FULL_PATH)/configs/$(NUTTX_BOARD)/include/* $(NUTTX_DESTINATION_DIR)/include/arch/board/
	$(QUIET)$(ECHO) Copy application configuration
	$(NUTTX_COPY_COMMAND) $(NUTTX_FULL_PATH)/configs/$(NUTTX_BOARD)/$(NUTTX_APP)/config.h $(NUTTX_DESTINATION_DIR)/include/nuttx/
