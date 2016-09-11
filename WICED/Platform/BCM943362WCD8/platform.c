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
#include "wwd_assert.h"
#include "wiced_platform.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* This table maps SAM4S pins to GPIO definitions on the schematic
 * A full pin definition is provided in <WICED-SDK>/include/platforms/BCM943362WCD8/platform.h
 */
const sam4s_pin_t const platform_gpio_pin_mapping[] =
{
    [WICED_GPIO_1]  = IOPORT_CREATE_PIN( PIOA, 20 ),
    [WICED_GPIO_2]  = IOPORT_CREATE_PIN( PIOA, 17 ),
    [WICED_GPIO_3]  = IOPORT_CREATE_PIN( PIOA, 18 ),
    [WICED_GPIO_4]  = IOPORT_CREATE_PIN( PIOA, 19 ),
    [WICED_GPIO_5]  = IOPORT_CREATE_PIN( PIOA, 11 ),
    [WICED_GPIO_6]  = IOPORT_CREATE_PIN( PIOA, 14 ),
    [WICED_GPIO_7]  = IOPORT_CREATE_PIN( PIOA, 12 ),
    [WICED_GPIO_8]  = IOPORT_CREATE_PIN( PIOA, 13 ),
    [WICED_GPIO_9]  = IOPORT_CREATE_PIN( PIOA, 22 ),
    [WICED_GPIO_10] = IOPORT_CREATE_PIN( PIOA, 21 ),
    [WICED_GPIO_11] = IOPORT_CREATE_PIN( PIOA, 24 ),
    [WICED_GPIO_12] = IOPORT_CREATE_PIN( PIOA, 25 ),
    [WICED_GPIO_13] = IOPORT_CREATE_PIN( PIOA, 2  ),
    [WICED_GPIO_14] = IOPORT_CREATE_PIN( PIOA, 1  ),
    [WICED_GPIO_15] = IOPORT_CREATE_PIN( PIOA, 3  ),
    [WICED_GPIO_16] = IOPORT_CREATE_PIN( PIOA, 4  ),
    [WICED_GPIO_17] = IOPORT_CREATE_PIN( PIOA, 0  ),
};

const sam4s_wakeup_pin_config_t const platform_wakeup_pin_config[] =
{
    [WICED_GPIO_1]  = { WICED_TRUE, 10, IOPORT_SENSE_FALLING },
    [WICED_GPIO_2]  = { WICED_FALSE, 0, 0                    },
    [WICED_GPIO_3]  = { WICED_FALSE, 0, 0                    },
    [WICED_GPIO_4]  = { WICED_TRUE,  9, IOPORT_SENSE_FALLING },
    [WICED_GPIO_5]  = { WICED_TRUE,  7, IOPORT_SENSE_FALLING },
    [WICED_GPIO_6]  = { WICED_TRUE,  8, IOPORT_SENSE_FALLING },
    [WICED_GPIO_7]  = { WICED_FALSE, 0, 0                    },
    [WICED_GPIO_8]  = { WICED_FALSE, 0, 0                    },
    [WICED_GPIO_9]  = { WICED_FALSE, 0, 0                    },
    [WICED_GPIO_10] = { WICED_FALSE, 0, 0                    },
    [WICED_GPIO_11] = { WICED_FALSE, 0, 0                    },
    [WICED_GPIO_12] = { WICED_FALSE, 0, 0                    },
    [WICED_GPIO_13] = { WICED_TRUE,  2, IOPORT_SENSE_FALLING },
    [WICED_GPIO_14] = { WICED_TRUE,  1, IOPORT_SENSE_FALLING },
    [WICED_GPIO_15] = { WICED_FALSE, 0, 0                    },
    [WICED_GPIO_16] = { WICED_TRUE,  3, IOPORT_SENSE_FALLING },
    [WICED_GPIO_17] = { WICED_TRUE,  0, 0                    },
};

const sam4s_uart_t const platform_uart[] =
{
    [WICED_UART_1] =
    {
        .uart             = WICED_UART_1,
        .peripheral       = USART1,
        .peripheral_id    = ID_USART1,
        .interrupt_vector = USART1_IRQn,
        .tx_pin           = (sam4s_pin_t*)&platform_gpio_pin_mapping[WICED_GPIO_9],
        .tx_pin_config    = ( IOPORT_MODE_MUX_A | IOPORT_MODE_PULLUP ),
        .rx_pin           = (sam4s_pin_t*)&platform_gpio_pin_mapping[WICED_GPIO_10],
        .rx_pin_config    = ( IOPORT_MODE_MUX_A | IOPORT_MODE_PULLUP ),
        .cts_pin          = NULL, /* flow control isn't supported */
        .cts_pin_config   = 0,    /* flow control isn't supported */
        .rts_pin          = NULL, /* flow control isn't supported */
        .rts_pin_config   = 0,    /* flow control isn't supported */
    }
};

const sam4s_spi_t const platform_spi[] =
{
    [WICED_SPI_1] =
    {
        .peripheral       = SPI,
        .peripheral_id    = ID_SPI,
        .clk_pin          = (sam4s_pin_t*)&platform_gpio_pin_mapping[WICED_GPIO_6],
        .clk_pin_config   = ( IOPORT_MODE_MUX_A | IOPORT_MODE_PULLUP ),
        .mosi_pin         = (sam4s_pin_t*)&platform_gpio_pin_mapping[WICED_GPIO_8],
        .mosi_pin_config  = ( IOPORT_MODE_MUX_A | IOPORT_MODE_PULLUP ),
        .miso_pin         = (sam4s_pin_t*)&platform_gpio_pin_mapping[WICED_GPIO_7],
        .miso_pin_config  = ( IOPORT_MODE_MUX_A | IOPORT_MODE_PULLUP ),
        .cs_gpio_pin      = (sam4s_pin_t*)&platform_gpio_pin_mapping[WICED_GPIO_5],
    }
};

const sam4s_adc_t const platform_adc[] =
{
    [WICED_ADC_1] =
    {
        .peripheral    = ADC,
        .peripheral_id = ID_ADC,
        .adc_pin       = (sam4s_pin_t*)&platform_gpio_pin_mapping[WICED_GPIO_2],
        .adc_clock_hz  = 64000000,
        .channel       = ADC_CHANNEL_0,
        .settling_time = ADC_SETTLING_TIME_3,
        .resolution    = ADC_MR_LOWRES_BITS_12,
        .trigger       = ADC_TRIG_SW,
    },
    [WICED_ADC_2] =
    {
        .peripheral    = ADC,
        .peripheral_id = ID_ADC,
        .adc_pin       = (sam4s_pin_t*)&platform_gpio_pin_mapping[WICED_GPIO_3],
        .adc_clock_hz  = 64000000,
        .channel       = ADC_CHANNEL_1,
        .settling_time = ADC_SETTLING_TIME_3,
        .resolution    = ADC_MR_LOWRES_BITS_12,
        .trigger       = ADC_TRIG_SW,
    },
    [WICED_ADC_3] =
    {
        .peripheral    = ADC,
        .peripheral_id = ID_ADC,
        .adc_pin       = (sam4s_pin_t*)&platform_gpio_pin_mapping[WICED_GPIO_4],
        .adc_clock_hz  = 64000000,
        .channel       = ADC_CHANNEL_2,
        .settling_time = ADC_SETTLING_TIME_3,
        .resolution    = ADC_MR_LOWRES_BITS_12,
        .trigger       = ADC_TRIG_SW,
    },
};

wiced_spi_device_t wiced_spi_flash =
{
    .port        = WICED_SPI_1,
    .chip_select = WICED_SPI_FLASH_CS,
    .speed       = 1000000,
    .mode        = (SPI_CLOCK_RISING_EDGE | SPI_CLOCK_IDLE_HIGH | SPI_NO_DMA | SPI_MSB_FIRST),
    .bits        = 8
};

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

wiced_result_t wiced_platform_init( void )
{
    WPRINT_PLATFORM_INFO( ("\r\nPlatform " PLATFORM " initialised\r\n") );

    if ( WICED_TRUE == sam4s_watchdog_check_last_reset() )
    {
        WPRINT_PLATFORM_ERROR(("WARNING: Watchdog reset occured previously. Please see watchdog.c for debugging instructions.\r\n"));
    }

    return WICED_SUCCESS;
}

void init_platform( void )
{
    /* Initialise LEDs and turn off by default */
    wiced_gpio_init( WICED_LED1, OUTPUT_PUSH_PULL );
    wiced_gpio_init( WICED_LED2, OUTPUT_PUSH_PULL );
    wiced_gpio_output_low( WICED_LED1 );
    wiced_gpio_output_low( WICED_LED2 );

    /* Initialise buttons to input by default */
    wiced_gpio_init( WICED_BUTTON1, INPUT_PULL_UP );
    wiced_gpio_init( WICED_BUTTON2, INPUT_PULL_UP );
}

wiced_result_t platform_enter_powersave( void )
{
	return WICED_SUCCESS;
}

wiced_result_t platform_exit_powersave( void )
{
	return WICED_SUCCESS;
}
