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
#include "sam4s_platform.h"
#include "wiced_platform.h"
#include "wiced_utilities.h"
#include "string.h"

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

wiced_result_t wiced_uart_init( wiced_uart_t uart, const wiced_uart_config_t* config, wiced_ring_buffer_t* optional_rx_buffer )
{
    wiced_result_t  result;
    sam_usart_opt_t settings;

    if ( config->flow_control != FLOW_CONTROL_DISABLED )
    {
        return WICED_UNSUPPORTED;
    }

    memset( &settings, 0, sizeof( settings ) );

    switch ( config->data_width )
    {
        case DATA_WIDTH_5BIT:
            settings.char_length = US_MR_CHRL_5_BIT;
            break;
        case DATA_WIDTH_6BIT:
            settings.char_length = US_MR_CHRL_6_BIT;
            break;
        case DATA_WIDTH_7BIT:
            settings.char_length = US_MR_CHRL_7_BIT;
            break;
        case DATA_WIDTH_8BIT:
            settings.char_length = US_MR_CHRL_8_BIT;
            break;
        case DATA_WIDTH_9BIT:
        default:
            return WICED_UNSUPPORTED;
    }

    switch ( config->parity )
    {
        case ODD_PARITY:
            settings.parity_type = US_MR_PAR_ODD;
            break;
        case EVEN_PARITY:
            settings.parity_type = US_MR_PAR_EVEN;
            break;
        case NO_PARITY:
            settings.parity_type = US_MR_PAR_NO;
            break;
        default:
            break;
    }

    switch ( config->stop_bits )
    {
        case STOP_BITS_1:
            settings.stop_bits = US_MR_NBSTOP_1_BIT;
            break;
        case STOP_BITS_2:
            settings.stop_bits = US_MR_NBSTOP_2_BIT;
            break;
        default:
            break;
    }

    settings.baudrate     = config->baud_rate;
    settings.channel_mode = US_MR_CHMODE_NORMAL;

    sam4s_powersave_clocks_needed( );

    result = sam4s_uart_init( &platform_uart[uart], &settings, optional_rx_buffer );

    sam4s_powersave_clocks_not_needed( );

    return result;
}

wiced_result_t wiced_uart_deinit( wiced_uart_t uart )
{
    wiced_result_t result;

    sam4s_powersave_clocks_needed( );

    result = sam4s_uart_deinit( &platform_uart[uart] );

    sam4s_powersave_clocks_not_needed( );

    return result;
}

wiced_result_t wiced_uart_transmit_bytes( wiced_uart_t uart, const void* data, uint32_t size )
{
    wiced_result_t result;

    sam4s_powersave_clocks_needed( );

    result = sam4s_uart_transmit( &platform_uart[uart], (const uint8_t*)data, size );

    sam4s_powersave_clocks_not_needed( );

    return result;
}

wiced_result_t wiced_uart_receive_bytes( wiced_uart_t uart, void* data, uint32_t size, uint32_t timeout )
{
    wiced_result_t result;

    sam4s_powersave_clocks_needed( );

    result = sam4s_uart_receive( &platform_uart[uart], (uint8_t*)data, size, timeout );

    sam4s_powersave_clocks_not_needed( );

    return result;
}
