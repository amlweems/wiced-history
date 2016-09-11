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
 * WICED Over The Air 2 Background Network interface (OTA2)
 *
 *        ***  PRELIMINARY - SUBJECT TO CHANGE  ***
 *
 *  This API allows for disconnecting from current network
 *      and connecting to an alternate network for accessing
 *      the internet and downloading an OTA2 Image File
 */
#include <ctype.h>
#include "wiced.h"
#include "internal/wwd_sdpcm.h"
#include "../../WICED/internal/wiced_internal_api.h"

#include "wiced_ota2_service.h"
#include "wiced_ota2_network.h"

#include "../../utilities/mini_printf/mini_printf.h"


/******************************************************
 *                      Macros
 ******************************************************/
#define CHECK_IOCTL_BUFFER( buff )  if ( buff == NULL ) {  wiced_assert("Allocation failed\n", 0 == 1); return WWD_BUFFER_ALLOC_FAIL; }

#if defined(DEBUG)
#define OTA2_LIB_PRINTF(arg)    mini_printf arg
#else
#define OTA2_LIB_PRINTF(arg)
#endif
/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                  Enumerations
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/


/******************************************************
 *               Variables Definitions
 ******************************************************/

/****************************************************************
 *  Internal functions
 ****************************************************************/

/****************************************************************
 *  External functions
 ****************************************************************/
wiced_result_t wiced_ota2_network_down( void )
{
    /*
     * Bring down the WiFi interface.
     */
    return wiced_wifi_down();
}

wiced_result_t wiced_ota2_network_up( wiced_config_ap_entry_t* ap_info )
{
    wiced_result_t  result = WICED_SUCCESS;
    wiced_buffer_t  buffer;
    wwd_result_t    retval;
    uint32_t*       data;
    int             tries;

    /* sanity check */
    if (ap_info == NULL)
    {
        return WICED_BADARG;
    }

    OTA2_LIB_PRINTF(("wiced_ota2_network_up: start\r\n"));

    OTA2_LIB_PRINTF(("Bringing Network down to adjust settings\r\n"));
    wiced_wifi_down();

    /* Turn APSTA off */
    data = (uint32_t*)wwd_sdpcm_get_iovar_buffer(&buffer, (uint16_t)4, IOVAR_STR_APSTA);
    CHECK_IOCTL_BUFFER(data);

    *data = (uint32_t)0;
    /* This will fail on manufacturing test build since it does not have APSTA available */
    retval = wwd_sdpcm_send_iovar(SDPCM_SET, buffer, 0, WWD_STA_INTERFACE);
    if ((retval != WWD_SUCCESS) && (retval != WWD_UNSUPPORTED))
    {
        OTA2_LIB_PRINTF(("wiced_ota2_network_up: Turn off APSTA error: %d\r\n", retval));
        result = WICED_SUCCESS; /* we want to try to continue anyway */
    }

    /*
     * Make sure that AP is 0.
     */

    data = (uint32_t*)wwd_sdpcm_get_ioctl_buffer(&buffer, 4);
    CHECK_IOCTL_BUFFER(data);
    *data = (uint32_t)0;
    retval = wwd_sdpcm_send_ioctl(SDPCM_SET, (uint32_t)WLC_SET_AP, buffer, 0, WWD_STA_INTERFACE);
    if (retval != WWD_SUCCESS)
    {
        OTA2_LIB_PRINTF(("wiced_ota2_network_up: Turn off AP error: %d\r\n", retval));
        result = WICED_SUCCESS; /* we want to try to continue anyway */
    }

    /*
     * Set MPC to 0.
     */

    data = (uint32_t*)wwd_sdpcm_get_iovar_buffer(&buffer, (uint16_t)8, IOVAR_STR_MPC);
    CHECK_IOCTL_BUFFER(data);
    data[0] = (uint32_t)CHIP_STA_INTERFACE;
    data[1] = (uint32_t)0;
    retval = wwd_sdpcm_send_iovar(SDPCM_SET, buffer, 0, WWD_STA_INTERFACE);
    if (retval != WWD_SUCCESS)
    {
        OTA2_LIB_PRINTF(("wiced_ota2_network_up: Set mpc to 0 error: %d\r\n", retval));
        result = WICED_SUCCESS; /* we want to try to continue anyway */
    }

    /* Set Power Management 0 */
    result = wiced_wifi_disable_powersave();
    if (result != WICED_SUCCESS)
    {
        OTA2_LIB_PRINTF(("wiced_ota2_network_up: wiced_wifi_disable_powersave() error: %d\r\n", retval));
        result = WICED_SUCCESS; /* we want to try to continue anyway */
    }

    /*
     * Set channel
     * First peer joining the ad-hoc network ultimately dictates which channel is going to be used by all the others
     */

    if (ap_info->details.channel != 0)
    {
        data = (uint32_t*)wwd_sdpcm_get_ioctl_buffer(&buffer, 4);
        CHECK_IOCTL_BUFFER(data);
        *data = (uint32_t)ap_info->details.channel;
        retval = wwd_sdpcm_send_ioctl(SDPCM_SET, (uint32_t)WLC_SET_CHANNEL, buffer, 0, WWD_STA_INTERFACE);
        if (retval != WWD_SUCCESS)
        {
            OTA2_LIB_PRINTF(("wiced_ota2_network_up: Set CHANNEL :%d error: %d\r\n", ap_info->details.channel, retval));
            result = WICED_SUCCESS; /* we want to try to continue anyway */
        }
    }

    /*
     * Set rmc_ackreq to 0 - no RMC when downloading update
     */

    data = (uint32_t*) wwd_sdpcm_get_iovar_buffer(&buffer, (uint16_t)4, IOVAR_STR_RMC_ACKREQ);
    CHECK_IOCTL_BUFFER(data);
    *data = 0;
    retval = wwd_sdpcm_send_iovar(SDPCM_SET, buffer, 0, WWD_STA_INTERFACE);
    if (retval != WWD_SUCCESS)
    {
        OTA2_LIB_PRINTF(("wiced_ota2_network_up: Could not set rmc_ackreq to 0 retval:%d\r\n", retval));
        result = WICED_SUCCESS; /* we want to try to continue anyway */
    }

    /*
     * Now bring up the network.
     */
    retval = wiced_wifi_up();
    if (retval != WWD_SUCCESS)
    {
        OTA2_LIB_PRINTF(("wiced_ota2_network_up: wwd_wifi_set_up() %d\r\n", retval));
        result = WICED_SUCCESS; /* we want to try to continue anyway */
    }

    /* connect to OTA2 AP */
    tries = 0;
    do
    {
        wiced_rtos_delay_milliseconds(500);

        result = wiced_join_ap_specific( &ap_info->details, ap_info->security_key_length, ap_info->security_key );
        if (result != WICED_SUCCESS)
        {
            if (result == (wiced_result_t)WWD_NETWORK_NOT_FOUND)
            {
                result = WICED_SUCCESS; /* so we retry */
                OTA2_LIB_PRINTF(("wiced_ota2_network_up: wiced_join_ap_specific() failed (NOT Found)! - we will retry %d\r\n", result));
            }
            else
            {
                OTA2_LIB_PRINTF(("wiced_ota2_network_up: wiced_join_ap_specific() failed! %d\r\n", result));
            }
        }
    } while ((result != WICED_SUCCESS) && (tries++ < 4));

    tries = 0;
     do
     {
         wiced_ip_address_t ip_addr;

         wiced_rtos_delay_milliseconds(500);

         /* get our IP address */
         result = wiced_ip_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );
         if ( result != WICED_SUCCESS )
         {
             OTA2_LIB_PRINTF(("wiced_ota2_network_up: wiced_ip_up: %d tries:%d \r\n", result, tries));
         }

         wiced_ip_get_ipv4_address(WICED_STA_INTERFACE, &ip_addr);
         OTA2_LIB_PRINTF(("         IP addr: %d.%d.%d.%d\r\n",
                (int)((ip_addr.ip.v4 >> 24) & 0xFF), (int)((ip_addr.ip.v4 >> 16) & 0xFF),
                (int)((ip_addr.ip.v4 >> 8) & 0xFF),  (int)(ip_addr.ip.v4 & 0xFF)));
         if ((ip_addr.ip.v4 != 0x0000) && (ip_addr.ip.v4 != 0x0001))
         {
             result = WICED_SUCCESS;
             break;
         }

     } while ((result != WICED_SUCCESS) && (tries++ < 4));


    OTA2_LIB_PRINTF(("wiced_ota2_network_up: done: %d tries:%d\r\n", result, tries));

    return result;
}

