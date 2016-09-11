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
 *
 */

#include "wiced_utilities.h"
#include <string.h>
#include "wwd_debug.h"

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

uint32_t utoa(uint32_t value, char* output, uint8_t min_length, uint8_t max_length)
{
    uint8_t buffer[10] = "0000000000";
    int8_t digits_left = max_length;
    while (value != 0 && digits_left != 0)
    {
        --digits_left;
        buffer[digits_left] = (value % 10) + '0';
        value = value / 10;
    }

    digits_left = MIN(max_length-min_length, digits_left);
    memcpy(output, &buffer[digits_left], max_length - digits_left);

    return (max_length - digits_left);
}


/*!
 ******************************************************************************
 * Convert a nibble into a hex character
 *
 * @param[in] nibble  The value of the nibble in the lower 4 bits
 *
 * @return    The hex character corresponding to the nibble
 */
char nibble_to_hexchar( uint8_t nibble )
{
    if (nibble > 9)
    {
        return 'A' + (nibble - 10);
    }
    else
    {
        return '0' + nibble;
    }
}

/*!
 ******************************************************************************
 * Prints partial details of a scan result on a single line
 *
 * @param[in] record  A pointer to the wiced_scan_result_t record
 *
 */
void print_scan_result( wiced_scan_result_t* record )
{
    WPRINT_APP_INFO( ( "%5s ", ( record->bss_type == WICED_BSS_TYPE_ADHOC ) ? "Adhoc" : "Infra" ) );
    WPRINT_APP_INFO( ( "%02X:%02X:%02X:%02X:%02X:%02X ", record->BSSID.octet[0], record->BSSID.octet[1], record->BSSID.octet[2], record->BSSID.octet[3], record->BSSID.octet[4], record->BSSID.octet[5] ) );
    WPRINT_APP_INFO( ( " %d ", record->signal_strength ) );
    if ( record->max_data_rate < 100000 )
    {
        WPRINT_APP_INFO( ( " %.1f ", (float) record->max_data_rate / 1000.0 ) );
    }
    else
    {
        WPRINT_APP_INFO( ( "%.1f ", (float) record->max_data_rate / 1000.0 ) );
    }
    WPRINT_APP_INFO( ( " %2d  ", record->channel ) );
    WPRINT_APP_INFO( ( "%-10s ", ( record->security == WICED_SECURITY_OPEN ) ? "Open" :
                                 ( record->security == WICED_SECURITY_WEP_PSK ) ? "WEP" :
                                 ( record->security == WICED_SECURITY_WPA_TKIP_PSK ) ? "WPA TKIP" :
                                 ( record->security == WICED_SECURITY_WPA_AES_PSK ) ? "WPA AES" :
                                 ( record->security == WICED_SECURITY_WPA2_AES_PSK ) ? "WPA2 AES" :
                                 ( record->security == WICED_SECURITY_WPA2_TKIP_PSK ) ? "WPA2 TKIP" :
                                 ( record->security == WICED_SECURITY_WPA2_MIXED_PSK ) ? "WPA2 Mixed" :
                                 "Unknown" ) );
    WPRINT_APP_INFO( ( " %-32s ", record->SSID.val ) );
    WPRINT_APP_INFO( ( "\r\n" ) );
}
