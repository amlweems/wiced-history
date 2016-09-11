/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "MK60N512VMD100.h"
#include "wiced_defaults.h"
#include "wwd_constants.h"

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

typedef struct
{
    SPI_MemMapPtr spi_peripheral;
    uint32_t      baud_rate_bps;
    uint8_t       chip_select;
    wiced_bool_t  polarity;
    wiced_bool_t  phase;
    wiced_bool_t  use_dma;

}k60_spi_driver_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t k60_spi_init     ( k60_spi_driver_t* spi_driver,
                                  SPI_MemMapPtr spi_peripheral,
                                  uint32_t baud_rate_bps,
                                  uint8_t chip_select,
                                  wiced_bool_t polarity,
                                  wiced_bool_t phase,
                                  wiced_bool_t use_dma );

wiced_result_t k60_spi_deinit   ( k60_spi_driver_t* spi_driver );

wiced_result_t k60_spi_write    ( k60_spi_driver_t* spi_driver, const uint8_t* data_out, uint32_t size );

wiced_result_t k60_spi_read     ( k60_spi_driver_t* spi_driver, uint8_t* data_in, uint32_t size );

wiced_result_t k60_spi_transfer ( k60_spi_driver_t* spi_driver, const uint8_t* data_out, uint8_t* data_in, uint32_t size );
