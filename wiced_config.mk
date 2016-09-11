#
# Copyright 2013, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

include $(SOURCE_ROOT)wiced_toolchain_common.mk
CONFIG_FILE_DIR := build/$(CLEANED_BUILD_STRING)
CONFIG_FILE := $(CONFIG_FILE_DIR)/config.mk

COMPONENT_DIRECTORIES := Wiced/Platform \
                         Wiced/Network \
                         Apps \
                         Wiced/RTOS \
                         Wiced/Security \
                         Library \
                         .

WICED_VERSION ?= 2.4.0

##################################
# Macros
##################################

# $(1) is component
GET_BARE_LOCATION =$(patsubst $(call ESCAPE_BACKSLASHES,$(SOURCE_ROOT))%,%,$(strip $($(1)_LOCATION)))

# $(1) is the list of components left to process. $(COMP) is set as the first element in the list
define PROCESS_COMPONENT
$(eval COMP := $(word 1,$(1)))
# Find the component makefile in directory list
$(eval TEMP_MAKEFILE := $(strip $(wildcard $(foreach dir, $(addprefix $(SOURCE_ROOT),$(COMPONENT_DIRECTORIES)), $(dir)/$(COMP)/$(notdir $(COMP)).mk))))

$(if $(TEMP_MAKEFILE),,$(error $(COMP) makefile not found. Ensure the $(COMP) directory contains $(notdir $(COMP)).mk))
$(if $(filter 1,$(words $(TEMP_MAKEFILE))),,$(error More than one component with the name "$(COMP)". See $(TEMP_MAKEFILE)))

# Clear all the temporary variables
$(eval GLOBAL_INCLUDES:=)
$(eval GLOBAL_LINK_SCRIPT:=)
$(eval DEFAULT_LINK_SCRIPT:=)
$(eval DCT_LINK_SCRIPT:=)
$(eval GLOBAL_DEFINES:=)
$(eval GLOBAL_CFLAGS:=)
$(eval GLOBAL_CXXFLAGS:=)
$(eval GLOBAL_ASMFLAGS:=)
$(eval GLOBAL_LDFLAGS:=)
$(eval GLOBAL_CERTIFICATES:=)
$(eval WIFI_CONFIG_DCT_H:=)
$(eval APPLICATION_DCT:=)
$(eval CERTIFICATE:=)
$(eval PRIVATE_KEY:=)
$(eval OLD_CURDIR := $(CURDIR))
$(eval CURDIR := $(CURDIR)$(dir $(TEMP_MAKEFILE)))

# Include the component makefile - This defines the NAME variable
$(eval include $(TEMP_MAKEFILE))

$(NAME)_CFLAGS   := $(call ADD_COMPILER_SPECIFIC_STANDARD_CFLAGS,$($(NAME)_CFLAGS))
$(NAME)_CXXFLAGS := $(call ADD_COMPILER_SPECIFIC_STANDARD_CFLAGS,$($(NAME)_CFLAGS))

$(eval $(NAME)_MAKEFILE :=$(TEMP_MAKEFILE))

# Expand the list of resources to point to the full location (either component local or the common Resources directory)
$(eval $(NAME)_RESOURCES_EXPANDED := $(foreach res,$($(NAME)_RESOURCES),$(word 1,$(wildcard $(addsuffix $(res),$(CURDIR) $(SOURCE_ROOT)Resources/)))))

$(eval CURDIR := $(OLD_CURDIR))

$(eval $(NAME)_LOCATION ?= $(dir $(TEMP_MAKEFILE)))
$(eval $(NAME)_MAKEFILE := $(TEMP_MAKEFILE))
WICED_MAKEFILES          +=$($(NAME)_MAKEFILE)

# Set debug/release specific options
$(eval $(NAME)_BUILD_TYPE := $(BUILD_TYPE))
$(eval $(NAME)_BUILD_TYPE := $(if $($(NAME)_NEVER_OPTIMISE),  debug,   $($(NAME)_BUILD_TYPE)))
$(eval $(NAME)_BUILD_TYPE := $(if $($(NAME)_ALWAYS_OPTIMISE), release, $($(NAME)_BUILD_TYPE)))

$(NAME)_CFLAGS   += $(if $(findstring debug,$($(NAME)_BUILD_TYPE)), $(COMPILER_SPECIFIC_DEBUG_CFLAGS),   $(COMPILER_SPECIFIC_RELEASE_CFLAGS))
$(NAME)_CXXFLAGS += $(if $(findstring debug,$($(NAME)_BUILD_TYPE)), $(COMPILER_SPECIFIC_DEBUG_CXXFLAGS), $(COMPILER_SPECIFIC_RELEASE_CXXFLAGS))
$(NAME)_ASMFLAGS += $(if $(findstring debug,$($(NAME)_BUILD_TYPE)), $(COMPILER_SPECIFIC_DEBUG_ASFLAGS),  $(COMPILER_SPECIFIC_RELEASE_ASFLAGS))
$(NAME)_LDFLAGS  += $(if $(findstring debug,$($(NAME)_BUILD_TYPE)), $(COMPILER_SPECIFIC_DEBUG_LDFLAGS),  $(COMPILER_SPECIFIC_RELEASE_LDFLAGS))

$(NAME)_CFLAGS   += -DWICED_VERSION=$(SLASH_QUOTE_START)$(WICED_VERSION)$(SLASH_QUOTE_END) -I$(OUTPUT_DIR)/Resources/
$(NAME)_CFLAGS   += -DPLATFORM=$(SLASH_QUOTE_START)$$(PLATFORM)$(SLASH_QUOTE_END)
$(NAME)_CFLAGS   += -DBUS=$(SLASH_QUOTE_START)$$(BUS)$(SLASH_QUOTE_END)
$(NAME)_CXXFLAGS += -DWICED_VERSION=$(SLASH_QUOTE_START)$(WICED_VERSION)$(SLASH_QUOTE_END) -I$(OUTPUT_DIR)/Resources/
$(NAME)_CXXFLAGS += -DPLATFORM=$(SLASH_QUOTE_START)$$(PLATFORM)$(SLASH_QUOTE_END)
$(NAME)_CXXFLAGS += -DBUS=$(SLASH_QUOTE_START)$$(BUS)$(SLASH_QUOTE_END)

WICED_INCLUDES           +=$(addprefix -I$($(NAME)_LOCATION),$(GLOBAL_INCLUDES))
WICED_LINK_SCRIPT        :=$(if $(GLOBAL_LINK_SCRIPT),$(GLOBAL_LINK_SCRIPT), $(WICED_LINK_SCRIPT))
WICED_DEFAULT_LINK_SCRIPT+=$(if $(DEFAULT_LINK_SCRIPT),$(addprefix $($(NAME)_LOCATION),$(DEFAULT_LINK_SCRIPT)),)
WICED_PREBUILT_LIBRARIES +=$(addprefix $($(NAME)_LOCATION),$($(NAME)_PREBUILT_LIBRARY))
WICED_LINK_FILES         +=$(addprefix $$(OUTPUT_DIR)/Modules/$(call GET_BARE_LOCATION,$(NAME)),$($(NAME)_LINK_FILES))
WICED_DEFINES            +=$(GLOBAL_DEFINES)
WICED_CFLAGS             +=$(GLOBAL_CFLAGS)
WICED_CXXFLAGS           +=$(GLOBAL_CXXFLAGS)
WICED_ASMFLAGS           +=$(GLOBAL_ASMFLAGS)
WICED_LDFLAGS            +=$(GLOBAL_LDFLAGS)
WICED_CERTIFICATE        +=$(if $(CERTIFICATE),$(addprefix $($(NAME)_LOCATION),$(CERTIFICATE)))
WICED_PRIVATE_KEY        +=$(if $(PRIVATE_KEY),$(addprefix $($(NAME)_LOCATION),$(PRIVATE_KEY)))
WICED_DCT_LINK_SCRIPT    +=$(if $(DCT_LINK_SCRIPT),$(addprefix $($(NAME)_LOCATION),$(DCT_LINK_SCRIPT)),)
WICED_WIFI_CONFIG_DCT_H  +=$(if $(WIFI_CONFIG_DCT_H),$(addprefix $($(NAME)_LOCATION),$(WIFI_CONFIG_DCT_H)),)
WICED_APPLICATION_DCT    +=$(if $(APPLICATION_DCT),$(addprefix $($(NAME)_LOCATION),$(APPLICATION_DCT)),)
# when wifi_config_dct.h file exists in the application directory, add 
# add its directory to includes and add a  WIFI_CONFIG_APPLICATION_DEFINED define
WICED_INCLUDES			 +=$(if $(WIFI_CONFIG_DCT_H),-I$($(NAME)_LOCATION),)
WICED_DEFINES 			 += $(if $(WIFI_CONFIG_DCT_H),WIFI_CONFIG_APPLICATION_DEFINED,)


$(eval PROCESSED_COMPONENTS += $(NAME))
$(eval PROCESSED_COMPONENTS_LOCS += $(COMP))
$(eval COMPONENTS += $($(NAME)_COMPONENTS))
$(if $(strip $(filter-out $(PROCESSED_COMPONENTS_LOCS),$(COMPONENTS))),$(eval $(call PROCESS_COMPONENT,$(filter-out $(PROCESSED_COMPONENTS_LOCS),$(COMPONENTS)))),)
#$(foreach component,$($(NAME)_COMPONENTS),$(if $(filter $(component),$(PROCESSED_COMPONENTS_LOCS)),,$(eval $(call PROCESS_COMPONENT,$(component)))))
endef

##################################
# Start of processing
##################################

# Separate the build string into components
COMPONENTS := $(subst -, ,$(MAKECMDGOALS))

BUS_LIST        := SPI \
                   SDIO

BUILD_TYPE_LIST := debug \
                   release

# Extract out: the bus option, the debug/release option, OTA option, and the lint option
BUS                 := $(if $(filter $(BUS_LIST),$(COMPONENTS)),$(firstword $(filter $(BUS_LIST),$(COMPONENTS))))
BUILD_TYPE          := $(if $(filter $(BUILD_TYPE_LIST),$(COMPONENTS)),$(firstword $(filter $(BUILD_TYPE_LIST),$(COMPONENTS))),release)
USES_BOOTLOADER_OTA := $(if $(filter ota,$(COMPONENTS)),yes,)
RUN_LINT            := $(filter lint,$(COMPONENTS))
COMPONENTS          := $(filter-out $(BUS_LIST) $(BUILD_TYPE_LIST),$(COMPONENTS))

# Set debug/release specific options
ifeq ($(BUILD_TYPE),debug)
WICED_LDFLAGS  += $(COMPILER_SPECIFIC_DEBUG_LDFLAGS)
else
WICED_LDFLAGS  += $(COMPILER_SPECIFIC_RELEASE_LDFLAGS)
endif

# Check if there are any unknown components; output error if so.
$(foreach comp, $(COMPONENTS), $(if $(wildcard $(foreach dir, $(addprefix $(SOURCE_ROOT),$(COMPONENT_DIRECTORIES)), $(dir)/$(comp) ) ),,$(error Unknown component: $(comp))))

# Find the matching network, platform, RTOS and application from the build string components
NETWORK_FULL :=$(strip $(foreach comp,$(COMPONENTS),$(if $(wildcard $(SOURCE_ROOT)Wiced/Network/$(comp)),$(comp),)))
RTOS_FULL     :=$(strip $(foreach comp,$(COMPONENTS),$(if $(wildcard $(SOURCE_ROOT)Wiced/RTOS/$(comp)),$(comp),)))
PLATFORM_FULL :=$(strip $(foreach comp,$(COMPONENTS),$(if $(wildcard $(SOURCE_ROOT)Wiced/Platform/$(comp)),$(comp),)))
APP_FULL      :=$(strip $(foreach comp,$(COMPONENTS),$(if $(wildcard $(SOURCE_ROOT)Apps/$(comp)),$(comp),)))

NETWORK  :=$(notdir $(NETWORK_FULL))
RTOS     :=$(notdir $(RTOS_FULL))
PLATFORM :=$(notdir $(PLATFORM_FULL))
APP      :=$(notdir $(APP_FULL))

# Check if APP is wwd; if so, build app with WWD only, no DCT, no bootloader
ifneq (,$(findstring wwd,$(APP_FULL)))
APP_WWD_ONLY        := 1
USES_BOOTLOADER_OTA := 0
NODCT               := 1
endif

# Load platform makefile to make variables like CHIP, HOST_MICRO & HOST_ARCH available to all makefiles
include $(SOURCE_ROOT)Wiced/Platform/$(PLATFORM_FULL)/$(PLATFORM).mk
MAIN_COMPONENT_PROCESSING :=1

# Now we know the target architecture - include all toolchain makefiles and check one of them can handle the architecture
CC :=
include $(SOURCE_ROOT)wiced_toolchain_ARM_GNU.mk
ifndef CC
$(error No matching toolchain found for architecture $(HOST_ARCH))
endif

# Process all the components + Wiced
COMPONENTS += Wiced
#$(info processing components: $(COMPONENTS))

CURDIR := 

$(eval $(call PROCESS_COMPONENT, $(COMPONENTS)))

# Add some default values
WICED_INCLUDES += -I$(SOURCE_ROOT)Wiced/WWD/internal/chips/$(CHIP) -I$(SOURCE_ROOT)Library -I$(SOURCE_ROOT)include
CUSTOM_DEFAULT_DCT:=$(if $(strip $(WICED_WIFI_CONFIG_DCT_H)),1,)
WICED_WIFI_CONFIG_DCT_H:=$(if $(strip $(WICED_WIFI_CONFIG_DCT_H)),$(strip $(WICED_WIFI_CONFIG_DCT_H)),$(SOURCE_ROOT)include/default_wifi_config_dct.h)
WICED_DEFINES += WICED_WIFI_CONFIG_DCT_H=$(SLASH_QUOTE_START)$(WICED_WIFI_CONFIG_DCT_H)$(SLASH_QUOTE_END)  $(if $(CUSTOM_DEFAULT_DCT),CUSTOM_DEFAULT_DCT=1,)
WICED_DEFINES += $(EXTERNAL_WICED_GLOBAL_DEFINES)

# Make sure the user has specified a component from each category
$(if $(NETWORK),,$(error No network stack specified. Options are: $(notdir $(wildcard Wiced/Network/*))))
$(if $(RTOS),,$(error No RTOS specified. Options are: $(notdir $(wildcard Wiced/RTOS/*))))
$(if $(PLATFORM),,$(error No platform specified. Options are: $(notdir $(wildcard Wiced/Platform/*))))
$(if $(APP),,$(error No application specified. Options are: $(notdir $(wildcard Apps/*))))
$(if $(BUS),,$(error No bus specified. Options are: SDIO SPI))

# Make sure a CHIP and HOST_MICRO have been defined
$(if $(CHIP),,$(error No CHIP has been defined))
$(if $(HOST_MICRO),,$(error No HOST_MICRO has been defined))

REMOVE_FIRST = $(wordlist 2,$(words $(1)),$(1))

$(CONFIG_FILE_DIR):
	$(QUIET)$(call MKDIR, $@)

# Summarize all the information into the config file



.PHONY: $(MAKECMDGOALS)
$(MAKECMDGOALS): | $(CONFIG_FILE_DIR)
	$(QUIET)$(call WRITE_FILE_CREATE, $(CONFIG_FILE) ,WICED_MAKEFILES           += $(WICED_MAKEFILES))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,TOOLCHAIN_NAME            := $(TOOLCHAIN_NAME))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,WICED_LDFLAGS             += $(strip $(WICED_LDFLAGS)))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,RESOURCE_CFLAGS           += $(strip $(WICED_CFLAGS)))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,WICED_LINK_SCRIPT         += $(strip $(if $(WICED_LINK_SCRIPT),$(WICED_LINK_SCRIPT),$(WICED_DEFAULT_LINK_SCRIPT))))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,WICED_LINK_SCRIPT_CMD     += $(call COMPILER_SPECIFIC_LINK_SCRIPT,$(strip $(if $(WICED_LINK_SCRIPT),$(WICED_LINK_SCRIPT),$(WICED_DEFAULT_LINK_SCRIPT)))))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,WICED_PREBUILT_LIBRARIES  += $(strip $(WICED_PREBUILT_LIBRARIES)))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,WICED_CERTIFICATES        += $(strip $(WICED_CERTIFICATES)))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,WICED_PRE_APP_BUILDS      += $(strip $(PRE_APP_BUILDS)))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,WICED_DCT_LINK_SCRIPT     += $(strip $(WICED_DCT_LINK_SCRIPT)))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,WICED_DCT_LINK_CMD        += $(strip $(addprefix $(COMPILER_SPECIFIC_LINK_SCRIPT_DEFINE_OPTION) ,$(WICED_DCT_LINK_SCRIPT))))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,WICED_APPLICATION_DCT     += $(strip $(WICED_APPLICATION_DCT)))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,WICED_WIFI_CONFIG_DCT_H   += $(strip $(WICED_WIFI_CONFIG_DCT_H)))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,WICED_LINK_FILES          += $(WICED_LINK_FILES))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,WICED_INCLUDES            += $(WICED_INCLUDES))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,WICED_DEFINES             += $(strip $(addprefix -D,$(WICED_DEFINES))))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,COMPONENTS                := $(PROCESSED_COMPONENTS))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,BUS                       := $(BUS))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,NETWORK_FULL              := $(NETWORK_FULL))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,RTOS_FULL                 := $(RTOS_FULL))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,PLATFORM_FULL             := $(PLATFORM_FULL))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,APP_FULL                  := $(APP_FULL))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,NETWORK                   := $(NETWORK))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,RTOS                      := $(RTOS))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,PLATFORM                  := $(PLATFORM))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,APP                       := $(APP))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,HOST_MICRO                := $(HOST_MICRO))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,HOST_ARCH                 := $(HOST_ARCH))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,WICED_CERTIFICATE         := $(WICED_CERTIFICATE))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,WICED_PRIVATE_KEY         := $(WICED_PRIVATE_KEY))	
	$(QUIET)$(foreach comp,$(PROCESSED_COMPONENTS), $(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,$(call CONV_COMP,$(comp))_LOCATION         := $($(comp)_LOCATION)))
	$(QUIET)$(foreach comp,$(PROCESSED_COMPONENTS), $(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,$(call CONV_COMP,$(comp))_SOURCES          += $($(comp)_SOURCES)))
	$(QUIET)$(foreach comp,$(PROCESSED_COMPONENTS), $(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,$(call CONV_COMP,$(comp))_CHECK_HEADERS    += $($(comp)_CHECK_HEADERS)))
	$(QUIET)$(foreach comp,$(PROCESSED_COMPONENTS), $(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,$(call CONV_COMP,$(comp))_INCLUDES         := $(addprefix -I$($(comp)_LOCATION),$($(comp)_INCLUDES))))
	$(QUIET)$(foreach comp,$(PROCESSED_COMPONENTS), $(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,$(call CONV_COMP,$(comp))_DEFINES          := $(addprefix -D,$($(comp)_DEFINES))))
	$(QUIET)$(foreach comp,$(PROCESSED_COMPONENTS), $(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,$(call CONV_COMP,$(comp))_CFLAGS           := $(WICED_CFLAGS) $($(comp)_CFLAGS)))
	$(QUIET)$(foreach comp,$(PROCESSED_COMPONENTS), $(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,$(call CONV_COMP,$(comp))_CXXFLAGS         := $(WICED_CXXFLAGS) $($(comp)_CXXFLAGS)))
	$(QUIET)$(foreach comp,$(PROCESSED_COMPONENTS), $(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,$(call CONV_COMP,$(comp))_ASMFLAGS         := $(WICED_ASMFLAGS) $($(comp)_ASMFLAGS)))
	$(QUIET)$(foreach comp,$(PROCESSED_COMPONENTS), $(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,$(call CONV_COMP,$(comp))_RESOURCES        := $($(comp)_RESOURCES_EXPANDED)))
	$(QUIET)$(foreach comp,$(PROCESSED_COMPONENTS), $(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,$(call CONV_COMP,$(comp))_MAKEFILE         := $($(comp)_MAKEFILE)))
	$(QUIET)$(foreach comp,$(PROCESSED_COMPONENTS), $(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,$(call CONV_COMP,$(comp))_PREBUILT_LIBRARY := $(addprefix $($(comp)_LOCATION),$($(comp)_PREBUILT_LIBRARY))))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,APP_WWD_ONLY              := $(APP_WWD_ONLY))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,USES_BOOTLOADER_OTA       := $(USES_BOOTLOADER_OTA))
	$(QUIET)$(call WRITE_FILE_APPEND, $(CONFIG_FILE) ,NODCT                     := $(NODCT))


