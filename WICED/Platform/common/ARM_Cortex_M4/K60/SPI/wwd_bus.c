/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "MK60N512VMD100.h"
#include "platform.h"
#include "k60_spi.h"
#include "k60_gpio.h"
#include "wwd_bus_protocol.h"
#include "Platform/wwd_platform_interface.h"
#include "Platform/wwd_bus_interface.h"
#include "Platform/wwd_spi_interface.h"
#include "internal/wifi_image/wwd_wifi_image_interface.h"
#include "wifi_nvram_image.h"
#include "Network/wwd_buffer_interface.h"
#include "RTOS/wwd_rtos_interface.h"
#include "wwd_assert.h"
#include "platform_common_config.h"

/******************************************************
 *             Constants
 ******************************************************/

#define WLAN_SPI_BAUD_RATE_BPS (10000000)

/******************************************************
 *             Structures
 ******************************************************/

/******************************************************
 *             Variables
 ******************************************************/

static k60_spi_driver_t spi_driver;

/******************************************************
 *             Function declarations
 ******************************************************/

static void spi_irq_handler( void* arg );

/******************************************************
 *             Function definitions
 ******************************************************/

wiced_result_t host_platform_bus_init( void )
{
    /* Configure WLAN GPIO0 pin and default to low */
    k60_gpio_init( WL_GPIO0_BANK, WL_GPIO0_PIN, OUTPUT_PUSH_PULL );
    k60_gpio_output_low( WL_GPIO0_BANK, WL_GPIO0_PIN );

    /* Configure WLAN GPIO0 pin and default to high for SPI mode */
    k60_gpio_init( WL_GPIO1_BANK, WL_GPIO1_PIN, OUTPUT_PUSH_PULL );
    k60_gpio_output_high( WL_GPIO1_BANK, WL_GPIO1_PIN );

    /* Configure Pin muxing for SPI CLK, MISO, MOSI */
    k60_gpio_select_mux( SPI_BUS_CLOCK_BANK, SPI_BUS_CLOCK_PIN, 2 );
    k60_gpio_select_mux( SPI_BUS_MISO_BANK,  SPI_BUS_MISO_PIN,  2 );
    k60_gpio_select_mux( SPI_BUS_MOSI_BANK,  SPI_BUS_MOSI_PIN,  2 );

    /* Init SPI CS GPIO and default to high. Use GPIO to allow better control over CS line */
    k60_gpio_init( SPI_BUS_CS_BANK, SPI_BUS_CS_PIN, OUTPUT_PUSH_PULL );
    k60_gpio_output_high( SPI_BUS_CS_BANK, SPI_BUS_CS_PIN );

    /* Init SPI interrupt */
    k60_gpio_init( SPI_IRQ_BANK, SPI_IRQ_PIN, INPUT_PULL_DOWN );
    k60_gpio_irq_enable( SPI_IRQ_BANK, SPI_IRQ_PIN, IRQ_TRIGGER_RISING_EDGE, spi_irq_handler, 0 );

    /* Init SPI port. CPHA = 0 and CPOL = 0 */
    return k60_spi_init( &spi_driver, WL_SPI, WLAN_SPI_BAUD_RATE_BPS, 0, WICED_FALSE, WICED_FALSE, WICED_FALSE );
}

wiced_result_t host_platform_bus_deinit( void )
{
    /* Configure WLAN GPIO0 pin and default to low */
    k60_gpio_output_low( WL_GPIO0_BANK, WL_GPIO0_PIN );

    /* Configure WLAN GPIO0 pin and default to high for SPI mode */
    k60_gpio_output_high( WL_GPIO1_BANK, WL_GPIO1_PIN );

    /* Init SPI CS GPIO and default to high. Use GPIO to allow better control over CS line */
    k60_gpio_output_high( SPI_BUS_CS_BANK, SPI_BUS_CS_PIN );

    /* Init SPI interrupt */
    k60_gpio_irq_disable( SPI_IRQ_BANK, SPI_IRQ_PIN );

    return k60_spi_deinit( &spi_driver );
}

wiced_result_t host_platform_spi_transfer( bus_transfer_direction_t dir, uint8_t* buffer, uint16_t buffer_length )
{
    wiced_result_t retval;

    /* Assert chip select */
    k60_gpio_output_low( SPI_BUS_CS_BANK, SPI_BUS_CS_PIN );

    if ( dir == BUS_WRITE )
    {
        retval = k60_spi_write( &spi_driver, buffer, buffer_length );
    }
    else
    {
        retval = k60_spi_transfer( &spi_driver, buffer, buffer, buffer_length );
    }

    /* Deassert chip select */
    k60_gpio_output_high( SPI_BUS_CS_BANK, SPI_BUS_CS_PIN );

    return retval;
}

static void spi_irq_handler( void* arg )
{
    UNUSED_PARAMETER( arg );
    wiced_platform_notify_irq();
}
