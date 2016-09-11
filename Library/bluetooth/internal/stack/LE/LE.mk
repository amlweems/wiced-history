#
# Copyright 2013, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_Bluetooth_LE_Stack

GLOBAL_INCLUDES := . \
                   include

ifneq ($(wildcard $(CURDIR)Lib_Bluetooth_LE_Stack.$(RTOS).$(HOST_ARCH).$(BUILD_TYPE).a),)
$(NAME)_PREBUILT_LIBRARY := Lib_Bluetooth_LE_Stack.$(RTOS).$(HOST_ARCH).$(BUILD_TYPE).a
else

$(NAME)_SOURCES := bleapp/lestack/att/leatt.c \
                   bleapp/lestack/blecm/bleapp.c \
                   bleapp/lestack/blecm/blecm.c \
                   bleapp/lestack/blecm/emconinfo.c \
                   bleapp/lestack/blecm/bleappcfa.c \
                   bleapp/lestack/blecm/bleappfifo.c \
                   bleapp/lestack/blecm/bleapptimer.c \
                   bleapp/lestack/blecm/blesampapp.c \
                   bleapp/lestack/gatt/legattdb.c \
                   bleapp/lestack/gatt/legattdbsamp.c \
                   bleapp/lestack/l2cap/lel2cap.c \
                   bleapp/lestack/profile/bleprofile.c \
                   bleapp/lestack/smp/lesmp.c \
                   bleapp/lestack/smp/lesmpapi.c \
                   bleapp/lestack/smp/lesmpkeys.c \
                   bleapp/lestack/smp/aes_cmac.c \
                   bleapp/lestack/smp/aes.c \
                   bleapp/utils/bleappevent.c \
                   cfa/cfa.c \
                   rtos/osapi.c \
                   management/bt_smart_stack.c \
                   profiles/gap/bt_smart_gap.c \
                   profiles/gatt/bt_smart_att.c \
                   profiles/gatt/bt_smart_gatt.c 
                   
$(NAME)_INCLUDES := . \
                    ./.. \
                    hci \
                    cfa \
                    rtos \
                    bleapp/include \
                    bleapp/app \
                    bleapp/lestack \
                    bleapp/lestack/att \
                    bleapp/lestack/blecm \
                    bleapp/lestack/gatt \
                    bleapp/lestack/l2cap \
                    bleapp/lestack/profile \
                    bleapp/lestack/smp \
                    bleapp/utils
                                        
$(NAME)_CFLAGS   += -Wno-missing-braces \
                    -Wno-implicit-function-declaration \
                    -Wno-unused-variable \
                    -Wno-unused-value \
                    -Wno-uninitialized \
                    -Wno-parentheses \
                    -Wno-strict-aliasing \
                    -Wno-pointer-sign \
                    -Wno-return-type \
                    -Wno-int-to-pointer-cast \
                    -Wno-unused-but-set-variable \
                    -Wno-comment \
                    -Wno-unknown-pragmas

# WICED port
# HCI Low-Energy (ULP)
# RPA address resolution
$(NAME)_DEFINES  += WICED_BLE_PORT \
                    ULP_ENABLE \
                    BLE_SLAVE_ONLY_ADDRESS_RESOLUTION

ifeq (1, $(WICED_BLE_SERVER_TEST))
# Blood Pressure Monitor for testing
$(NAME)_DEFINES  += BLE_BPM_APP \
                    WICED_BLE_SERVER_TEST
                    
$(NAME)_SOURCES  += bleapp/app/blebpm.c
else
# SMP Initiator Role
$(NAME)_DEFINES  += SMP_INITIATOR
endif

GLOBAL_DEFINES   += WICED_BLE_PORT

endif #ifneq ($(wildcard $(CURDIR)Lib_Bluetooth_LE_Stack.$(RTOS).$(HOST_ARCH).$(BUILD_TYPE).a),)
