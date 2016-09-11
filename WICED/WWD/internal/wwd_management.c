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
 *  Implements initialisation and other management functions for Wiced system
 *
 */

#include "wwd_management.h"
#include "internal/SDPCM.h"
#include "wwd_wlioctl.h"
#include "internal/wwd_internal.h"
#include "internal/wwd_thread.h"
#include <string.h>
#include "Platform/wwd_bus_interface.h"
#include "Platform/wwd_platform_interface.h"
#include "Network/wwd_network_interface.h"
#include "Network/wwd_buffer_interface.h"
#include "RTOS/wwd_rtos_interface.h"
#include "wwd_assert.h"
#include "internal/Bus_protocols/wwd_bus_protocol_interface.h"
#include "wwd_wifi.h"

#ifndef WICED_DISABLE_BOOTLOADER
#include "bootloader_app.h"
#endif

/******************************************************
 *             Constants
 ******************************************************/

#define MAX_POST_SET_COUNTRY_RETRY  3

/******************************************************
 *             Static Variables
 ******************************************************/

/******************************************************
 *             Function definitions
 ******************************************************/

/**
 * Initialises Wiced System
 *
 * - Initialises the required parts of the hardware platform
 *   i.e. pins for SDIO/SPI, interrupt, reset, power etc.
 *
 * - Initialises the Wiced thread which arbitrates access
 *   to the SDIO/SPI bus
 * - Sets the country code to set which radio channels are
 *   allowed
 * - Clear the events mask
 * - Bring the Wireless interface "Up"
 *
 * @param country : enumerated country identifier, which sets the allowed
 *                  radio channels according to country regulations
 * @param buffer_interface_arg : abstract parameter which is passed
 *                        to the buffer interface during init.
 *                        Look in @host_buffer_init for details
 *                        of the argument required for your particular buffering system
 *
 * @return WICED_SUCCESS if initialization is successful, WICED_ERROR otherwise
 */

wiced_result_t wiced_management_init( wiced_country_code_t country, void* buffer_interface_arg )
{
    if ( WICED_SUCCESS != host_buffer_init( buffer_interface_arg ) )
    {
        WPRINT_WWD_ERROR(("Could not initialize buffer interface\r\n"));
        return WICED_ERROR;
    }

    /* Remember the country code */
    wiced_wlan_status.country_code = country;

    return wiced_management_wifi_on( );
}

/**
 * Turn on the Wi-Fi device
 *
 * - Initialises the required parts of the hardware platform
 *   i.e. pins for SDIO/SPI, interrupt, reset, power etc.
 *
 * - Bring the Wireless interface "Up"
 * - Initialises the Wiced thread which arbitrates access
 *   to the SDIO/SPI bus
 *
 * @return WICED_SUCCESS if initialization is successful, WICED_ERROR otherwise
 */
wiced_result_t wiced_management_wifi_on( void )
{
    wl_country_t*  country_struct;
    uint32_t*      ptr;
    wiced_result_t retval;
    wiced_buffer_t buffer;
    uint8_t*       event_mask;
    uint32_t*      data;
    uint32_t       counter;
#ifdef WICED_DISABLE_AMPDU
    uint8_t        i;
#endif
#ifdef MAC_ADDRESS_SET_BY_HOST
    wiced_mac_t mac_address;
#endif

    if ( wiced_wlan_status.state == WLAN_UP )
    {
        return WICED_SUCCESS;
    }

    if ( WICED_SUCCESS != host_platform_init( ) )
    {
        WPRINT_WWD_ERROR(("Could not initialize platform interface\r\n"));
        return WICED_ERROR;
    }

    if ( WICED_SUCCESS != host_platform_bus_init( ) )
    {
        WPRINT_WWD_ERROR(("Could not initialize platform bus interface\r\n"));
        return WICED_ERROR;
    }

    if ( WICED_SUCCESS != wiced_bus_init( ) )
    {
        WPRINT_WWD_ERROR(("Could not initialize bus\r\n"));
        return WICED_ERROR;
    }

    if ( WICED_SUCCESS != wiced_thread_init( ) )
    {
        WPRINT_WWD_ERROR(("Could not initialize WICED thread\r\n"));
        return WICED_ERROR;
    }

    /* Enable 32K WLAN sleep clock */
    host_platform_init_wlan_powersave_clock();

#ifdef MAC_ADDRESS_SET_BY_HOST
    /* See <WICED-SDK>/generated_mac_address.txt for info about setting the MAC address  */
    host_platform_get_mac_address(&mac_address);
    wiced_wifi_set_mac_address(mac_address);
#endif

    /* Turn off SDPCM TX Glomming */
    /* Note: This is only required for later chips.
     * The 4319 has glomming off by default however the 43362 has it on by default.
     */
    data = wiced_get_iovar_buffer( &buffer, (uint16_t) 4, IOVAR_STR_TX_GLOM );
    if ( data == NULL )
    {
        wiced_assert( "Could not get buffer for IOVAR", 0 != 0 );
        return WICED_ERROR;
    }
    *data = 0;
    retval = wiced_send_iovar( SDPCM_SET, buffer, 0, SDPCM_STA_INTERFACE );
    wiced_assert("Could not turn off TX glomming\r\n", (retval == WICED_SUCCESS) || (retval == WICED_UNSUPPORTED) );

    /* Turn APSTA on */
    data = (uint32_t*) wiced_get_iovar_buffer( &buffer, (uint16_t) 4, IOVAR_STR_APSTA );
    if ( data == NULL )
    {
        wiced_assert( "Could not get buffer for IOVAR", 0 != 0 );
        return WICED_ERROR;
    }
    *data = (uint32_t) 1;
    /* This will fail on manufacturing test build since it does not have APSTA available */
    retval = wiced_send_iovar( SDPCM_SET, buffer, 0, SDPCM_STA_INTERFACE );
    if ( ( retval != WICED_SUCCESS ) &&  ( retval != WICED_UNSUPPORTED ) )  /* Manufacturing Test WLAN firmware does not have APSTA ability */
    {
        /* Could not turn on APSTA */
        WPRINT_WWD_DEBUG( ("Could not turn on APSTA\r\n") );
    }

#ifdef WICED_DISABLE_AMPDU
    /* Disable AMPDU for TX */
    for ( i = 0; i < 8; ++i )
    {
        struct ampdu_tid_control* ampdu_tid;
        ampdu_tid = (struct ampdu_tid_control*) wiced_get_iovar_buffer( &buffer, (uint16_t) sizeof(struct ampdu_tid_control), IOVAR_STR_AMPDU_TID );
        if ( ampdu_tid == NULL )
        {
            wiced_assert( "Could not get buffer for IOVAR", 0 != 0 );
            return WICED_ERROR;
        }
        ampdu_tid->tid    = i;
        ampdu_tid->enable = 0;
        retval = wiced_send_iovar( SDPCM_SET, buffer, 0, SDPCM_STA_INTERFACE );
        if ( retval != WICED_SUCCESS )
        {
            WPRINT_WWD_ERROR( ("Could not disable ampdu for tid: %u\r\n", i) );
        }
    }
#endif

    retval = wiced_wifi_set_ampdu_parameters();
    if ( retval != WICED_SUCCESS )
    {
        WPRINT_WWD_ERROR(("Could not set AMPDU parameters\r\n"));
        return retval;
    }

#ifdef WICED_STARTUP_DELAY
    host_rtos_delay_milliseconds(WICED_STARTUP_DELAY);
#endif

    /* Send set country command */
    /* This is the first time that the WLAN chip is required to respond
     * in it's normal run mode.
     * If you are porting a new system and it stalls here, it could
     * be one of the following problems:
     *   - Bus interrupt not triggering properly - the WLAN chip is unable to signal the host that there is data available.
     *   - Timing problems - if the timeouts on semaphores are not working correctly, then the
     *                       system might think that the IOCTL has timed out much faster than it should do.
     *
     */
    country_struct = (wl_country_t*) wiced_get_iovar_buffer( &buffer, (uint16_t) sizeof(wl_country_t), IOVAR_STR_COUNTRY );
    if ( country_struct == NULL )
    {
        wiced_assert( "Could not get buffer for IOCTL", 0 != 0 );
        return WICED_ERROR;
    }
    memset(country_struct, 0, sizeof(wl_country_t));

    ptr  = (uint32_t*)country_struct->ccode;
    *ptr = (uint32_t) wiced_wlan_status.country_code & 0x00ffffff;
    ptr  = (uint32_t*)country_struct->country_abbrev;
    *ptr = (uint32_t) wiced_wlan_status.country_code & 0x00ffffff;
    country_struct->rev = (int32_t) ( ( wiced_wlan_status.country_code & 0xff000000 ) >> 24 );

    retval = wiced_send_iovar( SDPCM_SET, buffer, 0, SDPCM_STA_INTERFACE );
    if ( retval != WICED_SUCCESS )
    {
        /* Could not set wifi country */
        WPRINT_WWD_ERROR(("Could not set Country code\r\n"));
        return retval;
    }

    /* NOTE: The set country command requires time to process on the wlan firmware and the following IOCTL may fail on initial attempts therefore we try a few times */

    /* Set the event mask, indicating initially we do not want any asynchronous events */
    for ( counter = 0, retval = WICED_ERROR; retval != WICED_SUCCESS && counter < (uint32_t)MAX_POST_SET_COUNTRY_RETRY; ++counter )
    {
        event_mask = (uint8_t*) wiced_get_iovar_buffer( &buffer, (uint16_t) 16, IOVAR_STR_EVENT_MSGS );
        if ( event_mask == NULL )
        {
            wiced_assert( "Could not get buffer for IOVAR", 0 != 0 );
            return WICED_ERROR;
        }
        memset( event_mask, 0, (size_t) 16 );
        retval = wiced_send_iovar( SDPCM_SET, buffer, 0, SDPCM_STA_INTERFACE );
    }
    if ( retval != WICED_SUCCESS )
    {
        /* Could not set the event mask */
        WPRINT_WWD_ERROR(("Could not set Event mask\r\n"));
        return retval;
    }

    /* Send UP command */
    data = wiced_get_ioctl_buffer( &buffer, 0 );
    if ( data == NULL )
    {
        wiced_assert( "Could not get buffer for IOCTL", 0 != 0 );
        return WICED_ERROR;
    }
    retval = wiced_send_ioctl( SDPCM_SET, (uint32_t) WLC_UP, buffer, 0, SDPCM_STA_INTERFACE );
    if ( retval != WICED_SUCCESS )
    {
        /* Could not bring wifi up */
        WPRINT_WWD_ERROR(("Error enabling 802.11\r\n")); /* May time out here if bus interrupts are not working properly */
        return retval;
    }

    /* Set the GMode */
    data = wiced_get_ioctl_buffer( &buffer, (uint16_t) 4 );
    if ( data == NULL )
    {
        wiced_assert( "Could not get buffer for IOCTL", 0 != 0 );
        return WICED_ERROR;
    }
    *data = (uint32_t) GMODE_PERFORMANCE;
    retval = wiced_send_ioctl( SDPCM_SET, WLC_SET_GMODE, buffer, 0, SDPCM_STA_INTERFACE );
    wiced_assert("Failed to set GMode\r\n", retval == WICED_SUCCESS );

    wiced_wlan_status.state = WLAN_UP;
    return WICED_SUCCESS;
}

/**
 * Turn off the Wi-Fi device
 *
 * - De-Initialises the required parts of the hardware platform
 *   i.e. pins for SDIO/SPI, interrupt, reset, power etc.
 *
 * - Bring the Wireless interface "Down"
 * - De-Initialises the Wiced thread which arbitrates access
 *   to the SDIO/SPI bus
 *
 * @return WICED_SUCCESS if deinitialization is successful, WICED_ERROR otherwise
 */
wiced_result_t wiced_management_wifi_off( void )
{
    if ( wiced_wlan_status.state == WLAN_DOWN )
    {
        return WICED_SUCCESS;
    }

    /* Send DOWN command */
    (void) wiced_wifi_set_down( WICED_STA_INTERFACE ); /* Ignore return - ensuring wifi will be brought down even if bus is completely broken */

    wiced_thread_quit( );

    if ( WICED_SUCCESS != wiced_bus_deinit( ) )
    {
        WPRINT_WWD_DEBUG(("Error de-initializing bus\r\n"));
        return WICED_ERROR;
    }

    if ( WICED_SUCCESS != host_platform_bus_deinit( ) )
    {
        WPRINT_WWD_DEBUG(("Error de-initializing platform bus interface\r\n"));
        return WICED_ERROR;
    }

    if ( WICED_SUCCESS != host_platform_deinit( ) )
    {
        WPRINT_WWD_DEBUG(("Error de-initializing platform interface\r\n"));
        return WICED_ERROR;
    }

    /* Disable 32K WLAN sleep clock */
    host_platform_deinit_wlan_powersave_clock();

    wiced_wlan_status.state = WLAN_DOWN;
    return WICED_SUCCESS;
}

void wiced_set_country(wiced_country_code_t code)
{
    wiced_wlan_status.country_code = code;
}
