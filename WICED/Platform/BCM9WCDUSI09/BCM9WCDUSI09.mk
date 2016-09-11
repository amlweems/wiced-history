#
# Copyright 2013, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Platform_BCM9WCDUSI09

CHIP       := 43362a2
HOST_MICRO := stm32f2x
HOST_ARCH  := ARM_Cortex_M3

$(NAME)_SOURCES := read_wifi_firmware.c \
                   platform.c

$(NAME)_COMPONENTS := common/ARM_Cortex_M3/STM32F2xx

PRE_APP_BUILDS := generated_mac_address.txt



GLOBAL_INCLUDES  := . \
                    ../include \
                    ../../../include/platforms/BCM9WCDUSI09

# HSE_VALUE = STM32 crystal frequency = 26MHz (needed to make UART work correctly)
GLOBAL_DEFINES += HSE_VALUE=26000000
GLOBAL_DEFINES += CPU_CLOCK_HZ=120000000
GLOBAL_DEFINES += APP_NAME=$$(SLASH_QUOTE_START)$(APP)$$(SLASH_QUOTE_END)
GLOBAL_DEFINES += PLATFORM_STM32_VOLTAGE_2V7_TO_3V6
# GLOBAL_DEFINES += STOP_MODE_SUPPORT PRINTF_BLOCKING



ifndef USES_BOOTLOADER_OTA
USES_BOOTLOADER_OTA :=1
endif

# NODCT := 1 # uncomment for non-bootloader apps to avoid adding a DCT

ifndef RTOS
RTOS:=ThreadX
COMPONENTS += ThreadX
endif

ifndef NETWORK
NETWORK:=NetX_Duo
COMPONENTS += NetX_Duo
endif


# Uncomment these lines to enable malloc debug.and checking
#GLOBAL_DEFINES += MALLOC_DEBUG
#WICED_LDFLAGS += -Wl,--wrap,malloc -Wl,--wrap,realloc -Wl,--wrap,calloc -Wl,--wrap,free



ifndef BUS
BUS:=SDIO
endif

VALID_BUSES:=SDIO

ifeq ($(BUS),SDIO)
WIFI_IMAGE_DOWNLOAD := direct
else
$(error This platform does not support SPI)
endif


ifneq ($(APP),bootloader)
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
endif
endif
