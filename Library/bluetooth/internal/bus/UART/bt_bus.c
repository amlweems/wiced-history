/**
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
#include "wiced.h"
#include "wiced_rtos.h"
#include "wiced_utilities.h"
#include "bt_bus.h"
#include "wiced_bt_platform.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* Should be overridden by application. If undefined, set to 512 bytes. */
#ifndef BT_BUS_RX_FIFO_SIZE
#define BT_BUS_RX_FIFO_SIZE (512)
#endif

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

static volatile wiced_bool_t bus_initialised = WICED_FALSE;
static volatile wiced_bool_t device_powered  = WICED_FALSE;

/* RX ring buffer. Bluetooth chip UART receive can be asynchronous, therefore a ring buffer is required */
static volatile wiced_ring_buffer_t rx_ring_buffer;
static volatile uint8_t             rx_data[BT_BUS_RX_FIFO_SIZE];

/* Default UART configuration. */
static const wiced_uart_config_t uart_config =
{
    .baud_rate    = BLUETOOTH_BAUD_RATE,
    .data_width   = BLUETOOTH_DATA_WIDTH,
    .parity       = BLUETOOTH_PARITY_BIT,
    .flow_control = BLUETOOTH_FLOW_CONTROL,
    .stop_bits    = BLUETOOTH_STOP_BITS
};

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t bt_bus_init( void )
{
    if ( bus_initialised == WICED_FALSE )
    {
        wiced_result_t result;

        /* Configure Reg Enable pin to output. Set to HIGH */
        wiced_gpio_init( BLUETOOTH_GPIO_REG_EN_PIN, OUTPUT_OPEN_DRAIN_PULL_UP );
        wiced_gpio_output_high( BLUETOOTH_GPIO_REG_EN_PIN );
        device_powered = WICED_TRUE;

        /* Configure Reset pin to output. Set to HIGH */
        wiced_gpio_init( BLUETOOTH_GPIO_RESET_PIN, OUTPUT_OPEN_DRAIN_PULL_UP );
        wiced_gpio_output_high( BLUETOOTH_GPIO_RESET_PIN );

        /* Configure RTS pin to output. Set to HIGH */
        wiced_gpio_init( BLUETOOTH_GPIO_RTS_PIN, OUTPUT_OPEN_DRAIN_PULL_UP );
        wiced_gpio_output_high( BLUETOOTH_GPIO_RTS_PIN );

        /* Configure CTS pin to input pull-up */
        wiced_gpio_init( BLUETOOTH_GPIO_CTS_PIN, INPUT_PULL_UP );

        /* Initialise RX ring buffer */
        ring_buffer_init( (wiced_ring_buffer_t*) &rx_ring_buffer, (uint8_t*) rx_data, sizeof( rx_data ) );

        /* Configure USART comms */
        result = wiced_uart_init( BLUETOOTH_UART, &uart_config, (wiced_ring_buffer_t*) &rx_ring_buffer );

        if ( result != WICED_SUCCESS )
        {
            wiced_assert("Error initialising Bluetooth UART bus\r\n", result == WICED_SUCCESS );
            return WICED_ERROR;
        }

        /* Reset bluetooth chip. Delay momentarily. */
        wiced_gpio_output_low( BLUETOOTH_GPIO_RESET_PIN );
        wiced_rtos_delay_milliseconds( 10 );
        wiced_gpio_output_high( BLUETOOTH_GPIO_RESET_PIN );

        /* Wait until the Bluetooth chip stabilises.  */
        wiced_rtos_delay_milliseconds( 500 );

        /* Bluetooth chip is ready. Pull host's RTS low */
        wiced_gpio_output_low( BLUETOOTH_GPIO_RTS_PIN );

        bus_initialised = WICED_TRUE;

        /* Wait for bluetooth chip to pull its RTS (host's CTS) low. From observation using CRO, it takes the bluetooth chip > 170ms to pull its RTS low after CTS low */
        while ( bt_bus_is_ready( ) == WICED_FALSE )
        {
            wiced_rtos_delay_milliseconds( 10 );
        }
    }

    return WICED_SUCCESS;
}

wiced_result_t bt_bus_deinit( void )
{
    if ( bus_initialised == WICED_TRUE )
    {
        wiced_result_t result;
        wiced_gpio_output_low ( BLUETOOTH_GPIO_RESET_PIN );  // Reset
        wiced_gpio_output_high( BLUETOOTH_GPIO_RTS_PIN );    // RTS deasserted
        wiced_gpio_output_low ( BLUETOOTH_GPIO_REG_EN_PIN ); // Bluetooth chip regulator off
        device_powered = WICED_FALSE;

        /* Deinitialise UART */
        result = wiced_uart_deinit( BLUETOOTH_UART );

        if ( result != WICED_SUCCESS )
        {
            wiced_assert("Error deinitialising Bluetooth UART bus\r\n", result == WICED_SUCCESS );
            return WICED_ERROR;
        }

        bus_initialised = WICED_FALSE;

        return WICED_SUCCESS;
    }
    else
    {
        return WICED_ERROR;
    }
}

wiced_result_t bt_bus_enable_irq( bt_bus_isr isr )
{
    UNUSED_PARAMETER( isr );
    return WICED_UNSUPPORTED;
}

wiced_result_t bt_bus_disable_irq( void )
{
    return WICED_UNSUPPORTED;
}

wiced_result_t bt_bus_transmit( const uint8_t* data_out, uint32_t size )
{
    if ( bus_initialised == WICED_FALSE )
    {
        return WICED_ERROR;
    }

    while ( bt_bus_is_ready() == WICED_FALSE )
    {
        wiced_rtos_delay_milliseconds( 10 );
    }

    return wiced_uart_transmit_bytes( BLUETOOTH_UART, data_out, size );
}

wiced_result_t bt_bus_receive( uint8_t* data_in, uint32_t size, uint32_t timeout_ms )
{
    return ( bus_initialised == WICED_TRUE ) ? wiced_uart_receive_bytes( BLUETOOTH_UART, (void*)data_in, size, timeout_ms ) : WICED_NOTREADY;
}

wiced_bool_t bt_bus_is_ready( void )
{
    return ( bus_initialised == WICED_FALSE ) ? WICED_FALSE : ( ( wiced_gpio_input_get( BLUETOOTH_GPIO_CTS_PIN ) == WICED_TRUE ) ? WICED_FALSE : WICED_TRUE );
}

wiced_bool_t bt_bus_is_on( void )
{
    return device_powered;
}
