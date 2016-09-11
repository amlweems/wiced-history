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
#include "stm32f4xx_platform.h"
#include "stm32f4xx_gpio.h"
#include "stdlib.h"
#include "platform.h"
#include "platform_common_config.h"
#include "wwd_assert.h"
#include "wiced_utilities.h"
#include "watchdog.h"
#include "platform_dct.h"
#include <string.h>
#include "gpio_irq.h"
#include "wiced_platform.h"
#include "Platform/wwd_platform_interface.h"
#include "crt0.h"
#include "platform_sleep.h"
#ifdef __GNUC__
#include "../../GCC/stdio_newlib.h"
#endif /* ifdef __GNUC__ */

#ifdef __GNUC__
#define WEAK __attribute__ ((weak))
#elif defined ( __IAR_SYSTEMS_ICC__ )
#define WEAK
#endif /* ifdef __GNUC__ */

#ifndef WICED_DISABLE_BOOTLOADER
#include "bootloader_app.h"
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define MAX_NUM_SPI_PRESCALERS     (8)
#define SPI_DMA_TIMEOUT_LOOPS      (10000)

#if defined( PLATFORM_STM32_VOLTAGE_1V8_TO_2V1 )
#define ERASE_VOLTAGE_RANGE ( VoltageRange_1 )
#define FLASH_WRITE_FUNC    ( FLASH_ProgramByte )
#define FLASH_WRITE_SIZE    ( 1 )
typedef uint8_t flash_write_t;
#elif defined( PLATFORM_STM32_VOLTAGE_2V1_TO_2V7 )
#define ERASE_VOLTAGE_RANGE ( VoltageRange_2 )
#define FLASH_WRITE_FUNC    ( FLASH_ProgramHalfWord )
#define FLASH_WRITE_SIZE    ( 2 )
typedef uint16_t flash_write_t;
#elif defined( PLATFORM_STM32_VOLTAGE_2V7_TO_3V6 )
#define ERASE_VOLTAGE_RANGE ( VoltageRange_3 )
#define FLASH_WRITE_FUNC    ( FLASH_ProgramWord )
#define FLASH_WRITE_SIZE    ( 4 )
typedef uint32_t flash_write_t;
#elif defined( PLATFORM_STM32_VOLTAGE_2V7_TO_3V6_EXTERNAL_VPP )
#define ERASE_VOLTAGE_RANGE ( VoltageRange_4 )
#define FLASH_WRITE_FUNC    ( FLASH_ProgramDoubleWord )
#define FLASH_WRITE_SIZE    ( 8 )
typedef uint64_t flash_write_t;
#else
/* No Voltage range defined for platform */
/* You need to define one of:
 *   PLATFORM_STM32_VOLTAGE_1V8_TO_2V1
 *   PLATFORM_STM32_VOLTAGE_2V1_TO_2V7
 *   PLATFORM_STM32_VOLTAGE_2V7_TO_3V6
 *   PLATFORM_STM32_VOLTAGE_2V7_TO_3V6_EXTERNAL_VPP
 */
#error Platform Voltage Range not defined
#endif

/* These come from the linker script */
extern void* dct1_start_addr_loc;
extern void* dct1_size_loc;
extern void* dct2_start_addr_loc;
extern void* dct2_size_loc;
extern void* app_hdr_start_addr_loc;
extern void* sram_start_addr_loc;
extern void* sram_size_loc;
#define APP_HDR_START_ADDR   ((uint32_t)&app_hdr_start_addr_loc)
#define DCT1_START_ADDR  ((uint32_t)&dct1_start_addr_loc)
#define DCT1_SIZE        ((uint32_t)&dct1_size_loc)
#define DCT2_START_ADDR  ((uint32_t)&dct2_start_addr_loc)
#define DCT2_SIZE        ((uint32_t)&dct2_size_loc)
#define SRAM_START_ADDR  ((uint32_t)&sram_start_addr_loc)
#define SRAM_SIZE        ((uint32_t)&sram_size_loc)

#define PLATFORM_DCT_COPY1_START_SECTOR      ( FLASH_Sector_1  )
#define PLATFORM_DCT_COPY1_START_ADDRESS     ( DCT1_START_ADDR )
#define PLATFORM_DCT_COPY1_END_SECTOR        ( FLASH_Sector_1 )
#define PLATFORM_DCT_COPY1_END_ADDRESS       ( DCT1_START_ADDR + DCT1_SIZE )
#define PLATFORM_DCT_COPY2_START_SECTOR      ( FLASH_Sector_2  )
#define PLATFORM_DCT_COPY2_START_ADDRESS     ( DCT2_START_ADDR )
#define PLATFORM_DCT_COPY2_END_SECTOR        ( FLASH_Sector_2 )
#define PLATFORM_DCT_COPY2_END_ADDRESS       ( DCT1_START_ADDR + DCT1_SIZE )

#define ERASE_DCT_1()              platform_erase_flash(PLATFORM_DCT_COPY1_START_SECTOR, PLATFORM_DCT_COPY1_END_SECTOR)
#define ERASE_DCT_2()              platform_erase_flash(PLATFORM_DCT_COPY2_START_SECTOR, PLATFORM_DCT_COPY2_END_SECTOR)

#ifndef STDIO_BUFFER_SIZE
#define STDIO_BUFFER_SIZE   64
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

typedef struct
{
    uint32_t               rx_size;
    wiced_ring_buffer_t*   rx_buffer;
    host_semaphore_type_t  rx_complete;
    host_semaphore_type_t  tx_complete;
    wiced_result_t         tx_dma_result;
    wiced_result_t         rx_dma_result;
} uart_interface_t;

typedef struct
{
    uint16_t factor;
    uint16_t prescaler_value;
} spi_baudrate_division_mapping_t;

/******************************************************
 *               Function Declarations
 ******************************************************/

static wiced_result_t wiced_spi_configure_baudrate( uint32_t speed, uint16_t* prescaler );
static wiced_result_t spi_dma_transfer( const wiced_spi_device_t* spi );
static void           spi_dma_config( const wiced_spi_device_t* spi, wiced_spi_message_segment_t* message );
static wiced_result_t platform_uart_init( wiced_uart_t uart, const wiced_uart_config_t* config, wiced_ring_buffer_t* optional_rx_buffer );
static wiced_result_t platform_uart_receive_bytes( wiced_uart_t uart, void* data, uint32_t size, uint32_t timeout );

#ifdef STOP_MODE_SUPPORT
void RTC_init(void);
#endif /* ifdef STOP_MODE_SUPPORT */

/* Interrupt service functions - called from interrupt vector table */
void RTC_WKUP_irq     ( void );
void usart2_rx_dma_irq( void );
void usart1_rx_dma_irq( void );
void usart6_rx_dma_irq( void );
void usart2_tx_dma_irq( void );
void usart1_tx_dma_irq( void );
void usart6_tx_dma_irq( void );
void usart2_irq       ( void );
void usart1_irq       ( void );
void usart6_irq       ( void );


/******************************************************
 *               Variables Definitions
 ******************************************************/

static uart_interface_t uart_interfaces[NUMBER_OF_UART_INTERFACES];

static const uint32_t spi_transfer_complete_flags[]=
{
    /* for every stream get a transfer complete flag */
    [0] =  DMA_FLAG_TCIF0,
    [1] =  DMA_FLAG_TCIF1,
    [2] =  DMA_FLAG_TCIF2,
    [3] =  DMA_FLAG_TCIF3,
    [4] =  DMA_FLAG_TCIF4,
    [5] =  DMA_FLAG_TCIF5,
    [6] =  DMA_FLAG_TCIF6,
    [7] =  DMA_FLAG_TCIF7,
};

static const spi_baudrate_division_mapping_t spi_baudrate_prescalers[MAX_NUM_SPI_PRESCALERS] =
{
    { 2,   SPI_BaudRatePrescaler_2   },
    { 4,   SPI_BaudRatePrescaler_4   },
    { 8,   SPI_BaudRatePrescaler_8   },
    { 16,  SPI_BaudRatePrescaler_16  },
    { 32,  SPI_BaudRatePrescaler_32  },
    { 64,  SPI_BaudRatePrescaler_64  },
    { 128, SPI_BaudRatePrescaler_128 },
    { 256, SPI_BaudRatePrescaler_256 },
};

static char stm32_platform_inited = 0;

#ifndef WICED_DISABLE_STDIO
static const wiced_uart_config_t stdio_uart_config =
{
    .baud_rate    = 115200,
    .data_width   = DATA_WIDTH_8BIT,
    .parity       = NO_PARITY,
    .stop_bits    = STOP_BITS_1,
    .flow_control = FLOW_CONTROL_DISABLED,
};


static volatile wiced_ring_buffer_t stdio_rx_buffer;
static volatile uint8_t             stdio_rx_data[STDIO_BUFFER_SIZE];
static host_semaphore_type_t        stdio_rx_mutex;
static host_semaphore_type_t        stdio_tx_mutex;
#endif /* #ifndef WICED_DISABLE_STDIO */

static volatile uint8_t uart_break;

static wiced_spi_device_t* current_spi_device = NULL;

static const uint16_t adc_sampling_cycle[] =
{
    [ADC_SampleTime_3Cycles  ] = 3,
    [ADC_SampleTime_15Cycles ] = 15,
    [ADC_SampleTime_28Cycles ] = 28,
    [ADC_SampleTime_56Cycles ] = 56,
    [ADC_SampleTime_84Cycles ] = 84,
    [ADC_SampleTime_112Cycles] = 112,
    [ADC_SampleTime_144Cycles] = 144,
    [ADC_SampleTime_480Cycles] = 480,
};

/******************************************************
 *               Function Definitions
 ******************************************************/

/* STM32F2 common clock initialisation function
 * - weak attribute is intentional in case a specific platform (board) needs to override this.
 */
WEAK void init_clocks( void )
{
    RCC_DeInit( );

    /* Configure Clocks */

    RCC_HSEConfig( HSE_SOURCE );
    RCC_WaitForHSEStartUp( );

    RCC_HCLKConfig( AHB_CLOCK_DIVIDER );
    RCC_PCLK2Config( APB2_CLOCK_DIVIDER );
    RCC_PCLK1Config( APB1_CLOCK_DIVIDER );

    /* Enable the PLL */
    FLASH_SetLatency( INT_FLASH_WAIT_STATE );
    FLASH_PrefetchBufferCmd( ENABLE );

    /* Use the clock configuration utility from ST to calculate these values
     * http://www.st.com/st-web-ui/static/active/en/st_prod_software_internet/resource/technical/software/utility/stsw-stm32091.zip
     */
    RCC_PLLConfig( PLL_SOURCE, PLL_M_CONSTANT, PLL_N_CONSTANT, PLL_P_CONSTANT, PPL_Q_CONSTANT ); /* NOTE: The CPU Clock Frequency is independently defined in <WICED-SDK>/Wiced/Platform/<platform>/<platform>.mk */
    RCC_PLLCmd( ENABLE );

    while ( RCC_GetFlagStatus( RCC_FLAG_PLLRDY ) == RESET )
    {
    }
    RCC_SYSCLKConfig( SYSTEM_CLOCK_SOURCE );

    while ( RCC_GetSYSCLKSource( ) != 0x08 )
    {
    }

    /* Configure HCLK clock as SysTick clock source. */
    SysTick_CLKSourceConfig( SYSTICK_CLOCK_SOURCE );

#ifndef WICED_DISABLE_STDIO
    {
		USART_ClockInitTypeDef usart_clock_init_structure;
		STDIO_CLOCK_CMD( STDIO_PERIPH_CLOCK, ENABLE );

		usart_clock_init_structure.USART_Clock   = USART_Clock_Disable;
		usart_clock_init_structure.USART_CPOL    = USART_CPOL_Low;
		usart_clock_init_structure.USART_CPHA    = USART_CPHA_2Edge;
		usart_clock_init_structure.USART_LastBit = USART_LastBit_Disable;

		USART_ClockInit( STDIO_USART, &usart_clock_init_structure );
    }
#endif /* ifdef WICED_DISABLE_STDIO */
}

WEAK void init_memory( void )
{

}

void init_architecture( void )
{
    uint8_t i;

    if ( stm32_platform_inited == 1 )
        return;

    watchdog_init( );

    /* Initialise the interrupt priorities to a priority lower than 0 so that the BASEPRI register can mask them */
    for ( i = 0; i < 81; i++ )
    {
        NVIC ->IP[i] = 0xff;
    }

    NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );

#ifndef WICED_DISABLE_STDIO
    host_rtos_init_semaphore( &stdio_tx_mutex );
    host_rtos_set_semaphore ( &stdio_tx_mutex, WICED_FALSE );
    host_rtos_init_semaphore( &stdio_rx_mutex );
    host_rtos_set_semaphore ( &stdio_rx_mutex, WICED_FALSE );
    ring_buffer_init  ( (wiced_ring_buffer_t*)&stdio_rx_buffer, (uint8_t*)stdio_rx_data, STDIO_BUFFER_SIZE );
    platform_uart_init( STDIO_UART, &stdio_uart_config, (wiced_ring_buffer_t*)&stdio_rx_buffer );
#endif

    /* Ensure 802.11 device is in reset. */
    host_platform_init( );

#ifdef STOP_MODE_SUPPORT
    RTC_init( );
#endif /* ifdef STOP_MODE_SUPPORT */

    stm32_platform_inited = 1;
}

static wiced_result_t wiced_spi_configure_baudrate( uint32_t speed, uint16_t* prescaler )
{
    uint8_t i;

    wiced_assert("Bad args", prescaler != NULL);

    for( i = 0 ; i < MAX_NUM_SPI_PRESCALERS ; i++ )
    {
        if( ( 60000000 / spi_baudrate_prescalers[i].factor ) <= speed )
        {
            *prescaler = spi_baudrate_prescalers[i].prescaler_value;
            return WICED_SUCCESS;
        }
    }

    return WICED_ERROR;
}

static wiced_result_t spi_dma_transfer( const wiced_spi_device_t* spi )
{
    uint32_t loop_count;

    /* Enable dma channels that have just been configured */
    DMA_Cmd(spi_mapping[spi->port].rx_dma_stream, ENABLE);
    DMA_Cmd(spi_mapping[spi->port].tx_dma_stream, ENABLE);

    /* Wait for DMA to complete */
    /* TODO: This should wait on a semaphore that is triggered from an IRQ */
    loop_count = 0;
    while ( ( DMA_GetFlagStatus( spi_mapping[spi->port].rx_dma_stream, spi_transfer_complete_flags[ spi_mapping[spi->port].rx_dma_stream_number ] ) == RESET ) )
    {
        loop_count++;
        /* Check if we've run out of time */
        if ( loop_count >= (uint32_t) SPI_DMA_TIMEOUT_LOOPS )
        {
            wiced_gpio_output_high(spi->chip_select);
            return WICED_TIMEOUT;
        }
    }

    wiced_gpio_output_high(spi->chip_select);
    return WICED_SUCCESS;
}

static void spi_dma_config( const wiced_spi_device_t* spi, wiced_spi_message_segment_t* message )
{
    DMA_InitTypeDef dma_init;
    uint8_t         dummy = 0xFF;

    wiced_assert("Bad args", (spi != NULL) && (message != NULL))

    /* Setup DMA for SPI TX if it is enabled */
    DMA_DeInit( spi_mapping[spi->port].tx_dma_stream );

    /* Setup DMA stream for TX */
    dma_init.DMA_Channel            = spi_mapping[spi->port].tx_dma_channel;
    dma_init.DMA_PeripheralBaseAddr = ( uint32_t )&spi_mapping[spi->port].spi_regs->DR;
    dma_init.DMA_DIR                = DMA_DIR_MemoryToPeripheral;
    dma_init.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_init.DMA_BufferSize         = message->length;
    dma_init.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    dma_init.DMA_Mode               = DMA_Mode_Normal;
    dma_init.DMA_Priority           = DMA_Priority_VeryHigh;
    dma_init.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    dma_init.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full;
    dma_init.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    dma_init.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;

    if ( message->tx_buffer != NULL )
    {
       dma_init.DMA_Memory0BaseAddr = ( uint32_t )message->tx_buffer;
       dma_init.DMA_MemoryInc       = DMA_MemoryInc_Enable;
    }
    else
    {
       dma_init.DMA_Memory0BaseAddr = ( uint32_t )(&dummy);
       dma_init.DMA_MemoryInc       = DMA_MemoryInc_Disable;
    }

    DMA_Init( spi_mapping[spi->port].tx_dma_stream, &dma_init );

    /* Activate SPI DMA mode for transmission */
    SPI_I2S_DMACmd( spi_mapping[spi->port].spi_regs, SPI_I2S_DMAReq_Tx, ENABLE );

    /* TODO: Init TX DMA finished semaphore  */

    /* Setup DMA for SPI RX stream */
    DMA_DeInit( spi_mapping[spi->port].rx_dma_stream );
    dma_init.DMA_Channel            = spi_mapping[spi->port].rx_dma_channel;
    dma_init.DMA_PeripheralBaseAddr = ( uint32_t )&spi_mapping[spi->port].spi_regs->DR;
    dma_init.DMA_DIR                = DMA_DIR_PeripheralToMemory;
    dma_init.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_init.DMA_BufferSize         = message->length;
    dma_init.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    dma_init.DMA_Mode               = DMA_Mode_Normal;
    dma_init.DMA_Priority           = DMA_Priority_VeryHigh;
    dma_init.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    dma_init.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full;
    dma_init.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    dma_init.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;
    if ( message->rx_buffer != NULL )
    {
        dma_init.DMA_Memory0BaseAddr = (uint32_t)message->rx_buffer;
        dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable;
    }
    else
    {
        dma_init.DMA_Memory0BaseAddr = (uint32_t)&dummy;
        dma_init.DMA_MemoryInc = DMA_MemoryInc_Disable;
    }

    /* Init and activate RX DMA channel */
    DMA_Init( spi_mapping[spi->port].rx_dma_stream, &dma_init );
    SPI_I2S_DMACmd( spi_mapping[spi->port].spi_regs, SPI_I2S_DMAReq_Rx, ENABLE );

    /* TODO: Init RX DMA finish semaphore */
}

wiced_result_t wiced_spi_init( const wiced_spi_device_t* spi )
{
    GPIO_InitTypeDef gpio_init_structure;
    wiced_result_t   result;
    SPI_InitTypeDef  spi_init;

    wiced_assert("Bad args", spi != NULL);

    /* Init SPI GPIOs */
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_AF;
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_100MHz;
    gpio_init_structure.GPIO_Pin   = ((uint32_t) (1 << spi_mapping[spi->port].pin_clock->number)) |
                                     ((uint32_t) (1 << spi_mapping[spi->port].pin_miso->number )) |
                                     ((uint32_t) (1 << spi_mapping[spi->port].pin_mosi->number ));
    GPIO_Init( spi_mapping[spi->port].pin_clock->bank, &gpio_init_structure );

    /* Init the chip select GPIO */
    wiced_gpio_init(spi->chip_select, OUTPUT_PUSH_PULL);
    wiced_gpio_output_high(spi->chip_select);

    GPIO_PinAFConfig( spi_mapping[spi->port].pin_clock->bank, spi_mapping[spi->port].pin_clock->number,  spi_mapping[spi->port].gpio_af );
    GPIO_PinAFConfig( spi_mapping[spi->port].pin_miso->bank,  spi_mapping[spi->port].pin_miso->number,   spi_mapping[spi->port].gpio_af );
    GPIO_PinAFConfig( spi_mapping[spi->port].pin_mosi->bank,  spi_mapping[spi->port].pin_mosi->number,   spi_mapping[spi->port].gpio_af );

    /* Configure baudrate */
    result = wiced_spi_configure_baudrate( spi->speed, &spi_init.SPI_BaudRatePrescaler );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Configure data-width */
    if ( spi->bits == 8 )
    {
        spi_init.SPI_DataSize = SPI_DataSize_8b;
    }
    else if ( spi->bits == 16 )
    {
        if ( spi->mode & SPI_USE_DMA )
        {
            /* 16 bit mode is not supported for a DMA */
            return WICED_ERROR;
        }
        spi_init.SPI_DataSize = SPI_DataSize_16b;
    }
    else
    {
        /* Requested mode is not supported */
        return WICED_BADOPTION;
    }

    /* Configure MSB or LSB */
    if ( spi->mode & SPI_MSB_FIRST )
    {
        spi_init.SPI_FirstBit = SPI_FirstBit_MSB;
    }
    else
    {
        spi_init.SPI_FirstBit = SPI_FirstBit_LSB;
    }

    /* Configure mode CPHA and CPOL */
    if ( spi->mode & SPI_CLOCK_IDLE_HIGH )
    {
        spi_init.SPI_CPOL = SPI_CPOL_High;
    }
    else
    {
        spi_init.SPI_CPOL = SPI_CPOL_Low;
    }

    if ( spi->mode & SPI_CLOCK_RISING_EDGE )
    {
        spi_init.SPI_CPHA = ( spi->mode & SPI_CLOCK_IDLE_HIGH )? SPI_CPHA_2Edge : SPI_CPHA_1Edge;
    }
    else
    {
        spi_init.SPI_CPHA = ( spi->mode & SPI_CLOCK_IDLE_HIGH )? SPI_CPHA_1Edge : SPI_CPHA_2Edge;
    }

    spi_init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi_init.SPI_Mode      = SPI_Mode_Master;
    spi_init.SPI_NSS       = SPI_NSS_Soft;
    SPI_CalculateCRC( spi_mapping[spi->port].spi_regs, DISABLE );

    /* Enable SPI peripheral clock */
    spi_mapping[spi->port].peripheral_clock_func( spi_mapping[spi->port].peripheral_clock_reg,  ENABLE );

    /* Init and enable SPI */
    SPI_Init( spi_mapping[spi->port].spi_regs, &spi_init );
    SPI_Cmd ( spi_mapping[spi->port].spi_regs, ENABLE );

    /* From this point SPI is properly configured and we can use it for transfer */

    current_spi_device = (wiced_spi_device_t*)spi;

    return WICED_SUCCESS;
}

wiced_result_t wiced_spi_transfer( const wiced_spi_device_t* spi, wiced_spi_message_segment_t* segments, uint16_t number_of_segments )
{
    wiced_result_t result = WICED_SUCCESS;
    uint16_t       i;
    uint32_t       count = 0;

    wiced_assert("Bad args", (spi != NULL) && (segments != NULL) && (number_of_segments != 0));


    /* If the given SPI device is not the current SPI device, initialise */
    if ( spi != current_spi_device )
    {
        wiced_spi_init( spi );
    }

    /* Activate chip select */
    wiced_gpio_output_low(spi->chip_select);

    for ( i = 0; i < number_of_segments; i++ )
    {
        /* Check if we are using DMA */
        if ( spi->mode & SPI_USE_DMA )
        {
            spi_dma_config( spi, &segments[i] );
            result = spi_dma_transfer( spi );
            if ( result != WICED_SUCCESS )
            {
                goto cleanup_transfer;
            }
        }
        else
        {
            /* in interrupt-less mode */
            if ( spi->bits == 8 )
            {
            	const uint8_t* send_ptr;
            	uint8_t*       rcv_ptr;

                count    = segments[i].length;
                send_ptr = segments[i].tx_buffer;
                rcv_ptr  = segments[i].rx_buffer;
                while ( count-- )
                {
                    uint8_t data;
                    if ( send_ptr != NULL )
                    {
                        data = *send_ptr;
                        send_ptr++;
                    }
                    else
                    {
                        data = 0xFF;
                    }

                    /* Wait until the transmit buffer is empty */
                    while ( SPI_I2S_GetFlagStatus( spi_mapping[spi->port].spi_regs, SPI_I2S_FLAG_TXE ) == RESET )
                    {}

                    /* Send the byte */
                    SPI_I2S_SendData( spi_mapping[spi->port].spi_regs, data );

                    /* Wait until a data is received */
                    while ( SPI_I2S_GetFlagStatus( spi_mapping[spi->port].spi_regs, SPI_I2S_FLAG_RXNE ) == RESET )
                    {}

                    /* Get the received data */
                    data = (uint8_t) (0xff & SPI_I2S_ReceiveData( spi_mapping[spi->port].spi_regs ));

                    if ( rcv_ptr != NULL )
                    {
                        *rcv_ptr++ = data;
                    }
                }
            }
            else if ( spi->bits == 16 )
            {
                const uint16_t* send_ptr = (const uint16_t *) segments[i].tx_buffer;
                uint16_t*       rcv_ptr  = (uint16_t *) segments[i].rx_buffer;

                /* Check that the message length is a multiple of 2 */
                if ( ( count % 2 ) == 0 )
                {
                    result = WICED_ERROR;
                    goto cleanup_transfer;
                }

                while ( count != 0)
                {
                	uint16_t data;

                    count -= 2;
                    data = 0xFFFF;

                    if ( send_ptr != NULL )
                    {
                        data = *send_ptr++;
                    }

                    /* Wait until the transmit buffer is empty */
                    while ( SPI_I2S_GetFlagStatus( spi_mapping[spi->port].spi_regs, SPI_I2S_FLAG_TXE ) == RESET )
                    {}

                    /* Send the byte */
                    SPI_I2S_SendData( spi_mapping[spi->port].spi_regs, data );

                    /* Wait until a data is received */
                    while ( SPI_I2S_GetFlagStatus( spi_mapping[spi->port].spi_regs, SPI_I2S_FLAG_RXNE ) == RESET )
                    {}

                    /* Get the received data */
                    data = SPI_I2S_ReceiveData( spi_mapping[spi->port].spi_regs );

                    if ( rcv_ptr != NULL )
                    {
                        *rcv_ptr++ = data;
                    }
                }
            }
        }
    }
    result = WICED_SUCCESS;

cleanup_transfer:
    wiced_gpio_output_high(spi->chip_select);
//    __asm("CPSIE i");
    return result;
}

wiced_result_t wiced_spi_deinit( const wiced_spi_device_t* spi )
{
    GPIO_InitTypeDef gpio_init_structure;

    /* De-init and disable SPI */
    SPI_Cmd( spi_mapping[ spi->port ].spi_regs, DISABLE );
    SPI_I2S_DeInit( spi_mapping[ spi->port ].spi_regs );

    /* Disable SPI peripheral clock */
    spi_mapping[spi->port].peripheral_clock_func( spi_mapping[spi->port].peripheral_clock_reg,  DISABLE );

    /* Reset all pins to input floating state */
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_IN;      // Input
    gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_NOPULL;  // Floating (No-pull)
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;     // Arbitrary. Only applicable for output
    gpio_init_structure.GPIO_Speed = GPIO_Speed_100MHz;
    gpio_init_structure.GPIO_Pin   = ((uint32_t) (1 << spi_mapping[spi->port].pin_clock->number)) |
                                     ((uint32_t) (1 << spi_mapping[spi->port].pin_miso->number )) |
                                     ((uint32_t) (1 << spi_mapping[spi->port].pin_mosi->number ));

    GPIO_Init( spi_mapping[spi->port].pin_clock->bank, &gpio_init_structure );

    /* Reset CS pin to input floating state */
    wiced_gpio_init( spi->chip_select, INPUT_HIGH_IMPEDANCE );

    if ( spi == current_spi_device )
    {
        current_spi_device = NULL;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_i2c_init( wiced_i2c_device_t* device )
{
    UNUSED_PARAMETER( device );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_SUCCESS;
}

wiced_bool_t wiced_i2c_probe_device( wiced_i2c_device_t* device, int retries )
{
    UNUSED_PARAMETER( device );
    UNUSED_PARAMETER( retries );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_SUCCESS;
}

wiced_result_t wiced_i2c_transfer( wiced_i2c_device_t* device, wiced_i2c_message_t* message, uint16_t number_of_messages )
{
    UNUSED_PARAMETER( device );
    UNUSED_PARAMETER( message );
    UNUSED_PARAMETER( number_of_messages );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_SUCCESS;
}

wiced_result_t wiced_i2c_deinit( wiced_i2c_device_t* device )
{
    UNUSED_PARAMETER( device );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_SUCCESS;
}

wiced_result_t wiced_adc_init( wiced_adc_t adc, uint32_t sample_cycle )
{
    /* Initialize the associated GPIO */
    GPIO_InitTypeDef gpio_init_structure;
    ADC_InitTypeDef adc_init_structure;
    ADC_CommonInitTypeDef adc_common_init_structure;
    uint8_t a;

    gpio_init_structure.GPIO_Pin   = (uint16_t) ( 1 << adc_mapping[adc].pin->number );
    gpio_init_structure.GPIO_Speed = (GPIOSpeed_TypeDef) 0;
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_AN;
    gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    gpio_init_structure.GPIO_OType = GPIO_OType_OD;
    GPIO_Init( adc_mapping[adc].pin->bank, &gpio_init_structure );

    /* Ensure the ADC and GPIOA are enabled */
    RCC_AHB1PeriphClockCmd( adc_mapping[adc].pin->peripheral_clock, ENABLE);
    RCC_APB2PeriphClockCmd( adc_mapping[adc].adc_peripheral_clock, ENABLE );

    /* Initialize the ADC */
    ADC_StructInit( &adc_init_structure );
    adc_init_structure.ADC_Resolution         = ADC_Resolution_12b;
    adc_init_structure.ADC_ScanConvMode       = DISABLE;
    adc_init_structure.ADC_ContinuousConvMode = DISABLE;
    adc_init_structure.ADC_ExternalTrigConv   = ADC_ExternalTrigConvEdge_None;
    adc_init_structure.ADC_DataAlign          = ADC_DataAlign_Right;
    adc_init_structure.ADC_NbrOfConversion    = 1;
    ADC_Init( adc_mapping[adc].adc, &adc_init_structure );

    ADC_CommonStructInit(&adc_common_init_structure);
    adc_common_init_structure.ADC_Mode             = ADC_Mode_Independent;
    adc_common_init_structure.ADC_DMAAccessMode    = ADC_DMAAccessMode_Disabled;
    adc_common_init_structure.ADC_Prescaler        = ADC_Prescaler_Div2;
    adc_common_init_structure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&adc_common_init_structure);

    ADC_Cmd( adc_mapping[adc].adc, ENABLE );

    /* Find the closest supported sampling time by the MCU */
    for ( a = 0; ( a < sizeof( adc_sampling_cycle ) / sizeof(uint16_t) ) && adc_sampling_cycle[a] < sample_cycle; a++ )
    {
    }

    /* Initialize the ADC channel */
    ADC_RegularChannelConfig( adc_mapping[adc].adc, adc_mapping[adc].channel, adc_mapping[adc].rank, (uint8_t)a );

    return WICED_SUCCESS;
}

wiced_result_t wiced_adc_take_sample( wiced_adc_t adc, uint16_t* output )
{
    /* Start conversion */
    ADC_SoftwareStartConv( adc_mapping[adc].adc );

    /* Wait until end of conversion */
    while ( ADC_GetFlagStatus( adc_mapping[adc].adc, ADC_FLAG_EOC ) == RESET )
    {
    }

    /* Read ADC conversion result */
    *output = ADC_GetConversionValue( adc_mapping[adc].adc );

    return WICED_SUCCESS;
}

wiced_result_t wiced_adc_take_sample_stream( wiced_adc_t adc, void* buffer, uint16_t buffer_length )
{
    UNUSED_PARAMETER(adc);
    UNUSED_PARAMETER(buffer);
    UNUSED_PARAMETER(buffer_length);
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_SUCCESS;
}

wiced_result_t wiced_adc_deinit( wiced_adc_t adc )
{
    UNUSED_PARAMETER(adc);
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_SUCCESS;
}

wiced_result_t wiced_gpio_init( wiced_gpio_t gpio, wiced_gpio_config_t configuration )
{
    GPIO_InitTypeDef gpio_init_structure;

    RCC_AHB1PeriphClockCmd( gpio_mapping[gpio].peripheral_clock, ENABLE );
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init_structure.GPIO_Mode  = ( (configuration == INPUT_PULL_UP ) || (configuration == INPUT_PULL_DOWN ) || (configuration == INPUT_HIGH_IMPEDANCE ) ) ? GPIO_Mode_IN : GPIO_Mode_OUT;
    gpio_init_structure.GPIO_OType = (configuration == OUTPUT_PUSH_PULL )? GPIO_OType_PP :  GPIO_OType_OD;

    if ( (configuration == INPUT_PULL_UP ) || (configuration == OUTPUT_OPEN_DRAIN_PULL_UP ) )
    {
        gpio_init_structure.GPIO_PuPd  =  GPIO_PuPd_UP;
    }
    else if (configuration == INPUT_PULL_DOWN )
    {
        gpio_init_structure.GPIO_PuPd  =  GPIO_PuPd_DOWN;
    }
    else
    {
        gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    }

    gpio_init_structure.GPIO_Pin = (uint16_t) ( 1 << gpio_mapping[gpio].number );
    GPIO_Init( gpio_mapping[gpio].bank, &gpio_init_structure );
    return WICED_SUCCESS;
}

wiced_result_t wiced_gpio_output_high( wiced_gpio_t gpio )
{
    GPIO_SetBits( gpio_mapping[gpio].bank, (uint16_t) (1 << gpio_mapping[gpio].number) );
    return WICED_SUCCESS;
}

wiced_result_t wiced_gpio_output_low( wiced_gpio_t gpio )
{
    GPIO_ResetBits( gpio_mapping[gpio].bank, (uint16_t) (1 << gpio_mapping[gpio].number) );
    return WICED_SUCCESS;
}

wiced_bool_t wiced_gpio_input_get( wiced_gpio_t gpio )
{
    return GPIO_ReadInputDataBit( gpio_mapping[gpio].bank, (uint16_t) ( 1 << gpio_mapping[gpio].number ) );
}

wiced_result_t wiced_gpio_input_irq_enable( wiced_gpio_t gpio, wiced_gpio_irq_trigger_t trigger, wiced_gpio_irq_handler_t handler, void* arg )
{
    return gpio_irq_enable( gpio_mapping[gpio].bank, gpio_mapping[gpio].number, trigger, handler, arg );
}

wiced_result_t wiced_gpio_input_irq_disable( wiced_gpio_t gpio )
{
    return gpio_irq_disable( gpio_mapping[gpio].bank, gpio_mapping[gpio].number );
}

wiced_result_t wiced_pwm_init( wiced_pwm_t pwm_peripheral, uint32_t frequency, float duty_cycle )
{
    TIM_TimeBaseInitTypeDef tim_time_base_structure;
    TIM_OCInitTypeDef       tim_oc_init_structure;
    GPIO_InitTypeDef        gpio_init_structure;
    RCC_ClocksTypeDef       rcc_clock_frequencies;
    const platform_pwm_mapping_t* pwm                 = &pwm_mappings[pwm_peripheral];
    uint16_t                      period              = 0;
    float                         adjusted_duty_cycle = ( ( duty_cycle > 100.0f ) ? 100.0f : duty_cycle );

    RCC_GetClocksFreq( &rcc_clock_frequencies );

    if ( pwm->tim == TIM1 || pwm->tim == TIM8 || pwm->tim == TIM9 || pwm->tim == TIM10 || pwm->tim == TIM11 )
    {
        RCC_APB2PeriphClockCmd( pwm->tim_peripheral_clock, ENABLE );
        period = (uint16_t)( rcc_clock_frequencies.PCLK2_Frequency / frequency - 1 ); /* Auto-reload value counts from 0; hence the minus 1 */
    }
    else
    {
        RCC_APB1PeriphClockCmd( pwm->tim_peripheral_clock, ENABLE );
        period = (uint16_t)( rcc_clock_frequencies.PCLK1_Frequency / frequency - 1 ); /* Auto-reload value counts from 0; hence the minus 1 */
    }

    RCC_AHB1PeriphClockCmd( pwm->pin->peripheral_clock, ENABLE );

    gpio_init_structure.GPIO_Pin   = (uint32_t) ( 1 << pwm->pin->number );
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_AF;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_100MHz;
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;
    gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init( pwm->pin->bank, &gpio_init_structure );
    GPIO_PinAFConfig( pwm->pin->bank, pwm->pin->number, pwm->gpio_af );

    /* Time base configuration */
    tim_time_base_structure.TIM_Period            = (uint32_t) period;
    tim_time_base_structure.TIM_Prescaler         = (uint16_t) 1;  /* Divide clock by 1+1 to enable a count of high cycle + low cycle = 1 PWM cycle */
    tim_time_base_structure.TIM_ClockDivision     = 0;
    tim_time_base_structure.TIM_CounterMode       = TIM_CounterMode_Up;
    tim_time_base_structure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit( pwm->tim, &tim_time_base_structure );

    /* PWM1 Mode configuration */
    tim_oc_init_structure.TIM_OCMode       = TIM_OCMode_PWM1;
    tim_oc_init_structure.TIM_OutputState  = TIM_OutputState_Enable;
    tim_oc_init_structure.TIM_OutputNState = TIM_OutputNState_Enable;
    tim_oc_init_structure.TIM_Pulse        = (uint16_t) ( adjusted_duty_cycle * (float) period / 100.0f );
    tim_oc_init_structure.TIM_OCPolarity   = TIM_OCPolarity_High;
    tim_oc_init_structure.TIM_OCNPolarity  = TIM_OCNPolarity_High;
    tim_oc_init_structure.TIM_OCIdleState  = TIM_OCIdleState_Reset;
    tim_oc_init_structure.TIM_OCNIdleState = TIM_OCIdleState_Set;

    switch ( pwm->channel )
    {
        case 1:
        {
            TIM_OC1Init( pwm->tim, &tim_oc_init_structure );
            TIM_OC1PreloadConfig( pwm->tim, TIM_OCPreload_Enable );
            break;
        }
        case 2:
        {
            TIM_OC2Init( pwm->tim, &tim_oc_init_structure );
            TIM_OC2PreloadConfig( pwm->tim, TIM_OCPreload_Enable );
            break;
        }
        case 3:
        {
            TIM_OC3Init( pwm->tim, &tim_oc_init_structure );
            TIM_OC3PreloadConfig( pwm->tim, TIM_OCPreload_Enable );
            break;
        }
        case 4:
        {
            TIM_OC4Init( pwm->tim, &tim_oc_init_structure );
            TIM_OC4PreloadConfig( pwm->tim, TIM_OCPreload_Enable );
            break;
        }
        default:
        {
            break;
        }
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_pwm_start( wiced_pwm_t pwm )
{
    TIM_Cmd(pwm_mappings[pwm].tim, ENABLE);
    TIM_CtrlPWMOutputs( pwm_mappings[pwm].tim, ENABLE );
    return WICED_SUCCESS;
}

wiced_result_t wiced_pwm_stop( wiced_pwm_t pwm )
{
    TIM_CtrlPWMOutputs( pwm_mappings[pwm].tim, DISABLE );
    TIM_Cmd(pwm_mappings[pwm].tim, DISABLE);
    return WICED_SUCCESS;
}

wiced_result_t wiced_watchdog_kick( void )
{
    return watchdog_kick();
}

wiced_result_t wiced_uart_init( wiced_uart_t uart, const wiced_uart_config_t* config, wiced_ring_buffer_t* optional_rx_buffer )
{
#ifndef WICED_DISABLE_STDIO
    if (uart == STDIO_UART)
    {
        return WICED_ERROR;
    }
#endif

    return platform_uart_init(uart, config, optional_rx_buffer);
}

static wiced_result_t platform_uart_init( wiced_uart_t uart, const wiced_uart_config_t* config, wiced_ring_buffer_t* optional_rx_buffer )
{
    GPIO_InitTypeDef  gpio_init_structure;
    USART_InitTypeDef usart_init_structure;
    NVIC_InitTypeDef  nvic_init_structure;
    DMA_InitTypeDef   dma_init_structure;

    host_rtos_init_semaphore(&uart_interfaces[uart].tx_complete);
    host_rtos_init_semaphore(&uart_interfaces[uart].rx_complete);

    /* Enable GPIO peripheral clocks for TX and RX pins */
    RCC_AHB1PeriphClockCmd( uart_mapping[uart].pin_rx->peripheral_clock, ENABLE );
    RCC_AHB1PeriphClockCmd( uart_mapping[uart].pin_tx->peripheral_clock, ENABLE );

    /* Configure USART TX Pin */
    gpio_init_structure.GPIO_Pin   = (uint32_t) ( 1 << uart_mapping[uart].pin_tx->number );
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_AF;
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;
    gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( uart_mapping[uart].pin_tx->bank, &gpio_init_structure );
    GPIO_PinAFConfig( uart_mapping[uart].pin_tx->bank, uart_mapping[uart].pin_tx->number, uart_mapping[uart].gpio_af );

    /* Configure USART RX Pin */
    gpio_init_structure.GPIO_Pin   = (uint32_t) ( 1 << uart_mapping[uart].pin_rx->number );
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_AF;
    gpio_init_structure.GPIO_OType = GPIO_OType_OD;
    gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init( uart_mapping[uart].pin_rx->bank, &gpio_init_structure );
    GPIO_PinAFConfig( uart_mapping[uart].pin_rx->bank, uart_mapping[uart].pin_rx->number, uart_mapping[uart].gpio_af );

    /* Check if any of the flow control is enabled */
    if ( uart_mapping[uart].pin_cts && (config->flow_control == FLOW_CONTROL_CTS || config->flow_control == FLOW_CONTROL_CTS_RTS) )
    {
        /* Enable peripheral clock */
        RCC_AHB1PeriphClockCmd( uart_mapping[uart].pin_cts->peripheral_clock, ENABLE );

        /* Configure CTS Pin */
        gpio_init_structure.GPIO_Pin   = (uint32_t) ( 1 << uart_mapping[uart].pin_cts->number );
        gpio_init_structure.GPIO_Mode  = GPIO_Mode_AF;
        gpio_init_structure.GPIO_OType = GPIO_OType_OD;
        gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
        GPIO_Init( uart_mapping[uart].pin_cts->bank, &gpio_init_structure );
        GPIO_PinAFConfig( uart_mapping[uart].pin_cts->bank, uart_mapping[uart].pin_cts->number, uart_mapping[uart].gpio_af );
    }

    if ( uart_mapping[uart].pin_cts && (config->flow_control == FLOW_CONTROL_RTS || config->flow_control == FLOW_CONTROL_CTS_RTS) )
    {
        /* Enable peripheral clock */
        RCC_AHB1PeriphClockCmd( uart_mapping[uart].pin_rts->peripheral_clock, ENABLE );

        /* Configure RTS Pin */
        gpio_init_structure.GPIO_Pin   = (uint32_t) ( 1 << uart_mapping[uart].pin_rts->number );
        gpio_init_structure.GPIO_Mode  = GPIO_Mode_AF;
        gpio_init_structure.GPIO_OType = GPIO_OType_OD;
        gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
        GPIO_Init( uart_mapping[uart].pin_rts->bank, &gpio_init_structure );
        GPIO_PinAFConfig( uart_mapping[uart].pin_rts->bank, uart_mapping[uart].pin_rts->number, uart_mapping[uart].gpio_af );
    }

    /* Enable UART peripheral clock */
    uart_mapping[uart].usart_peripheral_clock_func( uart_mapping[uart].usart_peripheral_clock, ENABLE );

    /**************************************************************************
     * Initialise STM32 USART registers
     * NOTE:
     * - Both transmitter and receiver are disabled until usart_enable_transmitter/receiver is called.
     * - Only 1 and 2 stop bits are implemented at the moment.
     **************************************************************************/
    usart_init_structure.USART_Mode       = 0;
    usart_init_structure.USART_BaudRate   = config->baud_rate;
    usart_init_structure.USART_WordLength = ( ( config->data_width == DATA_WIDTH_9BIT ) ||
                                              ( ( config->data_width == DATA_WIDTH_8BIT ) && ( config->parity != NO_PARITY ) ) ) ? USART_WordLength_9b : USART_WordLength_8b;
    usart_init_structure.USART_StopBits   = ( config->stop_bits == STOP_BITS_1 ) ? USART_StopBits_1 : USART_StopBits_2;

    switch ( config->parity )
    {
        case NO_PARITY:
            usart_init_structure.USART_Parity = USART_Parity_No;
            break;
        case EVEN_PARITY:
            usart_init_structure.USART_Parity = USART_Parity_Even;
            break;
        case ODD_PARITY:
            usart_init_structure.USART_Parity = USART_Parity_Odd;
            break;
        default:
            return WICED_BADARG;
    }

    switch ( config->flow_control )
    {
        case FLOW_CONTROL_DISABLED:
            usart_init_structure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
            break;
        case FLOW_CONTROL_CTS:
            usart_init_structure.USART_HardwareFlowControl = USART_HardwareFlowControl_CTS;
            break;
        case FLOW_CONTROL_RTS:
            usart_init_structure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS;
            break;
        case FLOW_CONTROL_CTS_RTS:
            usart_init_structure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
            break;
        default:
            return WICED_BADARG;
    }

    /* Initialise USART peripheral */
    USART_Init( uart_mapping[uart].usart, &usart_init_structure );


    /**************************************************************************
     * Initialise STM32 DMA registers
     * Note: If DMA is used, USART interrupt isn't enabled.
     **************************************************************************/
    /* Enable DMA peripheral clock */
    uart_mapping[uart].tx_dma_peripheral_clock_func( uart_mapping[uart].tx_dma_peripheral_clock, ENABLE );

    /* Fill init structure with common DMA settings */
    dma_init_structure.DMA_PeripheralInc   = DMA_PeripheralInc_Disable;
    dma_init_structure.DMA_MemoryInc       = DMA_MemoryInc_Enable;
    dma_init_structure.DMA_Priority        = DMA_Priority_VeryHigh;
    dma_init_structure.DMA_FIFOMode        = DMA_FIFOMode_Disable;
    dma_init_structure.DMA_FIFOThreshold   = DMA_FIFOThreshold_Full;
    dma_init_structure.DMA_MemoryBurst     = DMA_MemoryBurst_Single;
    dma_init_structure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

    if ( config->data_width == DATA_WIDTH_9BIT )
    {
        dma_init_structure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
        dma_init_structure.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
    }
    else
    {
        dma_init_structure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        dma_init_structure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    }

    /* Initialise TX DMA */
    DMA_DeInit( uart_mapping[uart].tx_dma_stream );
    dma_init_structure.DMA_Channel            = uart_mapping[uart].tx_dma_channel;
    dma_init_structure.DMA_PeripheralBaseAddr = (uint32_t) &uart_mapping[uart].usart->DR;
    dma_init_structure.DMA_Memory0BaseAddr    = (uint32_t) 0;
    dma_init_structure.DMA_DIR                = DMA_DIR_MemoryToPeripheral;
    dma_init_structure.DMA_BufferSize         = 0;
    dma_init_structure.DMA_Mode               = DMA_Mode_Normal;
    DMA_Init( uart_mapping[uart].tx_dma_stream, &dma_init_structure );

    /* Initialise RX DMA */
    DMA_DeInit( uart_mapping[uart].rx_dma_stream );
    dma_init_structure.DMA_Channel            = uart_mapping[uart].rx_dma_channel;
    dma_init_structure.DMA_PeripheralBaseAddr = (uint32_t) &uart_mapping[uart].usart->DR;
    dma_init_structure.DMA_Memory0BaseAddr    = 0;
    dma_init_structure.DMA_DIR                = DMA_DIR_PeripheralToMemory;
    dma_init_structure.DMA_BufferSize         = 0;
    dma_init_structure.DMA_Mode               = DMA_Mode_Normal;
    DMA_Init( uart_mapping[uart].rx_dma_stream, &dma_init_structure );

    /**************************************************************************
     * Initialise STM32 DMA interrupts
     * Note: Only TX DMA interrupt is enabled.
     **************************************************************************/

    /* Configure TX DMA interrupt on Cortex-M3 */
    nvic_init_structure.NVIC_IRQChannel                   = uart_mapping[uart].tx_dma_irq;
    nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0x5;
    nvic_init_structure.NVIC_IRQChannelSubPriority        = 0x8;
    nvic_init_structure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init( &nvic_init_structure );

    /* Enable TC (transfer complete) and TE (transfer error) interrupts on source */
    DMA_ITConfig( uart_mapping[uart].tx_dma_stream, DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_IT_FE, ENABLE );

    /* Enable USART's RX DMA interfaces */
    USART_DMACmd( uart_mapping[uart].usart, USART_DMAReq_Rx, ENABLE );

    /**************************************************************************
     * Initialise STM32 USART interrupt
     **************************************************************************/
    nvic_init_structure.NVIC_IRQChannel                   = uart_mapping[uart].usart_irq;
    nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0x6;
    nvic_init_structure.NVIC_IRQChannelSubPriority        = 0x7;
    nvic_init_structure.NVIC_IRQChannelCmd                = ENABLE;

    /* Enable USART interrupt vector in Cortex-M3 */
    NVIC_Init( &nvic_init_structure );

    /* Enable USART */
    USART_Cmd( uart_mapping[uart].usart, ENABLE );

    /* Enable both transmit and receive */
    uart_mapping[uart].usart->CR1 |= USART_CR1_TE;
    uart_mapping[uart].usart->CR1 |= USART_CR1_RE;

    /* Setup ring buffer */
    if (optional_rx_buffer != NULL)
    {
        /* Note that the ring_buffer should've been initialised first */
        uart_interfaces[uart].rx_buffer = optional_rx_buffer;
        uart_interfaces[uart].rx_size   = 0;
        platform_uart_receive_bytes( uart, optional_rx_buffer->buffer, optional_rx_buffer->size, 0 );
    }
    else
    {
        /* Not using ring buffer. Configure RX DMA interrupt on Cortex-M3 */
        nvic_init_structure.NVIC_IRQChannel                   = uart_mapping[uart].rx_dma_irq;
        nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0x5;
        nvic_init_structure.NVIC_IRQChannelSubPriority        = 0x8;
        nvic_init_structure.NVIC_IRQChannelCmd                = ENABLE;
        NVIC_Init( &nvic_init_structure );

        /* Enable TC (transfer complete) and TE (transfer error) interrupts on source */
        DMA_ITConfig( uart_mapping[uart].rx_dma_stream, DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_IT_FE, ENABLE );
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_uart_deinit( wiced_uart_t uart )
{
    NVIC_InitTypeDef nvic_init_structure;

    /* Disable USART */
    USART_Cmd( uart_mapping[uart].usart, DISABLE );

    /* Deinitialise USART */
    USART_DeInit( uart_mapping[uart].usart );

    /**************************************************************************
     * De-initialise STM32 DMA and interrupt
     **************************************************************************/

    /* Deinitialise DMA streams */
    DMA_DeInit( uart_mapping[uart].tx_dma_stream );
    DMA_DeInit( uart_mapping[uart].rx_dma_stream );

    /* Disable TC (transfer complete) interrupt at the source */
    DMA_ITConfig( uart_mapping[uart].tx_dma_stream, DMA_IT_TC | DMA_IT_TE, DISABLE );

    /* Disable transmit DMA interrupt at Cortex-M3 */
    nvic_init_structure.NVIC_IRQChannel                   = uart_mapping[uart].tx_dma_irq;
    nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0x5;
    nvic_init_structure.NVIC_IRQChannelSubPriority        = 0x8;
    nvic_init_structure.NVIC_IRQChannelCmd                = DISABLE;
    NVIC_Init( &nvic_init_structure );

    /* Disable DMA peripheral clocks */
    uart_mapping[uart].tx_dma_peripheral_clock_func( uart_mapping[uart].tx_dma_peripheral_clock, DISABLE );
    uart_mapping[uart].rx_dma_peripheral_clock_func( uart_mapping[uart].rx_dma_peripheral_clock, DISABLE );

    /**************************************************************************
     * De-initialise STM32 USART interrupt
     **************************************************************************/

    USART_ITConfig( uart_mapping[uart].usart, USART_IT_RXNE, DISABLE );

    /* Disable UART interrupt vector on Cortex-M3 */
    nvic_init_structure.NVIC_IRQChannel                   = uart_mapping[uart].usart_irq;
    nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0xf;
    nvic_init_structure.NVIC_IRQChannelSubPriority        = 0xf;
    nvic_init_structure.NVIC_IRQChannelCmd                = DISABLE;
    NVIC_Init( &nvic_init_structure );

    /* Disable registers clocks */
    uart_mapping[uart].usart_peripheral_clock_func( uart_mapping[uart].usart_peripheral_clock, DISABLE );

    host_rtos_deinit_semaphore(&uart_interfaces[uart].rx_complete);
    host_rtos_deinit_semaphore(&uart_interfaces[uart].tx_complete);

    return WICED_SUCCESS;
}

wiced_result_t wiced_uart_transmit_bytes( wiced_uart_t uart, const void* data, uint32_t size )
{
    /* Reset DMA transmission result. The result is assigned in interrupt handler */
    uart_interfaces[uart].tx_dma_result = WICED_ERROR;


    uart_mapping[uart].tx_dma_stream->CR &= ~(uint32_t) DMA_SxCR_CIRC;
    uart_mapping[uart].tx_dma_stream->NDTR = size;
    uart_mapping[uart].tx_dma_stream->M0AR = (uint32_t)data;

    USART_DMACmd( uart_mapping[uart].usart, USART_DMAReq_Tx, ENABLE );
    USART_ClearFlag( uart_mapping[uart].usart, USART_FLAG_TC );
    DMA_Cmd( uart_mapping[uart].tx_dma_stream, ENABLE );

    host_rtos_get_semaphore( &uart_interfaces[ uart ].tx_complete, WICED_NEVER_TIMEOUT, WICED_TRUE );

    while ( ( uart_mapping[ uart ].usart->SR & USART_SR_TC ) == 0 )
    {
    }

    DMA_Cmd( uart_mapping[uart].tx_dma_stream, DISABLE );
    USART_DMACmd( uart_mapping[uart].usart, USART_DMAReq_Tx, DISABLE );

    return uart_interfaces[uart].tx_dma_result;
}

wiced_result_t wiced_uart_receive_bytes( wiced_uart_t uart, void* data, uint32_t size, uint32_t timeout )
{
    if (uart_interfaces[uart].rx_buffer != NULL)
    {
        while (size != 0)
        {
            uint32_t transfer_size = MIN(uart_interfaces[uart].rx_buffer->size / 2, size);

            /* Check if ring buffer already contains the required amount of data. */
            if ( transfer_size > ring_buffer_used_space( uart_interfaces[uart].rx_buffer ) )
            {
                /* Set rx_size and wait in rx_complete semaphore until data reaches rx_size or timeout occurs */
                uart_interfaces[uart].rx_size = transfer_size;

                if ( host_rtos_get_semaphore( &uart_interfaces[uart].rx_complete, timeout, WICED_TRUE ) != WICED_SUCCESS )
                {
                    uart_interfaces[uart].rx_size = 0;
                    return WICED_TIMEOUT;
                }

                /* Reset rx_size to prevent semaphore being set while nothing waits for the data */
                uart_interfaces[uart].rx_size = 0;
            }

            size -= transfer_size;

            // Grab data from the buffer
            do
            {
                uint8_t* available_data;
                uint32_t bytes_available;

                ring_buffer_get_data( uart_interfaces[uart].rx_buffer, &available_data, &bytes_available );
                bytes_available = MIN( bytes_available, transfer_size );
                memcpy( data, available_data, bytes_available );
                transfer_size -= bytes_available;
                data = ( (uint8_t*) data + bytes_available );
                ring_buffer_consume( uart_interfaces[uart].rx_buffer, bytes_available );
            } while ( transfer_size != 0 );
        }

        if ( size != 0 )
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
        return platform_uart_receive_bytes( uart, data, size, timeout );
    }
}

static wiced_result_t platform_uart_receive_bytes( wiced_uart_t uart, void* data, uint32_t size, uint32_t timeout )
{
    if ( uart_interfaces[uart].rx_buffer != NULL )
    {
        uart_mapping[uart].rx_dma_stream->CR |= DMA_SxCR_CIRC;

        // Enabled individual byte interrupts so progress can be updated
        USART_ITConfig( uart_mapping[uart].usart, USART_IT_RXNE, ENABLE );
    }
    else
    {
        uart_mapping[uart].rx_dma_stream->CR &= ~(uint32_t) DMA_SxCR_CIRC;
    }

    /* Reset DMA transmission result. The result is assigned in interrupt handler */
    uart_interfaces[uart].rx_dma_result = WICED_ERROR;

    uart_mapping[uart].rx_dma_stream->NDTR = size;
    uart_mapping[uart].rx_dma_stream->M0AR = (uint32_t)data;
    uart_mapping[uart].rx_dma_stream->CR  |= DMA_SxCR_EN;

    if ( timeout > 0 )
    {
        host_rtos_get_semaphore( &uart_interfaces[uart].rx_complete, timeout, WICED_TRUE );
        return uart_interfaces[uart].rx_dma_result;
    }

    return WICED_SUCCESS;
}

/******************************************************
 *                 DCT Functions
 ******************************************************/

/******************************************************
 *                 DCT Functions
 ******************************************************/

#ifndef WICED_DISABLE_BOOTLOADER
platform_dct_data_t* platform_get_dct( void )
{
    platform_dct_header_t hdr =
    {
        .write_incomplete    = 0,
        .is_current_dct      = 1,
        .app_valid           = 1,
        .mfg_info_programmed = 0,
        .magic_number        = BOOTLOADER_MAGIC_NUMBER,
        .load_app_func       = NULL
    };

    platform_dct_header_t* dct1 = ((platform_dct_header_t*) PLATFORM_DCT_COPY1_START_ADDRESS);
    platform_dct_header_t* dct2 = ((platform_dct_header_t*) PLATFORM_DCT_COPY2_START_ADDRESS);

    if ( ( dct1->is_current_dct == 1 ) &&
         ( dct1->write_incomplete == 0 ) &&
         ( dct1->magic_number == BOOTLOADER_MAGIC_NUMBER ) )
    {
        return (platform_dct_data_t*)dct1;
    }

    if ( ( dct2->is_current_dct == 1 ) &&
         ( dct2->write_incomplete == 0 ) &&
         ( dct2->magic_number == BOOTLOADER_MAGIC_NUMBER ) )
    {
        return (platform_dct_data_t*)dct2;
    }

    /* No valid DCT! */
    /* Erase the first DCT and init it. */
    ERASE_DCT_1();
//    platform_bootloader_erase_dct( 1 );

    platform_write_flash_chunk( PLATFORM_DCT_COPY1_START_ADDRESS, (uint8_t*) &hdr, sizeof(hdr) );

    return (platform_dct_data_t*)dct1;
}

void platform_erase_dct( void )
{
    ERASE_DCT_1();
    ERASE_DCT_2();
}

/* TODO: Disable interrupts during function */
/* Note Function allocates a chunk of memory for the bootloader data on the stack - ensure the stack is big enough */
int platform_write_dct( uint16_t data_start_offset, const void* data, uint16_t data_length, int8_t app_valid, void (*func)(void) )
{
    platform_dct_header_t* curr_dct = &platform_get_dct( )->dct_header;
    platform_dct_header_t* new_dct;
    uint32_t bytes_after_data;
    uint8_t* new_app_data_addr;
    uint8_t* curr_app_data_addr;
    char zero_byte = 0;
    platform_dct_header_t hdr =
        {
            .write_incomplete = 1,
            .is_current_dct   = 1,
            .magic_number     = BOOTLOADER_MAGIC_NUMBER
        };

    if ( data_length + data_start_offset > ( PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS ) )
    {
        /* Data too big to write */
        return -1;
    }

    if ( curr_dct == ((platform_dct_header_t*)PLATFORM_DCT_COPY1_START_ADDRESS) )
    {
        new_dct = (platform_dct_header_t*)PLATFORM_DCT_COPY2_START_ADDRESS;
        /* Erase the non-current dct */
        ERASE_DCT_2();
    }
    else
    {
        new_dct = (platform_dct_header_t*)PLATFORM_DCT_COPY1_START_ADDRESS;
        /* Erase the non-current dct */
        ERASE_DCT_1();
    }

    /* Write the mfg data and initial part of app data before provided data */
    platform_write_flash_chunk( ((uint32_t) &new_dct[1]), (uint8_t*) &curr_dct[1], data_start_offset );

    /* Verify the mfg data */
    if ( 0 != memcmp( &new_dct[1], &curr_dct[1], data_start_offset ) )
    {
        /* Error writing mfg data */
        return -2;
    }

    /* Write the app data */
    new_app_data_addr  =  (uint8_t*) &new_dct[1]  + data_start_offset;
    curr_app_data_addr =  (uint8_t*) &curr_dct[1] + data_start_offset;

    platform_write_flash_chunk( (uint32_t)new_app_data_addr, data, data_length );
    /* Verify the app data */
    if ( 0 != memcmp( new_app_data_addr, data, data_length ) )
    {
        /* Error writing app data */
        return -3;
    }

    bytes_after_data = ( PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS ) - (sizeof(platform_dct_header_t) + data_start_offset + data_length );

    if ( bytes_after_data != 0 )
    {
        new_app_data_addr += data_length;
        curr_app_data_addr += data_length;

        platform_write_flash_chunk( (uint32_t)new_app_data_addr, curr_app_data_addr, bytes_after_data );
        /* Verify the app data */
        if ( 0 != memcmp( new_app_data_addr, curr_app_data_addr, bytes_after_data ) )
        {
            /* Error writing app data */
            return -4;
        }
    }



    hdr.app_valid = (char) (( app_valid == -1 )? curr_dct->app_valid : app_valid);
    hdr.load_app_func = func;
    hdr.mfg_info_programmed = curr_dct->mfg_info_programmed;

    /* Write the header data */
    platform_write_flash_chunk( (uint32_t)new_dct, (uint8_t*) &hdr, sizeof(hdr) );

    /* Verify the header data */
    if ( 0 != memcmp( new_dct, &hdr, sizeof(hdr) ) )
    {
        /* Error writing header data */
        return -5;
    }

    /* Mark new DCT as complete and current */
    platform_write_flash_chunk( (uint32_t)&new_dct->write_incomplete, (uint8_t*) &zero_byte, sizeof(zero_byte) );
    platform_write_flash_chunk( (uint32_t)&curr_dct->is_current_dct, (uint8_t*) &zero_byte, sizeof(zero_byte) );

    return 0;
}

int platform_erase_flash( uint16_t start_sector, uint16_t end_sector )
{
    uint32_t i;

    /* Unlock the STM32 Flash */
    FLASH_Unlock( );

    /* Clear any error flags */
    FLASH_ClearFlag( FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR );

    watchdog_kick( );

    for ( i = start_sector; i <= end_sector; i += 8 )
    {
        if ( FLASH_EraseSector( i, ERASE_VOLTAGE_RANGE ) != FLASH_COMPLETE )
        {
            /* Error occurred during erase. */
            /* TODO: Handle error */
            while ( 1 )
            {
            }
        }
        watchdog_kick( );
    }

    FLASH_Lock( );

    return 0;
}

int platform_write_app_chunk( uint32_t offset, const uint8_t* data, uint32_t size )
{
    return platform_write_flash_chunk( APP_HDR_START_ADDR + offset, data, size );
}

int platform_write_flash_chunk( uint32_t address, const uint8_t* data, uint32_t size )
{
    uint32_t write_address = address;
    flash_write_t* data_ptr = (flash_write_t*) data;
    flash_write_t* end_ptr  = (flash_write_t*) &data[size];

    FLASH_Unlock( );

    /* Clear any error flags */
    FLASH_ClearFlag( FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR );

    /* Write data to STM32 flash memory */
    while ( data_ptr <  end_ptr )
    {
        FLASH_Status status;

        if ( ( ( ((uint32_t)write_address) & 0x03 ) == 0 ) && ( end_ptr - data_ptr >= FLASH_WRITE_SIZE ) )
        {
            int tries = 0;
            /* enough data available to write as the largest size allowed by supply voltage */
            while ( ( FLASH_COMPLETE != ( status = FLASH_WRITE_FUNC( write_address, *data_ptr ) ) ) && ( tries < 10 ) )
            {
                tries++;
            }
            if ( FLASH_COMPLETE != status )
            {
                /* TODO: Handle error properly */
                wiced_assert("Error during write", 0 != 0 );
            }
            write_address += FLASH_WRITE_SIZE;
            data_ptr++;
        }
        else
        {
            int tries = 0;
            /* Limited data available - write in bytes */
            while ( ( FLASH_COMPLETE != ( status = FLASH_ProgramByte( write_address, (uint8_t) *data_ptr ) ) ) && ( tries < 10 ) )
            {
                tries++;
            }
            if ( FLASH_COMPLETE != status )
            {
                /* TODO: Handle error properly */
                wiced_assert("Error during write", 0 != 0 );
            }
            write_address++;
            data_ptr = (flash_write_t*)((uint32_t)data_ptr+1);
        }

    }

    FLASH_Lock();

    return 0;
}
#endif

/******************************************************
 *            Interrupt Service Routines
 ******************************************************/
void usart1_irq( void )
{
    // Clear all interrupts. It's safe to do so because only RXNE interrupt is enabled
    USART1->SR = (uint16_t) (USART1->SR | 0xffff);

    // Update tail
    uart_interfaces[0].rx_buffer->tail = uart_interfaces[0].rx_buffer->size - uart_mapping[0].rx_dma_stream->NDTR;

    // Notify thread if sufficient data are available
    if ( ( uart_interfaces[0].rx_size > 0 ) && ( ring_buffer_used_space( uart_interfaces[0].rx_buffer ) >= uart_interfaces[0].rx_size ) )
    {
        host_rtos_set_semaphore( &uart_interfaces[0].rx_complete, WICED_TRUE );
        uart_interfaces[0].rx_size = 0;
    }
}

void usart2_irq( void )
{
    // Clear all interrupts. It's safe to do so because only RXNE interrupt is enabled
    USART2->SR = (uint16_t) (USART2->SR | 0xffff);

    // Update tail
    uart_interfaces[1].rx_buffer->tail = uart_interfaces[1].rx_buffer->size - uart_mapping[1].rx_dma_stream->NDTR;

    // Notify thread if sufficient data are available
    if ( ( uart_interfaces[1].rx_size > 0 ) && ( ring_buffer_used_space( uart_interfaces[1].rx_buffer ) >= uart_interfaces[1].rx_size ) )
    {
        host_rtos_set_semaphore( &uart_interfaces[1].rx_complete, WICED_TRUE );
        uart_interfaces[1].rx_size = 0;
    }
}

void usart6_irq( void )
{
    // Clear all interrupts. It's safe to do so because only RXNE interrupt is enabled
    USART6->SR = (uint16_t) (USART6->SR | 0xffff);

    // Update tail
    uart_interfaces[0].rx_buffer->tail = uart_interfaces[0].rx_buffer->size - uart_mapping[0].rx_dma_stream->NDTR;

    // Notify thread if sufficient data are available
    if ( ( uart_interfaces[0].rx_size > 0 ) && ( ring_buffer_used_space( uart_interfaces[0].rx_buffer ) >= uart_interfaces[0].rx_size ) )
    {
        host_rtos_set_semaphore( &uart_interfaces[0].rx_complete, WICED_TRUE );
        uart_interfaces[0].rx_size = 0;
    }
}

void usart1_tx_dma_irq( void )
{
    wiced_bool_t tx_complete = WICED_FALSE;

    if ( ( DMA2->HISR & DMA_HISR_TCIF7 ) != 0 )
    {
        /* Clear interrupt */
        DMA2->HIFCR |= DMA_HISR_TCIF7;
        uart_interfaces[ 0 ].tx_dma_result = WICED_SUCCESS;
        tx_complete = WICED_TRUE;
    }

    /* TX DMA error */
    if ( ( DMA2->HISR & ( DMA_HISR_TEIF7 | DMA_HISR_DMEIF7 | DMA_HISR_FEIF7 ) ) != 0 )
    {
        /* Clear interrupt */
        DMA2->HIFCR |= ( DMA_HISR_TEIF7 | DMA_HISR_DMEIF7 | DMA_HISR_FEIF7 );

        if ( tx_complete == WICED_FALSE )
        {
            uart_interfaces[ 0 ].tx_dma_result = WICED_ERROR;
        }
    }

    /* Set semaphore regardless of result to prevent waiting thread from locking up */
    host_rtos_set_semaphore( &uart_interfaces[ 0 ].tx_complete, WICED_TRUE );
}

void usart2_tx_dma_irq( void )
{
    wiced_bool_t tx_complete = WICED_FALSE;

    if ( ( DMA1->HISR & DMA_HISR_TCIF6 ) != 0 )
    {
        DMA1->HIFCR |= DMA_HISR_TCIF6;
        uart_interfaces[ 1 ].tx_dma_result = WICED_SUCCESS;
        tx_complete = WICED_TRUE;
    }

    /* TX DMA error */
    if ( ( DMA1->HISR & ( DMA_HISR_TEIF6 | DMA_HISR_DMEIF6 | DMA_HISR_FEIF6 ) ) != 0 )
    {
        /* Clear interrupt */
        DMA1->HIFCR |= ( DMA_HISR_TEIF6 | DMA_HISR_DMEIF6 | DMA_HISR_FEIF6 );

        if ( tx_complete == WICED_FALSE )
        {
            uart_interfaces[1].tx_dma_result = WICED_ERROR;
        }
    }

    host_rtos_set_semaphore( &uart_interfaces[ 1 ].tx_complete, WICED_TRUE );
}

void usart6_tx_dma_irq( void )
{
    wiced_bool_t tx_complete = WICED_FALSE;

    if ( ( DMA2->HISR & DMA_HISR_TCIF6 ) != 0 )
    {
        DMA2->HIFCR |= DMA_HISR_TCIF6;
        uart_interfaces[ 0 ].tx_dma_result = WICED_SUCCESS;
        tx_complete = WICED_TRUE;
    }

    /* TX DMA error */
    if ( ( DMA2->HISR & ( DMA_HISR_TEIF6 | DMA_HISR_DMEIF6 | DMA_HISR_FEIF6 ) ) != 0 )
    {
        /* Clear interrupt */
        DMA2->HIFCR |= ( DMA_HISR_TEIF6 | DMA_HISR_DMEIF6 | DMA_HISR_FEIF6 );

        if ( tx_complete == WICED_FALSE )
        {
            uart_interfaces[0].tx_dma_result = WICED_ERROR;
        }
    }

    host_rtos_set_semaphore( &uart_interfaces[0].tx_complete, WICED_TRUE );
}

void usart1_rx_dma_irq( void )
{
    if ( ( DMA2->LISR & DMA_LISR_TCIF2 ) != 0 )
    {
        DMA2->LIFCR |= DMA_LISR_TCIF2;
        uart_interfaces[ 0 ].rx_dma_result = WICED_SUCCESS;
    }

    /* RX DMA error */
    if ( ( DMA2->LISR & ( DMA_LISR_TEIF2 | DMA_LISR_DMEIF2 | DMA_LISR_FEIF2 ) ) != 0 )
    {
        /* Clear interrupt */
        DMA2->LIFCR |= ( DMA_LISR_TEIF2 | DMA_LISR_DMEIF2 | DMA_LISR_FEIF2 );
        uart_interfaces[ 0 ].rx_dma_result = WICED_ERROR;
    }

    host_rtos_set_semaphore( &uart_interfaces[ 0 ].rx_complete, WICED_TRUE );
}

void usart2_rx_dma_irq( void )
{
    if ( ( DMA1->HISR & DMA_HISR_TCIF5 ) != 0 )
    {
        DMA1->HIFCR |= DMA_HISR_TCIF5;
        uart_interfaces[ 1 ].rx_dma_result = WICED_SUCCESS;
    }

    /* RX DMA error */
    if ( ( DMA1->HISR & ( DMA_HISR_TEIF5 | DMA_HISR_DMEIF5 | DMA_HISR_FEIF5 ) ) != 0 )
    {
        /* Clear interrupt */
        DMA1->HIFCR |= ( DMA_HISR_TEIF5 | DMA_HISR_DMEIF5 | DMA_HISR_FEIF5 );
        uart_interfaces[ 1 ].rx_dma_result = WICED_ERROR;
    }

    host_rtos_set_semaphore( &uart_interfaces[ 1 ].rx_complete, WICED_TRUE );
}

void usart6_rx_dma_irq( void )
{
    if ( ( DMA2->LISR & DMA_LISR_TCIF1 ) != 0 )
    {
        DMA2->LIFCR |= DMA_LISR_TCIF1;
        uart_interfaces[ 0 ].rx_dma_result = WICED_SUCCESS;

    }

    /* TX DMA error */
    if ( ( DMA2->LISR & ( DMA_LISR_TEIF1 | DMA_LISR_DMEIF1 | DMA_LISR_FEIF1 ) ) != 0 )
    {
        /* Clear interrupt */
        DMA2->LIFCR |= ( DMA_LISR_TEIF1 | DMA_LISR_DMEIF1 | DMA_LISR_FEIF1 );
        uart_interfaces[ 0 ].rx_dma_result = WICED_ERROR;
    }

    host_rtos_set_semaphore( &uart_interfaces[0].rx_complete, WICED_TRUE );
}

#ifdef __GNUC__
/* STDIO Read/Write functions for Newlib */
void platform_stdio_write( const char* str, uint32_t len )
{
#ifndef WICED_DISABLE_STDIO
    host_rtos_get_semaphore( &stdio_tx_mutex, WICED_NEVER_TIMEOUT, WICED_FALSE );
    wiced_uart_transmit_bytes( STDIO_UART, (void*)str, len );
    host_rtos_set_semaphore( &stdio_tx_mutex, WICED_FALSE );
#else
    UNUSED_PARAMETER( str );
    UNUSED_PARAMETER( len );
#endif
}

void platform_stdio_read( char* str, uint32_t len )
{
#ifndef WICED_DISABLE_STDIO
    host_rtos_get_semaphore( &stdio_rx_mutex, WICED_NEVER_TIMEOUT, WICED_FALSE );
    wiced_uart_receive_bytes( STDIO_UART, (void*)str, len, WICED_NEVER_TIMEOUT );
    host_rtos_set_semaphore( &stdio_rx_mutex, WICED_FALSE );
#else
    UNUSED_PARAMETER( str );
    UNUSED_PARAMETER( len );
#endif
}
#endif /* ifdef __GNUC__ */

unsigned long platform_power_down_hook( unsigned long delay_ms )
{
    UNUSED_PARAMETER( delay_ms );
    return 0;
}

void platform_idle_hook( void )
{
    return;
}

void wiced_platform_mcu_enable_powersave( void )
{
    return;
}

void wiced_platform_mcu_disable_powersave( void )
{
    return;
}


void host_platform_get_mac_address( wiced_mac_t* mac )
{
#ifndef WICED_DISABLE_BOOTLOADER
    memcpy(mac->octet, bootloader_api->get_wifi_config_dct()->mac_address.octet, sizeof(wiced_mac_t));
#else
    UNUSED_PARAMETER( mac );
#endif
}
