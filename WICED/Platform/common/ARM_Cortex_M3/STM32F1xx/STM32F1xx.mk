#
# Copyright 2013, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME = STM32F1xx_lib

GLOBAL_INCLUDES :=  . \
                    ..

ifeq ($(BUS),SPI)
$(NAME)_DEFINES += PLATFORM_DISABLE_UART_DMA
endif

$(NAME)_COMPONENTS  += common/$(TOOLCHAIN_NAME)
$(NAME)_COMPONENTS  += common/ARM_Cortex_M3/STM32F1xx/STM32F1xx_Drv

GLOBAL_DEFINES := USE_STDPERIPH_DRIVER
GLOBAL_ASMFLAGS += $$(CPU_ASMFLAGS)  $$(ENDIAN_ASMFLAGS_LITTLE)
GLOBAL_CFLAGS   += -D_STM32F103RET6_ -D_STM3x_ -D_STM32x_ $$(CPU_CFLAGS) $$(ENDIAN_CFLAGS_LITTLE)
GLOBAL_LDFLAGS  += -D_STM32F103RET6_ -D_STM3x_ -D_STM32x_ $$(CPU_CFLAGS) $$(ENDIAN_CFLAGS_LITTLE) -nostartfiles
GLOBAL_LDFLAGS  += -Wl,--defsym,__STACKSIZE__=$$($(RTOS)_START_STACK)


$(NAME)_SOURCES := ../crt0_$(TOOLCHAIN_NAME).c \
                   vector_table_$(TOOLCHAIN_NAME).s \
                   HardFault_handler.c \
                   wwd_platform.c \
                   $(BUS)/wwd_bus.c \
                   gpio_irq.c \
                   watchdog.c \
                   stm32f1xx_platform.c

# These need to be forced into the final ELF since they are not referenced otherwise
$(NAME)_LINK_FILES := ../crt0_$(TOOLCHAIN_NAME).o vector_table_$(TOOLCHAIN_NAME).o HardFault_handler.o

$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)

# Add maximum and default watchdog timeouts to definitions. Warning: Do not change MAX_WATCHDOG_TIMEOUT_SECONDS
MAX_WATCHDOG_TIMEOUT_SECONDS = 22
GLOBAL_DEFINES += MAX_WATCHDOG_TIMEOUT_SECONDS=$(MAX_WATCHDOG_TIMEOUT_SECONDS)


DCT_LINK_SCRIPT += GCC/dct_only_link.ld

# Check if building the bootloader
ifeq ($(APP),bootloader)

DEFAULT_LINK_SCRIPT := GCC/bootloader_link.ld
$(NAME)_SOURCES     += bootloader_ota/platform_bootloader.c
GLOBAL_INCLUDES     += bootloader_ota/
$(NAME)_COMPONENTS  += common/drivers/spi_flash

else # APP=bootloader
ifeq ($(APP),ota_upgrade)

$(error OTA upgrade app will not fit into STM32F1xx RAM)

else # APP=ota_upgrade
ifeq ($(APP),sflash_write)


# Building the Serial Flash Writer

WIFI_IMAGE_DOWNLOAD := buffered

$(NAME)_SOURCES     += bootloader_ota/platform_bootloader.c
GLOBAL_INCLUDES     += bootloader_ota/  ../../../../../Apps/waf/bootloader/
GLOBAL_DEFINES      += OTA_UPGRADE
DEFAULT_LINK_SCRIPT := GCC/ram_link.ld
$(NAME)_COMPONENTS  += common/drivers/spi_flash

PRE_APP_BUILDS += bootloader


else # APP=sflash_write
ifeq ($(USES_BOOTLOADER_OTA),1)

# Building an app to run with the bootloader

$(NAME)_SOURCES     += bootloader_ota/app_header.c
GLOBAL_INCLUDES     += bootloader_ota/  ../../../../../Apps/waf/bootloader/
$(NAME)_LINK_FILES  += bootloader_ota/app_header.o
$(NAME)_COMPONENTS  += common/drivers/spi_flash
DEFAULT_LINK_SCRIPT := GCC/bootapp_link.ld
#GLOBAL_DEFINES += WICED_DISABLE_STDIO

PRE_APP_BUILDS += bootloader

else # USES_BOOTLOADER_OTA = 1
ifeq ($(NODCT),1)

# Building a standalone app without DCT (no bootloader)

#GLOBAL_DEFINES += WICED_DISABLE_STDIO
DEFAULT_LINK_SCRIPT := GCC/link.ld
GLOBAL_DEFINES += WICED_DISABLE_BOOTLOADER

else # NODCT = 1

# Building a standalone app with DCT (no bootloader)
$(info right one)
DEFAULT_LINK_SCRIPT := GCC/dct_link.ld
#GLOBAL_DEFINES += WICED_DISABLE_STDIO
GLOBAL_DEFINES += WICED_DISABLE_BOOTLOADER

endif # NODCT = 1
endif # USES_BOOTLOADER_OTA = 1
endif # APP=sflash_write
endif # APP=ota_upgrade
endif # APP=bootloader
