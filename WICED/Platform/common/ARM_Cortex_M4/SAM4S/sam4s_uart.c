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
#include "RTOS/wwd_rtos_interface.h"
#include "string.h"
#ifdef __GNUC__
#include "../../GCC/stdio_newlib.h"
#endif /* ifdef __GNUC__ */

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define STDIO_RX_RING_BUFFER_SIZE              (128)
#define TOTAL_UARTS                              (4)
#define RAM_START_ADDR                   (IRAM_ADDR)
#define RAM_END_ADDR    (RAM_START_ADDR + IRAM_SIZE)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

#pragma pack(1)
typedef struct
{
    host_semaphore_type_t tx_dma_complete;
    host_semaphore_type_t rx_dma_complete;
    wiced_ring_buffer_t*  rx_ring_buffer;
    uint32_t              rx_transfer_size;
} sam4s_uart_runtime_data_t;
#pragma pack()

/******************************************************
 *               Function Declarations
 ******************************************************/

void usart1_irq( void );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/* Flag indicating whether UART runtime data has been initialised */
static wiced_bool_t uart_runtime_data_initted = WICED_FALSE;

/* UART runtime data */
static sam4s_uart_runtime_data_t uart_runtime_data[WICED_UART_MAX];

/* Pointer for runtime UART data. Used by ISR to access runtime data directy */
static sam4s_uart_runtime_data_t* uart_runtime_data_ptr[TOTAL_UARTS];

#ifndef WICED_DISABLE_STDIO
/* Standard I/O related variables */
static const sam_usart_opt_t const stdio_config =
{
    .baudrate     = 115200,
    .char_length  = US_MR_CHRL_8_BIT,
    .parity_type  = US_MR_PAR_NO,
    .stop_bits    = US_MR_NBSTOP_1_BIT,
    .channel_mode = US_MR_CHMODE_NORMAL,
};
static host_semaphore_type_t stdio_rx_mutex;
static host_semaphore_type_t stdio_tx_mutex;
static wiced_ring_buffer_t   stdio_rx_ring_buffer;
static uint8_t               stdio_rx_ring_buffer_data[STDIO_RX_RING_BUFFER_SIZE];
#endif /* WICED_DISABLE_STDIO */

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t sam4s_uart_init( const sam4s_uart_t* uart, const sam4s_uart_config_t* config, wiced_ring_buffer_t* ring_buffer )
{
    pdc_packet_t dma_packet;
    uint8_t      index;

    if ( uart_runtime_data_initted == WICED_FALSE )
    {
        /* Initialise when this function is first called */
        memset( &uart_runtime_data, 0, sizeof( uart_runtime_data ) );
        memset( &uart_runtime_data_ptr, 0, sizeof( uart_runtime_data_ptr ) );
        uart_runtime_data_initted = WICED_TRUE;
    }

    switch ( uart->peripheral_id )
    {
        case ID_USART0:
            index = 0;
            break;
        case ID_USART1:
            index = 1;
            break;
        case ID_UART0:
            index = 2;
            break;
        case ID_UART1:
            index = 3;
            break;
        default:
            return WICED_BADARG;
    }

    /* Set random-access runtime data pointer */
    uart_runtime_data_ptr[index] = &uart_runtime_data[uart->uart];

    /* Initialise TX and RX complete semaphores */
    host_rtos_init_semaphore( &uart_runtime_data[uart->uart].tx_dma_complete );
    host_rtos_init_semaphore( &uart_runtime_data[uart->uart].rx_dma_complete );

    /* Set Tx and Rx pin mode to UART peripheral */
    sam4s_peripheral_pin_init( uart->tx_pin, &uart->tx_pin_config );
    sam4s_peripheral_pin_init( uart->rx_pin, &uart->rx_pin_config );

    /* Init CTS and RTS pins (if available) */
    if ( uart->cts_pin != NULL )
    {
        sam4s_peripheral_pin_init( uart->cts_pin, &uart->cts_pin_config );
    }

    if ( uart->rts_pin != NULL )
    {
        sam4s_peripheral_pin_init( uart->rts_pin, &uart->rts_pin_config );
    }

    /* Enable the peripheral clock in the PMC. */
    sysclk_enable_peripheral_clock( uart->peripheral_id );

    /* Enable the receiver and transmitter. */
    usart_reset_tx( uart->peripheral );
    usart_reset_rx( uart->peripheral );

    /* Configure USART in serial mode. */
    usart_init_rs232( uart->peripheral, config, CPU_CLOCK_HZ );

    /* Disable all the interrupts. */
    usart_disable_interrupt( uart->peripheral, 0xffffffff );

    /* Configure and enable interrupt of USART. */
    NVIC_SetPriority( uart->interrupt_vector, SAM4S_UART_IRQ_PRIO );
    NVIC_EnableIRQ( uart->interrupt_vector );

    /* Enable PDC transmit */
    pdc_enable_transfer( usart_get_pdc_base( uart->peripheral ), PERIPH_PTCR_TXTEN | PERIPH_PTCR_RXTEN );

    uart_runtime_data[uart->uart].rx_ring_buffer = ring_buffer;

    dma_packet.ul_addr = (uint32_t)uart_runtime_data[uart->uart].rx_ring_buffer->buffer;
    dma_packet.ul_size = (uint32_t)uart_runtime_data[uart->uart].rx_ring_buffer->size;
    pdc_rx_init( usart_get_pdc_base( uart->peripheral ), &dma_packet, &dma_packet );

    usart_enable_interrupt( uart->peripheral, US_IER_ENDRX | US_IER_RXBUFF | US_IER_RXRDY | US_IER_ENDTX );

    /* Enable the receiver and transmitter. */
    usart_enable_tx( uart->peripheral );
    usart_enable_rx( uart->peripheral );

    return WICED_SUCCESS;
}

wiced_result_t sam4s_uart_deinit( const sam4s_uart_t* uart )
{
    uint8_t index;

    usart_disable_interrupt( uart->peripheral, 0xffffffff );

    NVIC_DisableIRQ( uart->interrupt_vector );

    pdc_disable_transfer( usart_get_pdc_base( uart->peripheral ), PERIPH_PTCR_TXTDIS | PERIPH_PTCR_RXTDIS );

    usart_disable_tx( uart->peripheral );
    usart_disable_rx( uart->peripheral );

    sysclk_disable_peripheral_clock( uart->peripheral_id );

    sam4s_pin_deinit( uart->tx_pin );
    sam4s_pin_deinit( uart->rx_pin );

    if ( uart->cts_pin != NULL )
    {
        sam4s_pin_deinit( uart->cts_pin );
    }

    if ( uart->rts_pin != NULL )
    {
        sam4s_pin_deinit( uart->rts_pin );
    }

    host_rtos_deinit_semaphore( &uart_runtime_data[uart->uart].tx_dma_complete );
    host_rtos_deinit_semaphore( &uart_runtime_data[uart->uart].rx_dma_complete );

    switch ( uart->peripheral_id )
    {
        case ID_USART0:
            index = 0;
            break;
        case ID_USART1:
            index = 1;
            break;
        case ID_UART0:
            index = 2;
            break;
        case ID_UART1:
            index = 3;
            break;
        default:
            return WICED_BADARG;
    }

    uart_runtime_data_ptr[index] = NULL;
    memset( &uart_runtime_data[uart->uart], 0, sizeof(sam4s_uart_runtime_data_t) );

    return WICED_SUCCESS;
}

wiced_result_t sam4s_uart_transmit( const sam4s_uart_t* uart, const uint8_t* data_out, uint32_t data_length )
{
    pdc_packet_t dma_packet;

    /* Limitation: SAM4S doesn't support DMA transfer from embedded flash.
     * If data_out address is not within RAM range, use normal write to THR.
     */
    if ( data_out >= (const uint8_t*)RAM_START_ADDR && data_out < (const uint8_t*)RAM_END_ADDR )
    {
        /* Initialise TPR and TCR register values. TNPR and TNCR are unused */
        dma_packet.ul_addr = (uint32_t)data_out;
        dma_packet.ul_size = (uint32_t)data_length;
        pdc_tx_init( usart_get_pdc_base( uart->peripheral ), &dma_packet, NULL );

        /* Enable Tx DMA transmission */
        pdc_enable_transfer( usart_get_pdc_base( uart->peripheral ), PERIPH_PTCR_TXTEN );

        host_rtos_get_semaphore( &uart_runtime_data[uart->uart].tx_dma_complete, WICED_NEVER_TIMEOUT, WICED_FALSE );
    }
    else
    {
        while ( data_length > 0 )
        {
            usart_putchar( uart->peripheral, (uint32_t)*data_out++ );
            data_length--;
        }
    }

    return WICED_SUCCESS;
}

wiced_result_t sam4s_uart_receive ( const sam4s_uart_t* uart, uint8_t* data_in, uint32_t expected_data_length, uint32_t timeout_ms )
{
    if ( uart_runtime_data[uart->uart].rx_ring_buffer != NULL )
    {
        while ( expected_data_length != 0 )
        {
            uint32_t transfer_size = MIN(uart_runtime_data[uart->uart].rx_ring_buffer->size / 2, expected_data_length);

            /* Check if ring buffer already contains the required amount of data. */
            if ( transfer_size > ring_buffer_used_space( uart_runtime_data[uart->uart].rx_ring_buffer ) )
            {
                /* Set rx_size and wait in rx_complete semaphore until data reaches rx_size or timeout occurs */
                uart_runtime_data[uart->uart].rx_transfer_size = transfer_size;

                if ( host_rtos_get_semaphore( &uart_runtime_data[uart->uart].rx_dma_complete, timeout_ms, WICED_FALSE ) != WICED_SUCCESS )
                {
                    uart_runtime_data[uart->uart].rx_transfer_size = 0;
                    return WICED_TIMEOUT;
                }

                /* Reset rx_size to prevent semaphore being set while nothing waits for the data */
                uart_runtime_data[uart->uart].rx_transfer_size = 0;
            }

            expected_data_length -= transfer_size;

            // Grab data from the buffer
            do
            {
                uint8_t* available_data;
                uint32_t bytes_available;

                ring_buffer_get_data( uart_runtime_data[uart->uart].rx_ring_buffer, &available_data, &bytes_available );
                bytes_available = MIN( bytes_available, transfer_size );
                memcpy( data_in, available_data, bytes_available );
                transfer_size -= bytes_available;
                data_in = ( (uint8_t*)data_in + bytes_available );
                ring_buffer_consume( uart_runtime_data[uart->uart].rx_ring_buffer, bytes_available );
            }
            while ( transfer_size != 0 );
        }

        if ( expected_data_length != 0 )
        {
            return WICED_ERROR;
        }
        else
        {
            return WICED_SUCCESS;
        }
    }
    else
    {
        /* TODO: need to implement this */
        return WICED_UNSUPPORTED;
    }
}

void usart1_irq( void )
{
    uint32_t status = usart_get_status( USART1 );
    uint32_t mask = usart_get_interrupt_mask( USART1 );
    Pdc* pdc_register = usart_get_pdc_base( USART1 );

    /* ENDTX flag is set when Tx DMA transfer is done
     */
    if ( ( mask & US_IMR_ENDTX ) && ( status & US_CSR_ENDTX ) )
    {
        pdc_packet_t dma_packet;

        /* ENDTX is cleared when TCR or TNCR is set to a non-zero value, which effectively
         * starts another Tx DMA transaction. To work around this, disable Tx before
         * performing a dummy Tx init.
         */
        pdc_disable_transfer( usart_get_pdc_base( USART1 ), PERIPH_PTCR_TXTDIS );

        dma_packet.ul_addr = (uint32_t)0;
        dma_packet.ul_size = (uint32_t)1;

        pdc_tx_init( usart_get_pdc_base( USART1 ), &dma_packet, NULL );

        /* Notifies waiting thread that Tx DMA transfer is complete */
        host_rtos_set_semaphore( &uart_runtime_data_ptr[1]->tx_dma_complete, WICED_TRUE );
    }

    /* ENDRX flag is set when RCR is 0. RNPR and RNCR values are then copied into
     * RPR and RCR, respectively, while the Tx tranfer continues. We now need to
     * prepare RNPR and RNCR for the next iteration.
     */
    if ( ( mask & US_IMR_ENDRX ) && ( status & US_CSR_ENDRX ) )
    {
        pdc_register->PERIPH_RNPR = (uint32_t)uart_runtime_data_ptr[1]->rx_ring_buffer->buffer;
        pdc_register->PERIPH_RNCR = (uint32_t)uart_runtime_data_ptr[1]->rx_ring_buffer->size;
    }

    /* RXRDY interrupt is triggered and flag is set when a new character has been
     * received but not yet read from the US_RHR. When this interrupt executes,
     * the DMA engine already read the character out from the US_RHR and RXRDY flag
     * is no longer asserted. The code below updates the ring buffer parameters
     * to keep them current
     */
    if ( mask & US_CSR_RXRDY )
    {
        uart_runtime_data_ptr[1]->rx_ring_buffer->tail = uart_runtime_data_ptr[1]->rx_ring_buffer->size - pdc_register->PERIPH_RCR;

        // Notify thread if sufficient data are available
        if ( ( uart_runtime_data_ptr[1]->rx_transfer_size > 0 ) && ( ring_buffer_used_space( uart_runtime_data_ptr[1]->rx_ring_buffer ) >= uart_runtime_data_ptr[1]->rx_transfer_size ) )
        {
            host_rtos_set_semaphore( &uart_runtime_data_ptr[1]->rx_dma_complete, WICED_TRUE );
            uart_runtime_data_ptr[1]->rx_transfer_size = 0;
        }
    }
}

void platform_stdio_init( void )
{
#ifndef WICED_DISABLE_STDIO
    sam4s_powersave_clocks_needed();

    /* Initialise STDIO UART RX ring buffer */
    ring_buffer_init( &stdio_rx_ring_buffer, stdio_rx_ring_buffer_data, STDIO_RX_RING_BUFFER_SIZE );

    /* Initialise STDIO UART */
    sam4s_uart_init( &platform_uart[STDIO_UART], &stdio_config, &stdio_rx_ring_buffer );

    /* Initialise mutex for stdio thread-safeness */
    host_rtos_init_semaphore( &stdio_tx_mutex );
    host_rtos_set_semaphore( &stdio_tx_mutex, WICED_FALSE );
    host_rtos_init_semaphore( &stdio_rx_mutex );
    host_rtos_set_semaphore( &stdio_rx_mutex, WICED_FALSE );

    sam4s_powersave_clocks_not_needed();
#endif /* ifdef WICED_DISABLE_STDIO */
}

void platform_stdio_write( const char* str, uint32_t len )
{
#ifndef WICED_DISABLE_STDIO
    sam4s_powersave_clocks_needed();
    host_rtos_get_semaphore( &stdio_tx_mutex, WICED_NEVER_TIMEOUT, WICED_FALSE );
    sam4s_uart_transmit( &platform_uart[STDIO_UART], (const uint8_t*)str, len );
    host_rtos_set_semaphore( &stdio_tx_mutex, WICED_FALSE );
    sam4s_powersave_clocks_not_needed();
#else
    UNUSED_PARAMETER( str );
    UNUSED_PARAMETER( len );
#endif
}

void platform_stdio_read( char* str, uint32_t len )
{
#ifndef WICED_DISABLE_STDIO
    sam4s_powersave_clocks_needed();
    host_rtos_get_semaphore( &stdio_rx_mutex, WICED_NEVER_TIMEOUT, WICED_FALSE );
    sam4s_uart_receive( &platform_uart[STDIO_UART], (uint8_t*)str, len, WICED_NEVER_TIMEOUT );
    host_rtos_set_semaphore( &stdio_rx_mutex, WICED_FALSE );
    sam4s_powersave_clocks_not_needed();
#else
    UNUSED_PARAMETER( str );
    UNUSED_PARAMETER( len );
#endif
}
