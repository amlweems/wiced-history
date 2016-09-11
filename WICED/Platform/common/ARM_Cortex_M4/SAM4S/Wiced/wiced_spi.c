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

wiced_result_t wiced_spi_init( const wiced_spi_device_t* spi )
{
    sam4s_spi_config_t config;
    wiced_result_t     result;

    sam4s_powersave_clocks_needed( );

    if ( ( spi->mode & SPI_CLOCK_IDLE_HIGH ) != 0 )
    {
        config.polarity = 1;
    }
    else
    {
        config.polarity = 0;
    }

    if ( ( spi->mode & SPI_CLOCK_FALLING_EDGE ) != 0 )
    {
        config.phase = 1;
    }
    else
    {
        config.phase = 0;
    }

    config.speed_hz = spi->speed;

    result = sam4s_spi_init( &platform_spi[spi->port], &config );

    sam4s_powersave_clocks_not_needed( );

    return result;
}

wiced_result_t wiced_spi_transfer( const wiced_spi_device_t* spi, wiced_spi_message_segment_t* segments, uint16_t number_of_segments )
{
    wiced_result_t result = WICED_SUCCESS;
    uint32_t a;

    sam4s_powersave_clocks_needed( );

    sam4s_spi_assert_chip_select( &platform_spi[spi->port] );

    for ( a = 0; a < number_of_segments; a++ )
    {
        result = sam4s_spi_transfer( &platform_spi[spi->port], segments[a].tx_buffer, segments[a].rx_buffer, segments[a].length );

        if ( result != WICED_SUCCESS )
        {
            break;
        }
    }

    sam4s_spi_deassert_chip_select( &platform_spi[spi->port] );

    sam4s_powersave_clocks_not_needed( );

    return result;
}

wiced_result_t wiced_spi_deinit( const wiced_spi_device_t* spi )
{
    wiced_result_t result;

    sam4s_powersave_clocks_needed( );

    result = sam4s_spi_deinit( &platform_spi[spi->port] );

    sam4s_powersave_clocks_not_needed( );

    return result;
}
