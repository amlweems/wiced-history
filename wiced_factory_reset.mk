#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

# Separate the DCT value file from the actual build target
DCT_VALUE_FILE :=$(word 2, $(MAKECMDGOALS))
EXTENSION      :=$(notdir $(DCT_VALUE_FILE:.txt=))
MAKECMDGOALS   :=$(word 1, $(MAKECMDGOALS))

include $(SOURCE_ROOT)wiced_toolchain_common.mk

CONFIG_FILE := build/$(CLEANED_BUILD_STRING)/config.mk

include $(CONFIG_FILE)

# Include all toolchain makefiles - one of them will handle the architecture
include $(SOURCE_ROOT)wiced_toolchain_ARM_GNU.mk

.PHONY: $(BUILD_STRING)

##################################
# Build rules
##################################

COMMA :=,

CFLAGS :=
CFLAGS += -I$(SOURCE_ROOT)Wiced/WWD/include
CFLAGS += -I$(SOURCE_ROOT)Wiced/RTOS/NoOS/wwd
CFLAGS += -I$(SOURCE_ROOT)Wiced/Network/NoNS/wwd
CFLAGS += -nostartfiles
CFLAGS += $(addprefix -Wl$(COMMA)-T ,$(WICED_DCT_LINK_SCRIPT))

$(BUILD_STRING):
	$(QUIET)$(ECHO) Making Factory Reset DCT image
	$(QUIET)$(call MKDIR, $(OUTPUT_DIR)/factory_reset)
	$(CAT) $(call CONV_SLASHES, $(DCT_VALUE_FILE)) | $(PERL) $(TOOLS_ROOT)/create_dct/generate_factory_reset_dct.pl $(SOURCE_ROOT)Apps/$(APP_FULL)/factory_reset_dct.c > $(OUTPUT_DIR)/factory_reset/factory_reset_dct_$(EXTENSION).c
	$(QUIET)$(CC) $(CFLAGS) $(OUTPUT_DIR)/factory_reset/factory_reset_dct_$(EXTENSION).c $(WICED_DEFINES) -I$(OUTPUT_DIR) -I$(SOURCE_ROOT). -I$(SOURCE_ROOT)Apps/$(APP_FULL) -o $(OUTPUT_DIR)/factory_reset/factory_reset_dct_$(EXTENSION).elf
	$(QUIET)$(STRIP) -o $(OUTPUT_DIR)/factory_reset/factory_reset_dct_$(EXTENSION).stripped.elf $(STRIPFLAGS) $(OUTPUT_DIR)/factory_reset/factory_reset_dct_$(EXTENSION).elf
	$(QUIET)$(OBJCOPY) -O binary -R .eh_frame -R .init -R .fini -R .comment -R .ARM.attributes $(OUTPUT_DIR)/factory_reset/factory_reset_dct_$(EXTENSION).stripped.elf $(OUTPUT_DIR)/factory_reset/factory_reset_dct_$(EXTENSION).bin
