#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := WWD_for_$(BUS)_$(RTOS)

GLOBAL_INCLUDES := . \
                   include \
                   include/Network \
                   include/RTOS \
                   internal/Bus_protocols/$(BUS)


$(NAME)_SOURCES := internal/wwd_thread.c \
                   internal/SDPCM.c \
                   internal/wwd_internal.c \
                   internal/wwd_management.c \
                   internal/wwd_wifi.c \
                   internal/wwd_crypto.c \
                   internal/wwd_logging.c \
                   internal/Bus_protocols/$(BUS)/wwd_bus_protocol.c \
                   internal/wwd_ring_buffer.c

$(NAME)_CHECK_HEADERS := internal/bcmendian.h \
                         internal/SDPCM.h \
                         internal/wwd_ap.h \
                         internal/wwd_internal.h \
                         internal/wwd_logging.h \
                         internal/wwd_thread.h \
                         internal/Bus_protocols/wwd_bus_protocol_interface.h \
                         internal/Bus_protocols/$(BUS)/wwd_bus_protocol.h \
                         internal/chips/$(CHIP)/chip_constants.h \
                         internal/wifi_image/wwd_wifi_image_interface.h \
                         include/wwd_wlioctl.h \
                         include/wwd_assert.h \
                         include/wwd_constants.h \
                         include/wwd_crypto.h \
                         include/wwd_debug.h \
                         include/wwd_events.h \
                         include/wwd_management.h \
                         include/wwd_poll.h \
                         include/wwd_wifi.h \
                         include/Network/wwd_buffer_interface.h \
                         include/Network/wwd_network_constants.h \
                         include/Network/wwd_network_interface.h \
                         include/Platform/wwd_bus_interface.h \
                         include/Platform/wwd_platform_interface.h \
                         include/Platform/wwd_sdio_interface.h \
                         include/Platform/wwd_spi_interface.h \
                         include/RTOS/wwd_rtos_interface.h

#                         internal/packed_section_end.h \
#                         internal/packed_section_start.h \
#


ifndef NO_WIFI_FIRMWARE
$(NAME)_COMPONENTS += Wiced/WWD/internal/wifi_image
endif

ifeq ($(CHIP),)
$(error ERROR: CHIP must be defined in your platform makefile)
endif

ifeq ($(HOST_MICRO),)
$(error ERROR: HOST_MICRO must be defined in your platform makefile)
endif

$(NAME)_SOURCES += internal/chips/$(CHIP)/wwd_ap.c
                   
$(NAME)_SOURCES += internal/wifi_image/$(strip $(WIFI_IMAGE_DOWNLOAD))/wwd_firmware.c

#$(error Must define either WIFI_IMAGE_DOWNLOAD := direct or WIFI_IMAGE_DOWNLOAD := buffered)


$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)

