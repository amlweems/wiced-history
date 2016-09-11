/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "http_server.h"
#include "wiced_bt_types.h"
#include "stdlib.h"
#include "string.h"
#include "big_stack_interface.h"
#include "blemesh_dct.h"

#ifdef __cplusplus
extern "C"
{
#endif


/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define PKT_CMD_CONFIG            0x00
#define PKT_CMD_ADD_GROUP         0x01
#define PKT_CMD_SEND_DATA         0x02
#define MESH_NODE_INFO_LENGTH     96
#define FLOODMESH_VENDOR_COMMAND  0x0192


// Broadcom flood mesh application packet type
#define  PKT_RAW                     0x00
#define  PKT_PTR_TO_MULTI_ACTUATORS  0x01
#define  PKT_PTR_TO_MULTI_PTR        0x02
#define  PKT_PTR_TO_PTR              0x03
#define  PKT_PTR_TO_PTR_WITH_ACK     0x13
#define  PKT_VISIBILITY_MAP          0x20

#define  CMD_RESET_BIG_MESH_NODE     0xFF
#define  CMD_DELETE_GROUP            0XF1
#define FLOOD_MESH_CNTRLR_ID       1
#define FLOOD_MESH_PKT_CNTR_LEN    5

#define FLOOD_MESH_NET_IV_LEN       8
#define FLOOD_MESH_NET_KEY_LEN     16

// Flood mesh IV (Initialization Vector) length
#define FLOOD_MESH_IV_LEN          8
// Flood mesh key length
#define FLOOD_MESH_KEY_LEN         16

// Flood mesh library version
#define FLOOD_MESH_LIB_VER_MAJOR    0
#define FLOOD_MESH_LIB_VER_MIDDLE   1
#define FLOOD_MESH_LIB_VER_MINOR    3

// Flood mesh packet related constants
#define FLOOD_MESH_PKT_HDR_LEN      8
#define FLOOD_MESH_MSG_HDR_LEN      3

#define FLOOD_MESH_MIC_LEN          4

#define FLOOD_MESH_MAX_DATA_LEN  (ADV_LEN_MAX - FLOOD_MESH_PKT_HDR_LEN - FLOOD_MESH_MIC_LEN)

// Maximum node number in the mesh network: 0xFF is fixed for broadcasting
#define FLOOD_MESH_MAX_NODE_NUM     255

// Flood mesh maximum group number for each device
#define FLOOD_MESH_MAX_GROUP_NUM    18

// Default minimum beacon adv interval.
// The actual interval is this value plus a random number between 0 and 255.

// Advertisement queue buffer number (must be 2^n) and length (32 bytes)
#define FLOOD_MESH_ADV_QUEUE_NUM          8

#define FLOOD_MESH_NONCE_LEN  (FLOOD_MESH_PKT_CNTR_LEN + FLOOD_MESH_IV_LEN)

/******************************************************
 *                   Enumerations
 ******************************************************/

enum
{
    FLOODMESH_SUB_OCF_VERSION           = 0x00,
    FLOODMESH_SUB_OCF_NET_CONFIG        = 0x01,
    FLOODMESH_SUB_OCF_NODE_CONFIG       = 0x02,
    FLOODMESH_SUB_OCF_ADD_GROUP         = 0x03,
    FLOODMESH_SUB_OCF_NET_START         = 0x04,
    FLOODMESH_SUB_OCF_NET_STOP          = 0x05,
    FLOODMESH_SUB_OCF_SEND_DATA         = 0x06,
    FLOODMESH_SUB_OCF_SEND_P2PDATA      = 0x07,
    FLOODMESH_SUB_OCF_SEND_ACK          = 0x08,
    FLOODMESH_SUB_OCF_NET_START_BEACON  = 0x09,
    FLOODMESH_SUB_OCF_NET_STOP_BEACON   = 0x0A,
    FLOODMESH_SUB_OCF_NODE_RESTORE      = 0x0B,
    FLOODMESH_SUB_OCF_NODE_GET_INFO     = 0x0C,
    FLOODMESH_SUB_OCF_NODE_SET_EVT_MASK = 0x0D
};

enum
{
    FLOODMESH_NET_RAW_DATA_EVT  =     0x00,
    FLOODMESH_NET_ACTUATOR_DATA_EVT = 0x01,
    FLOODMESH_NET_P2MP_DATA_EVT     = 0x02,
    FLOODMESH_NET_P2P_DATA_EVT      = 0x03,
    FLOODMESH_NET_P2P_ACK_EVT       = 0x04,
    FLOODMESH_NODE_SYNC_INFO_EVT    = 0x05,
    FLOODMESH_NET_P2MP_ACK_EVT      = 0x06,
};

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef unsigned char byte;

typedef wiced_result_t (*blemesh_dct_callback_t)( flood_mesh_dct_t* dct );

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    int  len;
    byte data[1];
} mesh_pkt;

typedef struct
{
    uint32_t length;
    uint8_t  value[1];
} mesh_value_handle_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

void blemesh_init( big_peer_device_link_keys_callback_t keys_callback, blemesh_dct_callback_t read_dct_callback, blemesh_dct_callback_t write_dct_callback );

mesh_pkt* build_pt_to_multi_act_pkt( mesh_value_handle_t* value );
mesh_pkt* build_pt_to_multi_pt_pkt( mesh_value_handle_t* value );
mesh_pkt* build_pt_to_pt_pkt( mesh_value_handle_t* value );

void wiced_mesh_vse_cback( UINT8 len, UINT8 *p );
void wiced_mesh_vsc_cback_net_start( wiced_bt_dev_vendor_specific_command_complete_params_t *p_vsc_result );
void wiced_mesh_vsc_cback_restore( wiced_bt_dev_vendor_specific_command_complete_params_t *p_vsc_result );
void wiced_mesh_vsc_cback_node_config( wiced_bt_dev_vendor_specific_command_complete_params_t *p_vsc_result );

void send_mesh_command( byte sub_cmd, mesh_pkt *dt, wiced_bt_dev_vendor_specific_command_complete_cback_t *p_cb );
wiced_result_t send_mesh_command_rest( wiced_http_response_stream_t* stream, byte sub_cmd, mesh_pkt *dt, wiced_bt_dev_vendor_specific_command_complete_cback_t *p_cb );
wiced_result_t send_mesh_command_proxy( byte sub_cmd, mesh_pkt *dt, wiced_bt_dev_vendor_specific_command_complete_cback_t *p_cb );

void wiced_mesh_vsc_proxy_data_cback( wiced_bt_dev_vendor_specific_command_complete_params_t *p_vsc_result );
void wiced_mesh_vsc_rest_data_cback( wiced_bt_dev_vendor_specific_command_complete_params_t *p_vsc_result );
void wiced_mesh_vsc_ver_cback( wiced_bt_dev_vendor_specific_command_complete_params_t *p_vsc_result );
void blemesh_node_config( byte *config_data );

#ifdef __cplusplus
} /* extern "C" */
#endif
