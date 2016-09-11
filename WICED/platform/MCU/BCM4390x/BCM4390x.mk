#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME = MCU_BCM4390x

$(NAME)_SOURCES := vector_table_$(TOOLCHAIN_NAME).S \
                   start_$(TOOLCHAIN_NAME).S \
                   BCM4390x_platform.c \
                   WAF/waf_platform.c \
                   ../platform_resource.c \
                   ../platform_stdio.c \
                   ../wiced_platform_common.c \
                   ../wiced_apps_common.c \
                   ../wiced_waf_common.c \
                   ../wiced_dct_external_common.c \
                   ../wiced_dct_update.c \
                   ../../$(HOST_ARCH)/crt0_$(TOOLCHAIN_NAME).c \
                   ../../$(HOST_ARCH)/platform_cache.c \
                   ../../$(HOST_ARCH)/platform_cache_asm.S \
                   platform_unhandled_isr.c \
                   platform_vector_table.c \
                   platform_filesystem.c \
                   platform_tick.c \
                   platform_chipcommon.c \
                   platform_deep_sleep.c \
                   wwd_platform.c \
                   ../platform_nsclock.c \
                   ../../$(HOST_ARCH)/exception_handlers.c      \
                   platform_audio_info.c

ifdef PLATFORM_SUPPORTS_BUTTONS
$(NAME)_SOURCES += ../platform_button.c
endif

#for DCT with crc checking
$(NAME)_COMPONENTS  += utilities/crc

# include the ota2 specific functions
ifeq (1, $(OTA2_SUPPORT))
$(NAME)_SOURCES += ../wiced_dct_external_ota2.c
endif


HOST_ARCH  := ARM_CR4

# Host MCU alias for OpenOCD
HOST_OPENOCD := BCM4390x

GLOBAL_INCLUDES +=  . \
                    .. \
                    ../../$(HOST_ARCH) \
                    WAF \
                    peripherals \
                    peripherals/include \
                    peripherals/tlsf

ifneq ($(wildcard WICED/platform/MCU/BCM4390x/$(HOST_MCU_VARIANT)/$(APPS_CHIP_REVISION)/$(HOST_MCU_VARIANT)$(APPS_CHIP_REVISION).mk),)
include WICED/platform/MCU/BCM4390x/$(HOST_MCU_VARIANT)/$(HOST_MCU_VARIANT).mk
endif # wildcard $(WICED ...)

include $(CURDIR)BCM94390x_common.mk

ifeq ($(BUS),SoC.43909)
$(NAME)_SOURCES += wwd_m2m.c
else
$(NAME)_SOURCES += ../wwd_resources.c \
                   wwd_SDIO.c
endif

ifdef BUILD_ROM
$(NAME)_COMPONENTS += WICED/platform/MCU/BCM4390x/ROM_build
else
ifeq ($(IMAGE_TYPE),rom)
$(NAME)_COMPONENTS += WICED/platform/MCU/$(HOST_MCU_FAMILY)/ROM_offload
endif # rom
endif # BUILD_ROM

# $(1) is the relative path to the platform directory
define PLATFORM_LOCAL_DEFINES_INCLUDES_43909
$(NAME)_INCLUDES += $(1)/peripherals/include

$(NAME)_DEFINES += BCMDRIVER \
                   BCM_WICED \
                   BCM_CPU_PREFETCH \
                   BCMCHIPID=BCM43909_CHIP_ID \
                   UNRELEASEDCHIP \
                   BCMM2MDEV_ENABLED \
                   CFG_GMAC

ifeq ($(APPS_CHIP_REVISION),A0)
$(NAME)_DEFINES += BCMCHIPREV=0
endif
ifeq ($(APPS_CHIP_REVISION),B0)
$(NAME)_DEFINES += BCMCHIPREV=1
endif
ifeq ($(APPS_CHIP_REVISION),B1)
$(NAME)_DEFINES += BCMCHIPREV=2
endif

$(NAME)_CFLAGS += -Wundef
endef

ifneq ($(APPS_CHIP_REVISION),A0)
ifneq ($(APPS_CHIP_REVISION),B0)
GLOBAL_DEFINES += PLATFORM_ALP_CLOCK_RES_FIXUP=0
endif
endif

# USB require some fixup to use ALP clock. Necessary for A0/B0/B1 chips
ifeq ($(filter $(APPS_CHIP_REVISION),A0 B0 B1),)
GLOBAL_DEFINES += PLATFORM_USB_ALP_CLOCK_RES_FIXUP=0
endif

$(eval $(call PLATFORM_LOCAL_DEFINES_INCLUDES_43909, .))

$(NAME)_COMPONENTS  += $(TOOLCHAIN_NAME)
$(NAME)_COMPONENTS  += MCU/BCM4390x/peripherals
$(NAME)_COMPONENTS  += utilities/ring_buffer

GLOBAL_CFLAGS   += $$(CPU_CFLAGS)    $$(ENDIAN_CFLAGS_LITTLE)
GLOBAL_CXXFLAGS += $$(CPU_CXXFLAGS)  $$(ENDIAN_CXXFLAGS_LITTLE)
GLOBAL_ASMFLAGS += $$(CPU_ASMFLAGS)  $$(ENDIAN_ASMFLAGS_LITTLE)
GLOBAL_LDFLAGS  += $$(CPU_LDFLAGS)   $$(ENDIAN_LDFLAGS_LITTLE)

ifndef $(RTOS)_SYS_STACK
$(RTOS)_SYS_STACK=0
endif
ifndef $(RTOS)_FIQ_STACK
$(RTOS)_FIQ_STACK=0
endif
ifndef $(RTOS)_IRQ_STACK
$(RTOS)_IRQ_STACK=1024
endif

ifeq ($(TOOLCHAIN_NAME),GCC)
GLOBAL_LDFLAGS  += -nostartfiles
GLOBAL_LDFLAGS  += -Wl,--defsym,START_STACK_SIZE=$($(RTOS)_START_STACK) \
                   -Wl,--defsym,FIQ_STACK_SIZE=$($(RTOS)_FIQ_STACK) \
                   -Wl,--defsym,IRQ_STACK_SIZE=$($(RTOS)_IRQ_STACK) \
                   -Wl,--defsym,SYS_STACK_SIZE=$($(RTOS)_SYS_STACK)
GLOBAL_ASMFLAGS += --defsym SYS_STACK_SIZE=$($(RTOS)_SYS_STACK) \
                   --defsym FIQ_STACK_SIZE=$($(RTOS)_FIQ_STACK) \
                   --defsym IRQ_STACK_SIZE=$($(RTOS)_IRQ_STACK)

# Pick-up MCU variant linker-scripts.
GLOBAL_LDFLAGS  += -L WICED/platform/MCU/BCM4390x/$(HOST_MCU_VARIANT)

# Let linker script include other generic linker scripts.
GLOBAL_LDFLAGS  += -L WICED/platform/MCU/BCM4390x
endif # GCC

ifdef NO_WIFI
$(NAME)_COMPONENTS += WICED/WWD
ifndef NETWORK
NETWORK := NoNS
$(NAME)_COMPONENTS += WICED/network/NoNS
endif
endif

# These need to be forced into the final ELF since they are not referenced otherwise
$(NAME)_LINK_FILES := ../../$(HOST_ARCH)/crt0_$(TOOLCHAIN_NAME).o \
                      vector_table_$(TOOLCHAIN_NAME).o \
                      start_$(TOOLCHAIN_NAME).o

#                      ../../$(HOST_ARCH)/hardfault_handler.o

$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS) -I$(SOURCE_ROOT)./

# Add maximum and default watchdog timeouts to definitions. Warning: Do not change MAX_WATCHDOG_TIMEOUT_SECONDS
MAX_WATCHDOG_TIMEOUT_SECONDS    = 22
MAX_WAKE_FROM_SLEEP_DELAY_TICKS = 500
GLOBAL_DEFINES += MAX_WATCHDOG_TIMEOUT_SECONDS=$(MAX_WATCHDOG_TIMEOUT_SECONDS) MAX_WAKE_FROM_SLEEP_DELAY_TICKS=$(MAX_WAKE_FROM_SLEEP_DELAY_TICKS)

# Tell that platform has data cache and set cache line size
GLOBAL_DEFINES += PLATFORM_L1_CACHE_SHIFT=5

# Tell that platform supports deep sleep
GLOBAL_DEFINES += PLATFORM_DEEP_SLEEP

# Tell that platform has special WLAN features
GLOBAL_DEFINES += BCM43909

# ARM DSP/SIMD instruction set support:
# use this define to embed specific instructions
# related to the ARM DSP set.
# If further defines are needed for specific sub-set (CR4, floating point)
# make sure you protect them under this global define.
GLOBAL_DEFINES += ENABLE_ARM_DSP_INSTRUCTIONS

# Use packet release hook to refill DMA
GLOBAL_DEFINES += PLAT_NOTIFY_FREE

# DCT linker script
DCT_LINK_SCRIPT += $(TOOLCHAIN_NAME)/dct$(LINK_SCRIPT_SUFFIX)

ifeq ($(APP),ota2_bootloader)
####################################################################################
# Building OTA2 bootloader
####################################################################################
DEFAULT_LINK_SCRIPT += $(TOOLCHAIN_NAME)/ota2_bootloader$(LINK_SCRIPT_SUFFIX)

else
ifeq ($(APP),bootloader)
####################################################################################
# Building bootloader
####################################################################################
DEFAULT_LINK_SCRIPT += $(TOOLCHAIN_NAME)/bootloader$(LINK_SCRIPT_SUFFIX)
GLOBAL_DEFINES      += bootloader_ota
LINK_BOOTLOADER_WITH_ROM_SYMBOLS ?=FALSE

else
ifeq ($(APP),tiny_bootloader)
####################################################################################
# Building tiny bootloader
####################################################################################

$(NAME)_SOURCES := $(filter-out vector_table_$(TOOLCHAIN_NAME).S, $($(NAME)_SOURCES))
$(NAME)_LINK_FILES := $(filter-out vector_table_$(TOOLCHAIN_NAME).o, $($(NAME)_LINK_FILES))
ifeq (1, $(OTA2_SUPPORT))
DEFAULT_LINK_SCRIPT += $(TOOLCHAIN_NAME)/ota2_tiny_bootloader$(LINK_SCRIPT_SUFFIX)	# temp
else
DEFAULT_LINK_SCRIPT += $(TOOLCHAIN_NAME)/tiny_bootloader$(LINK_SCRIPT_SUFFIX)
endif
GLOBAL_DEFINES      += TINY_BOOTLOADER
LINK_BOOTLOADER_WITH_ROM_SYMBOLS ?=FALSE

else
ifneq ($(filter ota_upgrade sflash_write, $(APP)),)
####################################################################################
# Building sflash_write OR ota_upgrade
####################################################################################

PRE_APP_BUILDS      += bootloader
WIFI_IMAGE_DOWNLOAD := buffered
DEFAULT_LINK_SCRIPT := $(TOOLCHAIN_NAME)/app_without_rom$(LINK_SCRIPT_SUFFIX)
GLOBAL_DEFINES      += WICED_DISABLE_BOOTLOADER
GLOBAL_DEFINES      += __JTAG_FLASH_WRITER_DATA_BUFFER_SIZE__=32768
ifeq ($(TOOLCHAIN_NAME),IAR)
GLOBAL_LDFLAGS      += --config_def __JTAG_FLASH_WRITER_DATA_BUFFER_SIZE__=16384
endif

else

####################################################################################
# Building a stand-alone application
####################################################################################
ifneq ($(IMAGE_TYPE),rom)
ifeq (1, $(OTA2_SUPPORT))
DEFAULT_LINK_SCRIPT := $(TOOLCHAIN_NAME)/ota2_app_without_rom$(LINK_SCRIPT_SUFFIX)
else
DEFAULT_LINK_SCRIPT := $(TOOLCHAIN_NAME)/app_without_rom$(LINK_SCRIPT_SUFFIX)
endif # OTA2_SUPPORT
else
ifeq (1, $(OTA2_SUPPORT))
DEFAULT_LINK_SCRIPT := $(TOOLCHAIN_NAME)/ota2_app_with_rom$(LINK_SCRIPT_SUFFIX)
else
DEFAULT_LINK_SCRIPT := $(TOOLCHAIN_NAME)/app_with_rom$(LINK_SCRIPT_SUFFIX)
endif # OTA2_SUPPORT
endif # IMAGE_TYPE

endif # APP=ota_upgrade OR sflash_write
endif # APP=tiny_bootloader
endif # APP=bootloader
endif # APP=ota2_bootloader

ifeq ($(LINK_BOOTLOADER_WITH_ROM_SYMBOLS),TRUE)
GLOBAL_LDFLAGS += -L ./WICED/platform/MCU/$(HOST_MCU_FAMILY)/common/$(ROM_OFFLOAD_CHIP)/rom_offload
DEFAULT_LINK_SCRIPT += common/$(ROM_OFFLOAD_CHIP)/rom_offload/GCC_rom_bootloader_symbols.ld
endif
