#
# Copyright 2013, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME = STM32F2xx

GLOBAL_INCLUDES :=  . \
                    ..

$(NAME)_COMPONENTS  += common/$(TOOLCHAIN_NAME)
$(NAME)_COMPONENTS  += common/ARM_Cortex_M3/STM32F2xx/STM32F2xx_Drv


GLOBAL_DEFINES  := USE_STDPERIPH_DRIVER
GLOBAL_DEFINES  += _STM32F215RGT6_
GLOBAL_DEFINES  += _STM3x_
GLOBAL_DEFINES  += _STM32x_

GLOBAL_CFLAGS   += $$(CPU_CFLAGS)    $$(ENDIAN_CFLAGS_LITTLE)
GLOBAL_CXXFLAGS += $$(CPU_CXXFLAGS)  $$(ENDIAN_CXXFLAGS_LITTLE)
GLOBAL_ASMFLAGS += $$(CPU_ASMFLAGS)  $$(ENDIAN_ASMFLAGS_LITTLE)
GLOBAL_LDFLAGS  += $$(CPU_LDFLAGS)   $$(ENDIAN_LDFLAGS_LITTLE) 

ifeq ($(TOOLCHAIN_NAME),GCC)
GLOBAL_LDFLAGS  += -nostartfiles
GLOBAL_LDFLAGS  += -Wl,--defsym,__STACKSIZE__=$$($(RTOS)_START_STACK)
else
ifeq ($(TOOLCHAIN_NAME),IAR)
GLOBAL_LDFLAGS  += --config_def __STACKSIZE__=$$($(RTOS)_START_STACK)
endif
endif


$(NAME)_SOURCES := ../crt0_$(TOOLCHAIN_NAME).c \
                   vector_table_$(TOOLCHAIN_NAME).s \
                   HardFault_handler.c \
                   wwd_platform.c \
                   $(BUS)/wwd_bus.c \
                   gpio_irq.c \
                   rtc.c \
                   watchdog.c \
                   stm32f2xx_platform.c
ifeq ($(TOOLCHAIN_NAME),IAR)
$(NAME)_SOURCES += cstartup.s 
endif

# These need to be forced into the final ELF since they are not referenced otherwise
$(NAME)_LINK_FILES := ../crt0_$(TOOLCHAIN_NAME).o vector_table_$(TOOLCHAIN_NAME).o HardFault_handler.o

$(NAME)_CFLAGS = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)

# Add maximum and default watchdog timeouts to definitions. Warning: Do not change MAX_WATCHDOG_TIMEOUT_SECONDS
MAX_WATCHDOG_TIMEOUT_SECONDS = 22
GLOBAL_DEFINES += MAX_WATCHDOG_TIMEOUT_SECONDS=$(MAX_WATCHDOG_TIMEOUT_SECONDS)


ifeq ($(TOOLCHAIN_NAME),IAR)
DCT_LINK_SCRIPT += IAR/dct_only_link.icf
else
DCT_LINK_SCRIPT += GCC/dct_only_link.ld
endif

# Check if building the bootloader
ifeq ($(APP),bootloader)

ifeq ($(TOOLCHAIN_NAME),IAR)
DEFAULT_LINK_SCRIPT := IAR/bootloader_link.icf
else
DEFAULT_LINK_SCRIPT := GCC/bootloader_link.ld
endif

$(NAME)_SOURCES     += bootloader_ota/platform_bootloader.c
GLOBAL_INCLUDES     += bootloader_ota/
$(NAME)_COMPONENTS  += common/drivers/spi_flash
else # APP=bootloader
ifneq ($(filter ota_upgrade sflash_write, $(APP)),)

# Building the Over-the-air upgrade or Serial Flash Writer app

WIFI_IMAGE_DOWNLOAD := buffered
ifeq ($(APP),ota_upgrade)
$(NAME)_SOURCES    += bootloader_ota/app_header.c
$(NAME)_LINK_FILES += bootloader_ota/app_header.o
endif

$(NAME)_SOURCES += bootloader_ota/platform_bootloader.c

GLOBAL_INCLUDES     += bootloader_ota/  ../../../../../Apps/waf/bootloader/
GLOBAL_DEFINES      += OTA_UPGRADE

ifeq ($(TOOLCHAIN_NAME),IAR)
DEFAULT_LINK_SCRIPT := IAR/ram_link.icf
GLOBAL_LDFLAGS += --config_def __JTAG_FLASH_WRITER_DATA_BUFFER_SIZE__=16384
GLOBAL_DEFINES += __JTAG_FLASH_WRITER_DATA_BUFFER_SIZE__=16384
else
DEFAULT_LINK_SCRIPT := GCC/ram_link.ld
GLOBAL_DEFINES += __JTAG_FLASH_WRITER_DATA_BUFFER_SIZE__=16384
endif

$(NAME)_COMPONENTS  += common/drivers/spi_flash

PRE_APP_BUILDS += bootloader

else # APP=ota_upgrade
ifeq ($(USES_BOOTLOADER_OTA),1)

# Building an app to run with the bootloader

$(NAME)_SOURCES     += bootloader_ota/app_header.c
GLOBAL_INCLUDES     += bootloader_ota/  ../../../../../Apps/waf/bootloader/
$(NAME)_LINK_FILES  += bootloader_ota/app_header.o
$(NAME)_COMPONENTS  += common/drivers/spi_flash

ifeq ($(TOOLCHAIN_NAME),IAR)
DEFAULT_LINK_SCRIPT := IAR/bootapp_link.icf
else
DEFAULT_LINK_SCRIPT := GCC/bootapp_link.ld
endif

PRE_APP_BUILDS += bootloader

else # USES_BOOTLOADER_OTA = 1
ifeq ($(NODCT),1)

# Building a standalone app without DCT (no bootloader)

#GLOBAL_DEFINES += WICED_DISABLE_STDIO
ifeq ($(TOOLCHAIN_NAME),IAR)
DEFAULT_LINK_SCRIPT := IAR/link.icf
else
DEFAULT_LINK_SCRIPT := GCC/link.ld
endif
GLOBAL_DEFINES += WICED_DISABLE_BOOTLOADER

else # NODCT = 1

# Building a standalone app with DCT (no bootloader)
$(info right one)

ifeq ($(TOOLCHAIN_NAME),IAR)
DEFAULT_LINK_SCRIPT := IAR/dct_link.icf
else
DEFAULT_LINK_SCRIPT := GCC/dct_link.ld

endif
#GLOBAL_DEFINES += WICED_DISABLE_STDIO
GLOBAL_DEFINES += WICED_DISABLE_BOOTLOADER

endif # NODCT = 1
endif # USES_BOOTLOADER_OTA = 1
endif # APP=ota_upgrade
endif # APP=bootloader
