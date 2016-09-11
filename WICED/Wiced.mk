#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME = WICED

$(NAME)_SOURCES := internal/wiced_core.c

ifndef USES_BOOTLOADER_OTA
USES_BOOTLOADER_OTA :=1
endif

ifeq ($(BUILD_TYPE),debug)
#$(NAME)_COMPONENTS += test/malloc_debug
endif

# Check if WICED level API is required
ifndef NO_WICED_API
$(NAME)_SOURCES += internal/time.c \
                   internal/system_monitor.c \
                   internal/wiced_lib.c \
                   internal/wiced_crypto.c \
                   internal/waf.c

# Check if Wi-Fi is not required
ifndef NO_WIFI
$(NAME)_SOURCES += internal/wifi.c \
                   internal/wiced_cooee.c \
                   internal/wiced_easy_setup.c \
                   internal/wiced_filesystem.c

$(NAME)_INCLUDES := security/BESL/crypto \
                    security/BESL/include \
		            security/BESL/host/WICED \
		            security/BESL/WPS

$(NAME)_COMPONENTS += WICED/WWD \
                      WICED/security/BESL \
                      protocols/DNS

GLOBAL_DEFINES += ADD_NETX_EAPOL_SUPPORT

ifndef NETWORK
NETWORK := NetX_Duo
$(NAME)_COMPONENTS += NetX_Duo
endif

else #ifndef NO_WIFI
GLOBAL_INCLUDES += WWD/include \
                   security/BESL/include \
                   security/BESL/host/WICED \
                   security/BESL/crypto

endif #ifndef NO_WIFI

else # ifndef NO_WICED_API
ifneq ($(NETWORK),)
$(NAME)_COMPONENTS += WICED/WWD
else # NETWORK
GLOBAL_INCLUDES += WWD/include
endif # NETWORK
GLOBAL_INCLUDES += security/BESL/include \
                   security/BESL/host/WICED \
                   security/BESL/crypto
endif # ifndef NO_WICED_API

$(NAME)_CHECK_HEADERS := internal/wiced_internal_api.h \
                         ../include/default_wifi_config_dct.h \
                         ../include/resource.h \
                         ../include/wiced.h \
                         ../include/wiced_defaults.h \
                         ../include/wiced_easy_setup.h \
                         ../include/wiced_framework.h \
                         ../include/wiced_management.h \
                         ../include/wiced_platform.h \
                         ../include/wiced_rtos.h \
                         ../include/wiced_security.h \
                         ../include/wiced_tcpip.h \
                         ../include/wiced_time.h \
                         ../include/wiced_utilities.h \
                         ../include/wiced_crypto.h \
                         ../include/wiced_wifi.h

# Add WICEDFS as standard filesystem
$(NAME)_COMPONENTS += filesystems/wicedfs

# Add MCU component
$(NAME)_COMPONENTS += WICED/platform/MCU/$(HOST_MCU_FAMILY)

# Define the default ThreadX and FreeRTOS starting stack sizes
FreeRTOS_START_STACK := 800
ThreadX_START_STACK  := 800

GLOBAL_DEFINES += WWD_STARTUP_DELAY=10 \
                  BOOTLOADER_MAGIC_NUMBER=0x4d435242
                  APP_NAME=$$(SLASH_QUOTE)$(APP)$$(SLASH_QUOTE)

GLOBAL_INCLUDES += . \
                   platform/include

$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)
