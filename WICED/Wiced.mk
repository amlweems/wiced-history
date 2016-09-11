#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME = Wiced

CERTIFICATE := ../Resources/config/device.cer
PRIVATE_KEY := ../Resources/config/id_rsa


$(NAME)_SOURCES := internal/malloc_debug.c

# Check if the WICED API is being used
ifeq (,$(APP_WWD_ONLY)$(NS_WWD_ONLY)$(RTOS_WWD_ONLY))

$(NAME)_SOURCES += internal/wifi.c \
                   internal/config.c \
                   internal/config_http_content.c \
                   internal/time.c \
                   internal/wiced_lib.c \
                   internal/management.c \
                   internal/system_monitor.c \
                   internal/wiced_cooee.c \
                   internal/wiced_easy_setup.c

$(NAME)_RESOURCES := images/brcmlogo.png \
                     images/brcmlogo_line.png \
                     images/favicon.ico \
                     images/scan_icon.png \
                     images/wps_icon.png \
                     scripts/general_ajax_script.js \
                     scripts/wpad.dat \
                     config/device_settings.html \
                     config/join.html \
                     config/scan_page_outer.html \
                     config/scan_results.html \
                     config/wps_pbc.html \
                     config/wps_pin.html \
                     config/redirect.html \
                     styles/buttons.css \
                     styles/border_radius.htc

$(NAME)_INCLUDES := Security/besl/tlv \
                    Security/besl/crypto \
                    Security/besl/include

ifeq (NetX,$(NETWORK))
$(NAME)_COMPONENTS += Wiced/Security/besl
$(NAME)_COMPONENTS += daemons/http_server
$(NAME)_COMPONENTS += daemons/dns_redirect
$(NAME)_COMPONENTS += protocols/dns
GLOBAL_DEFINES += ADD_NETX_EAPOL_SUPPORT USE_MICRORNG
endif

ifeq (NetX_Duo,$(NETWORK))
$(NAME)_COMPONENTS += Wiced/Security/besl
$(NAME)_COMPONENTS += daemons/http_server
$(NAME)_COMPONENTS += daemons/dns_redirect
$(NAME)_COMPONENTS += protocols/dns
GLOBAL_DEFINES += ADD_NETX_EAPOL_SUPPORT USE_MICRORNG
endif

ifeq (LwIP,$(NETWORK))
$(NAME)_COMPONENTS += Wiced/Security/besl
$(NAME)_COMPONENTS += daemons/http_server
$(NAME)_COMPONENTS += daemons/dns_redirect
$(NAME)_COMPONENTS += protocols/dns
endif

endif

# Add standard WICED 1.x components
$(NAME)_COMPONENTS += Wiced/WWD

# Define the default ThreadX and FreeRTOS starting stack sizes
FreeRTOS_START_STACK := 800
ThreadX_START_STACK  := 800

GLOBAL_DEFINES += WICED_STARTUP_DELAY=10 \
                  BOOTLOADER_MAGIC_NUMBER=0x4d435242

GLOBAL_INCLUDES := .
