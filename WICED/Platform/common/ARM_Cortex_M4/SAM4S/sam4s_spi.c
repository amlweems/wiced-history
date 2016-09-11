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
#include "sam4s_platform.h"
#include "wiced_platform.h"
#include "wwd_assert.h"

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
 *            WICED Function Definitions
 ******************************************************/

wiced_result_t sam4s_spi_init( const sam4s_spi_t* spi, const sam4s_spi_config_t* config )
{
    Pdc* spi_pdc = spi_get_pdc_base( spi->peripheral );
    sam4s_gpio_pin_config_t cs_pin_config;

    /* Setup chip select pin */
    cs_pin_config.direction = IOPORT_DIR_OUTPUT;
    cs_pin_config.mode      = 0;
    sam4s_gpio_pin_init( spi->cs_gpio_pin, &cs_pin_config );
    sam4s_spi_deassert_chip_select( spi );

    /* Setup other pins */
    sam4s_peripheral_pin_init( spi->mosi_pin, &spi->mosi_pin_config );
    sam4s_peripheral_pin_init( spi->miso_pin, &spi->miso_pin_config );
    sam4s_peripheral_pin_init( spi->clk_pin,  &spi->clk_pin_config  );

    /* Configure an SPI peripheral. */
    pmc_enable_periph_clk( spi->peripheral_id );
    spi_disable( spi->peripheral );
    spi_reset( spi->peripheral );
    spi_set_lastxfer( spi->peripheral );
    spi_set_master_mode( spi->peripheral );
    spi_disable_mode_fault_detect( spi->peripheral );
    spi_set_peripheral_chip_select_value( spi->peripheral, 0 );
    spi_set_clock_polarity( spi->peripheral, 0, config->polarity );
    spi_set_clock_phase( spi->peripheral, 0, config->phase );
    spi_set_bits_per_transfer( spi->peripheral, 0, SPI_CSR_BITS_8_BIT );
    spi_set_baudrate_div( spi->peripheral, 0, (uint8_t)( CPU_CLOCK_HZ / config->speed_hz ) );
    spi_set_transfer_delay( spi->peripheral, 0, 0, 0 );
    spi_enable( spi->peripheral );
    pdc_disable_transfer( spi_pdc, PERIPH_PTCR_RXTDIS | PERIPH_PTCR_TXTDIS );

    return WICED_SUCCESS;
}

wiced_result_t sam4s_spi_deinit( const sam4s_spi_t* spi )
{
    /* Disable the RX and TX PDC transfer requests */
    spi_disable( spi->peripheral );
    spi_reset( spi->peripheral );
    return WICED_SUCCESS;
}

wiced_result_t sam4s_spi_transfer( const sam4s_spi_t* spi, const uint8_t* data_out, uint8_t* data_in, uint32_t data_length )
{
    Pdc*         spi_pdc  = spi_get_pdc_base( spi->peripheral );
    pdc_packet_t pdc_spi_packet;

    pdc_spi_packet.ul_addr = (uint32_t)data_in;
    pdc_spi_packet.ul_size = (uint32_t)data_length;
    pdc_rx_init( spi_pdc, &pdc_spi_packet, NULL );

    pdc_spi_packet.ul_addr = (uint32_t)data_out;
    pdc_spi_packet.ul_size = (uint32_t)data_length;
    pdc_tx_init( spi_pdc, &pdc_spi_packet, NULL );

    /* Enable the RX and TX PDC transfer requests */
    pdc_enable_transfer( spi_pdc, PERIPH_PTCR_RXTEN | PERIPH_PTCR_TXTEN );

    /* Waiting transfer done*/
    while ( ( spi_read_status( spi->peripheral ) & SPI_SR_RXBUFF ) == 0 )
    {
    }

    /* Disable the RX and TX PDC transfer requests */
    pdc_disable_transfer(spi_pdc, PERIPH_PTCR_RXTDIS | PERIPH_PTCR_TXTDIS);

    return WICED_SUCCESS;
}

wiced_result_t sam4s_spi_assert_chip_select( const sam4s_spi_t* spi )
{
    return sam4s_gpio_output_low( spi->cs_gpio_pin );
}

wiced_result_t sam4s_spi_deassert_chip_select( const sam4s_spi_t* spi )
{
    return sam4s_gpio_output_high( spi->cs_gpio_pin );
}
