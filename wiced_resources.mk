#
# Copyright 2013, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

# filters for resources
TEXT_FILTERS   := %.html %.htm %.txt %.eml %.js %.css %.dat %.cer %.pem %.json %.xml
BINARY_FILTERS := %.jpg %.jpeg %.png %.ico %.gif
ALL_RESOURCES := $(sort $(foreach comp,$(COMPONENTS),$($(comp)_RESOURCES)))

###############################################################################
# MACRO: RESOURCE_FILENAME
# Makes an output path C file for a resource file, converting dots to underscores
# $(1) is resource filename in the resource directory
RESOURCE_FILENAME      =$(addprefix $(OUTPUT_DIR)/Resources/,$(addsuffix .c,$(subst .,_,$(notdir $(1)))))

###############################################################################
# MACRO: RESOURCE_VARIABLE_NAME
# Creates a variable name that will be used for a resource from it's filename
# slashes are converted to _DIR_ and dots to underscores
# $(1) is resource filename in the resource directory
RESOURCE_VARIABLE_NAME =$(addprefix resource_,$(subst /,_DIR_,$(subst .,_,$(subst $(SOURCE_ROOT)Resources/,,$(1)))))

###############################################################################
# MACRO: BUILD_RESOURCE_RULES
# Creates targets to build a resource file
# the first target converts the text resource file to a C file
# the second target compiles the C resource file into an object file
# $(1) is the name of a resource
define BUILD_RESOURCE_RULES
$(call RESOURCE_FILENAME, $(1)): $(1) $(STAGING_DIR).d
$(call RESOURCE_FILENAME, $(1)): $(1)
	$$(if $(RESOURCES_START_PRINT),,$(eval RESOURCES_START_PRINT:=1) $(QUIET)$(ECHO) Converting resources)
	$$(if $(filter $(TEXT_FILTERS),$(1)),$(QUIET)$(PERL) $(TOOLS_ROOT)/text_to_c/text_to_c.pl $(call RESOURCE_VARIABLE_NAME, $(1)) $(1) > $$@)
	$$(if $(filter $(BINARY_FILTERS),$(1)),$(QUIET)$(BIN2C) $(1) $$@ $(call RESOURCE_VARIABLE_NAME,$(1)))

$(patsubst %.c,%.o,$(call RESOURCE_FILENAME, $(1))): $(call RESOURCE_FILENAME, $(1))
ifeq (IAR,$(TOOLCHAIN_NAME))
	$(QUIET)$(CC) $(COMPILER_SPECIFIC_COMP_ONLY_FLAG) $(COMPILER_SPECIFIC_DEPS_FLAG) $(RESOURCE_CFLAGS) $($(1)_CFLAGS) -o $$@ $$< >> $$(IAR_BUILD_RESULTS_FILE)
else
	$(QUIET)$(CC) $(COMPILER_SPECIFIC_COMP_ONLY_FLAG) $(COMPILER_SPECIFIC_DEPS_FLAG) $(RESOURCE_CFLAGS) $($(1)_CFLAGS) -o $$@ $$<
endif

$(eval RESOURCE_OBJS += $(patsubst %.c,%.o,$(call RESOURCE_FILENAME, $(1))))

endef

###############################################################################
# MACRO: CREATE_ALL_RESOURCE_TARGETS
# Create build targets which convert resources from binary to C files and build 
# the C files
# Also creates a target for the overall resources variables header file
# $(1) is a list of resources
define CREATE_ALL_RESOURCE_TARGETS

$(foreach RESOURCE,$(1),$(eval $(call BUILD_RESOURCE_RULES,$(RESOURCE))))

$(STAGING_DIR).d: 
	$(QUIET)$$(call MKDIR, $$(dir $$@))
	$(QUIET)$(TOUCH) $$(@)

# Target for build-from-source
# The repeated lines avoid line-too-long errors in windows
$(OUTPUT_DIR)/Libraries/Resources.a: $$(RESOURCE_OBJS)
	$(QUIET)$(RM) $$@
	$$(if $$(wordlist 1,50,     $$(RESOURCE_OBJS)),$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_CREATE) $$@ $$(wordlist 1,50,     $$(RESOURCE_OBJS)))
	$$(if $$(wordlist 51,100,   $$(RESOURCE_OBJS)),$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_ADD) $$@ $$(wordlist 51,100,   $$(RESOURCE_OBJS)))
	$$(if $$(wordlist 101,150,  $$(RESOURCE_OBJS)),$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_ADD) $$@ $$(wordlist 101,150,  $$(RESOURCE_OBJS)))
	$$(if $$(wordlist 151,200,  $$(RESOURCE_OBJS)),$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_ADD) $$@ $$(wordlist 151,200,  $$(RESOURCE_OBJS)))
	$$(if $$(wordlist 201,250,  $$(RESOURCE_OBJS)),$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_ADD) $$@ $$(wordlist 201,250,  $$(RESOURCE_OBJS)))
	$$(if $$(wordlist 251,300,  $$(RESOURCE_OBJS)),$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_ADD) $$@ $$(wordlist 251,300,  $$(RESOURCE_OBJS)))
	$$(if $$(wordlist 301,350,  $$(RESOURCE_OBJS)),$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_ADD) $$@ $$(wordlist 301,350,  $$(RESOURCE_OBJS)))
	$$(if $$(wordlist 351,400,  $$(RESOURCE_OBJS)),$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_ADD) $$@ $$(wordlist 351,400,  $$(RESOURCE_OBJS)))
	$$(if $$(wordlist 401,450,  $$(RESOURCE_OBJS)),$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_ADD) $$@ $$(wordlist 401,450,  $$(RESOURCE_OBJS)))
	$$(if $$(wordlist 451,500,  $$(RESOURCE_OBJS)),$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_ADD) $$@ $$(wordlist 451,500,  $$(RESOURCE_OBJS)))
	$$(if $$(wordlist 501,550,  $$(RESOURCE_OBJS)),$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_ADD) $$@ $$(wordlist 501,550,  $$(RESOURCE_OBJS)))
	$$(if $$(wordlist 551,600,  $$(RESOURCE_OBJS)),$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_ADD) $$@ $$(wordlist 551,600,  $$(RESOURCE_OBJS)))
	$$(if $$(wordlist 601,650,  $$(RESOURCE_OBJS)),$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_ADD) $$@ $$(wordlist 601,650,  $$(RESOURCE_OBJS)))
	$$(if $$(wordlist 651,700,  $$(RESOURCE_OBJS)),$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_ADD) $$@ $$(wordlist 651,700,  $$(RESOURCE_OBJS)))
	$$(if $$(wordlist 701,750,  $$(RESOURCE_OBJS)),$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_ADD) $$@ $$(wordlist 701,750,  $$(RESOURCE_OBJS)))
	$$(if $$(wordlist 751,1000, $$(RESOURCE_OBJS)),$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_ADD) $$@ $$(wordlist 751,1000, $$(RESOURCE_OBJS)))


RESOURCE_HEADER_TARGET_CREATED := 1

$(eval RESOURCE_C_FILES += $(call RESOURCE_FILENAME, $(1)))

$(OUTPUT_DIR)/Resources/resources.h: $$(RESOURCE_C_FILES)

endef


# Resources header target - creates a header file of all the resource variables.
$(OUTPUT_DIR)/Resources/resources.h:  $(CONFIG_FILE)
	$(QUIET)$(ECHO) $(QUOTES_FOR_ECHO)/* Automatically generated file - this comment ensures resources.h file creation */$(QUOTES_FOR_ECHO) > $@
	$(if $(RESOURCE_C_FILES), $(QUIET)$(PERL) $(TOOLS_ROOT)/text_to_c/resources_header.pl $(RESOURCE_C_FILES) >> $@)


RESOURCES_DEPENDENCY = $(OUTPUT_DIR)/Resources/resources.h $(if $(RESOURCE_HEADER_TARGET_CREATED), $(OUTPUT_DIR)/Libraries/Resources.a, )
RESOURCES_LIBRARY = $(if $(RESOURCE_HEADER_TARGET_CREATED),$(OUTPUT_DIR)/Libraries/Resources.a)

# Expand the list of resources to point to the full location (either component local or the common Resources directory)
# $(1) is the resource name, $(2) is the current directory
RESOURCE_EXPAND_DIRECTORY = $(foreach res,$($(1)_RESOURCES),$(word 1,$(wildcard $(addsuffix $(res),$(2) $(SOURCE_ROOT)Resources/))))
RESOURCE_EXPAND_DIRECTORY = $$($(1)_RESOURCES)


ifneq ($(filter SFLASH_FILESYSTEM,$(RESOURCES_LOCATION)),)

RESOURCE_DOWNLOADER_TARGET := $(if $(findstring sflash_write,$(APP)),,external_resource_downloader)  
RESOURCE_DOWNLOAD = download_all_resources  

download_all_resources: $(RESOURCES_DEPENDENCY) $(RESOURCE_DOWNLOADER_TARGET) $(RESOURCE_DOWNLOADS) 

make_filesystem: $(RESOURCES_DEPENDENCY)
	


SFLASH_WRITE_TARGET := waf.sflash_write-NoOS-NoNS-$(PLATFORM_FULL)-$(BUS)
SFLASH_WRITE_LOG_FILE ?= build/sflash_write.log
SFLASH_WRITE_OUTFILE := $(BUILD_DIR)/$(call CONV_COMP,$(subst .,/,$(SFLASH_WRITE_TARGET)))/Binary/$(call CONV_COMP,$(subst .,/,$(SFLASH_WRITE_TARGET)))
ifneq ($(VERBOSE),1)
SFLASH_WRITE_REDIRECT	= > $(SFLASH_WRITE_LOG_FILE)
endif

external_resource_downloader:
	$(QUIET)$(ECHO) Building serial flash downloader
	$(QUIET)$(ECHO_BLANK_LINE)
	$(QUIET)$(MAKE) -r $(SILENT) -f $(SOURCE_ROOT)Makefile $(SFLASH_WRITE_TARGET) NO_BUILD_BOOTLOADER=1 -I$(OUTPUT_DIR)  SFLASH= $(SFLASH_WRITE_REDIRECT)

endif