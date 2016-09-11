/*
 * Copyright 2015, Broadcom Corporation
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
#include "wwd_constants.h"
#include "wwd_wifi.h"
#include "internal/wwd_sdpcm.h"
#include <string.h>

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wwd_result_t wwd_wifi_read_wlan_log( char* buffer, uint32_t buffer_size )
{
    UNUSED_PARAMETER(buffer);
    UNUSED_PARAMETER(buffer_size);
    return WWD_UNSUPPORTED;
}

wwd_result_t wwd_wifi_set_custom_country_code( const wiced_country_info_t* country_code )
{
    wiced_buffer_t buffer;
    wwd_result_t   result;
    wiced_country_info_t* data;

    data = (wiced_country_info_t*) wwd_sdpcm_get_ioctl_buffer( &buffer, (uint16_t) sizeof(wiced_country_info_t) + 10 );
    memcpy( data, country_code, sizeof(wiced_country_info_t) );
    result = wwd_sdpcm_send_ioctl( SDPCM_SET, WLC_SET_CUSTOM_COUNTRY, buffer, NULL, WWD_STA_INTERFACE );

    return result;
}
