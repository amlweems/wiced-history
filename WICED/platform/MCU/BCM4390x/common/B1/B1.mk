#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
OTP_SECUREKEYS_OFFSETS_DEFINED := 1
GLOBAL_DEFINES += OTP_WORD_NUM_SHA_KEY=258
GLOBAL_DEFINES += OTP_WORD_NUM_SHA_KEY_R=242
GLOBAL_DEFINES += OTP_WORD_NUM_AES_KEY=282
GLOBAL_DEFINES += OTP_WORD_NUM_AES_KEY_R=274
GLOBAL_DEFINES += SECUREBOOT_SHA_KEY_SIZE=32
GLOBAL_DEFINES += GSIO_I2C_REPEATED_START_NEEDS_GPIO=1
GLOBAL_DEFINES += PLATFORM_HIB_WAKE_CTRL_FREQ_BITS_FLIPPED

ifeq ($(LINK_BOOTLOADER_WITH_ROM_SYMBOLS),TRUE)
$(NAME)_SOURCES += $(SOURCE_ROOT)common/B1/rom_offload/bootloader_rom_symbols.c
endif
