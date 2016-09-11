#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Platform_BCM943362WCD8

$(NAME)_SOURCES := read_wifi_firmware.c \
                   platform.c

CHIP       := 43362a2
HOST_MICRO := at91sam4sXX
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
                    ../../../include/platforms/BCM943362WCD8

# SAM4S Definitions
GLOBAL_DEFINES  += __SAM4S16B__ \
                   ARM_MATH_CM4=true \
                   CONFIG_SYSCLK_SOURCE=SYSCLK_SRC_PLLACK \
                   CONFIG_SYSCLK_PRES=SYSCLK_PRES_1 \
                   CONFIG_PLL0_SOURCE=PLL_SRC_MAINCK_XTAL \
                   CONFIG_PLL0_MUL=41 \
                   CONFIG_PLL0_DIV=4 \
                   BOARD=USER_BOARD \
                   BOARD_FREQ_SLCK_XTAL=32768 \
                   BOARD_FREQ_SLCK_BYPASS=32768 \
                   BOARD_FREQ_MAINCK_XTAL=12000000 \
                   BOARD_FREQ_MAINCK_BYPASS=12000000 \
                   BOARD_OSC_STARTUP_US=2000 \
                   BOARD_MCK=123000000 \
                   CPU_CLOCK_HZ=123000000 \
                   PERIPHERAL_CLOCK_HZ=123000000 \
                   SDIO_4_BIT \
                   TRACE_LEVEL=0 \
                   WAIT_MODE_SUPPORT \
                   WAIT_MODE_ENTER_DELAY_CYCLES=3 \
                   WAIT_MODE_EXIT_DELAY_CYCLES=34 
                   
# WAIT_MODE_EXIT_DELAY_CYCLES is the delay SAM4S requires to switch the main clock source 
# from the 4MHz internal oscillator to the external high speed crystal.
# WAIT_MODE_EXIT_DELAY_CYCLES value is profiled using RTT with specified BOARD_OSC_STARTUP_US.
                   

$(NAME)_COMPONENTS := common/ARM_Cortex_M4/SAM4S \

$(NAME)_LINK_FILES :=

GLOBAL_DEFINES += APP_NAME=$$(SLASH_QUOTE_START)$(APP)$$(SLASH_QUOTE_END)

ifndef BUS
BUS:=SDIO
endif

VALID_BUSES:=SDIO

ifeq ($(BUS),SDIO)
WIFI_IMAGE_DOWNLOAD := buffered
else
$(error This platform does not support SPI)
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
