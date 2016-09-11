/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Defines common constants and types for WICED support for Bluetooth Smart
 */

#pragma once

#include "wiced_utilities.h"
#include "wiced_bt_constants.h"
#include "wiced_bt_smart_attribute.h"

/******************************************************
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/** @cond !ADDTHIS*/
/* Limits and Default Value for Scan Interval */
#define DEFAULT_SCAN_INTERVAL  0x0010
#define MIN_SCAN_INTERVAL      0x0004
#define MAX_SCAN_INTERVAL      0x4000

/* Limits and Default Value for Scan Window */
#define DEFAULT_SCAN_WINDOW    0x0010
#define MIN_SCAN_WINDOW        0x0004
#define MAX_SCAN_WINDOW        0x4000
/** @endcond */

/******************************************************
 *                   Enumerations
 ******************************************************/

/**
 *  Bluetooth Smart filter policy
 */
typedef enum
{
    FILTER_POLICY_NONE        = 0x00, /**< No filter policy         */
    FILTER_POLICY_WHITE_LIST  = 0x01, /**< White list filter policy */
} wiced_bt_smart_filter_policy_t;

/**
 *  Bluetooth Smart scan duplicates filter
 */
typedef enum
{
    DUPLICATES_FILTER_DISABLED = 0x00, /**< Duplicates filter is disabled */
    DUPLICATES_FILTER_ENABLED  = 0x01, /**< Duplicates filter is enabled  */
} wiced_bt_smart_filter_duplicated_t;

/**
 *  Bluetooth Smart address type
 */
typedef enum
{
    BT_SMART_ADDR_TYPE_PUBLIC  = 0x00, /**< Public address */
    BT_SMART_ADDR_TYPE_RANDOM  = 0x01  /**< Random address */
} wiced_bt_smart_address_type_t;

/**
 *  Bluetooth Smart scan type
 */
typedef enum
{
    BT_SMART_PASSIVE_SCAN  = 0x00, /**< Passive scan. Controller does not send SCAN_REQ and listens for Advertising from remote devices */
    BT_SMART_ACTIVE_SCAN   = 0x01, /**< Active scan. Controller sends SCAN_REQ. Controller listens for Advertising from remote devices and may receive SCAN_RSP from remote devices */
} wiced_bt_smart_scan_type_t;

/**
 *  Bluetooth Smart advertising event
 */
typedef enum
{
    BT_SMART_CONNECTABLE_UNDIRECTED_ADVERTISING_EVENT     = 0x00, /**< ADV_IND         : Connectable undirected advertising event     */
    BT_SMART_CONNECTABLE_DIRECTED_ADVERTISING_EVENT       = 0x01, /**< ADV_DIRECT_IND  : Connectable directed advertising event       */
    BT_SMART_SCANNABLE_UNDIRECTED_ADVERTISING_EVENT       = 0x02, /**< ADV_SCAN_IND    : Scannable undirected advertising event       */
    BT_SMART_NON_CONNECTABLE_UNDIRECTED_ADVERTISING_EVENT = 0x03, /**< ADV_NONCONN_IND : Non-connectable undirected advertising event */
} wiced_bt_smart_advertising_event_t;

/******************************************************
 *                    Structures
 ******************************************************/

#pragma pack(1)

/**
 *  Bluetooth Smart device
 */
typedef struct
{
    wiced_bt_device_address_t           address;      /**< Bluetooth device address */
    wiced_bt_smart_address_type_t       address_type; /**< Address Type             */
    char                                name[31];     /**< User-friendly name       */
} wiced_bt_smart_device_t;

/**
 *  Bluetooth Smart scan settings
 */
typedef struct
{
    wiced_bt_smart_scan_type_t          type;                /**< Scan type                                                                                     */
    wiced_bt_smart_filter_policy_t      filter_policy;       /**< Scan filter policy                                                                            */
    wiced_bt_smart_filter_duplicated_t  filter_duplicates;   /**< Scan duplicates filter                                                                        */
    uint16_t                            interval;            /**< Interval between scans.                Unit: 0.625ms. Range: 0x0004 - 0x4000 (2.5ms - 10.24s) */
    uint16_t                            window;              /**< Scan window. Must be <= scan interval. Unit: 0.625ms. Range: 0x0004 - 0x4000 (2.5ms - 10.24s) */
    uint16_t                            duration_second;     /**< Scan duration in seconds                                                                      */
} wiced_bt_smart_scan_settings_t;

/**
 *  Bluetooth Smart scan result
 */
typedef struct wiced_bt_smart_scan_result
{
    wiced_bt_smart_device_t             remote_device;                 /**< Remote device                                      */
    int8_t                              signal_strength;               /**< RSSI in dBm                                        */
    wiced_bt_smart_advertising_event_t  advertising_event;             /**< Advertising event received                         */
    uint8_t                             advertising_eir_data_length;   /**< Length of EIR data received with advertising event */
    uint8_t                             advertising_eir_data[31];      /**< EIR data of advertising event                      */
    uint8_t                             scan_response_eir_data_length; /**< Length of EIR data received with scan response     */
    uint8_t                             scan_response_eir_data[31];    /**< EIR data of scan response                          */
    struct wiced_bt_smart_scan_result*  next;                          /**< Pointer to the next scan result                    */

    /* Additional flag to help application filter scan results */
    wiced_bool_t                        filter_display;                /**< Set to WICED_TRUE if filter display                */

} wiced_bt_smart_scan_result_t;

/**
 *  Bluetooth Smart connection settings
 */
typedef struct
{
    uint16_t                            timeout_second;                /**< Connection timeout in seconds                                                              */
    wiced_bt_smart_filter_policy_t      filter_policy;                 /**< Connection initiator filter policy: No filter or using white list                          */
    uint16_t                            interval_min;                  /**< Connection Interval Min.      Unit: 1.25ms.            Range: 0x000A - 0x0C80 (7.5ms - 4s) */
    uint16_t                            interval_max;                  /**< Connection Interval Max.      Unit: 1.25ms.            Range: 0x000A - 0x0C80 (7.5ms - 4s) */
    uint16_t                            latency;                       /**< Connection Latency.           Unit: Connection Events. Range: 0x0000 - 0x01F4              */
    uint16_t                            supervision_timeout;           /**< Supervision Timeout.          Unit: 10ms.              Range: 0x000A - 0x0C80 (100ms - 32s)*/
    uint16_t                            ce_length_min;                 /**< Connection Event Length Min.  Unit: Connection Events. Range: 0x0000 - 0xFFFF              */
    uint16_t                            ce_length_max;                 /**< Connection Event Length Max.  Unit: Connection Events. Range: 0x0000 - 0xFFFF              */
    uint32_t                            attribute_protocol_timeout_ms; /**< Attribute protocol timeout in milliseconds                                                 */
} wiced_bt_smart_connection_settings_t;

/**
 *  Bluetooth Smart Extended Inquiry Response (EIR) data structure
 */
typedef struct
{
    uint8_t length;   /**< Length        */
    uint8_t type;     /**< Type          */
    uint8_t data[1];  /**< Start of data */
} wiced_bt_smart_eir_data_structure_t;


/**
 *  Bluetooth Smart security settings
 */
typedef struct
{
    uint16_t timeout_second; /**< Timeout in second. Default is 30 seconds */
} wiced_bt_smart_security_settings_t;

/**
 *  Bluetooth Smart Peer Device Bond Info
 */
typedef struct
{
    wiced_bt_device_address_t     peer_address; /**< Bonded peer device address                                                                            */
    wiced_bt_smart_address_type_t address_type; /**< Peer device's address type                                                                            */
    uint8_t                       irk[16];      /**< Peer device's Identity Resolving Key (IRK). Used for random address generation and resolution         */
    uint8_t                       csrk[16];     /**< Peer device's Connection Signature Resolving Key (CSRK). Used for signing data and verifying messages */
    uint8_t                       ltk[16];      /**< Peer device's Long Term Key (LTK). Used for encryption                                                */
    uint8_t                       rand[8];      /**< Peer device's Random Number (Rand). Used for identifying LTK                                          */
    uint16_t                      ediv;         /**< Peer device's Encrypted Diversifier (EDIV). Used for identifying LTK                                  */
} wiced_bt_smart_bond_info_t;
#pragma pack()

/******************************************************
 *                 Type Definitions
 ******************************************************/

/**
 * Bluetooth Smart scan complete callback
 */
typedef wiced_result_t (*wiced_bt_smart_scan_complete_callback_t)( void );

/**
 * Bluetooth Smart intermediate scan result callback
 */
typedef wiced_result_t (*wiced_bt_smart_scan_result_callback_t)  ( const wiced_bt_smart_scan_result_t* result );

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
