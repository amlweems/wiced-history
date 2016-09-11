#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Supplicant_besl

# Verify that NoNS isn't being used.
ifneq ($(NETWORK),NoNS)

ifneq ($(wildcard $(CURDIR)besl.$(HOST_ARCH).$(BUILD_TYPE).a),)
ifeq ($(HOST_HARDWARE_CRYPTO),1)
$(NAME)_PREBUILT_LIBRARY := besl.$(HOST_MICRO).$(BUILD_TYPE).a
else
$(NAME)_PREBUILT_LIBRARY := besl.$(HOST_ARCH).$(BUILD_TYPE).a
endif # ifeq ($(HOST_HARDWARE_CRYPTO),1)
else
$(NAME)_SOURCES += wps/wps_enrollee.c \
                   wps/wps_registrar.c \
                   wps/wps_common.c \
                   crypto/aes.c \
                   crypto/hmac_sha256.c \
                   crypto/rijndael-alg-fst.c \
                   crypto/sha256.c \
                   crypto/progressive_hmac.c \
                   tlv/tlv.c \
                   crypto/nn.c \
                   p2p/p2p.c 

$(NAME)_INCLUDES += include \
                    common \
                    common/proto \
                    scan

ifneq ($(RTOS),NoOS)
$(NAME)_SOURCES += tls/ssl_tls.c \
                   tls/ssl_cli.c \
                   tls/ssl_srv.c \
                   crypto/rsa.c \
                   crypto/aes_ussl.c \
                   crypto/arc4-x1.c \
                   crypto/base64.c \
                   crypto/md5.c \
                   crypto/x509parse.c \
                   crypto/sha1.c \
                   crypto/sha2.c \
                   crypto/bignum.c \
                   crypto/des.c \
                   crypto/microrng.c
           
endif # ifneq ($(RTOS),NoOS)
endif # ifneq ($(wildcard $(CURDIR)ThreadX.$(HOST_ARCH).$(BUILD_TYPE).a),)

$(NAME)_SOURCES += host/wiced/wiced_wps.c \
				   host/wiced/wiced_tls.c \
                   host/wiced/wiced_p2p.c \
                   host/wiced/besl_host.c

$(NAME)_HEADERS  := host/wiced/wps_host.h \
                    tls/tls_host_api.h \
                    host/wiced/wiced_tls.h

$(NAME)_INCLUDES += wps/include \
                    host/wiced \
                    tlv \
                    common

$(NAME)_CFLAGS =  -Wno-error=format \
                  -Wno-error=unused-variable \
                  -Wno-error=parentheses \
                  -Wno-error=unused-but-set-variable \
                  -Wno-error=strict-aliasing \
                  -Wno-error=switch \
                  -Wno-switch-enum \
                  -fno-strict-aliasing


$(NAME)_DEFINES := OPENSSL_SMALL_FOOTPRINT \
                   MYKROSSL_HAVE_LONGLONG \
                   MYKROSSL_GENPRIME \
                   MYKROSSL_AES_ROM_TABLES \
                   MYKROSSL_AES_C \
                   MYKROSSL_ARC4_C \
                   MYKROSSL_BASE64_C \
                   MYKROSSL_BIGNUM_C \
                   MYKROSSL_CERTS_C \
                   MYKROSSL_DEBUG_C \
                   MYKROSSL_DES_C \
                   MYKROSSL_MICRORNG_C \
                   MYKROSSL_MD5_C \
                   MYKROSSL_NET_C \
                   MYKROSSL_RSA_C \
                   MYKROSSL_SHA1_C \
                   MYKROSSL_SHA2_C \
                   MYKROSSL_SSL_CLI_C \
                   MYKROSSL_SSL_SRV_C \
                   MYKROSSL_SSL_TLS_C \
                   MYKROSSL_X509_PARSE_C

GLOBAL_INCLUDES := host/wiced \
                   tls \
                   crypto \
                   tlv \
                   wps \
                   include \
                   p2p

GLOBAL_DEFINES  := ADD_LWIP_EAPOL_SUPPORT  NXD_EXTENDED_BSD_SOCKET_SUPPORT

$(NAME)_ALWAYS_OPTIMISE := 1

endif # ifneq ($(NETWORK),NoNS)