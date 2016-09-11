#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Platform_TWRK60D100M

$(NAME)_SOURCES := read_wifi_firmware.c \
                   platform.c

CHIP       := 43362a2
HOST_MICRO := k60
HOST_ARCH  := ARM_Cortex_M4


ifndef USES_BOOTLOADER_OTA
USES_BOOTLOADER_OTA :=1
endif

# Define default RTOS, bus, and network stack.
ifndef RTOS
RTOS:=ThreadX
COMPONENTS += ThreadX
endif


ifndef NETWORK
NETWORK:=NetX_Duo
COMPONENTS += NetX_Duo
endif

GLOBAL_INCLUDES  := . \
                    ../include \
                    ../../../include/platforms/TWRK60D100M
                    
# TWRK60D100M uses MK60N512VMD100 MCU                    
GLOBAL_DEFINES  += CPU_MK60N512VMD100

$(NAME)_COMPONENTS := common/ARM_Cortex_M4/K60

$(NAME)_LINK_FILES :=

# HSE_VALUE = STM32 crystal frequency = 26MHz (needed to make UART work correctly)
GLOBAL_DEFINES += CPU_CLOCK_HZ=96000000
GLOBAL_DEFINES += PERIPHERAL_CLOCK_HZ=48000000

GLOBAL_DEFINES += APP_NAME=$$(SLASH_QUOTE_START)$(APP)$$(SLASH_QUOTE_END)


ifndef BUS
BUS:=SPI
endif

VALID_BUSES:=SPI

ifeq ($(BUS),SPI)
WIFI_IMAGE_DOWNLOAD := buffered
else
$(error This platform does not support SDIO)
endif

PRE_APP_BUILDS := generated_mac_address.txt


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

# Uncomment these lines to enable malloc debug.and checking
#GLOBAL_DEFINES += MALLOC_DEBUG
#WICED_LDFLAGS += -Wl,--wrap,malloc -Wl,--wrap,realloc -Wl,--wrap,calloc -Wl,--wrap,free

# GLOBAL_DEFINES += STOP_MODE_SUPPORT PRINTF_BLOCKING

# NODCT := 1 # uncomment for non-bootloader apps to avoid adding a DCT
