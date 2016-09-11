#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Supplicant_BESL

ifneq ($(wildcard $(CURDIR)BESL.$(HOST_ARCH).release.a),)
ifeq ($(HOST_HARDWARE_CRYPTO),1)
# Micro specific prebuilt library with hardware crypto support
$(NAME)_PREBUILT_LIBRARY := BESL.$(HOST_OPENOCD).release.a
else
# Architecture specific prebuilt library
$(NAME)_PREBUILT_LIBRARY := BESL.$(HOST_ARCH).release.a
endif # ifeq ($(HOST_HARDWARE_CRYPTO),1)
else
# Build from source (Broadcom internal)
include $(CURDIR)BESL_src.mk
endif # ifneq ($(wildcard $(CURDIR)ThreadX.$(HOST_ARCH).release.a),)


$(NAME)_SOURCES += host/WICED/besl_host.c \
                   host/WICED/wiced_tls.c \
                   host/WICED/wiced_wps.c \
                   host/WICED/wiced_p2p.c \
                   host/WICED/cipher_suites.c \
                   host/WICED/tls_cipher_suites.c \
                   host/WICED/dtls_cipher_suites.c \
                   host/WICED/p2p_internal.c \
                   host/WICED/wiced_supplicant.c \
                   P2P/p2p_events.c \
                   P2P/p2p_frame_writer.c \
                   host/WICED/wiced_dtls.c

GLOBAL_INCLUDES := host/WICED \
                   TLS \
                   crypto \
                   WPS \
                   include \
                   P2P \
                   crypto/homekit_srp \
                   crypto/ed25519 \
                   supplicant \
                   DTLS

GLOBAL_DEFINES  := ADD_LWIP_EAPOL_SUPPORT  NXD_EXTENDED_BSD_SOCKET_SUPPORT OPENSSL STDC_HEADERS

$(NAME)_COMPONENTS += utilities/base64
$(NAME)_COMPONENTS += utilities/TLV
$(NAME)_COMPONENTS += utilities/linked_list

$(NAME)_COMPONENTS += BESL/crypto_open \
                      crypto/micro-ecc

ifeq ($(IAR),)
$(NAME)_CFLAGS =  -fno-strict-aliasing
endif
