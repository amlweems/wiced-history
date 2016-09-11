/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Defines functions to perform Wi-Fi operations
 */

#pragma once

#include "wiced_utilities.h"
#include "wwd_wifi.h"
#include "wiced_rtos.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                    Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/** WPS Connection Mode
 */
typedef enum
{
    WICED_WPS_PBC_MODE,  /**< Push button mode */
    WICED_WPS_PIN_MODE   /**< PIN mode         */
} wiced_wps_mode_t;

/** WPS Device Category from the WSC2.0 spec
 */
typedef enum
{
    WICED_WPS_DEVICE_COMPUTER               = 1,     /**< COMPUTER               */
    WICED_WPS_DEVICE_INPUT                  = 2,     /**< INPUT                  */
    WICED_WPS_DEVICE_PRINT_SCAN_FAX_COPY    = 3,     /**< PRINT_SCAN_FAX_COPY    */
    WICED_WPS_DEVICE_CAMERA                 = 4,     /**< CAMERA                 */
    WICED_WPS_DEVICE_STORAGE                = 5,     /**< STORAGE                */
    WICED_WPS_DEVICE_NETWORK_INFRASTRUCTURE = 6,     /**< NETWORK_INFRASTRUCTURE */
    WICED_WPS_DEVICE_DISPLAY                = 7,     /**< DISPLAY                */
    WICED_WPS_DEVICE_MULTIMEDIA             = 8,     /**< MULTIMEDIA             */
    WICED_WPS_DEVICE_GAMING                 = 9,     /**< GAMING                 */
    WICED_WPS_DEVICE_TELEPHONE              = 10,    /**< TELEPHONE              */
    WICED_WPS_DEVICE_AUDIO                  = 11,    /**< AUDIO                  */
    WICED_WPS_DEVICE_OTHER                  = 0xFF,  /**< OTHER                  */
} wiced_wps_device_category_t;

/** WPS Configuration Methods from the WSC2.0 spec
 */
typedef enum
{
    WPS_CONFIG_USBA                  = 0x0001,  /**< USBA                 */
    WPS_CONFIG_ETHERNET              = 0x0002,  /**< ETHERNET             */
    WPS_CONFIG_LABEL                 = 0x0004,  /**< LABEL                */
    WPS_CONFIG_DISPLAY               = 0x0008,  /**< DISPLAY              */
    WPS_CONFIG_EXTERNAL_NFC_TOKEN    = 0x0010,  /**< EXTERNAL_NFC_TOKEN   */
    WPS_CONFIG_INTEGRATED_NFC_TOKEN  = 0x0020,  /**< INTEGRATED_NFC_TOKEN */
    WPS_CONFIG_NFC_INTERFACE         = 0x0040,  /**< NFC_INTERFACE        */
    WPS_CONFIG_PUSH_BUTTON           = 0x0080,  /**< PUSH_BUTTON          */
    WPS_CONFIG_KEYPAD                = 0x0100,  /**< KEYPAD               */
    WPS_CONFIG_VIRTUAL_PUSH_BUTTON   = 0x0280,  /**< VIRTUAL_PUSH_BUTTON  */
    WPS_CONFIG_PHYSICAL_PUSH_BUTTON  = 0x0480,  /**< PHYSICAL_PUSH_BUTTON */
    WPS_CONFIG_VIRTUAL_DISPLAY_PIN   = 0x2008,  /**< VIRTUAL_DISPLAY_PIN  */
    WPS_CONFIG_PHYSICAL_DISPLAY_PIN  = 0x4008   /**< PHYSICAL_DISPLAY_PIN */
} wiced_wps_configuration_method_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/** @cond TBD : add this doco? */
typedef struct
{
    wiced_scan_result_t ap_details;
    wiced_bool_t        scan_complete;
    void*               user_data;
} wiced_scan_handler_result_t;
/** @endcond */

/** @cond !ADDTHIS*/
/* Note: Do NOT modify this type definition.
 * Internal code make assumptions based on it's current definition
 * */
typedef wiced_result_t (*wiced_scan_result_handler_t)( wiced_scan_handler_result_t* malloced_scan_result );
/** @endcond */

/** WPS Device category holds WSC2.0 device category information
 */
typedef struct
{
    wiced_wps_device_category_t device_category; /**< Device category       */
    uint16_t sub_category;                       /**< Device sub-category   */
    char*    device_name;                        /**< Device name           */
    char*    manufacturer;                       /**< Manufacturer details  */
    char*    model_name;                         /**< Model name            */
    char*    model_number;                       /**< Model number          */
    char*    serial_number;                      /**< Serial number         */
    uint32_t config_methods;                     /**< Configuration methods */
} wiced_wps_device_detail_t;

/** WPS Credentials
 */
typedef struct
{
    wiced_ssid_t     ssid;               /**< AP SSID (name)       */
    wiced_security_t security;           /**< AP security type     */
    uint8_t          passphrase[64];     /**< AP passphrase        */
    uint16_t         passphrase_length;  /**< AP passphrase length */
} wiced_wps_credential_t;

/******************************************************
 *               Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/*****************************************************************************/
/** @addtogroup wifi       Wi-Fi (802.11) functions
 *
 *  WICED functions specific to Wi-Fi
 *
 *  @{
 */
/*****************************************************************************/


/** Negotiates securely with a Wi-Fi Protected Setup (WPS) registrar (usually an
 *  Access Point) and obtains credentials necessary to join the AP.
 *
 * @param[in] mode              : Indicates whether to use Push-Button (PBC) or PIN Number mode for WPS
 * @param[in] details           : Pointer to a structure containing manufacturing details
 *                                of this device
 * @param[in] password          : Password for WPS PIN mode connections
 * @param[out] credentials      : An array of credential structures that will receive the
 *                                securely negotiated credentials
 * @param[out] credential_count : The number of structures in the credentials parameter
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wps_enrollee(wiced_wps_mode_t mode, const wiced_wps_device_detail_t* details, char* password, wiced_wps_credential_t* credentials, uint16_t credential_count);


/** Negotiates securely with a Wi-Fi Protected Setup (WPS) enrollee (usually a
 *  client device) and provides credentials necessary to join a SoftAP.
 *
 * @param[in] mode              : Indicates whether to use Push-Button (PBC) or PIN Number mode for WPS
 * @param[in] details           : Pointer to a structure containing manufacturing details
 *                                of this device
 * @param[in] password          : Password for WPS PIN mode connections
 * @param[out] credentials      : An array of credential structures that will provide the
 *                                securely negotiated credentials
 * @param[out] credential_count : The number of structures in the credentials parameter
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wps_registrar(wiced_wps_mode_t mode, const wiced_wps_device_detail_t* details, char* password, wiced_wps_credential_t* credentials, uint16_t credential_count);


/** Scans for Wi-Fi networks
 *
 * @param[in] results_handler  : A function pointer for the handler that will process
 *                               the network details as they arrive.
 * @param[in] user_data        : An argument that will be passed to the results_handler function
 *                               of this device
 *
 * @note @li The results_handler and user_data variables will be referenced after the function returns.
 *           Those variables must remain valid until the scan is complete.
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_scan_networks( wiced_scan_result_handler_t results_handler, void* user_data);


/** Set roam trigger level
 *
 * @param[in] trigger_level : Trigger level in dBm. The Wi-Fi device will search for a new AP to connect to once the \n
 *                            signal from the AP (it is currently associated with) drops below the roam trigger level
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_set_roam_trigger( int32_t trigger_level );


/** Get the current channel on STA interface
 *
 * @param[out] channel : A pointer to the variable where the channel value will be written
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_get_channel( uint32_t* channel);


/** Get WLAN counter statistics for the interface provided
 *
 * @param[in] interface : The interface for which the counters are requested
 * @param[out] counters : A pointer to the structure where the counter data will be written
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_get_counters(wiced_interface_t interface, wl_cnt_v6_t* counters);


/** Sets the 802.11 powersave listen interval for a Wi-Fi client, and communicates
 *  the listen interval to the Access Point. The listen interval will be set to
 *  (listen_interval x time_unit) seconds.
 *
 *  The default value for the listen interval is 0. With the default value set,
 *  the Wi-Fi device wakes to listen for AP beacons every DTIM period.
 *
 *  If the DTIM listen interval is non-zero, the DTIM listen interval will over ride
 *  the beacon listen interval value.
 *
 *  If it is necessary to set the listen interval sent to the AP to a value other
 *  than the value set by this function, use the additional association listen
 *  interval API : wiced_wifi_set_listen_interval_assoc()
 *
 *  @note This function applies to 802.11 powersave operation. Please read the
 *  WICED Powersave Application Note provided in the WICED-SDK/Doc directory for further
 *  information about the operation of the 802.11 listen interval.
 *
 * @param[in] listen_interval : The desired beacon listen interval
 * @param[in] time_unit       : The listen interval time unit; options are beacon period or DTIM period
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_set_listen_interval( uint8_t listen_interval, wiced_listen_interval_time_unit_t time_unit );


/** Sets the 802.11 powersave beacon listen interval communicated to Wi-Fi Access Points
 *
 *  This function is used by Wi-Fi clients to set the value of the beacon
 *  listen interval sent to the AP (in the association request frame) during
 *  the association process.
 *
 *  To set the client listen interval as well, use the wiced_wifi_set_listen_interval() API
 *
 *  @note This function applies to 802.11 powersave operation. Please read the
 *  WICED Powersave Application Note provided in the WICED-SDK/Doc directory for further
 *  information about the operation of the 802.11 listen interval.
 *
 * @param listen_interval : The beacon listen interval sent to the AP during association.
 *                          The time unit is specified in multiples of beacon periods.
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_set_listen_interval_assoc( uint16_t listen_interval );


/** Gets the current value of all beacon listen interval variables
 *
 * @param[out] li : The current value of all listen interval settings
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_get_listen_interval( wiced_listen_interval_t* li );


/** Enables powersave mode without regard for throughput reduction
 *
 * This function enables (legacy) 802.11 PS-Poll mode and should be used
 * to achieve the lowest power consumption possible when the Wi-Fi device
 * is primarily passively listening to the network
 *
 * @warning An accurate 32kHz clock reference must be connected to the WLAN        \n
 *          sleep clock input pin while the WLAN chip is in powersave mode!        \n
 *          Failure to meet this requirement will result in poor WLAN performance. \n
 *          The sleep clock reference is typically configured in the file:         \n
 *          <WICED-SDK>/include/platforms/<PLATFORM_NAME>/platform.h
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_enable_powersave( void );


/** Enables powersave mode while attempting to maximise throughput
 *
 * Network traffic is typically bursty. Reception of a packet often means that another
 * packet will be received shortly afterwards (and vice versa for transmit)
 * \p
 * In high throughput powersave mode, rather then entering powersave mode immediately
 * after receiving or sending a packet, the WLAN chip will wait for a timeout period before
 * returning to sleep
 *
 * @note return_to_sleep_delay must be set to a multiple of 10. When set to 0, the timeout period
 *       defaults to 2 beacon intervals (approximately 204ms).
 *
 * @warning An accurate 32kHz clock reference must be connected to the WLAN        \n
 *          sleep clock input pin while the WLAN chip is in powersave mode!        \n
 *          Failure to meet this requirement will result in poor WLAN performance. \n
 *          The sleep clock reference is typically configured in the file:         \n
 *          <WICED-SDK>/include/platforms/<PLATFORM_NAME>/platform.h
 *
 * @param[in] return_to_sleep_delay : Timeout period (in milliseconds) before the WLAN chip returns to sleep
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_enable_powersave_with_throughput( uint8_t return_to_sleep_delay_ms );


/** Disable 802.11 power save mode
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_disable_powersave( void );


/** Sets the packet filter mode (or rule) to either forward or discard packets on a match
 *
 * @param[in] mode : Packet filter mode
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_set_packet_filter_mode( wiced_packet_filter_mode_t mode );


/** Adds an ethernet packet filter which causes the WLAN chip to drop all packets that
 * do NOT match the filter
 *
 * When a packet filter(s) is installed, incoming packets received by the WLAN chip are
 * run through the pre-installed filter(s). Filter criteria are added using this API function.
 * If the WLAN chip receives a packet that matches one of the currently installed filters,
 * the host MCU is notified, and the packet is forwarded to the MCU. Packets that do
 * not match any of the installed filters are dropped by the WLAN chip.
 * \p
 * If there are no packet filters installed, all received packets are passed from
 * the WLAN chip to the host MCU
 *
 * @param[in] filter_id : The unique user assigned ID for the filter
 * @param[in] settings  : Packet filter settings
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_add_packet_filter( uint8_t filter_id, const wiced_packet_filter_settings_t* settings );


/** Removes (uninstalls) a previously installed packet filter
 *
 * @param[in] filter_id : The unique user assigned ID for the filter
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_remove_packet_filter( uint8_t filter_id );


/** Enables a previously installed packet filter
 *
 * @param[in] filter_id : The unique user assigned ID for the filter
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_enable_packet_filter( uint8_t filter_id );


/** Disables a previously installed packet filter
 *
 * @param[in] filter_id : The unique user assigned ID for the filter
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_disable_packet_filter( uint8_t filter_id );


/** Gets packet filter statistics including packets matched, packets forwarded and packets discarded.
 *
 * @param[in]  filter_id : The unique user assigned ID for the filter
 * @param[out] stats     : A pointer to a structure that will be populated with filter statistics
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_get_packet_filter_stats( uint8_t filter_id, wiced_packet_filter_stats_t* stats );


/** Clear all packet filter statistics
 *
 * @param[in] filter_id : The unique user assigned ID for the filter
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_clear_packet_filter_stats( uint32_t filter_id );


/** Get a list of installed and enabled packet filters
 *
 * @warning wiced_wifi_delete_packet_filter_list() must be called after this
 *          function to release memory internally allocated for the list
 *
 * @param[out] count : Number of filters enabled
 * @param[out] list  : A pointer to the retrieved filter list (this function automatically allocates memory for the list!)
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_get_enabled_packet_filter_list( uint32_t* count, wiced_packet_filter_t** list );


/** Get a list of installed and disabled packet filters
 *
 * @warning wiced_wifi_delete_packet_filter_list() must be called after this
 *          function to release memory internally allocated for the list
 *
 * @param[out] count : Number of filters disabled
 * @param[out] list  : A pointer to the retrieved filter list (this function automatically allocates memory for the list!)
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_get_disabled_packet_filter_list( uint32_t* count, wiced_packet_filter_t** list );


/** Delete a list of packet filters that were previously retrieved
 *
 * This function deletes a list of packet filters created with one of the following functions:
 * \li @ref wiced_wifi_get_enabled_packet_filter_list()
 * \li @ref wiced_wifi_get_disabled_packet_filter_list()
 *
 * @param[in] list : Pointer to the list to be deleted
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_delete_packet_filter_list( wiced_packet_filter_t* list );


/** Add a network keep alive packet
 *
 * Keep alive functionality enables the WLAN chip to automatically send
 * an arbitrary IP packet and/or 802.11 Null Function data frame at
 * a regular interval
 * \p
 * This feature may be used to maintain connectivity with a Wi-Fi AP
 * and/or remote network application
 *
 * \li A maximum of 4 keep alive packets can be configured to operate concurrently
 * \li Keep alive packet functionality only works with client (STA) mode
 * \li If the keep alive packet length is set to 0, a Null-Function Data frame is automatically used as the keep alive
 * \li Any ethernet packet can be sent as a keep alive packet
 *
 * @param[in] keep_alive_packet_info : Pointer to a @ref wiced_keep_alive_packet_t structure used to setup the keep alive packet
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_add_keep_alive( wiced_keep_alive_packet_t* keep_alive_packet_info );


/** Get information about a keep alive packet
 *
 * \li The ID of the keep alive packet should be provided in the keep_alive_info structure
 * \li The application must pre-allocate a buffer to store the keep alive packet that is read from the WLAN chip
 * \li The length of the buffer must be provided in the packet_length field of the structure
 * \li The repeat period and keep alive packet bytes are populated by this function upon successful return
 *
 * @param[in,out] keep_alive_packet_info : Pointer to the wiced_keep_alive_t structure to be populated
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_get_keep_alive( wiced_keep_alive_packet_t* keep_alive_packet_info );


/** Disable a keep alive packet specified by id
 *
 * @param[in] id : ID of the keep alive packet to be disabled
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_disable_keep_alive( uint8_t id );

/** Gets information about associated clients.
 *
 * @note Only applicable if softAP interface is up
 *
 * @param[out] client_list_buffer : pointer to a buffer that will be populated with a variable length structure defined by @ref wiced_maclist_t
 * @param[in]  buffer_length      : length of the buffer
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_get_associated_client_list( void* client_list_buffer, uint16_t buffer_length );

/** Gets information about the AP the client interface is currently associated to
 *
 * @note Only applicable if STA (client) interface is associated to an AP
 *
 * @param[out] ap_info  : Pointer to structure that will be populated with AP information
 * @param[out] security : Pointer to structure that will be populated with AP security type
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_get_ap_info( wiced_bss_info_t* ap_info, wiced_security_t* security );


/** @} */

#ifdef __cplusplus
} /*extern "C" */
#endif
