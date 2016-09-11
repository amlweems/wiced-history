#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

include $(SOURCE_ROOT)wiced_toolchain_common.mk

CONFIG_FILE := build/$(CLEANED_BUILD_STRING)/config.mk

include $(CONFIG_FILE)

# Include all toolchain makefiles - one of them will handle the architecture
include $(SOURCE_ROOT)wiced_toolchain_ARM_GNU.mk

include $(SOURCE_ROOT)wiced_resources.mk

.PHONY: display_map_summary build_done

##################################
# Filenames
##################################

LINK_OUTPUT_FILE          := $(OUTPUT_DIR)/Binary/$(CLEANED_BUILD_STRING)$(LINK_OUTPUT_SUFFIX)
STRIPPED_LINK_OUTPUT_FILE :=$(LINK_OUTPUT_FILE:$(LINK_OUTPUT_SUFFIX)=.stripped$(LINK_OUTPUT_SUFFIX))
FINAL_OUTPUT_FILE         :=$(LINK_OUTPUT_FILE:$(LINK_OUTPUT_SUFFIX)=$(FINAL_OUTPUT_SUFFIX))
MAP_OUTPUT_FILE           :=$(LINK_OUTPUT_FILE:$(LINK_OUTPUT_SUFFIX)=.map)
MAP_CSV_OUTPUT_FILE       :=$(LINK_OUTPUT_FILE:$(LINK_OUTPUT_SUFFIX)=_map.csv)

LINK_DCT_FILE             := $(OUTPUT_DIR)/DCT$(LINK_OUTPUT_SUFFIX)
STRIPPED_LINK_DCT_FILE    :=$(LINK_DCT_FILE:$(LINK_OUTPUT_SUFFIX)=.stripped$(LINK_OUTPUT_SUFFIX))
FINAL_DCT_FILE            :=$(LINK_DCT_FILE:$(LINK_OUTPUT_SUFFIX)=$(FINAL_OUTPUT_SUFFIX))
MAP_DCT_FILE              :=$(LINK_DCT_FILE:$(LINK_OUTPUT_SUFFIX)=.map)

OPENOCD_LOG_FILE          ?= $(OUTPUT_DIR)/openocd_log.txt

LIBS_DIR                  := $(OUTPUT_DIR)/Libraries
LINK_OPTS_FILE            := $(OUTPUT_DIR)/Binary/link.opts


ifeq (,$(SUB_BUILD))
ifneq (,$(PLATFORM_TARGETS_MAKEFILE))
include $(PLATFORM_TARGETS_MAKEFILE)
endif
endif

##################################
# Macros
##################################

###############################################################################
# MACRO: GET_BARE_LOCATION
# Returns a the location of the given component relative to source-tree-root
# rather than from the cwd
# $(1) is component
GET_BARE_LOCATION =$(patsubst $(call ESCAPE_BACKSLASHES,$(SOURCE_ROOT))%,%,$(strip $($(call CONV_COMP,$(1))_LOCATION)))


###############################################################################
# MACRO: BUILD_C_RULE
# Creates a target for building C language files (*.c)
# $(1) is component, $(2) is the source file
define BUILD_C_RULE
-include $(OUTPUT_DIR)/Modules/$(call GET_BARE_LOCATION,$(1))$(2:.c=.d)
$(OUTPUT_DIR)/Modules/$(call GET_BARE_LOCATION,$(1))$(2:.c=.o): $(strip $($(1)_LOCATION))$(2) $(CONFIG_FILE) $$(dir $(OUTPUT_DIR)/Modules/$(call GET_BARE_LOCATION,$(1))$(2)).d $(RESOURCES_DEPENDENCY) $(LIBS_DIR)/$(1).c_opts | $(PLATFORM_PRE_BUILD_TARGETS)
	$$(if $($(1)_START_PRINT),,$(eval $(1)_START_PRINT:=1) $(QUIET)$(ECHO) Compiling $(1) ) 
	$(QUIET)$(CC) $(OPTIONS_IN_FILE_OPTION)$(LIBS_DIR)/$(1).c_opts -o $$@ $$< $(COMPILER_SPECIFIC_STDOUT_REDIRECT) 
endef


###############################################################################
# MACRO: BUILD_CPP_RULE
# Creates a target for building C++ language files (*.cpp)
# $(1) is component name, $(2) is the source file
define BUILD_CPP_RULE
-include $(OUTPUT_DIR)/Modules/$(call GET_BARE_LOCATION,$(1))$(2:.cpp=.d)
$(OUTPUT_DIR)/Modules/$(call GET_BARE_LOCATION,$(1))$(2:.cpp=.o): $(strip $($(1)_LOCATION))$(2) $(CONFIG_FILE) $$(dir $(OUTPUT_DIR)/Modules/$(call GET_BARE_LOCATION,$(1))$(2)).d $(RESOURCES_DEPENDENCY) $(LIBS_DIR)/$(1).cpp_opts | $(PLATFORM_PRE_BUILD_TARGETS) 
	$$(if $($(1)_START_PRINT),,$(eval $(1)_START_PRINT:=1) $(ECHO) Compiling $(1))
	$(QUIET)$(CXX) $(OPTIONS_IN_FILE_OPTION)$(LIBS_DIR)/$(1).cpp_opts -o $$@ $$<  $(COMPILER_SPECIFIC_STDOUT_REDIRECT) 
endef

###############################################################################
# MACRO: BUILD_S_RULE
# Creates a target for building Assembly language files (*.s & *.S)
# $(1) is component name, $(2) is the source file
define BUILD_S_RULE
$(OUTPUT_DIR)/Modules/$(call GET_BARE_LOCATION,$(1))$(strip $(patsubst %.S, %.o, $(2:.s=.o) )): $(strip $($(1)_LOCATION))$(2) $(CONFIG_FILE) $$(dir $(OUTPUT_DIR)/Modules/$(call GET_BARE_LOCATION,$(1))$(strip $(patsubst %.S, %.o, $(2)))).d $(RESOURCES_DEPENDENCY) | $(PLATFORM_PRE_BUILD_TARGETS)
	$$(if $($(1)_START_PRINT),,$(eval $(1)_START_PRINT:=1) $(ECHO) Compiling $(1))
	
	$(QUIET)$(AS) $($(1)_ASMFLAGS)  -o $$@ $$< $(WICED_INCLUDES) $(ASM_COMPILER_FLAGS) $(COMPILER_SPECIFIC_STDOUT_REDIRECT) 
endef

###############################################################################
# MACRO: BUILD_COMPONENT_RULES
# Creates targets for building an entire component
# Target for the component static library is created in this macro
# Targets for source files are created by calling the macros defined above
# $(1) is component name
define BUILD_COMPONENT_RULES

$(eval LINK_LIBS +=$(if $($(1)_SOURCES),$(LIBS_DIR)/$(1).a))

# Make a list of the object files that will be used to build the static library
$(eval $(1)_LIB_OBJS := $(addprefix $(strip $(OUTPUT_DIR)/Modules/$(call GET_BARE_LOCATION,$(1))),  $(filter %.o, $($(1)_SOURCES:.cpp=.o) $($(1)_SOURCES:.c=.o) $($(1)_SOURCES:.s=.o) $($(1)_SOURCES:.S=.o)))  $(patsubst %.c,%.o,$(call RESOURCE_FILENAME, $($(1)_RESOURCES))))

$(LIBS_DIR)/$(1).c_opts: $(CONFIG_FILE) | $(LIBS_DIR)
	$(QUIET)$$(call WRITE_FILE_CREATE, $$@ ,$(COMPILER_SPECIFIC_COMP_ONLY_FLAG) $(COMPILER_SPECIFIC_DEPS_FLAG) $($(1)_CFLAGS) $($(1)_INCLUDES) $($(1)_DEFINES) $(WICED_INCLUDES) $(WICED_DEFINES))
 
$(LIBS_DIR)/$(1).cpp_opts: $(CONFIG_FILE) | $(LIBS_DIR)
	 $(QUIET)$$(call WRITE_FILE_CREATE, $$@ ,$(COMPILER_SPECIFIC_COMP_ONLY_FLAG) $(COMPILER_SPECIFIC_DEPS_FLAG) $($(1)_CXXFLAGS)  $($(1)_INCLUDES) $($(1)_DEFINES) $(WICED_INCLUDES) $(WICED_DEFINES)) 

#$(LIBS_DIR)/$(1).as_opts: | $(LIBS_DIR)
#	$(QUIET)$$(call WRITE_FILE_CREATE, $$@ ,$($(1)_ASMFLAGS))  

$(LIBS_DIR)/$(1).ar_opts: $(CONFIG_FILE) | $(LIBS_DIR)
	$(QUIET)$$(call WRITE_FILE_CREATE, $$@ ,$($(1)_LIB_OBJS))
	
# Target for build-from-source
$(OUTPUT_DIR)/Libraries/$(1).a: $$($(1)_LIB_OBJS) $(OUTPUT_DIR)/Libraries/$(1).ar_opts
	$(QUIET)$(AR) $(WICED_ARFLAGS) $(COMPILER_SPECIFIC_ARFLAGS_CREATE) $$@ $(OPTIONS_IN_FILE_OPTION)$(OUTPUT_DIR)/Libraries/$(1).ar_opts
	
# Create targets to built the component's source files into object files
$(foreach src, $(filter %.c, $($(1)_SOURCES)),$(eval $(call BUILD_C_RULE,$(1),$(src))))
$(foreach src, $(filter %.cpp, $($(1)_SOURCES)),$(eval $(call BUILD_CPP_RULE,$(1),$(src))))
$(foreach src, $(filter %.s %.S, $($(1)_SOURCES)),$(eval $(call BUILD_S_RULE,$(1),$(src))))
endef

##################################
# Processing
##################################

# Create targets for resource files
ALL_RESOURCES := $(sort $(foreach comp,$(COMPONENTS),$($(comp)_RESOURCES)))
$(eval $(if $(ALL_RESOURCES),$(call CREATE_ALL_RESOURCE_TARGETS,$(ALL_RESOURCES))))
LINK_LIBS += $(RESOURCES_LIBRARY)

# Create targets for components
$(foreach comp,$(COMPONENTS),$(eval $(call BUILD_COMPONENT_RULES,$(comp))))

LINK_LIBS += $(WICED_PREBUILT_LIBRARIES)
##################################
# Build rules
##################################


$(LIBS_DIR):
	$(QUIET)$(call MKDIR, $@)

# Directory dependency - causes mkdir to be called once for each directory.
%/.d:
	$(QUIET)$(call MKDIR, $(dir $@))
	$(QUIET)$(TOUCH) $(@)

# Bin file target - uses objcopy to convert the stripped elf into a binary file
$(FINAL_OUTPUT_FILE): $(STRIPPED_LINK_OUTPUT_FILE)
	$(QUIET)$(ECHO) Making $(notdir $@)
	$(QUIET)$(OBJCOPY) -O binary -R .eh_frame -R .init -R .fini -R .comment -R .ARM.attributes $< $@

# Stripped elf file target - Strips the full elf file and outputs to a new .stripped.elf file
$(STRIPPED_LINK_OUTPUT_FILE): $(LINK_OUTPUT_FILE)
	$(QUIET)$(STRIP) -o $@ $(STRIPFLAGS) $<

$(LINK_OPTS_FILE):
#$(COMPILER_SPECIFIC_LINK_MAP) $(MAP_OUTPUT_FILE) $(LINK_OPTS_FILE)
	$(QUIET)$(call WRITE_FILE_CREATE, $@ ,$(WICED_LDFLAGS) $(WICED_LINK_SCRIPT_CMD) $(call COMPILER_SPECIFIC_LINK_MAP,$(MAP_OUTPUT_FILE))  $(call COMPILER_SPECIFIC_LINK_FILES, $(WICED_LINK_FILES) $(filter %.a,$^) $(LINK_LIBS)) )

# Linker output target - This links all component & resource libraries and objects into an output executable
# CXX is used for compatibility with C++
$(LINK_OUTPUT_FILE): $(LINK_LIBS) $(WICED_LINK_SCRIPT) $(LINK_OPTS_FILE)
	$(QUIET)$(ECHO) Making $(notdir $@)
	$(QUIET)$(LINKER) -o  $@ $(OPTIONS_IN_FILE_OPTION)$(LINK_OPTS_FILE) $(COMPILER_SPECIFIC_STDOUT_REDIRECT)
	$(QUIET)$(ECHO_BLANK_LINE)
	$(QUIET)$(call COMPILER_SPECIFIC_MAPFILE_TO_CSV,$(MAP_OUTPUT_FILE),$(MAP_CSV_OUTPUT_FILE))

display_map_summary: $(LINK_OUTPUT_FILE)
	$(QUIET) $(call COMPILER_SPECIFIC_MAPFILE_DISPLAY_SUMMARY,$(MAP_OUTPUT_FILE))
	

# Device Config Table (DCT) binary file target - compiles, links, strips, and objdumps DCT source into a binary output file
$(LINK_DCT_FILE): $(OUTPUT_DIR)/generated_security_dct.h $(WICED_DCT_LINK_SCRIPT) $(SOURCE_ROOT)Wiced/internal/dct.c $(WICED_APPLICATION_DCT) $(WICED_WIFI_CONFIG_DCT_H) $(CONFIG_FILE) | $(PLATFORM_PRE_BUILD_TARGETS)
	$(QUIET)$(ECHO) Making DCT image
	$(QUIET)$(CC) $(CPU_CFLAGS) $(COMPILER_SPECIFIC_COMP_ONLY_FLAG)  $(SOURCE_ROOT)Wiced/internal/dct.c $(WICED_DEFINES) $(WICED_INCLUDES) $(COMPILER_SPECIFIC_DEBUG_CFLAGS)  $(call ADD_COMPILER_SPECIFIC_STANDARD_CFLAGS, ) -I$(OUTPUT_DIR) -I$(SOURCE_ROOT). -o $(OUTPUT_DIR)/internal_dct.o $(COMPILER_SPECIFIC_STDOUT_REDIRECT)
	$(if $(WICED_APPLICATION_DCT),$(QUIET)$(CC) $(CPU_CFLAGS) $(COMPILER_SPECIFIC_COMP_ONLY_FLAG)  $(WICED_APPLICATION_DCT) $(WICED_DEFINES) $(WICED_INCLUDES) $(COMPILER_SPECIFIC_DEBUG_CFLAGS)  $(call ADD_COMPILER_SPECIFIC_STANDARD_CFLAGS, ) -I$(OUTPUT_DIR) -I$(SOURCE_ROOT). -o $(OUTPUT_DIR)/app_dct.o)
	$(QUIET)$(LINKER) $(WICED_LDFLAGS) $(WICED_DCT_LINK_CMD) $(call COMPILER_SPECIFIC_LINK_MAP,$(MAP_DCT_FILE)) -o $@  $(OUTPUT_DIR)/internal_dct.o $(if $(WICED_APPLICATION_DCT),$(OUTPUT_DIR)/app_dct.o)  $(COMPILER_SPECIFIC_STDOUT_REDIRECT) 

$(STRIPPED_LINK_DCT_FILE): $(LINK_DCT_FILE)
	$(QUIET)$(STRIP) -o $@ $(STRIPFLAGS) $<

$(FINAL_DCT_FILE): $(STRIPPED_LINK_DCT_FILE)
	$(QUIET)$(OBJCOPY) -O binary -R .eh_frame -R .init -R .fini -R .comment -R .ARM.attributes $< $@

# Certificates header target - Converts and concatenates text certificate files into a header file
$(OUTPUT_DIR)/generated_security_dct.h: $(WICED_CERTIFICATE) $(WICED_PRIVATE_KEY) | $(PLATFORM_PRE_BUILD_TARGETS)
	$(QUIET)$(ECHO) Creating security credentials
	$(QUIET)$(PERL) $(TOOLS_ROOT)/text_to_c/certs_to_h.pl CERTIFICATE_STRING $(WICED_CERTIFICATE) > $@
	$(QUIET)$(PERL) $(TOOLS_ROOT)/text_to_c/certs_to_h.pl PRIVATE_KEY_STRING $(WICED_PRIVATE_KEY) >> $@

# Main Target - Ensures the required parts get built
build_done: $(PLATFORM_PRE_BUILD_TARGETS) $(FINAL_OUTPUT_FILE) $(if $(filter 1,$(NODCT)),,$(FINAL_DCT_FILE)) display_map_summary 

$(BUILD_STRING): build_done $(PLATFORM_POST_BUILD_TARGETS)

# Stack usage target - Currently not working outputs a CSV file showing function stack usage
$(OUTPUT_DIR)/stack_usage.csv: $(OUTPUT_DIR)/Binary/$(CLEANED_BUILD_STRING)$(LINK_OUTPUT_SUFFIX)
	$(QUIET)$(ECHO) Extracting call tree
	$(QUIET)cd $(OUTPUT_DIR); find -name *.optimized | xargs cat > call_tree.txt
	$(QUIET)$(ECHO) Extracting individual stack usage
	$(QUIET)cd $(OUTPUT_DIR); find -name *.su | xargs cat > stack_usage.txt
	$(QUIET)$(ECHO) Processing stack data
	$(QUIET)cd $(OUTPUT_DIR); $(QUIET)$(PERL) $(TOOLS_ROOT)/stack_usage/stack_usage/stack_usage.pl > $@


