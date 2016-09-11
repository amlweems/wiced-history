#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_apollo_bt_service

$(NAME)_SOURCES := \
					apollo_bt_main_service.c \
					apollo_bt_a2dp_sink.c \
					apollo_bt_a2dp_sink_profiling.c \
					apollo_bt_a2dp_decoder.c \
					apollo_bt_remote_control.c \
					apollo_bt_nv.c \
					apollo_config_gatt_server.c \
					bluetooth_cfg_dual_mode.c \
					bluetooth_sdp_db.c \
					mem_pool.c

GLOBAL_INCLUDES += .

$(NAME)_COMPONENTS := drivers/bluetooth/dual_mode
$(NAME)_COMPONENTS += libraries/audio/codec/codec_framework
$(NAME)_COMPONENTS += audio/apollo/apollocore

GLOBAL_DEFINES  += BUILDCFG
# this needs to be FALSE to allow app to override the BTEWICED
# linkkey management
GLOBAL_DEFINES  += BTM_INTERNAL_LINKKEY_STORAGE_INCLUDED=FALSE
GLOBAL_DEFINES  += BT_AUDIO_USE_MEM_POOL
#GLOBAL_DEFINES  += MEM_POOL_DEBUG

$(NAME)_INCLUDES   += \
					../apollocore \
					../apollo_streamer \
					../../../drivers/bluetooth \
                    ../../../drivers/bluetooth/include \
                    ../../../drivers/bluetooth/BTE \
                    ../../../drivers/bluetooth/BTE/WICED \
                    ../../../drivers/bluetooth/BTE/Components/stack/include \
                    ../../../drivers/bluetooth/BTE/Projects/bte/main \
                    ../../codec/codec_framework/include
