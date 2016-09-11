#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

ifdef BUILD_ROM
$(NAME)_SOURCES += ROM_build/reference.S
$(NAME)_COMPONENTS += BESL
$(NAME)_COMPONENTS += WICED/RTOS/ThreadX/WICED
RTOS:=ThreadX
NETWORK:=NetX_Duo

endif

# FreeRTOS is not yet supported for Cortex-R4
VALID_OSNS_COMBOS := ThreadX-NetX ThreadX-NetX_Duo NoOS-NoNS

# BCM94390x-specific make targets
EXTRA_TARGET_MAKEFILES += WICED/platform/MCU/BCM4390x/BCM94390x_targets.mk

GLOBAL_DEFINES += $$(if $$(NO_CRLF_STDIO_REPLACEMENT),,CRLF_STDIO_REPLACEMENT)

ifndef USES_BOOTLOADER_OTA
USES_BOOTLOADER_OTA :=1
endif

INTERNAL_MEMORY_RESOURCES =

$(NAME)_COMPONENTS += filesystems/wicedfs
$(NAME)_COMPONENTS += inputs/gpio_button

# Uses spi_flash interface but implements own functions
GLOBAL_INCLUDES += $(WICED_BASE)/Library/drivers/spi_flash

#GLOBAL_DEFINES += PLATFORM_HAS_OTP


VALID_BUSES :=SoC.43909

ifndef BUS
BUS:=SoC.43909
endif

GLOBAL_INCLUDES  += $(WICED_BASE)/platforms/$(PLATFORM_DIRECTORY)
GLOBAL_INCLUDES  += $(PLATFORM_SOURCES)
GLOBAL_INCLUDES  += $(WICED_BASE)/WICED/platform/include
GLOBAL_INCLUDES  += $(WICED_BASE)/libraries/bluetooth/include
GLOBAL_INCLUDES  += $(WICED_BASE)/libraries/inputs/gpio_button

$(NAME)_LINK_FILES :=

GLOBAL_DEFINES += APP_NAME=$$(SLASH_QUOTE_START)$(APP)$$(SLASH_QUOTE_END)
GLOBAL_DEFINES += WICED_DISABLE_CONFIG_TLS
## FIXME: need to uncomment these with a proper solution post-43909 Alpha.
#GLOBAL_DEFINES += PLATFORM_BCM94390X
#GLOBAL_DEFINES += WICED_BT_USE_RESET_PIN
#GLOBAL_DEFINES += GSIO_UART
GLOBAL_DEFINES += WICED_DCT_INCLUDE_BT_CONFIG

WIFI_IMAGE_DOWNLOAD := buffered

ifneq ($(BUS),SoC.43909)
$(error This platform only supports SoC.43909 bus protocol. Currently set to "$(BUS)")
endif

ifeq (1, $(SECURE_SFLASH))
$(info SecureSflash Enabled)
GLOBAL_DEFINES += FEATURE_SECURESFLASH=1
APP0_SECURE := 1
else
GLOBAL_DEFINES += FEATURE_SECURESFLASH=0
endif
#PRE_APP_BUILDS := generated_mac_address.txt

# WICED APPS
# APP0 and FILESYSTEM_IMAGE are reserved main app and resources file system
# FR_APP :=
# DCT_IMAGE :=
# OTA_APP :=
# FILESYSTEM_IMAGE :=
# WIFI_FIRMWARE :=
# APP0 :=
# APP1 :=
# APP2 :=

# WICED APPS LOOKUP TABLE
APPS_LUT_HEADER_LOC := 0x10000
APPS_START_SECTOR := 17

ifneq ($(APP),bootloader)
ifneq ($(APP),tiny_bootloader)
ifneq ($(APP),sflash_write)
ifneq ($(APP),rom_test)


# Platform specific MIN MAX range for WM8533 DAC in decibels
GLOBAL_DEFINES += MIN_WM8533_DB_LEVEL=-53.0
GLOBAL_DEFINES += MAX_WM8533_DB_LEVEL=6.0

ifneq ($(MAIN_COMPONENT_PROCESSING),1)
$(info +-----------------------------------------------------------------------------------------------------+ )
$(info | IMPORTANT NOTES                                                                                     | )
$(info +-----------------------------------------------------------------------------------------------------+ )
$(info | Wi-Fi MAC Address                                                                                   | )
$(info |    The target Wi-Fi MAC address is defined in <WICED-SDK>/generated_mac_address.txt                 | )
$(info |    Ensure each target device has a unique address.                                                  | )
$(info +-----------------------------------------------------------------------------------------------------+ )
$(info | MCU & Wi-Fi Power Save                                                                              | )
$(info |    It is *critical* that applications using WICED Powersave API functions connect an accurate 32kHz | )
$(info |    reference clock to the sleep clock input pin of the WLAN chip. Please read the WICED Powersave   | )
$(info |    Application Note located in the documentation directory if you plan to use powersave features.   | )
$(info +-----------------------------------------------------------------------------------------------------+ )
endif  # ($(MAIN_COMPONENT_PROCESSING),1)
endif  # ($(APP),rom_test)
endif  # ($(APP),sflash_write)
endif  # ($(APP),tiny_bootloader)
endif  # ($(APP),bootloader)

# uncomment for non-bootloader apps to avoid adding a DCT
# NODCT := 1


# GLOBAL_DEFINES += STOP_MODE_SUPPORT PRINTF_BLOCKING
