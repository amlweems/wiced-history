/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *
 * Pre-built SDP server database
 *
 * (Available soon: tool for generating SDP database)
 *
 */
#include "wiced_bt_cfg.h"
#include "wiced_bt_sdp.h"

/******************************************************
 *                     Macros
*******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define HDLR_DEVICE_ID       0x10001 /* SDP Record for Device ID       */
#define HDLR_AVDT_SINK       0x10002 /* SDP Record for AVDT Sink       */
#define HDLR_AVRC_CONTROLLER 0x10003 /* SDP Record for AVRC Controller */
#define HDLR_AVRC_TARGET     0x10004 /* SDP Record for AVRC Target     */

/******************************************************
 *               Variables Definitions
 *****************************************************/

const uint8_t sdp_database[] = // Define SDP database
{
    SDP_ATTR_SEQUENCE_2(327),

    // SDP Record for Device ID
    SDP_ATTR_SEQUENCE_1(77),
        SDP_ATTR_RECORD_HANDLE(HDLR_DEVICE_ID),
        SDP_ATTR_CLASS_ID(UUID_SERVCLASS_PNP_INFORMATION),
        SDP_ATTR_PROTOCOL_DESC_LIST(1),
        SDP_ATTR_BROWSE_LIST,
        SDP_ATTR_UINT2(ATTR_ID_SPECIFICATION_ID, 0x103),
        SDP_ATTR_UINT2(ATTR_ID_VENDOR_ID, 0x000F),
        SDP_ATTR_UINT2(ATTR_ID_PRODUCT_ID, 0x0000),
        SDP_ATTR_UINT2(ATTR_ID_PRODUCT_VERSION, 0x0001),
        SDP_ATTR_BOOLEAN(ATTR_ID_PRIMARY_RECORD, 0x01),
        SDP_ATTR_UINT2(ATTR_ID_VENDOR_ID_SOURCE, DI_VENDOR_ID_SOURCE_BTSIG),

    // SDP Record for AVDT Sink
    SDP_ATTR_SEQUENCE_1(77),
        SDP_ATTR_RECORD_HANDLE(HDLR_AVDT_SINK),
        SDP_ATTR_CLASS_ID(UUID_SERVCLASS_AUDIO_SINK),
        SDP_ATTR_ID(ATTR_ID_PROTOCOL_DESC_LIST), SDP_ATTR_SEQUENCE_1(16),
            SDP_ATTR_SEQUENCE_1(6),
                SDP_ATTR_UUID16(UUID_PROTOCOL_L2CAP),
                SDP_ATTR_VALUE_UINT2(BT_PSM_AVDTP),
            SDP_ATTR_SEQUENCE_1(6),
                SDP_ATTR_UUID16(UUID_PROTOCOL_AVDTP),
                SDP_ATTR_VALUE_UINT2(0x100),
        SDP_ATTR_ID(ATTR_ID_BT_PROFILE_DESC_LIST), SDP_ATTR_SEQUENCE_1(8),
            SDP_ATTR_SEQUENCE_1(6),
                SDP_ATTR_UUID16(UUID_SERVCLASS_ADV_AUDIO_DISTRIBUTION),
                SDP_ATTR_VALUE_UINT2(0x100),
        SDP_ATTR_UINT2(ATTR_ID_SUPPORTED_FEATURES, 0x000B),
        SDP_ATTR_SERVICE_NAME(16),
            'W', 'I', 'C', 'E', 'D', ' ', 'A', 'u', 'd', 'i', 'o', ' ', 'S', 'i', 'n', 'k',

    // SDP Record for AVRC Controller
    SDP_ATTR_SEQUENCE_1(86),
        SDP_ATTR_RECORD_HANDLE(HDLR_AVRC_CONTROLLER),
        SDP_ATTR_ID(ATTR_ID_SERVICE_CLASS_ID_LIST), SDP_ATTR_SEQUENCE_1(6),
            SDP_ATTR_UUID16(UUID_SERVCLASS_AV_REMOTE_CONTROL),
            SDP_ATTR_UUID16(UUID_SERVCLASS_AV_REM_CTRL_CONTROL),
        SDP_ATTR_ID(ATTR_ID_PROTOCOL_DESC_LIST), SDP_ATTR_SEQUENCE_1(16),
            SDP_ATTR_SEQUENCE_1(6),
                SDP_ATTR_UUID16(UUID_PROTOCOL_L2CAP),
                SDP_ATTR_VALUE_UINT2(BT_PSM_AVCTP),
            SDP_ATTR_SEQUENCE_1(6),
                SDP_ATTR_UUID16(UUID_PROTOCOL_AVCTP),
                SDP_ATTR_VALUE_UINT2(0x104),
        SDP_ATTR_ID(ATTR_ID_BT_PROFILE_DESC_LIST), SDP_ATTR_SEQUENCE_1(8),
            SDP_ATTR_SEQUENCE_1(6),
                SDP_ATTR_UUID16(UUID_SERVCLASS_AV_REMOTE_CONTROL),
                SDP_ATTR_VALUE_UINT2(0x0103),
        SDP_ATTR_UINT2(ATTR_ID_SUPPORTED_FEATURES, 0x0001),
        SDP_ATTR_SERVICE_NAME(22),
            'W', 'I', 'C', 'E', 'D', ' ', 'A', 'u', 'd', 'i', 'o', ' ', 'C', 'o', 'n', 't', 'r', 'o', 'l', 'l', 'e', 'r',

    // SDP Record for AVRC Target
    SDP_ATTR_SEQUENCE_1(79),
        SDP_ATTR_RECORD_HANDLE(HDLR_AVRC_TARGET),
        SDP_ATTR_CLASS_ID(UUID_SERVCLASS_AV_REM_CTRL_TARGET),
        SDP_ATTR_ID(ATTR_ID_PROTOCOL_DESC_LIST), SDP_ATTR_SEQUENCE_1(16),
            SDP_ATTR_SEQUENCE_1(6),
                SDP_ATTR_UUID16(UUID_PROTOCOL_L2CAP),
                SDP_ATTR_VALUE_UINT2(BT_PSM_AVCTP),
            SDP_ATTR_SEQUENCE_1(6),
                SDP_ATTR_UUID16(UUID_PROTOCOL_AVCTP),
                SDP_ATTR_VALUE_UINT2(0x104),
        SDP_ATTR_ID(ATTR_ID_BT_PROFILE_DESC_LIST), SDP_ATTR_SEQUENCE_1(8),
            SDP_ATTR_SEQUENCE_1(6),
                SDP_ATTR_UUID16(UUID_SERVCLASS_AV_REMOTE_CONTROL),
                SDP_ATTR_VALUE_UINT2(0x0105),
        SDP_ATTR_UINT2(ATTR_ID_SUPPORTED_FEATURES, 0x0002),
        SDP_ATTR_SERVICE_NAME(18),
            'W', 'I', 'C', 'E', 'D', ' ', 'A', 'u', 'd', 'i', 'o', ' ', 'T', 'a', 'r', 'g', 'e', 't',
};

const uint16_t wiced_bt_sdp_db_size = (sizeof(sdp_database));
