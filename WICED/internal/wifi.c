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
 *
 */

#include <stdlib.h>
#include "wiced_wifi.h"
#include "wwd_debug.h"
#include "wwd_assert.h"
#include "wiced_network.h"
#include "wwd_events.h"
#include "platform_dct.h"
#include "string.h"
#include "wwd_wifi.h"
#include "wiced_wps.h"
#include "wiced_utilities.h"
#include "internal/wiced_internal_api.h"
#include "wwd_management.h"
#include "wiced_management.h"
#include "wiced_platform.h"
#include "wiced_dct.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define CMP_MAC( a, b )  (((a[0])==(b[0]))&& \
                          ((a[1])==(b[1]))&& \
                          ((a[2])==(b[2]))&& \
                          ((a[3])==(b[3]))&& \
                          ((a[4])==(b[4]))&& \
                          ((a[5])==(b[5])))

#define NULL_MAC( a )  (((a[0])==0)&& \
                        ((a[1])==0)&& \
                        ((a[2])==0)&& \
                        ((a[3])==0)&& \
                        ((a[4])==0)&& \
                        ((a[5])==0))

/******************************************************
 *                    Constants
 ******************************************************/

#define WLC_EVENT_MSG_LINK      (0x01)    /** link is up */

#define SCAN_BSSID_LIST_LENGTH   (100)
#define SCAN_LONGEST_WAIT_TIME  (3000)
#define HANDSHAKE_TIMEOUT_MS    (3000)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_scan_result_handler_t results_handler;
    void*                       user_data;
} internal_scan_handler_t;

/******************************************************
 *               Function Declarations
 ******************************************************/

static void*          wiced_link_events_handler ( wiced_event_header_t* event_header, uint8_t* event_data, void* handler_user_data );
static void           scan_result_handler       ( wiced_scan_result_t** result_ptr, void* user_data );
static void           handshake_timeout_handler ( void* arg );
static wiced_result_t handshake_error_callback  ( void* arg );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/* Link management variables */
static const wiced_event_num_t      link_events[]           = { WLC_E_LINK, WLC_E_DEAUTH_IND, WLC_E_ROAM, WLC_E_PSK_SUP, WLC_E_NONE };
static wiced_bool_t                 wiced_initialised       =   WICED_FALSE;
static wiced_bool_t                 wiced_sta_link_up       =   WICED_FALSE;
static wiced_security_t             wiced_sta_security_type =   WICED_SECURITY_UNKNOWN;
static wiced_timer_t                wiced_sta_handshake_timer;

/* Scanning variables */
static wiced_semaphore_t            scan_semaphore;
static wiced_scan_result_t*         off_channel_results;
static wiced_scan_handler_result_t* scan_result_ptr;
static wiced_mac_t*                 scan_bssid_list = NULL;
static int                          current_bssid_list_length = 0;

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t wiced_init( void )
{
    wiced_result_t result;
    wiced_mac_t    mac;

    if ( wiced_initialised == WICED_TRUE )
        return WICED_SUCCESS;

    result = wiced_platform_init( );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }
    result = wiced_rtos_init( );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }
    result = wiced_network_init( );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Initialise Wiced */
    wiced_country_code_t country;
    const platform_dct_wifi_config_t* wifi_config = wiced_dct_get_wifi_config_section( );
    if (wifi_config->device_configured == WICED_TRUE)
    {
        country = wifi_config->country_code;
    }
    else
    {
        country = WICED_COUNTRY_UNITED_STATES;
    }
    WPRINT_NETWORK_INFO( ("Starting Wiced v" WICED_VERSION "\r\n") );
    result = wiced_management_init( country, wiced_packet_pools );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_NETWORK_ERROR( ("Error %d while starting WICED!\r\n", result) );
    }

    WPRINT_NETWORK_INFO( ( "WWD " BUS " interface initialised\r\n") );

    wiced_wifi_get_mac_address( &mac );
    WPRINT_APP_INFO(("WLAN MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\r\n", mac.octet[0],mac.octet[1],mac.octet[2],mac.octet[3],mac.octet[4],mac.octet[5]));

    result = wiced_rtos_init_semaphore( &scan_semaphore );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }
    wiced_rtos_set_semaphore(&scan_semaphore); /* Semaphore starts at 1 */

    wiced_rtos_init_timer( &wiced_sta_handshake_timer, HANDSHAKE_TIMEOUT_MS, handshake_timeout_handler, 0 );

    wiced_initialised = WICED_TRUE;
    return WICED_SUCCESS;
}

wiced_result_t wiced_deinit( void )
{
    wiced_network_down( WICED_AP_INTERFACE );
    wiced_network_down( WICED_STA_INTERFACE );

    wiced_rtos_deinit_timer( &wiced_sta_handshake_timer );

    wiced_rtos_deinit_semaphore(&scan_semaphore);

    wiced_management_wifi_off( );

    wiced_network_deinit( );

    wiced_rtos_deinit();

    wiced_initialised = WICED_FALSE;
    return WICED_SUCCESS;
}

static void link_up( void )
{
    if ( wiced_sta_link_up == WICED_FALSE )
    {
        wiced_network_notify_link_up();
        wiced_rtos_send_asynchronous_event( WICED_NETWORKING_WORKER_THREAD, wiced_network_link_up_handler, 0 );
        wiced_sta_link_up = WICED_TRUE;
    }
}

static void link_down( void )
{
    if ( wiced_sta_link_up == WICED_TRUE )
    {
        /* Notify network stack that the link is down. Further processing will be done in the link down handler */
        wiced_network_notify_link_down();

        /* Force awake the networking thread. It might still be blocked on receive or send timeouts */
        wiced_rtos_thread_force_awake( &( WICED_NETWORKING_WORKER_THREAD->thread ) );

        wiced_rtos_send_asynchronous_event( WICED_NETWORKING_WORKER_THREAD, wiced_network_link_down_handler, 0 );
        wiced_sta_link_up = WICED_FALSE;
    }
}

static void link_renew( void )
{
    if ( wiced_sta_link_up == WICED_TRUE )
    {
        wiced_network_link_renew_handler();
    }
}

static void link_handshake_error( void )
{
    wiced_rtos_send_asynchronous_event( WICED_NETWORKING_WORKER_THREAD, handshake_error_callback, 0 );
}

static wiced_result_t handshake_error_callback( void* arg )
{
    int   a;
    int   retries;
    const wiced_config_ap_entry_t* ap;

    /* Explicitly leave AP and then rejoin */
    wiced_leave_ap();

    for ( retries = WICED_JOIN_RETRY_ATTEMPTS; retries != 0; --retries )
    {
        /* Try all stored APs */
        for ( a = 0; a < CONFIG_AP_LIST_SIZE; ++a )
        {
            /* Check if the entry is valid */
            ap = &wiced_dct_get_wifi_config_section( )->stored_ap_list[ a ];
            if ( ap->details.SSID.len != 0 )
            {
                wiced_result_t      join_result = WICED_ERROR;
                wiced_scan_result_t temp_scan_result;

                memcpy( &temp_scan_result, &ap->details, sizeof( ap->details ) );

                /* Try join AP with last know specific details */
                if ( !( NULL_MAC(ap->details.BSSID.octet) ) && ap->details.channel != 0 )
                {
                    join_result = wiced_wifi_join_specific( &temp_scan_result, (uint8_t*) ap->security_key, ap->security_key_length, NULL );
                }

                if ( join_result != WICED_SUCCESS )
                {
                    /* If join-specific failed, try scan and join AP */
                    join_result = wiced_wifi_join( (char*) ap->details.SSID.val, ap->details.security, (uint8_t*) ap->security_key, ap->security_key_length, NULL );
                }

                if ( join_result == WICED_SUCCESS )
                {
                    wiced_sta_security_type = ap->details.security;

                    wiced_management_set_event_handler( link_events, wiced_link_events_handler, 0 );

                    /* Extracting calculated PMK and store it in the DCT to speed up future associations */
                    if ( ap->security_key_length != WSEC_MAX_PSK_LEN )
                    {
                        wiced_config_ap_entry_t ap_temp;
                        memcpy( &ap_temp, ap, sizeof(wiced_config_ap_entry_t) );
                        if ( wiced_wifi_get_pmk( ap_temp.security_key, ap_temp.security_key_length, ap_temp.security_key ) == WICED_SUCCESS )
                        {
                            ap_temp.security_key_length = WSEC_MAX_PSK_LEN;
                            if ( bootloader_api ->write_ap_list_dct( a, &ap_temp ) != WICED_SUCCESS )
                            {
                                WPRINT_WICED_INFO( ("Failed to write ap list to DCT: \r\n") );
                            }

                        }
                    }

                    goto success;
                }
            }
        }
    }

    /* Error. failed to join successfully */
    return WICED_ERROR;


    success:
    /* Call link up when successful */
    link_up();

    return WICED_SUCCESS;
}

static void handshake_timeout_handler( void* arg )
{
    UNUSED_PARAMETER( arg );

    wiced_rtos_stop_timer( &wiced_sta_handshake_timer );

    /* This RTOS timer callback runs in interrupt context. Defer event management to WICED_NETWORKING_WORKER_THREAD */
    wiced_rtos_send_asynchronous_event( WICED_NETWORKING_WORKER_THREAD, handshake_error_callback, 0 );
}

wiced_result_t wiced_start_ap(char* ssid, wiced_security_t security, const char* key, uint8_t channel)
{
    return wiced_wifi_start_ap(ssid, security, (uint8_t*)key, strlen(key), channel);
}

wiced_result_t wiced_stop_ap( void )
{
    return wiced_wifi_stop_ap();
}

wiced_result_t wiced_enable_powersave( void )
{
    wiced_result_t result;

    /* Enable MCU powersave */
    wiced_platform_mcu_enable_powersave();

    /* Enable Wi-Fi powersave */
    result = wiced_wifi_enable_powersave();
    if (result != WICED_SUCCESS)
    {
    	goto done;
    }

    /* Suspend networking timers */
    result = wiced_network_suspend();

    done:
    	return result;
}

wiced_result_t wiced_disable_powersave( void )
{
    wiced_network_resume();
    wiced_wifi_disable_powersave();
    wiced_platform_mcu_disable_powersave();
    return WICED_SUCCESS;
}

/** Join a Wi-Fi network as a client
 *      The AP SSID/Passphrase credentials used to join the AP must be available in the DCT
 *      Use the low-level wiced_wifi_join() API to directly join a specific AP if required
 *
 * @return WICED_SUCCESS : Connection to the AP was successful
 *         WICED_ERROR   : Connection to the AP was NOT successful
 */
wiced_result_t wiced_join_ap( void )
{
    int a;
    int retries;
    const wiced_config_ap_entry_t* ap;

    for ( retries = WICED_JOIN_RETRY_ATTEMPTS; retries != 0; --retries )
    {
        /* Try all stored APs */
        for ( a = 0; a < CONFIG_AP_LIST_SIZE; ++a )
        {
            /* Check if the entry is valid */
            ap = &wiced_dct_get_wifi_config_section( )->stored_ap_list[a];
            if ( ap->details.SSID.len != 0 )
            {
                wiced_result_t      join_result = WICED_ERROR;
                wiced_scan_result_t temp_scan_result;

                WPRINT_WICED_INFO( ("Joining : %s\r\n", (char*)ap->details.SSID.val) );

                memcpy( &temp_scan_result, &ap->details, sizeof( ap->details ) );

                /* Try join AP with last know specific details */
                if ( !( NULL_MAC(ap->details.BSSID.octet) ) && ap->details.channel != 0 )
                {
                    join_result = wiced_wifi_join_specific( &temp_scan_result, (uint8_t*) ap->security_key, ap->security_key_length, NULL );
                }

                if ( join_result != WICED_SUCCESS )
                {
                    /* If join-specific failed, try scan and join AP */
                    join_result = wiced_wifi_join( (char*) ap->details.SSID.val, ap->details.security, (uint8_t*) ap->security_key, ap->security_key_length, NULL );
                }

                if ( join_result == WICED_SUCCESS )
                {
                    WPRINT_WICED_INFO( ("Successfully joined : %s\r\n", (char*)ap->details.SSID.val) );

                    wiced_sta_link_up       = WICED_TRUE;
                    wiced_sta_security_type = ap->details.security;

                    wiced_management_set_event_handler( link_events, wiced_link_events_handler, 0 );

                    /* Extract the calculated PMK and store it in the DCT to speed up future associations */
                    if ( ap->security_key_length != WSEC_MAX_PSK_LEN )
                    {
                        wiced_config_ap_entry_t ap_temp;
                        memcpy( &ap_temp, ap, sizeof(wiced_config_ap_entry_t) );
                        if ( wiced_wifi_get_pmk( ap_temp.security_key, ap_temp.security_key_length, ap_temp.security_key ) == WICED_SUCCESS )
                        {
                            ap_temp.security_key_length = WSEC_MAX_PSK_LEN;
                            if ( bootloader_api->write_ap_list_dct( a, &ap_temp ) != WICED_SUCCESS )
                            {
                                WPRINT_WICED_INFO( ("Failed to write ap list to DCT: \r\n") );
                            }

                        }
                    }

                    return WICED_SUCCESS;
                }
                else
                {
                    WPRINT_WICED_INFO( ("Failed to join: %s\r\n", (char*)ap->details.SSID.val) );
                }
            }
        }
    }

    return WICED_ERROR;
}

wiced_result_t wiced_leave_ap( void )
{
    // Deregister the link event handler and leave the current AP
    wiced_management_set_event_handler(link_events, NULL, 0);
    wiced_wifi_leave();
    wiced_sta_link_up = WICED_FALSE;
    return WICED_SUCCESS;
}

wiced_result_t wiced_join_specific_ap(wiced_scan_result_t* ap, char* key)
{
    return WICED_ERROR;
}

static void* wiced_link_events_handler( wiced_event_header_t* event_header, uint8_t* event_data, void* handler_user_data )
{
    if ( event_header->interface != (uint8_t) WICED_STA_INTERFACE )
    {
        return handler_user_data;
    }

    switch ( event_header->event_type )
    {
        case WLC_E_ROAM:
            /* when roam attempt completed successfully, we will renew the existing link */
            /* otherwise ignore all roam events */
            /* please keep in mind that if roaming was successful wlan chip wont send any link down event to the host */
            /* driver */
            if ( event_header->status == WLC_E_STATUS_SUCCESS )
            {
                link_renew( );
            }
            break;

        case WLC_E_LINK:
            if ( ( event_header->flags & WLC_EVENT_MSG_LINK ) != 0 )
            {
                switch ( wiced_sta_security_type )
                {
                    case WICED_SECURITY_OPEN:
                    case WICED_SECURITY_WPS_OPEN:
                    case WICED_SECURITY_WPS_SECURE:
                    case WICED_SECURITY_WEP_PSK:
                    case WICED_SECURITY_WEP_SHARED:
                    {
                        /* Advertise link-up immediately as no EAPOL is required */
                        link_up();
                        break;
                    }

                    case WICED_SECURITY_WPA_TKIP_PSK:
                    case WICED_SECURITY_WPA_AES_PSK:
                    case WICED_SECURITY_WPA2_AES_PSK:
                    case WICED_SECURITY_WPA2_TKIP_PSK:
                    case WICED_SECURITY_WPA2_MIXED_PSK:
                    {
                        /* Start a timer and wait for WLC_E_PSK_SUP event */
                        wiced_rtos_reload_timer( &wiced_sta_handshake_timer );
                        wiced_rtos_start_timer( &wiced_sta_handshake_timer );
                        break;
                    }

                    case WICED_SECURITY_UNKNOWN:
                    case WICED_SECURITY_FORCE_32_BIT:
                    default:
                    {
                        wiced_assert( "Bad Security type\r\n", 0 != 0 );
                        break;
                    }
                }
            }
            else
            {
                link_down( );
            }
            break;

        case WLC_E_DEAUTH_IND:
            link_down( );
            break;

        case WLC_E_PSK_SUP:
        {
            /* WLC_E_PSK_SUP is received. Stop timer, check for status and handle appropriately */
            wiced_rtos_stop_timer( &wiced_sta_handshake_timer );

            if ( event_header->status == WLC_SUP_KEYED )
            {
                /* Successful WPA key exchange. Announce link up event to application */
                link_up();
            }
            else if ( event_header->status != WLC_SUP_LAST_BASIC_STATE && event_header->status != WLC_SUP_KEYXCHANGE )
            {
                /* WPA handshake error */
                link_handshake_error();
            }
            break;
        }

        default:
            wiced_assert( "Received event which was not registered\r\n", 0 != 0 );
            break;
    }
    return handler_user_data;
}

wiced_result_t wiced_wifi_scan_networks( wiced_scan_result_handler_t results_handler, void* user_data )
{
    static internal_scan_handler_t scan_handler;
    wiced_result_t result;
    const uint16_t chlist[] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,0 };
    const wiced_scan_extended_params_t extparam = { 5, 110, 110, 50 };

    wiced_assert("Bad args", results_handler != NULL);

    /* Initialise the semaphore that will prevent simultaneous access - cannot be a mutex, since
     * we don't want to allow the same thread to start a new scan */
    result = wiced_rtos_get_semaphore( &scan_semaphore, SCAN_LONGEST_WAIT_TIME );
    if ( result != WICED_SUCCESS )
    {
        /* Return error result, but set the semaphore to work the next time */
        wiced_rtos_set_semaphore( &scan_semaphore );
        return result;
    }

    off_channel_results = NULL;
    current_bssid_list_length = 0;

    /* Prepare space to keep track of seen BSSIDs */
    if (scan_bssid_list != NULL)
    {
        free(scan_bssid_list);
    }
    scan_bssid_list = malloc(SCAN_BSSID_LIST_LENGTH * sizeof(wiced_mac_t));
    if (scan_bssid_list == NULL)
    {
        goto exit;
    }
    memset(scan_bssid_list, 0, SCAN_BSSID_LIST_LENGTH * sizeof(wiced_mac_t));

    /* Convert function pointer to object so it can be passed around */
    scan_handler.results_handler  = results_handler;
    scan_handler.user_data        = user_data;

    /* Initiate scan */
    scan_result_ptr = MALLOC_OBJECT("scan result", wiced_scan_handler_result_t);
    if (scan_result_ptr == NULL)
    {
        goto error_with_bssid_list;
    }

    scan_result_ptr->scan_complete = WICED_FALSE;
    scan_result_ptr->user_data = user_data;


    if (wiced_wifi_scan( WICED_SCAN_TYPE_ACTIVE, WICED_BSS_TYPE_ANY, NULL, NULL, chlist, &extparam, scan_result_handler, (wiced_scan_result_t**) &scan_result_ptr, &scan_handler ) != WICED_SUCCESS)
    {
        goto error_with_result_ptr;
    }

    return WICED_SUCCESS;

error_with_result_ptr:
    free(scan_result_ptr);
    scan_result_ptr = NULL;

error_with_bssid_list:
    free(scan_bssid_list);
    scan_bssid_list = NULL;
exit:
    wiced_rtos_set_semaphore(&scan_semaphore);
    return WICED_ERROR;
}

static void scan_result_handler( wiced_scan_result_t** result_ptr, void* user_data )
{
    wiced_scan_result_t*     result_iter;
    wiced_mac_t*             mac_iter     = NULL;
    internal_scan_handler_t* scan_handler = user_data;

    /* Check if scan is complete */
    if ( result_ptr == NULL )
    {
        /* Go through the list of remaining off-channel results and report them to the application */
        for ( result_iter = off_channel_results; result_iter != NULL ; result_iter = result_iter->next )
        {
            if ( wiced_rtos_send_asynchronous_event( WICED_NETWORKING_WORKER_THREAD, (event_handler_t) scan_handler->results_handler, (void*) ( result_iter ) ) != WICED_SUCCESS )
            {
                free( result_iter  );
            }
        }

        /* Indicate to the scan handler that scanning is complete */
        scan_result_ptr->scan_complete = WICED_TRUE;
        if ( wiced_rtos_send_asynchronous_event( WICED_NETWORKING_WORKER_THREAD, (event_handler_t) scan_handler->results_handler, (void*) ( scan_result_ptr ) ) != WICED_SUCCESS )
        {
            free( scan_result_ptr );
        }
        malloc_transfer_to_curr_thread(scan_bssid_list);
        free(scan_bssid_list);
        scan_bssid_list     = NULL;
        off_channel_results = NULL;
        wiced_rtos_set_semaphore(&scan_semaphore);
        return;
    }

    /* Check if we've seen this AP before, if not then mac_iter will point to the place where we should put the new result */
    for ( mac_iter = scan_bssid_list; ( mac_iter < scan_bssid_list + current_bssid_list_length + 1 ); ++mac_iter )
    {
        /* Check for matching MAC address */
        if ( CMP_MAC( mac_iter->octet, (*result_ptr)->BSSID.octet ) )
        {
            /* Ignore this result. Clean up the result and let it be reused */
            memset( *result_ptr, 0, sizeof(wiced_scan_result_t) );
            return;
        }
    }

    /* Check if the result is "on channel" */
    if ((*result_ptr)->on_channel == WICED_TRUE)
    {
        /* Check if this result obsoletes any off channel results we've received earlier */
        wiced_scan_result_t* previous_result = NULL;
        for ( result_iter = off_channel_results; result_iter != NULL ; result_iter = result_iter->next )
        {
            /* Check for matching MAC address */
            if ( CMP_MAC( result_iter->BSSID.octet, (*result_ptr)->BSSID.octet ) )
            {
                /* Delete the off channel result */
                if ( previous_result != NULL )
                {
                    previous_result->next = result_iter->next;
                }
                else
                {
                    off_channel_results = NULL;
                }
                free( result_iter );
                break;
            }
            previous_result = result_iter;
        }

        /* New BSSID - add it to the list */
        memcpy( &mac_iter->octet, ( *result_ptr )->BSSID.octet, sizeof(wiced_mac_t) );
        current_bssid_list_length++;

        /* Post event in worker thread */
        if ( wiced_rtos_send_asynchronous_event( WICED_NETWORKING_WORKER_THREAD, (event_handler_t) scan_handler->results_handler, (void*) ( scan_result_ptr ) ) != WICED_SUCCESS )
        {
            free( *result_ptr );
        }
    }
    else
    {
        /* Check if its already in the off-channel list */
        for ( result_iter = off_channel_results; result_iter != NULL ; result_iter = result_iter->next )
        {
            /* Check for matching MAC address */
            if ( CMP_MAC( result_iter->BSSID.octet, (*result_ptr)->BSSID.octet ) )
            {
                /* Ignore this result. Clean up the result and let it be reused */
                memset(*result_ptr, 0, sizeof(wiced_scan_result_t));
                return;
            }
        }

        /* Add it to the list of off-channel results */
        (*result_ptr)->next = off_channel_results;
        off_channel_results = *result_ptr;
    }

    /* Allocate new storage space for next scan result */
    scan_result_ptr = MALLOC_OBJECT("scan result", wiced_scan_handler_result_t);
    if (scan_result_ptr != NULL)
    {
        scan_result_ptr->scan_complete = WICED_FALSE;
        scan_result_ptr->user_data = scan_handler->user_data;
    }
    *result_ptr = (wiced_scan_result_t*)scan_result_ptr;
}

wiced_result_t wiced_wps_enrollee(wiced_wps_mode_t mode, const wiced_wps_device_detail_t* details, char* password, wiced_wps_credential_t* credentials, uint16_t credential_count)
{
    wiced_result_t result    = WICED_SUCCESS;
    wps_agent_t*   workspace = calloc_named("wps", 1, sizeof(wps_agent_t));

    if ( workspace == NULL )
    {
        return WICED_NOMEM;
    }

    besl_wps_init ( workspace, (besl_wps_device_detail_t*) details, WPS_ENROLLEE_AGENT, WICED_STA_INTERFACE );
    result = besl_wps_start( workspace, mode, password, (besl_wps_credential_t*) credentials, credential_count );
    if ( result == WICED_SUCCESS )
    {
        besl_wps_wait_till_complete( workspace );
        result = besl_wps_get_result( workspace );
    }

    besl_wps_deinit( workspace );
    free( workspace );

    return result;
}

wiced_result_t wiced_wps_registrar(wiced_wps_mode_t mode, const wiced_wps_device_detail_t* details, char* password, wiced_wps_credential_t* credentials, uint16_t credential_count)
{
    wiced_result_t result    = WICED_SUCCESS;
    wps_agent_t*   workspace = calloc_named("wps", 1, sizeof(wps_agent_t));

    if ( workspace == NULL )
    {
        return WICED_NOMEM;
    }

    besl_wps_init ( workspace, (besl_wps_device_detail_t*) details, WPS_REGISTRAR_AGENT, WICED_AP_INTERFACE );
    result = besl_wps_start( workspace, mode, password, (besl_wps_credential_t*) credentials, credential_count );
    if ( result == WICED_SUCCESS )
    {
        besl_wps_wait_till_complete( workspace );
        result = besl_wps_get_result( workspace );
    }

    besl_wps_deinit( workspace );
    free( workspace );

    return result;
}
