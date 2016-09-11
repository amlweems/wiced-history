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
 *  Provides an APSTA functionality specific to the 4319B0
 */

#include "wwd_rtos.h"
#include "wwd_events.h"
#include "internal/SDPCM.h"
#include "internal/wwd_internal.h"
#include "string.h"
#include "wwd_assert.h"
#include "wwd_management.h"
#include "Network/wwd_buffer_interface.h"
#include "internal/wwd_ap.h"

/******************************************************
 * @cond       Constants
 ******************************************************/

#define WLC_EVENT_MSG_LINK      (0x01)

typedef enum
{
    BSS_AP   = 3,
    BSS_STA  = 2,
    BSS_UP   = 1,
    BSS_DOWN = 0
} bss_arg_option_t;

/** @endcond */

#define htod32(i) ((uint32_t)(i))
#define htod16(i) ((uint16_t)(i))
#define dtoh32(i) ((uint32_t)(i))
#define dtoh16(i) ((uint16_t)(i))
#define CHECK_IOCTL_BUFFER( buff )  if ( buff == NULL ) { return WICED_ERROR; }
#define CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( buff, sema )  if ( buff == NULL ) { (void) host_rtos_deinit_semaphore( sema ); return WICED_ERROR; }

/******************************************************
 *             Local Structures
 ******************************************************/


/******************************************************
 *             Static Variables
 ******************************************************/

wiced_bool_t                   wiced_wifi_ap_is_up = WICED_FALSE;
static host_semaphore_type_t   wiced_wifi_sleep_flag;
static const wiced_event_num_t apsta_events[] = { WLC_E_IF, WLC_E_LINK, WLC_E_NONE };

/******************************************************
 *             Static Function prototypes
 ******************************************************/

static void* wiced_handle_apsta_event( wiced_event_header_t* event_header, uint8_t* event_data, /*@returned@*/ void* handler_user_data );

/******************************************************
 *             Function definitions
 ******************************************************/

static void* wiced_handle_apsta_event( wiced_event_header_t* event_header, uint8_t* event_data, /*@returned@*/ void* handler_user_data )
{
    /*@-noeffect@*/
    UNUSED_PARAMETER( event_header );
    UNUSED_PARAMETER( event_data );
    UNUSED_PARAMETER( handler_user_data );
    /*@+noeffect@*/

    if ( (wiced_interface_t) event_header->interface != WICED_AP_INTERFACE)
    {
        return handler_user_data;
    }

    if ( ( ( event_header->event_type == (wiced_event_num_t) WLC_E_LINK ) &&
           ( event_header->interface == (uint8_t) SDPCM_AP_INTERFACE ) ) ||
         ( event_header->event_type == WLC_E_IF ) )
    {
        wiced_result_t result;
        result = host_rtos_set_semaphore( &wiced_wifi_sleep_flag, WICED_FALSE );
        wiced_assert( "failed to post ap link semaphore", result == WICED_SUCCESS );
        /*@-noeffect@*/
        (void) result;  /* not used in release mode */
        /*@-noeffect@*/
    }
    return handler_user_data;
}

/** Starts an infrastructure WiFi network
 * @param ssid      : A null terminated string containing the SSID name of the network to join
 * @param auth_type  : Authentication type:
 *                    - WICED_SECURITY_OPEN - Open Security
 *                    - WICED_SECURITY_WPA_TKIP_PSK   - WPA Security
 *                    - WICED_SECURITY_WPA2_AES_PSK   - WPA2 Security using AES cipher
 *                    - WICED_SECURITY_WPA2_MIXED_PSK - WPA2 Security using AES and/or TKIP ciphers
 *                    - WEP security is currently unimplemented due to lack of security
 * @param security_key : A byte array containing the cleartext security key for the network
 * @param key_length   : The length of the security_key in bytes.
 * @param channel     : 802.11 Channel number
 *
 * @return    WICED_SUCCESS : if successfully creates an AP
 *            WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_wifi_start_ap( char* ssid, wiced_security_t auth_type, uint8_t* security_key, uint8_t key_length, uint8_t channel )
{
    wiced_bool_t wait_for_interface = WICED_FALSE;
    wiced_buffer_t response;
    wiced_buffer_t buffer;
    uint32_t* data;
    wiced_result_t retval;

    if ( ( auth_type == WICED_SECURITY_WEP_PSK ) ||
         ( ( ( auth_type == WICED_SECURITY_WPA_TKIP_PSK ) || ( auth_type == WICED_SECURITY_WPA2_AES_PSK ) || ( auth_type == WICED_SECURITY_WPA2_MIXED_PSK ) ) &&
         ( ( key_length < (uint8_t) 8 ) || ( key_length > (uint8_t) 64 ) ) ) )
    {
        return WICED_BADARG;
    }

    /* Query bss state (does it exist? if so is it UP?) */
    data = (uint32_t*) wiced_get_iovar_buffer( &buffer, (uint16_t) 4, IOVAR_STR_BSS );
    CHECK_IOCTL_BUFFER( data );
    *data = (uint32_t) SDPCM_AP_INTERFACE;
    if ( wiced_send_iovar( SDPCM_GET, buffer, &response, SDPCM_STA_INTERFACE ) != WICED_SUCCESS )
    {
        /* Note: We don't need to release the response packet since the iovar failed */
        wait_for_interface = WICED_TRUE;
    }
    else
    {
        /* Check if the BSS is already UP, if so return */
        uint32_t* data2 = (uint32_t*) host_buffer_get_current_piece_data_pointer( response );
        if ( *data2 == (uint32_t) BSS_UP )
        {
            host_buffer_release( response, WICED_NETWORK_RX );
            return WICED_SUCCESS;
        }
        else
        {
            host_buffer_release( response, WICED_NETWORK_RX );
        }
    }

    if ( host_rtos_init_semaphore( &wiced_wifi_sleep_flag ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    /* Register for interested events */
    if ( WICED_SUCCESS != ( retval = wiced_management_set_event_handler( apsta_events, wiced_handle_apsta_event, NULL ) ) )
    {

        (void) host_rtos_deinit_semaphore( &wiced_wifi_sleep_flag );
        return retval;
    }

    /*@-noeffect@*/
    (void)retval; /* This is here to not avoid causing errors when in release builds */
    /*@+noeffect@*/

    /* Set the SSID */
    data = (uint32_t*) wiced_get_iovar_buffer( &buffer, (uint16_t) 40, IOVAR_STR_BSSCFG_SSID );
    CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( data, &wiced_wifi_sleep_flag );
    data[0] = (uint32_t) SDPCM_AP_INTERFACE; /* Set the bsscfg index */
    data[1] = strlen( ssid ); /* Set the ssid length */
    memcpy( &data[2], (uint8_t*) ssid, data[1] );
    retval = wiced_send_iovar( SDPCM_SET, buffer, 0, SDPCM_STA_INTERFACE );

    wiced_assert("start_ap: Failed to set SSID\r\n", retval == WICED_SUCCESS );

    /* Check if we need to wait for interface to be created */
    if ( wait_for_interface == WICED_TRUE )
    {
        if ( ( retval = host_rtos_get_semaphore( &wiced_wifi_sleep_flag, (uint32_t) 10000, WICED_FALSE ) ) != WICED_SUCCESS)
        {
            wiced_assert("Did not receive APSTA link up event\r\n", 0 != 0 );
            return retval;
        }
    }

    /* Set the channel */
    data = (uint32_t*) wiced_get_ioctl_buffer( &buffer, (uint16_t) 4 );
    CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( data, &wiced_wifi_sleep_flag );
    *data  = channel;
    retval = wiced_send_ioctl( SDPCM_SET, WLC_SET_CHANNEL, buffer, 0, SDPCM_AP_INTERFACE );
    wiced_assert("start_ap: Failed to set channel\r\n", retval == WICED_SUCCESS );

    data = (uint32_t*) wiced_get_iovar_buffer( &buffer, (uint16_t) 8, IOVAR_STR_BSSCFG_WSEC );
    CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( data, &wiced_wifi_sleep_flag );
    data[0] = (uint32_t) SDPCM_AP_INTERFACE;
    if ((auth_type & WPS_ENABLED) != 0)
    {
        data[1] = (uint32_t) ( ( auth_type & ( ~WPS_ENABLED ) ) | SES_OW_ENABLED );
    }
    else
    {
        data[1] = (uint32_t) auth_type;
    }
    retval = wiced_send_iovar( SDPCM_SET, buffer, 0, SDPCM_STA_INTERFACE );
    wiced_assert("start_ap: Failed to set wsec\r\n", retval == WICED_SUCCESS );

    if ( auth_type != WICED_SECURITY_OPEN )
    {
        wsec_pmk_t* psk;

        /* Set the wpa auth */
        data = (uint32_t*) wiced_get_iovar_buffer( &buffer, (uint16_t) 8, IOVAR_STR_BSSCFG_WPA_AUTH );
        CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( data, &wiced_wifi_sleep_flag );
        data[0] = (uint32_t) SDPCM_AP_INTERFACE;
        data[1] = (uint32_t) (auth_type == WICED_SECURITY_WPA_TKIP_PSK) ? ( WPA_AUTH_PSK ) : ( WPA2_AUTH_PSK | WPA_AUTH_PSK );
        retval = wiced_send_iovar( SDPCM_SET, buffer, 0, SDPCM_STA_INTERFACE );
        wiced_assert("start_ap: Failed to set wpa_auth\r\n", retval == WICED_SUCCESS );

        /* Set the passphrase */
        psk = (wsec_pmk_t*) wiced_get_ioctl_buffer( &buffer, sizeof(wsec_pmk_t) );
        CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( psk, &wiced_wifi_sleep_flag );
        memcpy( psk->key, security_key, key_length );
        psk->key_len = key_length;
        psk->flags   = (uint16_t) WSEC_PASSPHRASE;
        host_rtos_delay_milliseconds( 1 ); // Delay required to allow radio firmware to be ready to receive PMK and avoid intermittent failure
        retval = wiced_send_ioctl( SDPCM_SET, WLC_SET_WSEC_PMK, buffer, 0, SDPCM_AP_INTERFACE );
        wiced_assert("start_ap: Failed to set PMK\r\n", retval == WICED_SUCCESS );
    }

    /* Set the GMode */
    data = wiced_get_ioctl_buffer( &buffer, (uint16_t) 4 );
    if ( data == NULL )
    {
        wiced_assert( "start_ap: Could not get buffer for IOCTL", 0 != 0 );
        return WICED_ERROR;
    }

    *data = (uint32_t) GMODE_AUTO;
    retval = wiced_send_ioctl( SDPCM_SET, WLC_SET_GMODE, buffer, 0, SDPCM_AP_INTERFACE );
    wiced_assert("start_ap: Failed to set GMode\r\n", ( retval == WICED_SUCCESS ) || ( retval == WICED_ASSOCIATED ) );

    /* Set DTIM period */
    data = wiced_get_ioctl_buffer( &buffer, (uint16_t) 4 );
    CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( data, &wiced_wifi_sleep_flag );
    *data = (uint32_t) WICED_DEFAULT_SOFT_AP_DTIM_PERIOD;
    retval = wiced_send_ioctl( SDPCM_SET, WLC_SET_DTIMPRD, buffer, 0, SDPCM_AP_INTERFACE );
    wiced_assert("start_ap: Failed to set DTIM\r\n", retval == WICED_SUCCESS );

#ifdef WICED_DISABLE_SSID_BROADCAST
    /* Make the AP "hidden" */
    data = (uint32_t*) wiced_get_iovar_buffer( &buffer, (uint16_t) 4, IOVAR_STR_CLOSEDNET );
    CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( data, &wiced_wifi_sleep_flag );
    data[0] = (uint32_t) 1;
    retval = wiced_send_iovar( SDPCM_SET, buffer, 0, SDPCM_AP_INTERFACE );
    wiced_assert("start_ap: Failed to hide SSID\r\n", retval == WICED_SUCCESS );
#endif

    data = (uint32_t*) wiced_get_iovar_buffer( &buffer, (uint16_t) 8, IOVAR_STR_BSS );
    CHECK_IOCTL_BUFFER_WITH_SEMAPHORE( data, &wiced_wifi_sleep_flag );
    data[0] = (uint32_t) SDPCM_AP_INTERFACE;
    data[1] = (uint32_t) BSS_UP;
    retval  = wiced_send_iovar( SDPCM_SET, buffer, 0, SDPCM_STA_INTERFACE );
    wiced_assert("start_ap: Failed to set BSS up\r\n", retval == WICED_SUCCESS );

    /* Wait until AP is brought up */
    retval = host_rtos_get_semaphore( &wiced_wifi_sleep_flag, (uint32_t) 10000, WICED_FALSE );

    wiced_wifi_ap_is_up = WICED_TRUE;
    return retval;
}



wiced_result_t wiced_wifi_stop_ap( void )
{
    uint32_t* data;
    wiced_buffer_t buffer;
    wiced_buffer_t response;
    wiced_result_t result;
    wiced_result_t result2;

    /* Query bss state (does it exist? if so is it UP?) */
    data = wiced_get_iovar_buffer( &buffer, (uint16_t) 4, IOVAR_STR_BSS );
    CHECK_IOCTL_BUFFER( data );
    *data = (uint32_t) SDPCM_AP_INTERFACE;
    result = wiced_send_iovar( SDPCM_GET, buffer, &response, SDPCM_STA_INTERFACE );
    if ( ( result != WICED_SUCCESS ) &&
         ( result != WICED_NOTFOUND ) )
    {
        return result;
    }

    if ( result == WICED_NOTFOUND )
    {
        /* AP interface does not exist - i.e. it is down */
        wiced_wifi_ap_is_up = WICED_FALSE;
        return WICED_SUCCESS;
    }
    data = (uint32_t*) host_buffer_get_current_piece_data_pointer( response );
    if ( data[0] != (uint32_t) BSS_UP )
    {
        /* AP interface indicates it is not up - i.e. it is down */
        host_buffer_release( response, WICED_NETWORK_RX );
        wiced_wifi_ap_is_up = WICED_FALSE;
        return WICED_SUCCESS;
    }

    host_buffer_release( response, WICED_NETWORK_RX );

    data = wiced_get_iovar_buffer( &buffer, (uint16_t) 8, IOVAR_STR_BSS );
    CHECK_IOCTL_BUFFER( data );
    data[0] = (uint32_t) SDPCM_AP_INTERFACE;
    data[1] = (uint32_t) BSS_DOWN;
    result = wiced_send_iovar( SDPCM_SET, buffer, 0, SDPCM_AP_INTERFACE );
    wiced_assert("stop_ap: Failed to set BSS down\r\n", result == WICED_SUCCESS );

    /* Wait until AP is brought down */
    result = host_rtos_get_semaphore( &wiced_wifi_sleep_flag, (uint32_t) 10000, WICED_FALSE );
    result2 = host_rtos_deinit_semaphore( &wiced_wifi_sleep_flag );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }
    if ( result2 != WICED_SUCCESS )
    {
        return result2;
    }

    result = wiced_management_set_event_handler( apsta_events, NULL, NULL );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    wiced_wifi_ap_is_up = WICED_FALSE;
    return WICED_SUCCESS;

}

wiced_bool_t wiced_wifi_is_packet_from_ap( uint8_t flags2 )
{
#define BDC_FLAG2_IF_MASK   0x0f
    if ((flags2 & BDC_FLAG2_IF_MASK) != 0)
    {
        return WICED_TRUE;
    }
    else
    {
        return WICED_FALSE;
    }
}
