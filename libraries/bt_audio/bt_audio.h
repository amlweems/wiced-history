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
  *  Defines the APIs for the core Bluetooth stack and audio profiles.
*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define BT_AUDIO_DEVICE_NAME "WICED AV Sink"

/******************************************************
 *                   Enumerations
 ******************************************************/
/** Bluetooth Device Address */
typedef struct
{
   uint8_t address[6];
} __attribute__((packed)) bt_bdaddr_t;

/** Bluetooth Device Name */
typedef struct {
    uint8_t name[249];
} __attribute__((packed))bt_bdname_t;

/** Bluetooth Device Visibility Modes*/
typedef enum {
    BT_SCAN_MODE_NONE,
    BT_SCAN_MODE_CONNECTABLE,
    BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE
} bt_scan_mode_t;

/** Bluetooth Device info data structure */
typedef struct
{
   bt_bdaddr_t bd_addr;
   bt_bdname_t bd_name;
   bt_scan_mode_t scan_mode;
} bt_audio_device_info_t;

/** Bluetooth Disconnection Reason */
typedef enum
{
   BT_AUDIO_DISCONNECT_REASON_NONE = 0,
   BT_AUDIO_DISCONNECT_REASON_CLOSED,
   BT_AUDIO_DISCONNECT_REASON_LINK_LOSS
} bt_audio_disconnect_reason_t;

/** Generic event data that is sent as part of the bt_audio_callback_t */
typedef struct
{
    wiced_result_t status; /* Status of the event */
} bt_audio_generic_event_data_t;

/** Connection status event data */
typedef struct
{
    /* Bluetooth address of the remote connected/disconnected phone*/
    bt_bdaddr_t bd_addr;
    /* Status of the connection/disconnection */
    wiced_result_t status;
} bt_audio_connection_event_data_t;

/** Disconnection status event data */
typedef struct
{
    /* Reason for the disconnection - Link-loss or user initiated */
    bt_audio_disconnect_reason_t reason;
} bt_audio_disconnection_event_data_t;

/* Bluetooth events triggered as part of the bt_audio_callback_t.
 * Callback shall include 'data' if any */
typedef enum
{
    /* Triggered when Bluetooth is enabled.
            Param data of bt_audio_callback_t is bt_audio_generic_event_data_t*/
    BT_AUDIO_EVENT_ENABLE,

    /* Triggered when Bluetooth is disabled.
            Param data of bt_audio_callback_t is bt_audio_generic_event_data_t.*/
    BT_AUDIO_EVENT_DISABLE,

    /* Triggered when one of the bt_property_type_t changes
            Param data of bt_audio_callback_t is bt_audio_device_info_t */
    BT_AUDIO_EVENT_DEVICE_INFO,

    /* Triggered when connected over A2DP+AVRCP to the phone.
            Param data of bt_audio_callback_t is bt_audio_connection_event_data_t*/
    BT_AUDIO_EVENT_CONNECT,

    /* Triggered when A2DP streaming is started.
            Param data of bt_audio_callback_t is NULL. */
    BT_AUDIO_EVENT_STREAM_START,

    /* Triggered when A2DP streaming is started.
             Param data of bt_audio_callback_t is NULL. */
    BT_AUDIO_EVENT_STREAM_SUSPEND,

    /* Triggered when disconnected from the phone.
            Param data of bt_audio_callback_t is bt_audio_disconnection_event_data_t*/
    BT_AUDIO_EVENT_DISCONNECT
} bt_audio_event_t;

/* Remote control commands */
typedef enum
{
   BT_AUDIO_RCC_PLAY,
   BT_AUDIO_RCC_PAUSE,
   BT_AUDIO_RCC_STOP,
   BT_AUDIO_RCC_SKIP_FORWARD,
   BT_AUDIO_RCC_SKIP_BACKWARD
} bt_audio_rcc_command_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/
/**
  * Callback type to be registered by the application.
  * Data shall be defined by the event. Upon return, the data
  * may be erased or re-used by the library.
  */
typedef void (*bt_audio_callback_t)( bt_audio_event_t event, void* data );

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/** Enable and Start Bluetooth
 *
 * This function enables Bluetooth and starts the A2DP and AVRCP profiles. Upon return,
 * the Bluetooth device is powered and discoverable or connectable, depending on whether it
 * has been paired with at least one phone; the paired phone can then connect and stream audio
 * or transfer voice calls over this device.
 *
 * @param callback A callback of type bt_audio_callback_t.
 *
 * @return        WICED_SUCCESS : on success;
 *                WICED_ERROR   : if an error occurred
 */
wiced_result_t bt_audio_init( bt_audio_callback_t callback );

/** Stop and Disable Bluetooth
 *
 * This function tears down any Bluetooth connections, and shuts down the services and Bluetooth.
 * The device shall no longer be available for Bluetooth operations.
 *
 * @return    WICED_SUCCESS : on success;
 *            WICED_ERROR   : if an error occurred
 */
 wiced_result_t bt_audio_deinit( void );


/** Retrieve local adapter device info
 *
 * This function fetches the local device information such as Bluetooth address, Bluetooth device
 * name and current scan mode.
 *
 * @return    WICED_SUCCESS : on success;
 *            WICED_ERROR   : if an error occurred
 */
wiced_result_t bt_audio_get_device_info( void );

/** Sets the Bluetooth adapter scan mode
 *
 * This function allows the application to configure the adapter's scan mode.
 * By default, upon enable, the adapter shall be in connectable mode.
 *
 * @param mode The desired scan mode defined by bt_scan_mode_t
 *
 * @return    WICED_SUCCESS : on success;
 *            WICED_ERROR   : if an error occurred
 */
wiced_result_t bt_audio_set_scan_mode( bt_scan_mode_t mode );

/** Initiate connection to the remote phone
 *
 * This function initates A2DP and AVRCP connection to the remote paired phone.
 *
 * @param bd_addr Remote Bluetooth address of the phone. If NULL, connection will be
 *                           initiated to the last connected device
 *
 * @return    WICED_SUCCESS : on success;
 *            WICED_ERROR   : if an error occurred
 */
 wiced_result_t bt_audio_connect( bt_bdaddr_t *bd_addr );

/** Disconnect from the remote phone
 *
 * This function disconnects A2DP and AVRCP from the remote connected phone.
 *
 * @return    WICED_SUCCESS : on success;
 *            WICED_ERROR   : if an error occurred
 */
wiced_result_t bt_audio_disconnect( void );

/** Stops the current opened stream(if any); and prevents any new stream requests until
 *  "allow_playback" has been called from the application.
 *
 * @return    WICED_SUCCESS : on success;
 *            WICED_ERROR   : if an error occurred
 */
wiced_result_t bt_audio_prevent_playback( void );

/** Allows the BT audio library to accept incoming stream request.
 *
 * @return WICED_SUCCESS: on Success;
 *         WICED_ERROR: if an error occured
 */
wiced_result_t bt_audio_allow_playback( void );

/** Send AVRCP remote control command
 *
 * This function allows the application to send AVRCP remote control commands to the
 * AVRCP target (phone)
 *
 * @param cmd Remote control command to send defined by bt_audio_rcc_command_t
 *
 * @return    WICED_SUCCESS : on success;
 *            WICED_ERROR   : if an error occurred
 */
wiced_result_t bt_audio_send_rcc_command( bt_audio_rcc_command_t command );

/** Increment the speaker volume by one unit
 *
 * This function increases the volume of the speaker by one unit. If the remote AVRCP target
 * supports AVRCP 1.4 Absolute Volume, then this command also synchronizes the volume
 * with the remote phone; otherwise the increase is local to the DAC.
 *
 * The volume increases within the range of 0 - 127 in increments of 8
 *
 * This command works when Bluetooth A2DP is connected or streaming
 *
 * @return    WICED_SUCCESS : on success;
 *            WICED_ERROR   : if an error occurred
 */
wiced_result_t bt_audio_volume_up( void );

/** Decrement the speaker volume by one unit
 *
 * This function decreases the volume of the speaker by one unit. If the remote AVRCP target
 * supports AVRCP 1.4 Absolute Volume, then this command also synchronizes the volume
 * with the remote phone; otherwise the decrease is local to the DAC.
 *
 * The volume decreases within the range of 0 - 127 in decrements of 8
*
 * This command works when Bluetooth A2DP is connected or streaming
 *
 * @return    WICED_SUCCESS : on success;
 *            WICED_ERROR   : if an error occurred
 */
wiced_result_t bt_audio_volume_down( void );

/** Set the speaker volume by value provided by the application
 *
 * This function sets the volume of the speaker to the value provided by the application.
 * If the remote AVRCP target supports AVRCP 1.4 Absolute Volume, then this command also
 * synchronizes the volume with the remote phone; otherwise the change in volume level is
 * local to the DAC.
 *
 * @param new volume level within the range of 0 - 127
 *
 * This command works when Bluetooth A2DP is connected or streaming
 *
 * @return    WICED_SUCCESS : on success;
 *            WICED_ERROR   : if an error occurred
 */
wiced_result_t bt_audio_set_volume( uint8_t new_volume );

/** Get the speaker volume to the application
 *
 * This function retrieves the volume of the speaker and passes it to the application.
 *
 * @param Current Speaker volume level within the range of 0 - 127
 *
 * This command works when Bluetooth A2DP is connected or streaming
 *
 * @return    WICED_SUCCESS : on success;
 *            WICED_ERROR   : if an error occurred
 */
wiced_result_t bt_audio_get_volume( uint8_t *current_volume );

/** Delete the device info (BD Address, link key, etc) of all the paired peer devices stored in NV
 * This function deletes the paired info of all devices stored in NV.
 *
 * @return    WICED_SUCCESS : on success;
 *            WICED_ERROR   : if an error occurred
 */

wiced_result_t bt_audio_delete_all_paired_device_info( void );

#ifdef __cplusplus
} /* extern "C" */
#endif
