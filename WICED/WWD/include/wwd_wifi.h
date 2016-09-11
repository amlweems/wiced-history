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
 *  Prototypes of functions for controlling the Wi-Fi system
 *
 *  This file provides prototypes for end-user functions which allow
 *  actions such as scanning for Wi-Fi networks, joining Wi-Fi
 *  networks, getting the MAC address, etc
 *
 */

#ifndef INCLUDED_WWD_WIFI_H
#define INCLUDED_WWD_WIFI_H

#include <stdint.h>
#include "wwd_constants.h"                  /* For wiced_result_t */
#include "RTOS/wwd_rtos_interface.h"        /* For semaphores */
#include "Network/wwd_network_interface.h"  /* For interface definitions */

#ifdef __cplusplus
extern "C"
{
#endif

/** @cond !ADDTHIS*/

#ifndef WICED_MAXIMUM_LINK_CALLBACK_SUBSCRIPTIONS
#define WICED_MAXIMUM_LINK_CALLBACK_SUBSCRIPTIONS     (5)
#endif

/* Packet Filter Offsets for Ethernet Frames */
#define FILTER_OFFSET_PACKET_START                       0
#define FILTER_OFFSET_ETH_HEADER_DESTINATION_ADDRESS     0
#define FILTER_OFFSET_ETH_HEADER_SOURCE_ADDRESS          6
#define FILTER_OFFSET_ETH_HEADER_ETHERTYPE              12
#define FILTER_OFFSET_ETH_DATA                          14

/* Packet Filter Offsets for ARP Packets */
#define FILTER_OFFSET_ARP_HEADER_START                  14
#define FILTER_OFFSET_ARP_HEADER_HTYPE                  14
#define FILTER_OFFSET_ARP_HEADER_PTYPE                  16
#define FILTER_OFFSET_ARP_HEADER_HLEN                   18
#define FILTER_OFFSET_ARP_HEADER_PLEN                   19
#define FILTER_OFFSET_ARP_HEADER_OPER                   20
#define FILTER_OFFSET_ARP_HEADER_SHA                    22
#define FILTER_OFFSET_ARP_HEADER_SPA                    28
#define FILTER_OFFSET_ARP_HEADER_THA                    30
#define FILTER_OFFSET_ARP_HEADER_TPA                    36

/* Packet Filter Offsets for IPv4 Packets */
#define FILTER_OFFSET_IPV4_HEADER_START                 14
#define FILTER_OFFSET_IPV4_HEADER_VER_IHL               14
#define FILTER_OFFSET_IPV4_HEADER_DSCP_ECN              15
#define FILTER_OFFSET_IPV4_HEADER_TOTAL_LEN             16
#define FILTER_OFFSET_IPV4_HEADER_ID                    18
#define FILTER_OFFSET_IPV4_HEADER_FLAGS_FRAGMENT_OFFSET 20
#define FILTER_OFFSET_IPV4_HEADER_TTL                   22
#define FILTER_OFFSET_IPV4_HEADER_PROTOCOL              23
#define FILTER_OFFSET_IPV4_HEADER_CHECKSUM              24
#define FILTER_OFFSET_IPV4_HEADER_SOURCE_ADDR           26
#define FILTER_OFFSET_IPV4_HEADER_DESTINATION_ADDR      30
#define FILTER_OFFSET_IPV4_HEADER_OPTIONS               34
#define FILTER_OFFSET_IPV4_DATA_START                   38

/* Packet Filter Offsets for IPv4 Packets */
#define FILTER_OFFSET_IPV6_HEADER_START                 14
#define FILTER_OFFSET_IPV6_HEADER_PAYLOAD_LENGTH        18
#define FILTER_OFFSET_IPV6_HEADER_NEXT_HEADER           20
#define FILTER_OFFSET_IPV6_HEADER_HOP_LIMIT             21
#define FILTER_OFFSET_IPV6_HEADER_SOURCE_ADDRESS        22
#define FILTER_OFFSET_IPV6_HEADER_DESTINATION_ADDRESS   38
#define FILTER_OFFSET_IPV6_DATA_START                   54

/* Packet Filter Offsets for ICMP Packets */
#define FILTER_OFFSET_ICMP_HEADER_START                 14

/** @endcond */

/******************************************************
 * Enumerations
 ******************************************************/

/******************************************************
 *             Structures
 ******************************************************/


/******************************************************
 *             Function declarations
 ******************************************************/

/** Scan result callback function pointer type
 *
 * @param result_ptr  : A pointer to the pointer that indicates where to put the next scan result
 * @param user_data   : User provided data
 */
typedef void (*wiced_scan_result_callback_t)( wiced_scan_result_t** result_ptr, void* user_data );

/** Initiates a scan to search for 802.11 networks.
 *
 *  The scan progressively accumulates results over time, and may take between 1 and 10 seconds to complete.
 *  The results of the scan will be individually provided to the callback function.
 *  Note: The callback function will be executed in the context of the WICED thread and so must not perform any
 *  actions that may cause a bus transaction.
 *
 * @param[in]  scan_type     : Specifies whether the scan should be Active, Passive or scan Prohibited channels
 * @param[in]  bss_type      : Specifies whether the scan should search for Infrastructure networks (those using
 *                             an Access Point), Ad-hoc networks, or both types.
 * @param[in]  optional_ssid : If this is non-Null, then the scan will only search for networks using the specified SSID.
 * @param[in]  optional_mac  : If this is non-Null, then the scan will only search for networks where
 *                             the BSSID (MAC address of the Access Point) matches the specified MAC address.
 * @param[in] optional_channel_list    : If this is non-Null, then the scan will only search for networks on the
 *                                       specified channels - array of channel numbers to search, terminated with a zero
 * @param[in] optional_extended_params : If this is non-Null, then the scan will obey the specifications about
 *                                       dwell times and number of probes.
 * @param callback[in]   : the callback function which will receive and process the result data.
 * @param result_ptr[in] : a pointer to a pointer to a result storage structure.
 * @param user_data[in]  : user specific data that will be passed directly to the callback function
 *
 * @note : When scanning specific channels, devices with a strong signal strength on nearby channels may be detected
 * @note : Callback must not use blocking functions, nor use WICED functions, since it is called from the context of the
 *         WICED thread.
 * @note : The callback, result_ptr and user_data variables will be referenced after the function returns.
 *         Those variables must remain valid until the scan is complete.
 *
 * @return    WICED_SUCCESS or WICED_ERROR
 */
extern wiced_result_t wiced_wifi_scan( wiced_scan_type_t                    scan_type,
                                       wiced_bss_type_t                     bss_type,
                                       const wiced_ssid_t*                  optional_ssid,
                                       const wiced_mac_t*                   optional_mac,
                                       /*@unique@*/ const uint16_t*         optional_channel_list,
                                       const wiced_scan_extended_params_t*  optional_extended_params,
                                       wiced_scan_result_callback_t         callback,
                                       wiced_scan_result_t**                result_ptr,
                                       void*                                user_data );


/** Joins a Wi-Fi network
 *
 * Scans for, associates and authenticates with a Wi-Fi network.
 * On successful return, the system is ready to send data packets.
 *
 * @param[in] ssid       : A null terminated string containing the SSID name of the network to join
 * @param[in] auth_type  : Authentication type:
 *                         - WICED_SECURITY_OPEN           - Open Security
 *                         - WICED_SECURITY_WEP_PSK        - WEP Security with open authentication
 *                         - WICED_SECURITY_WEP_SHARED     - WEP Security with shared authentication
 *                         - WICED_SECURITY_WPA_TKIP_PSK   - WPA Security
 *                         - WICED_SECURITY_WPA2_AES_PSK   - WPA2 Security using AES cipher
 *                         - WICED_SECURITY_WPA2_TKIP_PSK  - WPA2 Security using TKIP cipher
 *                         - WICED_SECURITY_WPA2_MIXED_PSK - WPA2 Security using AES and/or TKIP ciphers
 * @param[in] security_key : A byte array containing either the cleartext security key for WPA/WPA2
 *                           secured networks, or a pointer to an array of wiced_wep_key_t structures
 *                           for WEP secured networks
 * @param[in] key_length  : The length of the security_key in bytes.
 * @param[in] semaphore   : A user provided semaphore that is flagged when the join is complete
 *
 * @return    WICED_SUCCESS : when the system is joined and ready to send data packets
 *            WICED_ERROR   : if an error occurred
 */
extern wiced_result_t wiced_wifi_join( const char* ssid, wiced_security_t auth_type, const uint8_t* security_key, uint8_t key_length, host_semaphore_type_t* semaphore );


/** Joins a specific Wi-Fi network
 *
 * Associates and authenticates with a specific Wi-Fi access point.
 * On successful return, the system is ready to send data packets.
 *
 * @param[in] ap           : A pointer to a wiced_scan_result_t structure containing AP details
 * @param[in] security_key : A byte array containing either the cleartext security key for WPA/WPA2
 *                           secured networks, or a pointer to an array of wiced_wep_key_t structures
 *                           for WEP secured networks
 * @param[in] key_length   : The length of the security_key in bytes.
 * @param[in] semaphore    : A user provided semaphore that is flagged when the join is complete
 *
 * @return    WICED_SUCCESS : when the system is joined and ready to send data packets
 *            WICED_ERROR   : if an error occurred
 */
extern wiced_result_t wiced_wifi_join_specific( const wiced_scan_result_t* ap, const uint8_t* security_key, uint8_t key_length, host_semaphore_type_t* semaphore );

/** Disassociates from a Wi-Fi network.
 *
 * @return    WICED_SUCCESS : On successful disassociation from the AP
 *            WICED_ERROR   : If an error occurred
 */
extern wiced_result_t wiced_wifi_leave( void );

/** Deauthenticates a STA which may or may not be associated to SoftAP.
 *
 * @param[in] mac    : Pointer to a variable containing the MAC address to which the deauthentication will be sent
 * @param[in] reason : Deauthentication reason code

 * @return    WICED_SUCCESS : On successful deauthentication of the other STA
 *            WICED_ERROR   : If an error occurred
 */
extern wiced_result_t wiced_wifi_deauth_sta( const wiced_mac_t* mac, wiced_dot11_reason_code_t reason );

/** Retrieves the current Media Access Control (MAC) address
 *  (or Ethernet hardware address) of the 802.11 device
 *
 * @param mac Pointer to a variable that the current MAC address will be written to
 * @return    WICED_SUCCESS or WICED_ERROR
 */
extern wiced_result_t wiced_wifi_get_mac_address( wiced_mac_t* mac );

/** ----------------------------------------------------------------------
 *  WARNING : This function is for internal use only!
 *  ----------------------------------------------------------------------
 *  This function sets the current Media Access Control (MAC) address of the
 *  802.11 device.  To override the MAC address in the Wi-Fi OTP or NVRAM add
 *  a global define in the application makefile as shown below. With this define
 *  in place, the MAC address stored in the DCT is used instead of the MAC in the
 *  OTP or NVRAM.
 *
 *  In <WICED-SDK>/App/my_app/my_app.mk add the following global define
 *    GLOBAL_DEFINES := MAC_ADDRESS_SET_BY_HOST
 *  Further information about MAC addresses is available in the following
 *  automatically generated file AFTER building your first application
 *  <WICED-SDK>/generated_mac_address.txt
 *
 * @param[in] mac Wi-Fi MAC address
 * @return    WICED_SUCCESS or WICED_ERROR
 */
extern wiced_result_t wiced_wifi_set_mac_address( wiced_mac_t mac );

/** Starts an infrastructure WiFi network
 *
 * @warning If a STA interface is active when this function is called, the softAP will\n
 *          start on the same channel as the STA. It will NOT use the channel provided!
 *
 * @param[in] ssid       : A null terminated string containing the SSID name of the network to join
 * @param[in] auth_type  : Authentication type: \n
 *                         - WICED_SECURITY_OPEN           - Open Security \n
 *                         - WICED_SECURITY_WPA_TKIP_PSK   - WPA Security \n
 *                         - WICED_SECURITY_WPA2_AES_PSK   - WPA2 Security using AES cipher \n
 *                         - WICED_SECURITY_WPA2_MIXED_PSK - WPA2 Security using AES and/or TKIP ciphers \n
 *                         - WEP security is NOT IMPLEMENTED. It is NOT SECURE! \n
 * @param[in] security_key : A byte array containing the cleartext security key for the network
 * @param[in] key_length   : The length of the security_key in bytes.
 * @param[in] channel      : 802.11 channel number
 *
 * @return    WICED_SUCCESS : if successfully creates an AP
 *            WICED_ERROR   : if an error occurred
 */
extern wiced_result_t wiced_wifi_start_ap( char* ssid, wiced_security_t auth_type, /*@unique@*/ uint8_t* security_key, uint8_t key_length, uint8_t channel );

/** Stops an existing infrastructure WiFi network
 *
 * @return    WICED_SUCCESS : if the AP is successfully stopped
 *            WICED_ERROR   : if an error occurred
 */
extern wiced_result_t wiced_wifi_stop_ap( void );

/** Determines if a particular interface is ready to transceive ethernet packets
 *
 * @param     Radio interface to check, options are WICED_STA_INTERFACE, WICED_AP_INTERFACE
 * @return    WICED_SUCCESS  : if the interface is ready to transceive ethernet packets
 * @return    WICED_NOTFOUND : no AP with a matching SSID was found
 * @return    WICED_NOT_AUTHENTICATED: a matching AP was found but it won't let you authenticate.
 *                                     This can occur if this device is in the block list on the AP.
 * @return    WICED_NOT_KEYED: the device has authenticated and associated but has not completed the
 *                             key exchange. This can occur if the passphrase is incorrect.
 * @return    WICED_ERROR    : if the interface is not ready to transceive ethernet packets
 */
extern wiced_result_t wiced_wifi_is_ready_to_transceive( wiced_interface_t interface );

/** Enables powersave mode without regard for throughput reduction
 *
 *  This function enables (legacy) 802.11 PS-Poll mode and should be used
 *  to achieve the lowest power consumption possible when the Wi-Fi device
 *  is primarily passively listening to the network
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_enable_powersave( void );

/* Enables powersave mode while attempting to maximise throughput
 *
 * Network traffic is typically bursty. Reception of a packet often means that another
 * packet will be received shortly afterwards (and vice versa for transmit).
 *
 * In high throughput powersave mode, rather then entering powersave mode immediately
 * after receiving or sending a packet, the WLAN chip waits for a timeout period before
 * returning to sleep.
 *
 * @return  WICED_SUCCESS : if power save mode was successfully enabled
 *          WICED_ERROR   : if power save mode was not successfully enabled
 *
 * @param[in] return_to_sleep_delay : The variable to set return to sleep delay.*
 *
 * return to sleep delay must be set to a multiple of 10. When it is zero, the return to sleep
 * delay will be equal to 2 beacon intervals( approx 204ms ).
 */
extern wiced_result_t wiced_wifi_enable_powersave_with_throughput( uint8_t return_to_sleep_delay );


/** Disables 802.11 power save mode
 *
 * @return  WICED_SUCCESS : if power save mode was successfully disabled
 *          WICED_ERROR   : if power save mode was not successfully disabled
 */
extern wiced_result_t wiced_wifi_disable_powersave( void );

/** Gets the tx power in dBm units
 *
 * @param dbm : The variable to receive the tx power in dbm.
 *
 * @return  WICED_SUCCESS : if successful
 *          WICED_ERROR   : if not successful
 */
extern wiced_result_t wiced_wifi_get_tx_power( uint8_t* dbm );

/** Sets the tx power in dBm units
 *
 * @param dbm : The desired tx power in dbm. If set to -1 (0xFF) the default value is restored.
 *
 * @return  WICED_SUCCESS : if tx power was successfully set
 *          WICED_ERROR   : if tx power was not successfully set
 */
extern wiced_result_t wiced_wifi_set_tx_power( uint8_t dbm );

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
 *  NOTE: This function applies to 802.11 powersave operation. Please read the
 *  WICED powersave application note for further information about the
 *  operation of the 802.11 listen interval.
 *
 * @param listen_interval : The desired beacon listen interval
 * @param time_unit       : The listen interval time unit; options are beacon period or DTIM period.
 *
 * @return  WICED_SUCCESS : If the listen interval was successfully set.
 *          WICED_ERROR   : If the listen interval was not successfully set.
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
 *  This function applies to 802.11 powersave operation. Please read the
 *  WICED powersave application note for further information about the
 *  operation of the 802.11 listen interval.
 *
 * @param listen_interval : The beacon listen interval sent to the AP during association.
 *                          The time unit is specified in multiples of beacon periods.
 *
 * @return  WICED_SUCCESS : if listen interval was successfully set
 *          WICED_ERROR   : if listen interval was not successfully set
 */
extern wiced_result_t wiced_wifi_set_listen_interval_assoc( uint16_t listen_interval );

/** Gets the current value of all beacon listen interval variables
 *
 * @param listen_interval_beacon : The current value of the listen interval set as a multiple of the beacon period
 * @param listen_interval_dtim   : The current value of the listen interval set as a multiple of the DTIM period
 * @param listen_interval_assoc  : The current value of the listen interval sent to access points in an association request frame
 *
 * @return  WICED_SUCCESS : If all listen interval values are read successfully
 *          WICED_ERROR   : If at least one of the listen interval values are NOT read successfully
 */
extern wiced_result_t wiced_wifi_get_listen_interval( wiced_listen_interval_t* li );

/** Registers interest in a multicast address
 * Once a multicast address has been registered, all packets detected on the
 * medium destined for that address are forwarded to the host.
 * Otherwise they are ignored.
 *
 * @param mac: Ethernet MAC address
 *
 * @return  WICED_SUCCESS : if the address was registered successfully
 *          WICED_ERROR   : if the address was not registered
 */
extern wiced_result_t wiced_wifi_register_multicast_address( wiced_mac_t* mac );

/** Unregisters interest in a multicast address
 * Once a multicast address has been unregistered, all packets detected on the
 * medium destined for that address are ignored.
 *
 * @param mac: Ethernet MAC address
 *
 * @return  WICED_SUCCESS : if the address was unregistered successfully
 *          WICED_ERROR   : if the address was not unregistered
 */
extern wiced_result_t wiced_wifi_unregister_multicast_address( wiced_mac_t* mac );

/** Retrieve the latest RSSI value
 *
 * @param rssi: The location where the RSSI value will be stored
 *
 * @return  WICED_SUCCESS : if the RSSI was succesfully retrieved
 *          WICED_ERROR   : if the RSSI was not retrieved
 */
extern wiced_result_t wiced_wifi_get_rssi( int32_t* rssi );

/** Retrieve the latest RSSI value of the AP client
 *
 * @param rssi: The location where the RSSI value will be stored
 * @param client_mac_addr: Mac address of the AP client
 *                         Please note that you can get the full list of AP clients
 *                         currently connected to Wiced AP by calling a function
 *                         wiced_wifi_get_associated_client_list
 *
 * @return  WICED_SUCCESS : if the RSSI was succesfully retrieved
 *          WICED_ERROR   : if the RSSI was not retrieved
 */
extern wiced_result_t wiced_wifi_get_ap_client_rssi( int32_t* rssi, wiced_mac_t* client_mac_addr );

/** Select the Wi-Fi antenna
 *    antenna = 0 -> select antenna 0
 *    antenna = 1 -> select antenna 1
 *    antenna = 3 -> enable auto antenna selection ie. automatic diversity
 *
 * @param antenna: The antenna configuration to use
 *
 * @return  WICED_SUCCESS : if the antenna selection was successfully set
 *          WICED_ERROR   : if the antenna selection was not set
 */
extern wiced_result_t wiced_wifi_select_antenna( wiced_antenna_t antenna );

/** Manage the addition and removal of custom IEs
 *
 * @param action       : the action to take (add or remove IE)
 * @param out          : the oui of the custom IE
 * @param subtype      : the IE sub-type
 * @param data         : a pointer to the buffer that hold the custom IE
 * @param length       : the length of the buffer pointed to by 'data'
 * @param which_packets: a mask of which packets this IE should be included in. See wiced_ie_packet_flag_t
 *
 * @return WICED_SUCCESS : if the custom IE action was successful
 *         WICED_ERROR   : if the custom IE action failed
 */
extern wiced_result_t wiced_wifi_manage_custom_ie( wiced_interface_t interface, wiced_custom_ie_action_t action, /*@unique@*/ uint8_t* oui, uint8_t subtype, void* data, uint16_t length, uint16_t which_packets );

/** Set roam trigger level
 *
 * @param trigger_level   : Trigger level in dBm. The Wi-Fi device will search for a new AP to connect to once the
 *                          signal from the AP (it is currently associated with) drops below the roam trigger level
 *
 * @return  WICED_SUCCESS : if the roam trigger was successfully set
 *          WICED_ERROR   : if the roam trigger was not successfully set
 */
extern wiced_result_t wiced_wifi_set_roam_trigger( int32_t trigger_level );

/** Send a pre-prepared action frame
 *
 * @param action_frame   : A pointer to a pre-prepared action frame structure
 *
 * @return WICED_SUCCESS or WICED_ERROR
 */
extern wiced_result_t wiced_wifi_send_action_frame(wl_action_frame_t* action_frame);

/** Retrieve the latest STA EDCF AC parameters
 *
 * @param acp: The location where the array of AC parameters will be stored
 *
 * @return  WICED_SUCCESS : if the AC Parameters were successfully retrieved
 *          WICED_ERROR   : if the AC Parameters were not retrieved
 */
extern wiced_result_t wiced_wifi_get_acparams_sta( edcf_acparam_t *acp );

/** Prioritize access category parameters as a function of min and max contention windows and backoff slots
 *
 * @param acp:       Pointer to an array of AC parameters
 * @param priority:  Pointer to a matching array of priority values
 *
 * @return
 */
extern void wiced_wifi_prioritize_acparams( const edcf_acparam_t *acp, int *priority );

/** Find the highest available access category (AC), which is equal to or less than the given AC,
 *  which does not require admission control and map it to a type of service (TOS) value.
 *
 * @param ac:        Access Category which is to be mapped to a TOS value
 * @param acp:       Pointer to an array of AC parameters
 * @return tos:      Highest available type of service that the selected AC maps to
 */
extern int wiced_wifi_get_available_tos( wiced_qos_access_category_t ac, const edcf_acparam_t *acp );

/** Print access category parameters with their priority (1-4, where 4 is highest priority)
 *
 * @param acp:       Pointer to an array of AC parameters
 * @param priority:  Pointer to a matching array of priority values
 *
 * @return
 */
extern void print_ac_params( const edcf_acparam_t *acp, int *priority );


/** Get the current channel on STA interface
 *
 * @param channel   : A pointer to the variable where the channel value will be written
 *
 * @return  WICED_SUCCESS : if the channel was successfully read
 *          WICED_ERROR   : if the channel was not successfully read
 */
extern wiced_result_t wiced_wifi_get_channel( uint32_t* channel);

/** Set the current channel on STA interface
 *
 * @param channel   : The desired channel
 *
 * @return  WICED_SUCCESS : if the channel was successfully set
 *          WICED_ERROR   : if the channel was not successfully set
 */
extern wiced_result_t wiced_wifi_set_channel( uint32_t channel);

/** Get the counters for the provided interface
 *
 * @param interface  : The interface from which the counters are requested
 *        counters   : A pointer to the structure where the counter data will be written
 *
 * @return  WICED_SUCCESS : if the counters were successfully read
 *          WICED_ERROR   : if the counters were not successfully read
 */
extern wiced_result_t wiced_wifi_get_counters(wiced_interface_t interface, wl_cnt_v6_t* counters);

/*@+exportlocal@*/
/** @} */

/* AP & STA info API */
extern wiced_result_t wiced_wifi_get_associated_client_list( void* client_list_buffer, uint16_t buffer_length );
extern wiced_result_t wiced_wifi_get_ap_info( wiced_bss_info_t* ap_info, wiced_security_t* security );

/* Monitor Mode API */
extern wiced_result_t wiced_wifi_enable_monitor_mode( void );
extern wiced_result_t wiced_wifi_disable_monitor_mode( void );

/* Duty cycle control API */
extern wiced_result_t wiced_wifi_set_ofdm_dutycycle( uint8_t duty_cycle_val );
extern wiced_result_t wiced_wifi_set_cck_dutycycle( uint8_t duty_cycle_val );
extern wiced_result_t wiced_wifi_get_ofdm_dutycycle( uint8_t* duty_cycle_val );
extern wiced_result_t wiced_wifi_get_cck_dutycycle( uint8_t* duty_cycle_val );

/* PMK retrieval API */
extern wiced_result_t wiced_wifi_get_pmk( char* psk, uint8_t psk_length, char* pmk );

/* Packet filter API */
extern wiced_result_t wiced_wifi_add_packet_filter( uint8_t filter_id, const wiced_packet_filter_settings_t* settings );
extern wiced_result_t wiced_wifi_set_packet_filter_mode( wiced_packet_filter_mode_t mode );
extern wiced_result_t wiced_wifi_remove_packet_filter( uint8_t filter_id );
extern wiced_result_t wiced_wifi_enable_packet_filter( uint8_t filter_id );
extern wiced_result_t wiced_wifi_disable_packet_filter( uint8_t filter_id );
extern wiced_result_t wiced_wifi_get_packet_filter_stats( uint8_t filter_id, wiced_packet_filter_stats_t* stats );
extern wiced_result_t wiced_wifi_clear_packet_filter_stats( uint32_t filter_id );
extern wiced_result_t wiced_wifi_get_enabled_packet_filter_list( uint32_t* count, wiced_packet_filter_t** list );
extern wiced_result_t wiced_wifi_get_disabled_packet_filter_list( uint32_t* count, wiced_packet_filter_t** list );
extern wiced_result_t wiced_wifi_delete_packet_filter_list( wiced_packet_filter_t* list );

/* These functions are not exposed to the external WICED API */
extern wiced_result_t wiced_wifi_get_packet_filter_list ( uint32_t* count, wiced_packet_filter_t** list, wiced_bool_t enabled_list );
extern wiced_result_t wiced_wifi_toggle_packet_filter   ( uint8_t filter_id, wiced_bool_t enable );

/* Network Keep Alive API */
extern wiced_result_t wiced_wifi_add_keep_alive( wiced_keep_alive_packet_t* keep_alive_packet_info );
extern wiced_result_t wiced_wifi_get_keep_alive( wiced_keep_alive_packet_t* keep_alive_packet_info );
extern wiced_result_t wiced_wifi_disable_keep_alive( uint8_t id );
/** @endcond */

/** Retrieves the WLAN firmware version
 *
 * @param[out] Pointer to a buffer that version information will be written to
 * @param[in]  Length of the buffer
 * @return     @ref wiced_result_t
 */
extern wiced_result_t wiced_wifi_get_wifi_version( char* version, uint8_t length );


wiced_result_t wwd_wifi_set_channel( wiced_interface_t interface, uint32_t channel );
wiced_result_t wwd_wifi_get_channel( wiced_interface_t interface, uint32_t* channel );

extern wiced_result_t wiced_wifi_test_credentials( wiced_scan_result_t* ap, const uint8_t* security_key, uint8_t key_length );

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ifndef INCLUDED_WWD_WIFI_H */
