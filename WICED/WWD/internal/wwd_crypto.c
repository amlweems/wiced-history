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
 *  Cryptographic functions
 *
 *  Provides cryptographic functions for use in applications
 */

#include "internal/SDPCM.h"
#include "wwd_wlioctl.h"
#include "internal/wwd_internal.h"
#include "string.h" /* For memcpy */
#include "Network/wwd_buffer_interface.h"
#include "wwd_crypto.h"
#include "RTOS/wwd_rtos_interface.h"

/**
 * Gets a 16 bit random number from the 802.11 device
 *
 * Allows user applications to retrieve 16 bit random numbers
 * which are generated on the Broadcom 802.11 chip using a
 * pseudo-random number generator.
 *
 * @param val : pointer to a variable which will receive the
 *              generated random number
 *
 * @return WICED_SUCCESS or WICED_ERROR
 */

wiced_result_t wiced_wifi_get_random( uint16_t* val )
{
    wiced_buffer_t buffer;
    wiced_buffer_t response;
    wiced_result_t ret;
    static uint16_t pseudo_random = 0;

    (void) wiced_get_iovar_buffer( &buffer, (uint16_t) 2, IOVAR_STR_RAND ); /* Do not need to put anything in buffer hence void cast */
    ret = wiced_send_iovar( SDPCM_GET, buffer, &response, SDPCM_STA_INTERFACE );
    if ( ret == WICED_SUCCESS )
    {
        uint8_t* data = (uint8_t*) host_buffer_get_current_piece_data_pointer( response );
        memcpy( val, data, (size_t) 2 );
        host_buffer_release( response, WICED_NETWORK_RX );
    }
    else
    {
        // Use a pseudo random number
        if (pseudo_random == 0)
        {
            pseudo_random = (uint16_t)host_rtos_get_time();
        }
        pseudo_random = (uint16_t)((pseudo_random * 32719 + 3) % 32749);
        *val = pseudo_random;
    }
    return WICED_SUCCESS;
}
