/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "k60_spi.h"
#include "RTOS/wwd_rtos_interface.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define CTAR_REG_USED       (0)
#define DOUBLE_BAUD_RATE    (0)
#define BAUD_RATE_PRESCALER (2)
#define CTAR_PBR            (0)
#define MAX_LOOP_COUNT  (10000)

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

static void    set_spi_peripheral_clock( SPI_MemMapPtr spi_peripheral, wiced_bool_t enable );
static void    clear_spi_fifos( SPI_MemMapPtr spi_peripheral );
static uint8_t get_baud_rate_scaler_register_value( uint32_t baud_rate_Mbps );

/******************************************************
 *               Variables Definitions
 ******************************************************/

extern int periph_clk_khz;

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t k60_spi_init( k60_spi_driver_t* spi_driver, SPI_MemMapPtr spi_peripheral, uint32_t baud_rate_bps, uint8_t chip_select, wiced_bool_t polarity, wiced_bool_t phase, wiced_bool_t use_dma )
{
    uint8_t br = get_baud_rate_scaler_register_value( baud_rate_bps );

    if ( use_dma == WICED_TRUE )
    {
        /* DMA is unsupported at the moment. TODO: implement SPI DMA transfer */
        return WICED_UNSUPPORTED;
    }

    spi_driver->spi_peripheral = spi_peripheral;
    spi_driver->baud_rate_bps  = baud_rate_bps;
    spi_driver->chip_select    = chip_select;
    spi_driver->polarity       = polarity;
    spi_driver->phase          = phase;
    spi_driver->use_dma        = use_dma;

    /* Enable SPI peripheral clock */
    set_spi_peripheral_clock( spi_peripheral, WICED_TRUE );

    /* Enable SPI peripheral and clean up (stop) any previous transfer
     * MDIS     = 0 to enable
     * HALT     = 1 to stop transfer
     * MSTR     = 1 for master mode
     * DCONF    = 0 for SPI
     * PCSIS[x] = 1 for CS active low
     */
    SPI_MCR_REG( spi_peripheral ) &= ~(uint32_t) ( SPI_MCR_MDIS_MASK | SPI_MCR_DCONF(0) );
    SPI_MCR_REG( spi_peripheral ) |=  (uint32_t) ( SPI_MCR_HALT_MASK | SPI_MCR_MSTR_MASK | SPI_MCR_PCSIS( 1 << chip_select ) );

    /* Select Clock and Transfer Attributes Register (CTAR). Always use CTAR0 */
    SPI_PUSHR_REG( spi_peripheral ) &= ~(uint32_t) SPI_PUSHR_CTAS(CTAR_REG_USED);

    /* Reset Clock and Transfer Attributes (CTAR) register */
    SPI_CTAR_REG( spi_peripheral, CTAR_REG_USED ) = 0;

    /* Set SPI configuration
     * FMSZ   = 7. Set frame size to 8-bit. frame size = FMSZ + 1
     * CPOL   = phase
     * CPHA   = polarity
     * DBR    = 00
     * PBR    = 2
     * BR     = calculate based on baud_rate_Mbps
     * PCSSCK = 0
     * PASC   = 0
     * PDT    = 0
     * CSSCK  = BR - 1
     * ASC    = BR - 1
     * DT     = 0
     */
    SPI_CTAR_REG( spi_peripheral, CTAR_REG_USED ) |= (uint32_t) ( SPI_CTAR_CPOL_MASK & (uint32_t)( polarity << SPI_CTAR_CPOL_SHIFT ) ) |
                                                     (uint32_t) ( SPI_CTAR_CPHA_MASK & (uint32_t)( phase    << SPI_CTAR_CPHA_SHIFT ) ) |
                                                     (uint32_t) ( SPI_CTAR_FMSZ( 8 - 1 ) ) |
                                                     (uint32_t) ( SPI_CTAR_DBR_MASK & ( DOUBLE_BAUD_RATE << SPI_CTAR_DBR_SHIFT ) ) |
                                                     (uint32_t) ( SPI_CTAR_PBR( CTAR_PBR ) ) |
                                                     (uint32_t) ( SPI_CTAR_BR( br ) ) |
                                                     (uint32_t) ( SPI_CTAR_CSSCK( br - 1 ) ) |
                                                     (uint32_t) ( SPI_CTAR_ASC( br - 1 ) );

    /* Enable the start transfer bit */
    SPI_MCR_REG( spi_peripheral ) &= ~(uint32_t) ( SPI_MCR_HALT_MASK );

    clear_spi_fifos( spi_peripheral );

    return WICED_SUCCESS;
}

wiced_result_t k60_spi_deinit( k60_spi_driver_t* spi_driver )
{
    clear_spi_fifos( spi_driver->spi_peripheral );

    /* Halt transfer */
    SPI_MCR_REG( spi_driver->spi_peripheral ) |=  (uint32_t) ( SPI_MCR_HALT_MASK );

    /* Disable module */
    SPI_MCR_REG( spi_driver->spi_peripheral ) &= ~(uint32_t) ( SPI_MCR_MDIS_MASK );

    /* Disable SPI peripheral clock */
    set_spi_peripheral_clock( spi_driver->spi_peripheral, WICED_FALSE );
    return WICED_SUCCESS;
}

wiced_result_t k60_spi_write( k60_spi_driver_t* spi_driver, const uint8_t* data_out, uint32_t size )
{
    uint32_t loop_count = MAX_LOOP_COUNT;

    clear_spi_fifos( spi_driver->spi_peripheral );

    while ( size > 0 )
    {
        /* Push frame to TX FIFO */
        SPI_PUSHR_REG( spi_driver->spi_peripheral ) = (uint32_t) ( *data_out++ | SPI_PUSHR_PCS( 1 << spi_driver->chip_select ) | ( ( size != 1 ) ? SPI_PUSHR_CONT_MASK : 0 ) );

        /* Wait until RX FIFO is not empty */
        while ( ( SPI_SR_RXCTR_MASK & SPI_SR_REG( spi_driver->spi_peripheral ) ) == 0 && loop_count > 0 )
        {
            loop_count--;
        }

        if ( loop_count == 0 )
        {
            return WICED_TIMEOUT;
        }

        /* Pop frame from RX FIFO */
        (void)SPI_POPR_REG( spi_driver->spi_peripheral );

        size--;
    }

    return WICED_SUCCESS;
}

wiced_result_t k60_spi_read( k60_spi_driver_t* spi_driver, uint8_t* data_in, uint32_t size )
{
    uint32_t loop_count = MAX_LOOP_COUNT;

    clear_spi_fifos( spi_driver->spi_peripheral );

    while ( size > 0 )
    {
        /* Push frame to TX FIFO */
        SPI_PUSHR_REG( spi_driver->spi_peripheral ) = (uint32_t) ( SPI_PUSHR_PCS( 1 << spi_driver->chip_select ) | ( ( size != 1 ) ? SPI_PUSHR_CONT_MASK : 0 ) );

        /* Wait until RX FIFO is not empty */
        while ( ( SPI_SR_RXCTR_MASK & SPI_SR_REG( spi_driver->spi_peripheral ) ) == 0 && loop_count > 0 )
        {
            loop_count--;
        }

        if ( loop_count == 0 )
        {
            return WICED_TIMEOUT;
        }

        /* Pop frame from RX FIFO */
        *data_in++ = (uint8_t) SPI_POPR_REG( spi_driver->spi_peripheral );

        size--;
    }

    return WICED_SUCCESS;
}

wiced_result_t k60_spi_transfer( k60_spi_driver_t* spi_driver, const uint8_t* data_out, uint8_t* data_in, uint32_t size )
{
    uint32_t loop_count = MAX_LOOP_COUNT;

    clear_spi_fifos( spi_driver->spi_peripheral );

    while ( size > 0 )
    {
        /* Push frame to TX FIFO */
        SPI_PUSHR_REG( spi_driver->spi_peripheral ) = (uint32_t) ( *data_out++ | SPI_PUSHR_PCS( 1 << spi_driver->chip_select ) | ( ( size != 1 ) ? SPI_PUSHR_CONT_MASK : 0 ) );

        /* Wait until RX FIFO is not empty */
        while ( ( SPI_SR_RXCTR_MASK & SPI_SR_REG( spi_driver->spi_peripheral ) ) == 0 && loop_count > 0 )
        {
            loop_count--;
        }

        if ( loop_count == 0 )
        {
            return WICED_TIMEOUT;
        }

        /* Pop frame from RX FIFO */
        *data_in++ = (uint8_t)SPI_POPR_REG( spi_driver->spi_peripheral );

        size--;
    }

    return WICED_SUCCESS;
}

static void set_spi_peripheral_clock( SPI_MemMapPtr spi_peripheral, wiced_bool_t enable )
{
    switch ( (uint32_t) spi_peripheral )
    {
        case SPI0_BASE_VAL :
            SIM_SCGC6 = ( enable == WICED_TRUE ) ? ( SIM_SCGC6 | (uint32_t) ( 1 << 12 ) ) : ( SIM_SCGC6 & ~(uint32_t) ( 1 << 12 ) );
            return;

        case SPI1_BASE_VAL :
            SIM_SCGC6 = ( enable == WICED_TRUE ) ? ( SIM_SCGC6 | (uint32_t) ( 1 << 13 ) ) : ( SIM_SCGC6 & ~(uint32_t) ( 1 << 13 ) );
            return;

        case SPI2_BASE_VAL :
            SIM_SCGC3 = ( enable == WICED_TRUE ) ? ( SIM_SCGC6 | (uint32_t) ( 1 << 12 ) ) : ( SIM_SCGC6 & ~(uint32_t) ( 1 << 12 ) );
            return;

        default:
            return;
    }
}

static uint8_t get_baud_rate_scaler_register_value( uint32_t baud_rate_bps )
{
    uint8_t baud_rate_scaler_reg_val = 0;
    uint32_t baud_rate_scaler         = 2;
    uint32_t current_baud_rate_bps    = PERIPHERAL_CLOCK_HZ * ( 1 + DOUBLE_BAUD_RATE ) / ( BAUD_RATE_PRESCALER * baud_rate_scaler );

    /* Find closest scaler value */
    while ( current_baud_rate_bps >= baud_rate_bps )
    {
        baud_rate_scaler        = ( baud_rate_scaler < 8 ) ? ( baud_rate_scaler + 2 ) : ( baud_rate_scaler * 2 );
        current_baud_rate_bps   = PERIPHERAL_CLOCK_HZ * ( 1 + DOUBLE_BAUD_RATE ) / ( BAUD_RATE_PRESCALER * baud_rate_scaler );
        baud_rate_scaler_reg_val++;
    }

    return baud_rate_scaler_reg_val;
}

static void clear_spi_fifos( SPI_MemMapPtr spi_peripheral  )
{
    SPI_MCR_REG( spi_peripheral ) |= (uint32_t)( SPI_MCR_CLR_RXF_MASK | SPI_MCR_CLR_TXF_MASK );
    SPI_SR_REG ( spi_peripheral ) |= (uint32_t)( SPI_SR_RFOF_MASK );
}
