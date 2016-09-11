#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME = 43909_Peripheral_Drivers

$(eval $(call PLATFORM_LOCAL_DEFINES_INCLUDES_43909, ..))

$(NAME)_SOURCES := platform_watchdog.c        \
                   platform_chipcontrol.c     \
                   platform_m2m.c             \
                   platform_pinmux.c          \
                   platform_gpio.c            \
                   platform_spi_i2c.c         \
                   platform_mcu_powersave.c   \
                   platform_cores_powersave.c \
                   platform_backplane.c       \
                   platform_rtc.c             \
                   platform_hib.c             \
                   platform_otp.c             \
                   platform_8021as_clock.c    \
                   platform_audio_timer.c     \
                   platform_adc.c             \
                   platform_ascu.c

ifeq (,$(PLATFORM_NO_I2S))
$(NAME)_SOURCES += platform_i2s.c
else
GLOBAL_DEFINES  += PLATFORM_NO_I2S=1
endif

ifeq (,$(PLATFORM_NO_PWM))
$(NAME)_SOURCES += platform_pwm.c
else
GLOBAL_DEFINES  += PLATFORM_NO_PWM=1
endif

ifeq (,$(PLATFORM_NO_DDR))
$(NAME)_SOURCES += platform_ddr.c
$(NAME)_COMPONENTS += MCU/BCM4390x/peripherals/tlsf
else
GLOBAL_DEFINES  += PLATFORM_NO_DDR=1
endif

ifneq (,$(PLATFORM_NO_SOCSRAM_POWERDOWN))
GLOBAL_DEFINES  += PLATFORM_NO_SOCSRAM_POWERDOWN=1
endif

$(NAME)_COMPONENTS += MCU/BCM4390x/peripherals/spi_flash
$(NAME)_COMPONENTS += MCU/BCM4390x/peripherals/uart
$(NAME)_COMPONENTS += MCU/BCM4390x/peripherals/shared
$(NAME)_COMPONENTS += MCU/BCM4390x/peripherals/crypto/tiny_crypto

ifneq ($(WICED_USB_SUPPORT),)
$(NAME)_COMPONENTS += MCU/BCM4390x/peripherals/usb_host
endif

ifneq ($(WICED_SDIO_SUPPORT),)
$(NAME)_SOURCES    += platform_sdio_host.c
$(NAME)_COMPONENTS += MCU/BCM4390x/peripherals/sdio_host
endif

ifeq (,$(APP_WWD_ONLY)$(NS_WWD_ONLY)$(RTOS_WWD_ONLY)$(PLATFORM_NO_GMAC))
$(NAME)_COMPONENTS += MCU/BCM4390x/peripherals/ethernet
else
GLOBAL_DEFINES     += PLATFORM_NO_GMAC=1
endif
