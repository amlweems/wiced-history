#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

export SOURCE_ROOT:=$(dir $(word $(words $(MAKEFILE_LIST)), $(MAKEFILE_LIST)))


MAKEFILE_TARGETS := clean testlist iar_project

include $(SOURCE_ROOT)wiced_toolchain_common.mk

define USAGE_TEXT
Aborting due to invalid targets

Usage: make <target> [download] [run | debug] [JTAG=xxx] [no_dct]
       make run

  <target>
    One each of the following mandatory [and optional] components separated by '-'
      * Application (Apps in sub-directories are referenced by subdir.appname)
      * Hardware Platform ($(filter-out common  include README.txt,$(notdir $(wildcard Wiced/Platform/*))))
      * [RTOS] ($(notdir $(wildcard Wiced/RTOS/*)))
      * [Network Stack] ($(notdir $(wildcard Wiced/Network/*)))
      * [MCU-WLAN Interface Bus] (SDIO SPI)

  [download]
    Download firmware image to target platform

  [run]
    Reset and run an application on the target hardware (no download)

  [debug]
    Connect to the target platform and run the debugger

  [JTAG=xxx]
    JTAG interface configuration file from the Tools/OpenOCD dirctory
    Default option is BCM9WCD1EVAL1, direct JTAG option is jlink

  [no_dct]
    DCT downloading is disabled and the DCT remains unmodified.
    Only valid when the 'download' option is present
    
  [factory_reset_dct]
    Generates a factory reset DCT

  [VERBOSE=1]
    Shows the commands as they are being executed
  
  [JOBS=x]
    Sets the maximum number of parallel build threads (default=4)

  Notes
    * Component names are case sensitive
    * 'Wiced', 'SDIO', 'SPI' and 'debug' are reserved component names
    * Component names MUST NOT include space or '-' characters
    * Building for release is assumed unless '-debug' is appended to the target
    * Some platforms may only support a single interface bus option

  Example Usage
    Build for Release
      $> make snip.scan-BCM943362WCD4
      $> make snip.scan-BCM943362WCD4-ThreadX-NetX_Duo-SDIO

    Build, Download and Run using the default USB-JTAG programming interface
      $> make snip.scan-BCM943362WCD4 download run

    Build for Debug
      $> make snip.scan-BCM943362WCD4-debug

    Build, Download and Debug using command line GDB
      $> make snip.scan-BCM943362WCD4-debug download debug

    Reset and run an application on the target hardware
      $> make run

    Clean output directory
      $> make clean

    Build a Factory Reset image for the SPI serial flash
      $> make snip.ping-BCM943362WCD4  OTA=waf.ota_upgrade  SFLASH=app-dct-ota-download

         where: [OTA]=<OTA application build string>
                         Build string options are App name, RTOS and TCP stack ONLY
                <SFLASH>=<app>-[dct]-[ota]-[download]
                         where:
                            app      : Add the application to the image
                            dct      : Add the DCT to the image
                            ota      : Add the OTA application to the image
                            download : Download the image after creation

endef

############################
# Extra options:
#                LINT=1 : Sends source files for parsing by LINT
#                CHECK_HEADERS=1 : builds header files to test for their completeness
############################

OPENOCD_LOG_FILE ?= build/openocd_log.txt
DOWNLOAD_LOG := >> $(OPENOCD_LOG_FILE)

BOOTLOADER_LOG_FILE ?= build/bootloader.log
export HOST_OS
export VERBOSE
export EXTERNAL_WICED_GLOBAL_DEFINES

.PHONY: $(BUILD_STRING) main_app bootloader download_only testlist clean iar_project Help download no_dct download_dct download_work copy_elf_for_eclipse run debug download_bootloader sflash_image .gdbinit factory_reset_dct

Help:
	$(error $(USAGE_TEXT))

IAR_PROJECT_BUILDER = $(TOOLS_ROOT)/IAR/wiced_sdk_project/build_iar_project.exe

iar_project:
	$(QUIET)$(ECHO) Generating Wiced_SDK IAR project file...
	$(QUIET) $(RM) $(TOOLS_ROOT)/IAR/wiced_sdk_project/wiced_sdk_project.ewp
	$(QUIET) cd $(TOOLS_ROOT)/IAR/wiced_sdk_project
	$(QUIET) $(IAR_PROJECT_BUILDER) $(subst /,\,$(CURDIR))
	$(QUIET)$(ECHO) Generated. Now opening.... This may take several seconds
	$(QUIET)cmd /C start "$(IAR_WORKBENCH_EXECUTABLE)" "$(TOOLS_ROOT)/IAR/wiced_sdk_project/wiced_sdk_workspace.eww"
		
	

clean:
	$(QUIET)$(ECHO) Cleaning...
	$(QUIET)$(CLEAN_COMMAND)
	$(QUIET)$(RM) -rf .gdbinit
	$(QUIET)$(ECHO) Done

testlist:
	$(QUIET)$(MAKE) $(SILENT) -f $(SOURCE_ROOT)wiced_test.mk


DOWNLOAD_STRING := $(CLEANED_BUILD_STRING)
ifneq (,$(findstring download_only,$(MAKECMDGOALS)))
$(BUILD_STRING):
BUILD_STRING :=
endif

# Processing of factory_reset_dct
ifneq (,$(findstring factory_reset_dct,$(MAKECMDGOALS)))
$(filter-out factory_reset_dct, $(MAKECMDGOALS)):
	@:
FACTORY_RESET_TARGET := $(BUILD_STRING)
BUILD_STRING :=
endif

ifneq ($(BUILD_STRING),)
-include build/$(CLEANED_BUILD_STRING)/config.mk
# Now we know the target architecture - include all toolchain makefiles and check one of them can handle the architecture
include $(SOURCE_ROOT)wiced_toolchain_ARM_GNU.mk

build/$(CLEANED_BUILD_STRING)/config.mk: $(SOURCE_ROOT)Makefile $(SOURCE_ROOT)wiced_config.mk $(SOURCE_ROOT)wiced_toolchain_common.mk $(SOURCE_ROOT)wiced_toolchain_ARM_GNU.mk $(WICED_MAKEFILES)
	$(QUIET)$(ECHO) $(if $(WICED_MAKEFILES),Applying changes made to: $?,Making config file for first time)
	$(QUIET)$(MAKE) -r $(SILENT) -f $(SOURCE_ROOT)wiced_config.mk $(DIR_BUILD_STRING)
endif

ifeq ($(IAR),1)
#IAR cannot support multi threaded build
JOBS=1
endif

JOBS ?=4
ifeq (,$(SUB_BUILD))
JOBSNO := $(if $(findstring 1,$(LINT)), , -j$(JOBS) )
endif



$(BUILD_STRING): main_app $(if $(SFLASH),sflash_image) copy_elf_for_eclipse

main_app: build/$(CLEANED_BUILD_STRING)/config.mk $(WICED_PRE_APP_BUILDS) $(SOURCE_ROOT)wiced_elf.mk .gdbinit
	$(QUIET)$(COMMON_TOOLS_PATH)mkdir -p $(OUTPUT_DIR)/Binary $(OUTPUT_DIR)/Modules $(OUTPUT_DIR)/Libraries $(OUTPUT_DIR)/Resources
	$(QUIET)$(MAKE) -r $(JOBSNO) $(SILENT) -f $(SOURCE_ROOT)wiced_elf.mk $(DIR_BUILD_STRING)
	$(QUIET)$(ECHO) Build complete

.gdbinit: build/$(CLEANED_BUILD_STRING)/config.mk $(SOURCE_ROOT)wiced_toolchain_common.mk
	$(QUIET)$(ECHO) Making .gdbinit
	$(QUIET)$(ECHO) set remotetimeout 20 > .gdbinit
	$(QUIET)$(ECHO) $(GDBINIT_STRING) >> .gdbinit

download_only: download_work

ifneq (,$(findstring wwd.,$(MAKECMDGOALS)))
download: $(BUILD_STRING) download_work copy_elf_for_eclipse
else
ifneq ($(NO_BUILD_BOOTLOADER),1)
download: $(BUILD_STRING) download_bootloader copy_bootloader_elf_for_eclipse download_work copy_elf_for_eclipse $(if $(findstring no_dct,$(MAKECMDGOALS)),,download_dct)
else
download: $(BUILD_STRING) download_work copy_elf_for_eclipse $(if $(findstring no_dct,$(MAKECMDGOALS)),,download_dct)
endif
endif

no_dct:
	$(QUIET)$(ECHO) DCT unmodified

download_dct:
	$(QUIET)$(ECHO) Downloading DCT ...
	$(QUIET)$(call CONV_SLASHES,$(OPENOCD_FULL_NAME)) -f $(OPENOCD_PATH)$(JTAG).cfg -f $(OPENOCD_PATH)$(HOST_MICRO).cfg -f $(OPENOCD_PATH)$(HOST_MICRO)-flash-app.cfg -c "verify_image_checksum $(BUILD_DIR)/$(DOWNLOAD_STRING)/DCT.stripped.elf" -c shutdown $(DOWNLOAD_LOG) 2>&1 && $(ECHO) No changes detected && $(ECHO_BLANK_LINE) || $(call CONV_SLASHES,$(OPENOCD_FULL_NAME)) -f $(OPENOCD_PATH)$(JTAG).cfg -f $(OPENOCD_PATH)$(HOST_MICRO).cfg -f $(OPENOCD_PATH)$(HOST_MICRO)-flash-app.cfg -c "flash write_image erase $(BUILD_DIR)/$(DOWNLOAD_STRING)/DCT.stripped.elf" -c shutdown $(DOWNLOAD_LOG) 2>&1 && $(ECHO) Download complete && $(ECHO_BLANK_LINE) || $(ECHO) "**** OpenOCD failed - ensure you have installed the driver from the drivers directory, and that the debugger is not running **** In Linux this may be due to USB access permissions. In a virtual machine it may be due to USB passthrough settings. Check in the task list that another OpenOCD process is not running. Check that you have the correct target and JTAG device plugged in. ****"

download_work:
ifneq (,$(and $(OPENOCD_PATH),$(OPENOCD_FULL_NAME)))
	$(QUIET)$(ECHO) Downloading Application ...
	$(QUIET)$(call CONV_SLASHES,$(OPENOCD_FULL_NAME)) -f $(OPENOCD_PATH)$(JTAG).cfg -f $(OPENOCD_PATH)$(HOST_MICRO).cfg -f $(OPENOCD_PATH)$(HOST_MICRO)-flash-app.cfg -c "verify_image_checksum $(BUILD_DIR)/$(DOWNLOAD_STRING)/Binary/$(DOWNLOAD_STRING).stripped.elf" -c shutdown $(DOWNLOAD_LOG) 2>&1 && $(ECHO) No changes detected && $(ECHO_BLANK_LINE) || $(call CONV_SLASHES,$(OPENOCD_FULL_NAME)) -f $(OPENOCD_PATH)$(JTAG).cfg -f $(OPENOCD_PATH)$(HOST_MICRO).cfg -f $(OPENOCD_PATH)$(HOST_MICRO)-flash-app.cfg -c "flash write_image erase $(BUILD_DIR)/$(DOWNLOAD_STRING)/Binary/$(DOWNLOAD_STRING).stripped.elf" -c shutdown $(DOWNLOAD_LOG) 2>&1 && $(ECHO) Download complete && $(ECHO_BLANK_LINE) || $(ECHO) "**** OpenOCD failed - ensure you have installed the driver from the drivers directory, and that the debugger is not running **** In Linux this may be due to USB access permissions. In a virtual machine it may be due to USB passthrough settings. Check in the task list that another OpenOCD process is not running. Check that you have the correct target and JTAG device plugged in. ****"
else
	$(error Path to OpenOCD has not been set using OPENOCD_PATH and OPENOCD_FULL_NAME)
endif

copy_elf_for_eclipse: main_app
	$(QUIET)$(call MKDIR, $(BUILD_DIR)/eclipse_debug/)
	$(QUIET)$(CP) build/$(DOWNLOAD_STRING)/Binary/$(DOWNLOAD_STRING).elf $(BUILD_DIR)/eclipse_debug/last_built.elf

BOOTLOADER_TARGET := waf.bootloader-NoOS-NoNS-$(PLATFORM_FULL)-$(BUS)
BOOTLOADER_OUTFILE := $(BUILD_DIR)/$(call CONV_COMP,$(subst .,/,$(BOOTLOADER_TARGET)))/Binary/$(call CONV_COMP,$(subst .,/,$(BOOTLOADER_TARGET)))

copy_bootloader_elf_for_eclipse:
	$(QUIET)$(call MKDIR, $(BUILD_DIR)/eclipse_debug/)
	$(QUIET)$(CP) $(BOOTLOADER_OUTFILE).elf $(BUILD_DIR)/eclipse_debug/last_bootloader.elf

run: $(SHOULD_I_WAIT_FOR_DOWNLOAD)
	$(QUIET)$(ECHO) Resetting target
	$(QUIET)$(call CONV_SLASHES,$(OPENOCD_FULL_NAME)) -c "log_output $(OPENOCD_LOG_FILE)" -f $(OPENOCD_PATH)$(JTAG).cfg -f $(OPENOCD_PATH)$(HOST_MICRO).cfg -c init -c "reset run" -c shutdown $(DOWNLOAD_LOG) 2>&1 && $(ECHO) Target running

debug: $(BUILD_STRING)
ifneq (,$(and $(OPENOCD_PATH),$(OPENOCD_FULL_NAME)))
	$(QUIET)$(GDB_COMMAND) $(OUTPUT_DIR)/Binary/$(CLEANED_BUILD_STRING).elf -x .gdbinit_attach

else
	$(error Path to OpenOCD has not been set using OPENOCD_PATH and OPENOCD_FULL_NAME)
endif

ifneq ($(VERBOSE),1)
BOOTLOADER_REDIRECT	= > $(BOOTLOADER_LOG_FILE)
endif

ifneq ($(NO_BUILD_BOOTLOADER),1)
bootloader:
	$(QUIET)$(ECHO) Building Bootloader
	$(QUIET)$(MAKE) -r $(SILENT) -f $(SOURCE_ROOT)Makefile $(BOOTLOADER_TARGET) -I$(OUTPUT_DIR)  SFLASH= $(BOOTLOADER_REDIRECT)
	$(QUIET)$(ECHO_BLANK_LINE)
	$(QUIET)$(ECHO) Building App
	$(QUIET)$(ECHO_BLANK_LINE)

download_bootloader:
ifneq (,$(and $(OPENOCD_PATH),$(OPENOCD_FULL_NAME)))
	$(QUIET)$(ECHO) Downloading Bootloader ...
	$(QUIET)$(call CONV_SLASHES,$(OPENOCD_FULL_NAME)) -f $(OPENOCD_PATH)$(JTAG).cfg -f $(OPENOCD_PATH)$(HOST_MICRO).cfg -f $(OPENOCD_PATH)$(HOST_MICRO)-flash-app.cfg -c "verify_image_checksum $(BOOTLOADER_OUTFILE).stripped.elf" -c shutdown $(DOWNLOAD_LOG) 2>&1 && $(ECHO) No changes detected && $(ECHO_BLANK_LINE) || $(call CONV_SLASHES,$(OPENOCD_FULL_NAME)) -f $(OPENOCD_PATH)$(JTAG).cfg -f $(OPENOCD_PATH)$(HOST_MICRO).cfg -f $(OPENOCD_PATH)$(HOST_MICRO)-flash-app.cfg -c "flash write_image erase $(BOOTLOADER_OUTFILE).stripped.elf" -c shutdown $(DOWNLOAD_LOG) 2>&1 && $(ECHO) Download complete && $(ECHO_BLANK_LINE) || $(ECHO) "**** OpenOCD failed - ensure you have installed the driver from the drivers directory, and that the debugger is not running **** In Linux this may be due to USB access permissions. In a virtual machine it may be due to USB passthrough settings. Check in the task list that another OpenOCD process is not running. Check that you have the correct target and JTAG device plugged in. ****"
else
	$(error Path to OpenOCD has not been set using OPENOCD_PATH and OPENOCD_FULL_NAME)
endif

else
bootloader:
	$(QUIET)$(ECHO) Skipping building bootloader due to commandline spec
endif

generated_mac_address.txt: $(TOOLS_ROOT)/mac_generator/mac_generator.pl
	$(QUIET)$(PERL) $(TOOLS_ROOT)/mac_generator/mac_generator.pl > $@


ifneq ($(SFLASH),)
sflash_image: main_app bootloader
	$(QUIET)$(ECHO) Building Serial Flash Image
	$(QUIET)$(MAKE) $(SILENT) -f $(SOURCE_ROOT)mfg_image.mk $(SFLASH) FRAPP=$(CLEANED_BUILD_STRING) SFLASH= 
endif

factory_reset_dct: $(SOURCE_ROOT)wiced_factory_reset.mk Makefile
	$(QUIET)$(MAKE) $(SILENT) -f $(SOURCE_ROOT)wiced_factory_reset.mk $(FACTORY_RESET_TARGET)
