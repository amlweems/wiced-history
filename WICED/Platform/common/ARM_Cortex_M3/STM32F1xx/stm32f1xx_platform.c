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
#include "platform.h"
#include "wiced_platform.h"
#include "stm32f10x.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_pwr.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_rtc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_bkp.h"
#include "gpio_irq.h"
#include "watchdog.h"
#include "stdio.h"
#include "string.h"
#include "wwd_assert.h"
#include "platform_common_config.h"
#include "stm32f1xx_platform.h"
#include "platform_dct.h"
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

#define FLASH_ADDRESS_TO_PAGE( address )     ((uint16_t) ( ( address - FLASH_START_ADDR ) / PLATFORM_FLASH_PAGE_SIZE ))

#define ERASE_DCT_1()              platform_erase_flash( FLASH_ADDRESS_TO_PAGE(PLATFORM_DCT_COPY1_START_ADDRESS), FLASH_ADDRESS_TO_PAGE(PLATFORM_DCT_COPY1_END_ADDRESS) )
#define ERASE_DCT_2()              platform_erase_flash( FLASH_ADDRESS_TO_PAGE(PLATFORM_DCT_COPY2_START_ADDRESS), FLASH_ADDRESS_TO_PAGE(PLATFORM_DCT_COPY2_END_ADDRESS) )

/******************************************************
 *                    Constants
 ******************************************************/

#define MAX_NUM_SPI_PRESCALERS    (8)
#define SPI_DMA_TIMEOUT_LOOPS     (10000)

#define FLASH_START_ADDR     ((uint32_t)&flash_start_addr_loc)
#define APP_HDR_START_ADDR   ((uint32_t)&app_hdr_start_addr_loc)
#define DCT1_START_ADDR      ((uint32_t)&dct1_start_addr_loc)
#define DCT1_SIZE            ((uint32_t)&dct1_size_loc)
#define DCT2_START_ADDR      ((uint32_t)&dct2_start_addr_loc)
#define DCT2_SIZE            ((uint32_t)&dct2_size_loc)
#define SRAM_START_ADDR      ((uint32_t)&sram_start_addr_loc)
#define SRAM_SIZE            ((uint32_t)&sram_size_loc)


#define FLASH_WRITE_FUNC    ( FLASH_ProgramHalfWord )
#define FLASH_WRITE_SIZE    ( 4 )

/* first dct copy definitions */
#define PLATFORM_DCT_COPY1_START_ADDRESS     ( DCT1_START_ADDR )
#define PLATFORM_MAX_DCT_SIZE                ( DCT1_SIZE     )
#define PLATFORM_DCT_COPY1_END_ADDRESS       ( PLATFORM_DCT_COPY1_START_ADDRESS + PLATFORM_MAX_DCT_SIZE )

/* second dct copy definitions  */
#define PLATFORM_DCT_COPY2_START_ADDRESS     ( DCT2_START_ADDR )
#define PLATFORM_DCT_COPY2_END_ADDRESS       ( PLATFORM_DCT_COPY2_START_ADDRESS + PLATFORM_MAX_DCT_SIZE )
/* for stm33f103 the size of the page is 2k !!!! , it depends on the type of the stm32f1 device, high, low and medium density devices exist */
#define PLATFORM_FLASH_PAGE_SIZE             ( 2*1024    )

#ifndef STDIO_BUFFER_SIZE
#define STDIO_BUFFER_SIZE   64
#endif

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef uint32_t flash_write_t;

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    uint32_t               rx_size;
    wiced_ring_buffer_t*   rx_buffer;
    host_semaphore_type_t  rx_complete;
    host_semaphore_type_t  tx_complete;
    const uint8_t*         tx_buffer;
    uint32_t               tx_size;
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


#ifndef WICED_DISABLE_MCU_POWERSAVE
#define STOP_MODE_SUPPORT
#endif /* ifndef WICED_DISABLE_MCU_POWERSAVE */



void RTC_init(void);
void stm32f1xx_clocks_needed( void );
void stm32f1xx_clocks_not_needed( void );

void usart1_irq( void );
void usart2_irq( void );
void RTCAlarm_irq( void );

#ifndef PLATFORM_DISABLE_UART_DMA
void usart1_tx_dma_irq( void );
void usart2_tx_dma_irq( void );
void usart1_rx_dma_irq( void );
void usart2_rx_dma_irq( void );
#endif

/******************************************************
 *               Variables Definitions
 ******************************************************/

/* These come from the linker script */
extern void* dct1_start_addr_loc;
extern void* dct1_size_loc;
extern void* dct2_start_addr_loc;
extern void* dct2_size_loc;
extern void* app_hdr_start_addr_loc;
extern void* sram_start_addr_loc;
extern void* sram_size_loc;
extern void* flash_start_addr_loc;

static uart_interface_t uart_interfaces[NUMBER_OF_UART_INTERFACES];

static const uint32_t spi_transfer_complete_flags[]=
{
    /* for every stream get a transfer complete flag */
    [0] =  DMA1_FLAG_TC1,
    [1] =  DMA1_FLAG_TC2,
    [2] =  DMA1_FLAG_TC3,
    [3] =  DMA1_FLAG_TC4,
    [4] =  DMA1_FLAG_TC5,
    [5] =  DMA1_FLAG_TC6,
    [6] =  DMA1_FLAG_TC7,
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

static const uint16_t adc_sampling_cycle[] =
{
    [ADC_SampleTime_1Cycles5  ] = 1,
    [ADC_SampleTime_7Cycles5  ] = 7,
    [ADC_SampleTime_13Cycles5 ] = 13,
    [ADC_SampleTime_28Cycles5 ] = 28,
    [ADC_SampleTime_41Cycles5 ] = 41,
    [ADC_SampleTime_55Cycles5 ] = 55,
    [ADC_SampleTime_71Cycles5 ] = 71,
    [ADC_SampleTime_239Cycles5] = 239,
};

/******************************************************
 *               Function Definitions
 ******************************************************/

/* STM32F1 common clock initialisation function
 * - weak attribute is intentional in case a specific platform (board) needs to override this.
 */
WEAK void init_clocks( void )
{
    /* Configure Clocks */
    RCC_DeInit( );

    RCC_HSEConfig( HSE_SOURCE );
    RCC_WaitForHSEStartUp( );

    RCC_HCLKConfig( AHB_CLOCK_DIVIDER );
    RCC_PCLK1Config( APB1_CLOCK_DIVIDER ); /* PCLK1 max = 36MHz */
    RCC_PCLK2Config( APB2_CLOCK_DIVIDER ); /* PCLK2 max = 36MHz */

    /* Enable the PLL */
    FLASH_SetLatency( INT_FLASH_WAIT_STATE );
    FLASH_PrefetchBufferCmd( ENABLE );

    /* CPU_CLOCK_HZ = RCC_PLLSource * RCC_PLLMul
     */
    RCC_PLLConfig( PLL_SOURCE, PLL_MULTIPLIER_CONSTANT ); /* NOTE: The CPU Clock Frequency is independently defined in <WICED-SDK>/Wiced/Platform/<platform>/<platform>.mk */
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
        NVIC->IP[i] = 0xff;
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

    /* Disable MCU powersave at start-up. Application must explicitly enable MCU powersave if desired */
    stm32f1xx_clocks_needed();
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

    /* enable dma channels that have just been configured */
    DMA_Cmd(spi_mapping[spi->port].rx_dma_channel, ENABLE);
    DMA_Cmd(spi_mapping[spi->port].tx_dma_channel, ENABLE);

    /* Wait for DMA to complete */
    loop_count = 0;
    while ( ( DMA_GetFlagStatus( spi_transfer_complete_flags[spi_mapping[spi->port].rx_dma_channel_number - 1] ) == RESET ) && ( loop_count < (uint32_t) SPI_DMA_TIMEOUT_LOOPS ) )
    {
        loop_count++;
    }

    if ( loop_count >= (uint32_t) SPI_DMA_TIMEOUT_LOOPS )
    {
        /* dma is still in transfer and timeout occured, perhaps dma was not configured properly */
        /* it cant still run */
        return WICED_TIMEOUT;
    }

    /* dma is finished */
    return WICED_SUCCESS;
}

static void spi_dma_config( const wiced_spi_device_t* spi, wiced_spi_message_segment_t* message )
{
    uint8_t dummy = 0xFF;
    DMA_InitTypeDef dma_init;

    /* check arguments */
    if( !message )
    {
        /* no message is specified */
        return;
    }

    /* Setup DMA for SPI TX if it is enabled */
    DMA_DeInit( spi_mapping[spi->port].tx_dma_channel );
    /* setup dma stream for TX */
    dma_init.DMA_PeripheralBaseAddr = ( uint32_t )&spi_mapping[spi->port].spi_regs->DR;
    dma_init.DMA_DIR = DMA_DIR_PeripheralDST;
    dma_init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_init.DMA_BufferSize = message->length;
    dma_init.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma_init.DMA_Mode = DMA_Mode_Normal;
    dma_init.DMA_Priority = DMA_Priority_VeryHigh;

    if( message->tx_buffer )
    {
       dma_init.DMA_MemoryBaseAddr = ( uint32_t )message->tx_buffer;
       dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable;
    }
    else
    {
       dma_init.DMA_MemoryBaseAddr = ( uint32_t )(&dummy);
       dma_init.DMA_MemoryInc = DMA_MemoryInc_Disable;
    }

    DMA_Init( spi_mapping[spi->port].tx_dma_channel, &dma_init );

    /* activate spi dma mode for transmission */
    SPI_I2S_DMACmd( spi_mapping[spi->port].spi_regs, SPI_I2S_DMAReq_Tx, ENABLE );
    /* init tx dma finish semaphore  */
    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

    /* setup dma for SPI RX dma stream */
    DMA_DeInit( spi_mapping[spi->port].rx_dma_channel );
    dma_init.DMA_PeripheralBaseAddr = ( uint32_t )&spi_mapping[spi->port].spi_regs->DR;
    dma_init.DMA_DIR = DMA_DIR_PeripheralSRC;
    dma_init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_init.DMA_BufferSize = message->length;
    dma_init.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma_init.DMA_Mode = DMA_Mode_Normal;
    dma_init.DMA_Priority = DMA_Priority_VeryHigh;
    if( message->rx_buffer )
    {
        dma_init.DMA_MemoryBaseAddr = (uint32_t)message->rx_buffer;
        dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable;
    }
    else
    {
        dma_init.DMA_MemoryBaseAddr = (uint32_t)&dummy;
        dma_init.DMA_MemoryInc = DMA_MemoryInc_Disable;
    }
    /* init st dma channel */
    DMA_Init( spi_mapping[spi->port].rx_dma_channel, &dma_init );
    /* activate spi dma mode for reception */
    SPI_I2S_DMACmd( spi_mapping[spi->port].spi_regs, SPI_I2S_DMAReq_Rx, ENABLE );
    /* init rx dma finish semaphore */
    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

    return;
}

wiced_result_t wiced_spi_init( const wiced_spi_device_t* spi )
{
    uint16_t prescaler;
    wiced_result_t res;
    SPI_InitTypeDef spi_init;
    GPIO_InitTypeDef gpio_init_structure;

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed( );
#endif

    /* Init SPI GPIOs */
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init_structure.GPIO_Pin   = ((uint16_t) (1 << spi_mapping[spi->port].pin_clock->number) ) |
                                     ((uint16_t) (1 << spi_mapping[spi->port].pin_miso->number ) ) |
                                     ((uint16_t) (1 << spi_mapping[spi->port].pin_mosi->number ) );
    GPIO_Init( spi_mapping[spi->port].pin_clock->bank, &gpio_init_structure );

    /* Init the chip select GPIO */
    wiced_gpio_init(spi->chip_select, OUTPUT_PUSH_PULL);
    wiced_gpio_output_high(spi->chip_select);

    /* Ensure the ADC and GPIOA are enabled */
    RCC_APB2PeriphClockCmd( spi_mapping[spi->port].pin_clock->peripheral_clock, ENABLE);
    RCC_APB2PeriphClockCmd( spi_mapping[spi->port].peripheral_clock_reg, ENABLE );

    /* Configure baudrate */
    if ( ( res = wiced_spi_configure_baudrate( spi->speed, &prescaler ) != WICED_SUCCESS ) )
    {
#ifdef STOP_MODE_SUPPORT
        stm32f1xx_clocks_not_needed( );
#endif
        return res;
    }
    spi_init.SPI_BaudRatePrescaler = prescaler;

    /* Configure data-width */
    if ( spi->bits == 8 )
    {
        spi_init.SPI_DataSize = SPI_DataSize_8b;
    }
    else if ( spi->bits == 16 )
    {
        if ( spi->mode & SPI_USE_DMA )
        {
#ifdef STOP_MODE_SUPPORT
            stm32f1xx_clocks_not_needed( );
#endif
            return WICED_ERROR;
        }
        spi_init.SPI_DataSize = SPI_DataSize_16b;
    }
    else
    {
#ifdef STOP_MODE_SUPPORT
        stm32f1xx_clocks_not_needed( );
#endif
        return WICED_NOTFOUND;
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

    /* Configure mode CPHA and CPOL, msb or lsb first */
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
        spi_init.SPI_CPHA = ( spi->mode & SPI_CLOCK_IDLE_HIGH ) ? SPI_CPHA_2Edge : SPI_CPHA_1Edge;
    }
    else
    {
        spi_init.SPI_CPHA = ( spi->mode & SPI_CLOCK_IDLE_HIGH ) ? SPI_CPHA_1Edge : SPI_CPHA_2Edge;
    }
    spi_init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi_init.SPI_Mode = SPI_Mode_Master;
    spi_init.SPI_NSS = SPI_NSS_Soft;
    SPI_CalculateCRC( spi_mapping[spi->port].spi_regs, DISABLE );

    /* Init and enable SPI */
    SPI_Init( spi_mapping[spi->port].spi_regs, &spi_init );
    SPI_Cmd ( spi_mapping[spi->port].spi_regs, ENABLE );

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed( );
#endif /* #ifdef STOP_MODE_SUPPORT */

    return WICED_SUCCESS;
}

wiced_result_t wiced_spi_transfer( const wiced_spi_device_t* spi, wiced_spi_message_segment_t* segments, uint16_t number_of_segments )
{
    wiced_result_t res;
    uint32_t count = 0;
    uint16_t i = 0;

    /* check whether SPI port is supported by the current platform */
    if( spi->port != WICED_SPI_1 )
    {
        return WICED_NOTFOUND;
    }
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

//    if( ( !msgs ) || ( !num_msgs ) )
//    {
//        return WICED_BADARG;
//    }
    /* Activate chip select */
    wiced_gpio_output_low(spi->chip_select);


    for( i = 0; i < number_of_segments; i++ )
    {
        if( spi->mode & SPI_USE_DMA )
        {
            spi_dma_config(spi, &segments[i]);
            res = spi_dma_transfer( spi );
            if( res != WICED_SUCCESS )
            {
#ifdef STOP_MODE_SUPPORT
                stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
                /* return with an error */
                return res;
            }
        }
        else
        {
            /* in interrupt-less mode */
            if( spi->bits == 8 )
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
                    SPI_I2S_SendData(spi_mapping[spi->port].spi_regs, data);

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
            else if( spi->bits == 16 )
            {
                const uint16_t * send_ptr = ( const uint16_t * )segments[i].tx_buffer;
                uint16_t * rcv_ptr = ( uint16_t * )segments[i].rx_buffer;
                if( ( count % 2 ) )
                {
#ifdef STOP_MODE_SUPPORT
                    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
                    /* message length must be an even number if word tranfer mode was selected */
                    return WICED_ERROR;
                }

                while( count )
                {
                	uint16_t data;

                    count -= 2;
                    data = 0xFFFF;

                    if ( send_ptr != NULL )
                    {
                        data = *send_ptr++;
                    }

                    /* Wait until the transmit buffer is empty */
                    while (SPI_I2S_GetFlagStatus(spi_mapping[spi->port].spi_regs, SPI_I2S_FLAG_TXE) == RESET)
                    {}

                    /* Send the byte */
                    SPI_I2S_SendData(spi_mapping[spi->port].spi_regs, data);

                    /* Wait until a data is received */
                    while (SPI_I2S_GetFlagStatus(spi_mapping[spi->port].spi_regs, SPI_I2S_FLAG_RXNE) == RESET)
                    {}

                    /* Get the received data */
                    data = SPI_I2S_ReceiveData(spi_mapping[spi->port].spi_regs);

                    if ( rcv_ptr != NULL )
                    {
                        *rcv_ptr++ = data;
                    }
                }
            }
        }
    }
    /* Activate chip select */
    wiced_gpio_output_high(spi->chip_select);

#if 0
    /* for interrupt */

#endif
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
    return WICED_SUCCESS;
}

wiced_result_t wiced_spi_deinit( const wiced_spi_device_t* spi )
{
    GPIO_InitTypeDef gpio_init_structure;

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    /* De-init and disable SPI */
    SPI_Cmd( spi_mapping[ spi->port ].spi_regs, DISABLE );
    SPI_I2S_DeInit( spi_mapping[ spi->port ].spi_regs );

    /* Disable SPI peripheral clock */
    RCC_APB2PeriphClockCmd( spi_mapping[spi->port].pin_clock->peripheral_clock, DISABLE);
    RCC_APB2PeriphClockCmd( spi_mapping[spi->port].peripheral_clock_reg, DISABLE );


    gpio_init_structure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init_structure.GPIO_Pin   = ((uint16_t) (1 << spi_mapping[spi->port].pin_clock->number) ) |
                                     ((uint16_t) (1 << spi_mapping[spi->port].pin_miso->number ) ) |
                                     ((uint16_t) (1 << spi_mapping[spi->port].pin_mosi->number ) );
    GPIO_Init( spi_mapping[spi->port].pin_clock->bank, &gpio_init_structure );

    /* Reset CS pin to input floating state */
    wiced_gpio_init( spi->chip_select, INPUT_HIGH_IMPEDANCE );

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

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

wiced_result_t wiced_adc_init( wiced_adc_t adc, uint32_t sampling_cycle )
{
    /* Initialize the associated GPIO */
    GPIO_InitTypeDef gpio_init_structure;
    ADC_InitTypeDef adc_init_structure;
    uint8_t a;

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    gpio_init_structure.GPIO_Pin   = (uint16_t) ( 1 << adc_mapping[adc].pin->number );
    gpio_init_structure.GPIO_Speed = (GPIOSpeed_TypeDef) 0;
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_AIN;
    GPIO_Init( adc_mapping[adc].pin->bank, &gpio_init_structure );

    /* Ensure the ADC and GPIOA are enabled */
    RCC_APB2PeriphClockCmd( adc_mapping[adc].pin->peripheral_clock, ENABLE);
    RCC_APB2PeriphClockCmd( adc_mapping[adc].adc_peripheral_clock, ENABLE );

    /* Initialize the ADC */
    ADC_StructInit( &adc_init_structure );
    /* stm32f1 supports only 12 bit mode */
    adc_init_structure.ADC_ScanConvMode       = DISABLE;
    adc_init_structure.ADC_ContinuousConvMode = DISABLE;
    adc_init_structure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    adc_init_structure.ADC_DataAlign          = ADC_DataAlign_Right;
    adc_init_structure.ADC_NbrOfChannel    = 1;
    ADC_Init( adc_mapping[adc].adc, &adc_init_structure );

#if 0
    ADC_CommonInitTypeDef adc_common_init_structure;
    ADC_CommonStructInit(&adc_common_init_structure);
    adc_common_init_structure.ADC_Mode             = ADC_Mode_Independent;
    adc_common_init_structure.ADC_DMAAccessMode    = ADC_DMAAccessMode_Disabled;
    adc_common_init_structure.ADC_Prescaler        = ADC_Prescaler_Div2;
    adc_common_init_structure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&adc_common_init_structure);
#endif
    ADC_Cmd( adc_mapping[adc].adc, ENABLE );

    /* Find the closest supported sampling time by the MCU */
    for ( a = 0; ( a < sizeof( adc_sampling_cycle ) / sizeof(uint16_t) ) && adc_sampling_cycle[a] < sampling_cycle; a++ )
    {
    }

    /* Initialize the ADC channel */
    ADC_RegularChannelConfig( adc_mapping[adc].adc, adc_mapping[adc].channel, adc_mapping[adc].rank, a );

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    return WICED_SUCCESS;
}

wiced_result_t wiced_adc_take_sample( wiced_adc_t adc, uint16_t* output )
{
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
    /* Start conversion */
    ADC_SoftwareStartConvCmd( adc_mapping[adc].adc, ENABLE );

    /* Wait until end of conversion */
    while ( ADC_GetFlagStatus( adc_mapping[adc].adc, ADC_FLAG_EOC ) == RESET )
    {
    }

    /* Read ADC conversion result */
    *output = ADC_GetConversionValue( adc_mapping[adc].adc );

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
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
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    RCC_APB2PeriphClockCmd( gpio_mapping[gpio].peripheral_clock, ENABLE );
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    switch( configuration )
    {
        case INPUT_PULL_UP:
            gpio_init_structure.GPIO_Mode = GPIO_Mode_IPU;
            break;
        case INPUT_PULL_DOWN:
            gpio_init_structure.GPIO_Mode = GPIO_Mode_IPD;
            break;
        case INPUT_HIGH_IMPEDANCE:
            gpio_init_structure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
            break;
        case OUTPUT_PUSH_PULL:
            gpio_init_structure.GPIO_Mode = GPIO_Mode_Out_PP;
            break;
        case OUTPUT_OPEN_DRAIN_NO_PULL:
            gpio_init_structure.GPIO_Mode = GPIO_Mode_Out_OD;
            break;
        case OUTPUT_OPEN_DRAIN_PULL_UP:
            gpio_init_structure.GPIO_Mode = GPIO_Mode_IPU;
            break;
        default:
        	return WICED_BADARG;
    }
    gpio_init_structure.GPIO_Pin = (uint16_t) ( 1 << gpio_mapping[gpio].number );
    GPIO_Init( gpio_mapping[gpio].bank, &gpio_init_structure );

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
    return WICED_SUCCESS;
}

wiced_result_t wiced_gpio_output_high( wiced_gpio_t gpio )
{
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    GPIO_SetBits( gpio_mapping[gpio].bank, (uint16_t) (1 << gpio_mapping[gpio].number) );

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
    return WICED_SUCCESS;
}

wiced_result_t wiced_gpio_output_low( wiced_gpio_t gpio )
{
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    GPIO_ResetBits( gpio_mapping[gpio].bank, (uint16_t) (1 << gpio_mapping[gpio].number) );

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    return WICED_SUCCESS;
}

wiced_bool_t wiced_gpio_input_get( wiced_gpio_t gpio )
{
    wiced_bool_t result;
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    result = GPIO_ReadInputDataBit( gpio_mapping[gpio].bank, (uint16_t) ( 1 << gpio_mapping[gpio].number ) );

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
    return result;
}

wiced_result_t wiced_gpio_input_irq_enable( wiced_gpio_t gpio, wiced_gpio_irq_trigger_t trigger, wiced_gpio_irq_handler_t handler, void* arg )
{
    wiced_bool_t result;
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
    result = gpio_irq_enable( gpio_mapping[gpio].bank, gpio_mapping[gpio].number, trigger, handler, arg );

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
    return result;
}

wiced_result_t wiced_gpio_input_irq_disable( wiced_gpio_t gpio )
{
    wiced_bool_t result;
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    result = gpio_irq_disable( gpio_mapping[gpio].bank, gpio_mapping[gpio].number );

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
    return result;
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

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    RCC_GetClocksFreq( &rcc_clock_frequencies );

    if ( pwm->tim == TIM1 || pwm->tim == TIM8 || pwm->tim == TIM9 || pwm->tim == TIM10 || pwm->tim == TIM11 || pwm->tim == TIM15 || pwm->tim == TIM16 || pwm->tim == TIM17 )
    {
        RCC_APB2PeriphClockCmd( pwm->tim_peripheral_clock, ENABLE );
        period = (uint16_t)( rcc_clock_frequencies.PCLK2_Frequency / frequency - 1 ); /* Auto-reload value counts from 0; hence the minus 1 */
    }
    else
    {
        RCC_APB1PeriphClockCmd( pwm->tim_peripheral_clock, ENABLE );
        period = (uint16_t)( rcc_clock_frequencies.PCLK1_Frequency / frequency - 1 ); /* Auto-reload value counts from 0; hence the minus 1 */
    }

    RCC_APB2PeriphClockCmd( pwm->pin->peripheral_clock, ENABLE );

    gpio_init_structure.GPIO_Pin = (uint16_t) ( 1 << pwm->pin->number );
    gpio_init_structure.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( pwm->pin->bank, &gpio_init_structure );

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

    switch(pwm->channel)
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
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
    return WICED_SUCCESS;
}

wiced_result_t wiced_pwm_start( wiced_pwm_t pwm )
{
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    TIM_Cmd(pwm_mappings[pwm].tim, ENABLE);
    TIM_CtrlPWMOutputs( pwm_mappings[pwm].tim, ENABLE );

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
    return WICED_SUCCESS;
}

wiced_result_t wiced_pwm_stop( wiced_pwm_t pwm )
{
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    TIM_CtrlPWMOutputs( pwm_mappings[pwm].tim, DISABLE );
    TIM_Cmd(pwm_mappings[pwm].tim, DISABLE);

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
    return WICED_SUCCESS;
}

wiced_result_t wiced_watchdog_kick( void )
{
    wiced_result_t result;
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    result = watchdog_kick();

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
    return result;

}

wiced_result_t wiced_uart_init( wiced_uart_t uart, const wiced_uart_config_t* config, wiced_ring_buffer_t* optional_rx_buffer )
{
    wiced_result_t result;
#ifndef WICED_DISABLE_STDIO
    if (uart == STDIO_UART)
    {
        return WICED_ERROR;
    }
#endif

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    result = platform_uart_init(uart, config, optional_rx_buffer);

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
    return result;
}

static wiced_result_t platform_uart_init( wiced_uart_t uart, const wiced_uart_config_t* config, wiced_ring_buffer_t* optional_rx_buffer )
{
    GPIO_InitTypeDef  gpio_init_structure;
    USART_InitTypeDef usart_init_structure;
    NVIC_InitTypeDef  nvic_init_structure;
//    DMA_InitTypeDef   dma_init_structure;

    host_rtos_init_semaphore(&uart_interfaces[uart].tx_complete);
    host_rtos_init_semaphore(&uart_interfaces[uart].rx_complete);

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    /* Enable GPIO peripheral clocks for TX and RX pins */
    RCC_APB2PeriphClockCmd( uart_mapping[uart].pin_rx->peripheral_clock, ENABLE );
    RCC_APB2PeriphClockCmd( uart_mapping[uart].pin_tx->peripheral_clock, ENABLE );

    /* Configure USART TX Pin */
    gpio_init_structure.GPIO_Pin   = (uint16_t) ( 1 << uart_mapping[uart].pin_tx->number );
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( uart_mapping[uart].pin_tx->bank, &gpio_init_structure );

    /* Configure USART RX Pin */
    gpio_init_structure.GPIO_Pin   = (uint16_t) ( 1 << uart_mapping[uart].pin_rx->number );
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init( uart_mapping[uart].pin_rx->bank, &gpio_init_structure );

    /* Check if any of the flow control is enabled */
    if ( uart_mapping[uart].pin_cts && (config->flow_control == FLOW_CONTROL_CTS || config->flow_control == FLOW_CONTROL_CTS_RTS) )
    {
        /* Enable peripheral clock */
        RCC_APB2PeriphClockCmd( uart_mapping[uart].pin_cts->peripheral_clock, ENABLE );

        /* Configure CTS Pin */
        gpio_init_structure.GPIO_Pin   = (uint16_t) ( 1 << uart_mapping[uart].pin_cts->number );
        gpio_init_structure.GPIO_Mode  = GPIO_Mode_AF_OD;
        GPIO_Init( uart_mapping[uart].pin_cts->bank, &gpio_init_structure );
    }

    if ( uart_mapping[uart].pin_cts && (config->flow_control == FLOW_CONTROL_RTS || config->flow_control == FLOW_CONTROL_CTS_RTS) )
    {
        /* Enable peripheral clock */
        RCC_APB2PeriphClockCmd( uart_mapping[uart].pin_rts->peripheral_clock, ENABLE );

        /* Configure RTS Pin */
        gpio_init_structure.GPIO_Pin   = (uint16_t) ( 1 << uart_mapping[uart].pin_rts->number );
        gpio_init_structure.GPIO_Mode  = GPIO_Mode_AF_OD;
        GPIO_Init( uart_mapping[uart].pin_rts->bank, &gpio_init_structure );
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

#ifndef PLATFORM_DISABLE_UART_DMA
    /**************************************************************************
     * Initialise STM32 DMA registers
     * Note: If DMA is used, USART interrupt isn't enabled.
     **************************************************************************/
    /* Enable DMA peripheral clock */
    uart_mapping[uart].tx_dma_peripheral_clock_func( uart_mapping[uart].tx_dma_peripheral_clock, ENABLE );
    uart_mapping[uart].rx_dma_peripheral_clock_func( uart_mapping[uart].rx_dma_peripheral_clock, ENABLE );

    /* Fill init structure with common DMA settings */
//    dma_init_structure.DMA_PeripheralInc   = DMA_PeripheralInc_Disable;
//    dma_init_structure.DMA_MemoryInc       = DMA_MemoryInc_Enable;
//    dma_init_structure.DMA_Priority        = DMA_Priority_VeryHigh;
//
//    if ( config->data_width == DATA_WIDTH_9BIT )
//    {
//        dma_init_structure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
//        dma_init_structure.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
//    }
//    else
//    {
//        dma_init_structure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//        dma_init_structure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
//    }

    /* Initialise TX DMA */
    uart_mapping[uart].tx_dma_channel->CCR   = 0;
    uart_mapping[uart].tx_dma_channel->CNDTR = 0;
    uart_mapping[uart].tx_dma_channel->CMAR  = (uint32_t)0;
    uart_mapping[uart].tx_dma_channel->CPAR  = (uint32_t) &uart_mapping[uart].usart->DR;
//    DMA_DeInit( uart_mapping[uart].tx_dma_channel );
//    dma_init_structure.DMA_PeripheralBaseAddr = (uint32_t) &uart_mapping[uart].usart->DR;
//    dma_init_structure.DMA_MemoryBaseAddr     = (uint32_t) 0;
//    dma_init_structure.DMA_DIR                = DMA_DIR_PeripheralDST;
//    dma_init_structure.DMA_BufferSize         = 0;
//    dma_init_structure.DMA_Mode               = DMA_Mode_Normal;
//    DMA_Init( uart_mapping[uart].tx_dma_channel, &dma_init_structure );

    /* Initialise RX DMA */
    uart_mapping[uart].rx_dma_channel->CCR   = 0;
    uart_mapping[uart].rx_dma_channel->CNDTR = 0;
    uart_mapping[uart].rx_dma_channel->CMAR  = (uint32_t)0;
    uart_mapping[uart].rx_dma_channel->CPAR  = (uint32_t) &uart_mapping[uart].usart->DR;
//    DMA_DeInit( uart_mapping[uart].rx_dma_channel );
//    dma_init_structure.DMA_PeripheralBaseAddr = (uint32_t) &uart_mapping[uart].usart->DR;
//    dma_init_structure.DMA_MemoryBaseAddr     = 0;
//    dma_init_structure.DMA_DIR                = DMA_DIR_PeripheralSRC;
//    dma_init_structure.DMA_BufferSize         = 0;
//    dma_init_structure.DMA_Mode               = DMA_Mode_Normal;
//    DMA_Init( uart_mapping[uart].rx_dma_channel, &dma_init_structure );

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
    DMA_ITConfig( uart_mapping[uart].tx_dma_channel, DMA_IT_TC | DMA_IT_TE, ENABLE );

    /* Enable USART's TX and RX DMA interfaces */
    USART_DMACmd( uart_mapping[uart].usart, USART_DMAReq_Rx | USART_DMAReq_Tx, ENABLE );
#endif

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
    uart_mapping[uart].usart->CR1 |= USART_CR1_TE | USART_CR1_RE;

    /* Setup ring buffers */
    if (optional_rx_buffer != NULL)
    {
        /* Note that the ring_buffer should've been initialised first */
        uart_interfaces[uart].rx_buffer = optional_rx_buffer;
        uart_interfaces[uart].rx_size   = 0;
#ifndef PLATFORM_DISABLE_UART_DMA
        platform_uart_receive_bytes( uart, optional_rx_buffer->buffer, optional_rx_buffer->size, 0 );
#else
        uart_mapping[uart].usart->CR1 |= USART_CR1_RXNEIE;
#endif
    }
    else
    {
#ifndef PLATFORM_DISABLE_UART_DMA
        /* Configure RX DMA interrupt on Cortex-M3 */
       nvic_init_structure.NVIC_IRQChannel                   = uart_mapping[uart].rx_dma_irq;
       nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0x5;
       nvic_init_structure.NVIC_IRQChannelSubPriority        = 0x8;
       nvic_init_structure.NVIC_IRQChannelCmd                = ENABLE;
       NVIC_Init( &nvic_init_structure );

       /* Enable TC (transfer complete) and TE (transfer error) interrupts on source */
       DMA_ITConfig( uart_mapping[uart].rx_dma_channel, DMA_IT_TC | DMA_IT_TE, ENABLE );
#endif
    }

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    return WICED_SUCCESS;
}

wiced_result_t wiced_uart_deinit( wiced_uart_t uart )
{
    NVIC_InitTypeDef nvic_init_structure;

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    /* Disable USART */
    USART_Cmd( uart_mapping[uart].usart, DISABLE );

    /* Deinitialise USART */
    USART_DeInit( uart_mapping[uart].usart );

#ifndef PLATFORM_DISABLE_UART_DMA
    /**************************************************************************
     * De-initialise STM32 DMA and interrupt
     **************************************************************************/

    /* Deinitialise DMA streams */
    DMA_DeInit( uart_mapping[uart].tx_dma_channel );
    DMA_DeInit( uart_mapping[uart].tx_dma_channel );

    /* Disable TC (transfer complete) interrupt at the source */
    DMA_ITConfig( uart_mapping[uart].tx_dma_channel, DMA_IT_TC | DMA_IT_TE, DISABLE );

    /* Disable transmit DMA interrupt at Cortex-M3 */
    nvic_init_structure.NVIC_IRQChannel                   = uart_mapping[uart].tx_dma_irq;
    nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0x5;
    nvic_init_structure.NVIC_IRQChannelSubPriority        = 0x8;
    nvic_init_structure.NVIC_IRQChannelCmd                = DISABLE;
    NVIC_Init( &nvic_init_structure );

    /* Disable DMA peripheral clocks */
    uart_mapping[uart].tx_dma_peripheral_clock_func( uart_mapping[uart].tx_dma_peripheral_clock, DISABLE );
    uart_mapping[uart].rx_dma_peripheral_clock_func( uart_mapping[uart].rx_dma_peripheral_clock, DISABLE );
#endif

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

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */
    return WICED_SUCCESS;
}

wiced_result_t wiced_uart_transmit_bytes( wiced_uart_t uart, const void* data, uint32_t size )
{
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif

#ifndef PLATFORM_DISABLE_UART_DMA
    uart_mapping[uart].tx_dma_channel->CCR   = 0;
    uart_mapping[uart].tx_dma_channel->CNDTR = size;
    uart_mapping[uart].tx_dma_channel->CMAR  = (uint32_t)data;
    uart_mapping[uart].tx_dma_channel->CCR   = DMA_Priority_VeryHigh | DMA_DIR_PeripheralDST | DMA_MemoryInc_Enable | DMA_CCR1_EN | DMA_CCR1_TCIE;

    host_rtos_get_semaphore( &uart_interfaces[uart].tx_complete, WICED_NEVER_TIMEOUT, WICED_TRUE );
    while( ( uart_mapping[uart].usart->SR & USART_SR_TC )== 0 );
#else

    uart_interfaces[uart].tx_buffer = data;
    uart_interfaces[uart].tx_size   = size;
    uart_mapping[uart].usart->CR1  |= USART_CR1_TXEIE;
    host_rtos_get_semaphore( &uart_interfaces[uart].tx_complete, WICED_NEVER_TIMEOUT, WICED_TRUE );
    while( ( uart_mapping[uart].usart->SR & USART_SR_TC )== 0 );
#endif

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif

    return WICED_SUCCESS;
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
    UNUSED_PARAMETER(uart);
    UNUSED_PARAMETER(data);
    UNUSED_PARAMETER(size);
    UNUSED_PARAMETER(timeout);
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

#ifndef PLATFORM_DISABLE_UART_DMA
    if ( uart_interfaces[uart].rx_buffer != NULL )
    {
        uart_mapping[uart].rx_dma_channel->CCR = DMA_CCR1_CIRC;

        // Enabled individual byte interrupts so progress can be updated
        uart_mapping[uart].usart->CR1 |= USART_CR1_RXNEIE;
//        USART_ITConfig( uart_mapping[uart].usart, USART_IT_RXNE, ENABLE );
    }
    else
    {
        uart_mapping[uart].rx_dma_channel->CCR = 0;
    }

    uart_mapping[uart].rx_dma_channel->CNDTR = size;
    uart_mapping[uart].rx_dma_channel->CMAR  = (uint32_t)data;
    uart_mapping[uart].rx_dma_channel->CCR   |= DMA_MemoryInc_Enable | DMA_DIR_PeripheralSRC | DMA_CCR1_EN;

    if ( timeout > 0 )
    {
        host_rtos_get_semaphore( &uart_interfaces[uart].rx_complete, timeout, WICED_TRUE );
    }
#endif

    #ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* #ifdef STOP_MODE_SUPPORT */

    return WICED_SUCCESS;
}

/******************************************************
 *                 DCT Functions
 ******************************************************/

#ifndef WICED_DISABLE_BOOTLOADER
platform_dct_data_t* platform_get_dct( void )
{
    platform_dct_header_t hdr =
    {
        .write_incomplete = 0,
        .is_current_dct = 1,
        .app_valid = 1,
        .mfg_info_programmed = 0,
        .magic_number = BOOTLOADER_MAGIC_NUMBER,
        .load_app_func = NULL
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

    /* Not a valid DCT! */
    /* Erase the first DCT and init it. */
    ERASE_DCT_1();

    platform_write_flash_chunk( PLATFORM_DCT_COPY1_START_ADDRESS, (uint8_t*) &hdr, sizeof(hdr) );

    return (platform_dct_data_t*)dct1;
}

void platform_erase_dct( void )
{
    ERASE_DCT_1();
    ERASE_DCT_2();
}

/* ---------------------------------------------------------------------------------------------
 *  STM32F1 Flash programming limitation (taken from PM0075 section 2.3.3)
 * ---------------------------------------------------------------------------------------------
 * In this mode the CPU programs the main Flash memory by performing standard half-word
 * write operations. The PG bit in the FLASH_CR register must be set. FPEC preliminarily
 * reads the value at the addressed main Flash memory location and checks that it has been
 * erased. If not, the program operation is skipped and a warning is issued by the PGERR bit in
 * FLASH_SR register (the only exception to this is when 0x0000 is programmed. In this case,
 * the location is correctly programmed to 0x0000 and the PGERR bit is not set)
 * ---------------------------------------------------------------------------------------------
 *
 * TODO: Disable interrupts during function
 * Note Function allocates a chunk of memory for the bootloader data on the stack - ensure the stack is big enough
 */
int platform_write_dct( uint16_t data_start_offset, const void* data, uint16_t data_length, int8_t app_valid, void (*func)(void) )
{
    platform_dct_header_t* curr_dct = &platform_get_dct( )->dct_header;
    platform_dct_header_t* new_dct;
    uint32_t write_size;
    uint32_t bytes_after_data    = ( PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS ) - ( sizeof(platform_dct_header_t) + data_start_offset + data_length );
    uint8_t* orig_section1_start;
    uint8_t* orig_section1_end;
    uint8_t* orig_section2_start;
    uint8_t* orig_section2_end;
    uint8_t* orig_section3_start;
    uint8_t* orig_section3_end;
    uint8_t* section1_start;
    uint8_t* section1_end;
    uint8_t* section2_start;
    uint8_t* section2_end;
    uint8_t* section3_start;
    uint8_t* section3_end;
    uint8_t* orig_section1_src;
    uint8_t* orig_section2_src;
    uint8_t* orig_section3_src;
    uint8_t* section1_src;
    uint8_t* section2_src;
    uint8_t* section3_src;
    int      result = 0;

    char zero_byte[2] = {0};

    platform_dct_header_t hdr =
    {
        .write_incomplete = 1,
        .is_current_dct   = 1,
        .magic_number     = BOOTLOADER_MAGIC_NUMBER
    };

    if ( data_length + data_start_offset > ( PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS ) )
    {
        /* Data too big to write */
        result = -1;
        goto exit;
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

    orig_section1_start = (uint8_t*)&new_dct[1];
    orig_section1_end   = orig_section1_start + data_start_offset;
    orig_section2_start = orig_section1_end;
    orig_section2_end   = orig_section2_start + data_length;
    orig_section3_start = orig_section2_end;
    orig_section3_end   = orig_section3_start + bytes_after_data;
    section1_start      = ( ( (uint32_t)orig_section1_start % 2) != 0 ) ? orig_section1_start + 1 : orig_section1_start;
    section1_end        = ( ( (uint32_t)orig_section1_end   % 2) != 0 ) ? orig_section1_end   - 1 : orig_section1_end;
    section2_start      = ( ( (uint32_t)orig_section2_start % 2) != 0 ) ? orig_section2_start + 1 : orig_section2_start;
    section2_end        = ( ( (uint32_t)orig_section2_end   % 2) != 0 ) ? orig_section2_end   - 1 : orig_section2_end;
    section3_start      = ( ( (uint32_t)orig_section3_start % 2) != 0 ) ? orig_section3_start + 1 : orig_section3_start;
    section3_end        = ( ( (uint32_t)orig_section3_end   % 2) != 0 ) ? orig_section3_end   - 1 : orig_section3_end;
    orig_section1_src   = (uint8_t*)&curr_dct[1];
    orig_section2_src   = (uint8_t*)data;
    orig_section3_src   = (uint8_t*)orig_section1_src + data_start_offset + data_length;
    section1_src        = ( ( (uint32_t)orig_section1_start % 2 ) != 0 ) ? orig_section1_src + 1 : orig_section1_src;
    section2_src        = ( ( (uint32_t)orig_section2_start % 2 ) != 0 ) ? orig_section2_src + 1 : orig_section2_src;
    section3_src        = ( ( (uint32_t)orig_section3_start % 2 ) != 0 ) ? orig_section3_src + 1 : orig_section3_src;

    write_size = (uint32_t)( section1_end - section1_start );
    platform_write_flash_chunk( (uint32_t)section1_start, section1_src, write_size );

    if ( section1_end != section2_start )
    {
        uint8_t data_to_write[2];

        data_to_write[0] = *( orig_section1_src + data_start_offset - 1 );
        data_to_write[1] = *( orig_section2_src );

        platform_write_flash_chunk( (uint32_t)section1_end, data_to_write, 2 );
    }

    if ( memcmp( orig_section1_start, orig_section1_src, (uint32_t)( orig_section1_end - orig_section1_start ) ) != 0 )
    {
        result = -2;
        goto exit;
    }

    write_size = (uint32_t)( section2_end - section2_start );
    platform_write_flash_chunk( (uint32_t)section2_start, section2_src, write_size );

    if ( section2_end != section3_start )
    {
        uint8_t data_to_write[2];

        data_to_write[0] = *( orig_section2_src + data_length - 1 );
        data_to_write[1] = *( orig_section3_src );

        platform_write_flash_chunk( (uint32_t)section2_end, data_to_write, 2 );
    }

    if ( memcmp( orig_section2_start, orig_section2_src, (uint32_t)( orig_section2_end - orig_section2_start ) ) != 0 )
    {
        result = -3;
        goto exit;
    }

    write_size = (uint32_t)( section3_end - section3_start );
    platform_write_flash_chunk( (uint32_t)section3_start, section3_src, write_size );

    if ( memcmp( orig_section3_start, orig_section3_src, (uint32_t)( orig_section3_end - orig_section3_start ) ) != 0 )
    {
        result = -4;
        goto exit;
    }

    hdr.app_valid           = (char) (( app_valid == -1 )? curr_dct->app_valid : app_valid);
    hdr.load_app_func       = func;
    hdr.mfg_info_programmed = curr_dct->mfg_info_programmed;
    hdr.write_incomplete    = 0;
    hdr.is_current_dct      = 1;

    /* Write the header data */
    platform_write_flash_chunk( (uint32_t)new_dct, (uint8_t*) &hdr, sizeof(hdr) );

    /* Verify the header data */
    if ( memcmp( (void*)new_dct, (void*)&hdr, 16 ) != 0 )
    {
        result = -5;
        goto exit;
    }

    /* Mark new DCT as complete and current */
    platform_write_flash_chunk( (uint32_t)&curr_dct->is_current_dct, (uint8_t*) &zero_byte, sizeof(zero_byte) );

    exit:
    return result;
}

int platform_erase_flash( uint16_t start_sector, uint16_t end_sector )
{
    uint32_t i;

    /* Unlock the STM32 Flash */
    FLASH_Unlock( );

    /* Clear any error flags */
    FLASH_ClearFlag( FLASH_FLAG_EOP | FLASH_FLAG_WRPRTERR | FLASH_FLAG_PGERR );

    watchdog_kick( );

    for ( i = start_sector; i < end_sector; i += 1 )
    {
        if ( FLASH_ErasePage( i * PLATFORM_FLASH_PAGE_SIZE + FLASH_START_ADDR ) != FLASH_COMPLETE )
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
    uint32_t  remainder  = address % sizeof(uint16_t);
    uint16_t* dst_ptr    = ( remainder > 0 ) ? (uint16_t*)((uint8_t*)address - sizeof(uint16_t) + remainder ) : (uint16_t*)address;
    uint8_t*  src_ptr    = (uint8_t*)dst_ptr;
    uint8_t*  data_ptr   = (uint8_t*)data;
    uint8_t*  data_start = (uint8_t*)address;
    uint8_t*  data_end   = data_start + size;

    FLASH_Unlock( );

    /* Clear any error flags */
    FLASH_ClearFlag( FLASH_FLAG_EOP | FLASH_FLAG_WRPRTERR | FLASH_FLAG_PGERR );

    while ( (uint32_t)dst_ptr < (uint32_t)data_end )
    {
        uint8_t   word_to_write[2];
        uint16_t* word_to_write_ptr = (uint16_t*)word_to_write;
        uint16_t  byte;
        FLASH_Status status;

        UNUSED_PARAMETER( word_to_write );

        for ( byte = 0; byte < 2; byte++ )
        {
            if ( ( src_ptr >= data_start ) && ( src_ptr < data_end ) )
            {
                word_to_write[byte] = *data_ptr++;
            }
            else
            {
                word_to_write[byte] = *src_ptr;
            }

            src_ptr++;
        }

        status = FLASH_WRITE_FUNC( (uint32_t)dst_ptr, *word_to_write_ptr );

        if ( status != FLASH_COMPLETE)
        {
            FLASH_ClearFlag( FLASH_FLAG_PGERR );
        }

        dst_ptr++;
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
#ifndef PLATFORM_DISABLE_UART_DMA
    // Clear all interrupts. It's safe to do so because only RXNE interrupt is enabled
//    USART1->SR = 0;

    // Update tail
    uart_interfaces[0].rx_buffer->tail = uart_interfaces[0].rx_buffer->size - uart_mapping[0].rx_dma_channel->CNDTR;

    // Notify thread if sufficient data are available
    if ( ( uart_interfaces[0].rx_size > 0 ) && ( ring_buffer_used_space( uart_interfaces[0].rx_buffer ) >= uart_interfaces[0].rx_size ) )
    {
        host_rtos_set_semaphore( &uart_interfaces[0].rx_complete, WICED_TRUE );
        uart_interfaces[0].rx_size = 0;
    }
#else
    if (USART1->SR & USART_SR_RXNE)
    {
        uint8_t data = (uint8_t) (USART1->DR & 0xff);
        ring_buffer_write(uart_interfaces[0].rx_buffer, &data, 1);

        if ( ( uart_interfaces[0].rx_size > 0 ) && ( ring_buffer_used_space( uart_interfaces[0].rx_buffer ) >= uart_interfaces[0].rx_size ) )
        {
            host_rtos_set_semaphore( &uart_interfaces[0].rx_complete, WICED_TRUE );
            uart_interfaces[0].rx_size = 0;
        }
    }

    if (USART1->SR & USART_SR_TXE)
    {
        if (uart_interfaces[0].tx_size != 0)
        {
            USART1->DR = *uart_interfaces[0].tx_buffer;
            ++uart_interfaces[0].tx_buffer;
            --uart_interfaces[0].tx_size;
        }
        else
        {
            if (uart_interfaces[0].tx_buffer != NULL)
            {
                uart_interfaces[0].tx_buffer = NULL;
                uart_mapping[0].usart->CR1 &= (uint16_t) ~USART_CR1_TXEIE;
                USART1->SR = (uint16_t) ~USART_SR_TXE;
                host_rtos_set_semaphore( &uart_interfaces[0].tx_complete, WICED_TRUE );
            }
        }
    }
#endif
}

void usart2_irq( void )
{
    // Clear all interrupts. It's safe to do so because only RXNE interrupt is enabled
    USART2->SR = 0;

    // Update tail
    uart_interfaces[1].rx_buffer->tail = uart_interfaces[1].rx_buffer->size - uart_mapping[1].rx_dma_channel->CNDTR;

    // Notify thread if sufficient data are available
    if ( ( uart_interfaces[1].rx_size > 0 ) && ( ring_buffer_used_space( uart_interfaces[1].rx_buffer ) >= uart_interfaces[1].rx_size ) )
    {
        host_rtos_set_semaphore( &uart_interfaces[1].rx_complete, WICED_TRUE );
        uart_interfaces[1].rx_size = 0;
    }
}
#ifndef PLATFORM_DISABLE_UART_DMA
void usart1_tx_dma_irq( void )
{
    if ( ( uart_mapping[0].tx_dma->ISR & DMA_IFCR_CTCIF4 ) != 0 )
    {
        uart_mapping[0].tx_dma->IFCR |= DMA_IFCR_CTCIF4;
        host_rtos_set_semaphore( &uart_interfaces[0].tx_complete, WICED_TRUE );
    }

    /* TX DMA error */
    if ( ( uart_mapping[0].tx_dma->ISR & DMA_IFCR_CTEIF4 )!= 0 )
    {
        /* Clear interrupt */
    	uart_mapping[0].tx_dma->IFCR |= DMA_IFCR_CTEIF4;
    }
}

void usart2_tx_dma_irq( void )
{
    if ( (uart_mapping[1].tx_dma->ISR & DMA_IFCR_CTCIF6) != 0 )
    {
        uart_mapping[1].tx_dma->IFCR |= DMA_IFCR_CTCIF6;
        host_rtos_set_semaphore( &uart_interfaces[1].tx_complete, WICED_TRUE );
    }

    /* TX DMA error */
    if ( (uart_mapping[1].tx_dma->ISR & DMA_IFCR_CTEIF6) != 0 )
    {
        /* Clear interrupt */
        uart_mapping[1].tx_dma->IFCR |= DMA_IFCR_CTEIF6;
    }
}

void usart1_rx_dma_irq( void )
{
    if ( ( DMA2->ISR & DMA_IFCR_CTCIF5)  != 0 )
    {
        DMA2->IFCR |= DMA_IFCR_CTCIF5;
        host_rtos_set_semaphore( &uart_interfaces[0].rx_complete, WICED_TRUE );
    }

    /* TX DMA error */
    if ( ( DMA2->ISR & DMA_IFCR_CTEIF5 ) != 0 )
    {
        /* Clear interrupt */
        DMA2->IFCR |= DMA_IFCR_CTEIF5;
    }
}

void usart2_rx_dma_irq( void )
{
    if ( ( DMA1->ISR & DMA_IFCR_CTCIF5 ) != 0 )
    {
        DMA1->IFCR |= DMA_IFCR_CTCIF5;
        host_rtos_set_semaphore( &uart_interfaces[1].rx_complete, WICED_TRUE );
//        DMA1_Stream5 ->NDTR = uart_interfaces[1].rx_buffer->size;
    }

    /* TX DMA error */
    if ( ( DMA1->ISR & DMA_IFCR_CTEIF5 ) != 0 )
    {
        /* Clear interrupt */
        DMA1->IFCR |= DMA_IFCR_CTEIF5;
    }
}
#endif

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

#define RTC_INTERRUPT_EXTI_LINE EXTI_Line17

#define ENABLE_INTERRUPTS   __asm("CPSIE i")
#define DISABLE_INTERRUPTS  __asm("CPSID i")

int stm32f2_clock_needed_counter = 0;

#ifdef STOP_MODE_SUPPORT
void stm32f1xx_clocks_needed( void )
{
    DISABLE_INTERRUPTS;
    if ( stm32f2_clock_needed_counter <= 0 )
    {
        SCB->SCR &= (~((unsigned long)SCB_SCR_SLEEPDEEP_Msk));
        stm32f2_clock_needed_counter = 0;
    }
    stm32f2_clock_needed_counter++;
    ENABLE_INTERRUPTS;
}

void stm32f1xx_clocks_not_needed( void )
{
    DISABLE_INTERRUPTS;
    stm32f2_clock_needed_counter--;
    if ( stm32f2_clock_needed_counter <= 0 )
    {
        SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
        stm32f2_clock_needed_counter = 0;
    }
    ENABLE_INTERRUPTS;
}
#endif /* #ifdef STOP_MODE_SUPPORT */

void RTC_init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    GPIO_InitTypeDef gpio_init_structure;

    /* set external oscillator gpios to high impedance state */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC, ENABLE );
    gpio_init_structure.GPIO_Pin   = (uint16_t) ( 1 << 14 );
    gpio_init_structure.GPIO_Speed = (GPIOSpeed_TypeDef) 0;
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init( GPIOC, &gpio_init_structure );

    gpio_init_structure.GPIO_Pin   = (uint16_t) ( 1 << 15 );
    gpio_init_structure.GPIO_Speed = (GPIOSpeed_TypeDef) 0;
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init( GPIOC, &gpio_init_structure );


    /* Enable the PWR clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    /* Allow access to BKP Domain */
    PWR_BackupAccessCmd(ENABLE);

    /* RTC clock source configuration ------------------------------------------*/
    /* Enable the LSE OSC */
    RCC_LSEConfig(RCC_LSE_ON);
    /* Wait till LSE is ready */
    while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
    {
    }
    /* Select the RTC Clock Source */
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

    /* Enable the RTC Clock */
    RCC_RTCCLKCmd( ENABLE );
    /* RTC configuration -------------------------------------------------------*/
    /* Wait for RTC APB registers synchronisation */
    RTC_WaitForSynchro();

    /* clear alarm flag in the RTC control register and in , all previous alarms will be cleared */
    RTC_ClearFlag( RTC_FLAG_ALR );
    RTC_WaitForLastTask();
    RTC_ClearITPendingBit( RTC_IT_ALR );
    RTC_WaitForLastTask();
    /* setup a prescaler of the RTC clock to 32 to get 1KHz frequency, RTC counter will get incremented every millisecond */
    RTC_SetPrescaler( 32 );
    RTC_WaitForLastTask();

    /* clear external interrupt line that is wired to the the RTC alarm interrupt */
    EXTI_ClearITPendingBit( RTC_INTERRUPT_EXTI_LINE );
    EXTI_InitStructure.EXTI_Line = RTC_INTERRUPT_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    /* initialise the external interrupt for RTC ALARM trigger */
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init( &NVIC_InitStructure );
    RTC_ITConfig( RTC_IT_ALR, DISABLE );
    /* Prepare Stop-Mode but leave disabled */
//    PWR_ClearFlag(PWR_FLAG_WU);
    PWR->CR |= PWR_CR_LPDS;
    PWR->CR &= (unsigned long)(~(PWR_CR_PDDS));
    SCB->SCR |= ((unsigned long)SCB_SCR_SLEEPDEEP_Msk);
}

#ifdef STOP_MODE_SUPPORT
static uint32_t next_alarm_time = 0;
static wiced_bool_t wake_up_interrupt_triggered = WICED_FALSE;

void wake_up_interrupt_notify( void );
static unsigned long stop_mode_power_down_hook( unsigned long delay_ms );
#else
static unsigned long idle_power_down_hook( unsigned long delay_ms );
#endif

#ifdef STOP_MODE_SUPPORT

void wake_up_interrupt_notify( void )
{
    wake_up_interrupt_triggered = WICED_TRUE;
}


/**
 * This function will return the value of time read from the on board CPU real time clock. Time value must be given in the format of
 * the structure wiced_rtc_time_t
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_platform_get_rtc_time(wiced_rtc_time_t* time)
{
    UNUSED_PARAMETER(time);
    /* just stubs for now */
    return WICED_UNSUPPORTED;
}

/**
 * This function will set MCU RTC time to a new value. Time value must be given in the format of
 * the structure wiced_rtc_time_t
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_platform_set_rtc_time(wiced_rtc_time_t* time)
{
    UNUSED_PARAMETER(time);
    /* just stubs for now */
    return WICED_UNSUPPORTED;
}


unsigned long stop_mode_power_down_hook( unsigned long delay_ms )
{
    /* check if we are about to go to deep sleep, if not we will doe clock gating on a cpu rather */
    /* then on the MCU */
    if ( ( ( SCB->SCR & (unsigned long)SCB_SCR_SLEEPDEEP_Msk) ) != 0 )
    {
        unsigned long retval;
        /* next alarm time will be equal to the number of ticks the system will stay in power down mode */
        if( delay_ms != 0xFFFFFFFF )
        {
            next_alarm_time = delay_ms ;
        }
        else
        {
            next_alarm_time =  0xFFFFFFFF;
        }

        DISABLE_INTERRUPTS;
        SysTick->CTRL &= (~(SysTick_CTRL_TICKINT_Msk|SysTick_CTRL_ENABLE_Msk)); /* systick IRQ off */

        /* clear external interrupt flag that is wired to the RTC ALARM interrupt  */
        EXTI_ClearITPendingBit( RTC_INTERRUPT_EXTI_LINE );
        //PWR_ClearFlag(PWR_FLAG_WU);

        /* reset real time clock counter and set alarm counter */
        RTC_WaitForLastTask();
        RTC_SetCounter(0);
        RTC_WaitForLastTask();
        RTC_SetAlarm( next_alarm_time );
        RTC_WaitForLastTask();
        /* activate alarm */
        /* enable RTC alarm interrupt */
        RTC_ITConfig(RTC_IT_ALR, ENABLE);
        RTC_WaitForLastTask();
        RTC_ClearFlag( RTC_FLAG_ALR );
        /* Enable debug in stop mode */
        DBGMCU->CR |= 0x03;
        ENABLE_INTERRUPTS; /* !!!!!!!! */
        /* wfi instruction must came first after ENABLE_INTERRUPTS otherwise interrupts that occured */
        /* between DISABLE_INTERRUPTS and ENABLE_INTERRUPTS can get processed before the CPU goes to sleep */
        /* if the interrupt which is intended to wake up a CPU is already in pending state */
        /* CPU will jump to the interrupt vector, execute the interrupt handler and clear the pending */
        /* interrupt bit. Afterwards it will jump back to the wfi instruction address */
        /* and go to sleep thinking that the interrupt should come and expecting it */

        /* check whether the wake-up capable interrupt occured, while interrupts were disabled */
        if( wake_up_interrupt_triggered == 0 )
        {
            /* the only place where we can miss a wake up interrupt is 1 instruction gap( read and compare ), when interrupt */
            /* may still be triggered */
            __asm("wfi");
        }
        else
        {
            DISABLE_INTERRUPTS;
            RTC_ITConfig(RTC_IT_ALR, DISABLE);
            SysTick->CTRL |= (SysTick_CTRL_TICKINT_Msk|SysTick_CTRL_ENABLE_Msk);
            wake_up_interrupt_triggered = WICED_FALSE;
            ENABLE_INTERRUPTS;
            return 0;
        }

        DISABLE_INTERRUPTS;
        RTC_WaitForLastTask();
        /* deactivate alarm */
        RTC_ITConfig(RTC_IT_ALR, DISABLE);
        RTC_WaitForLastTask();
        /* setup clocks again, cause they were switched of completely prior to the entry to the stop mode */
        init_clocks( );

        /* systick IRQ on */
        SysTick->CTRL |= (SysTick_CTRL_TICKINT_Msk|SysTick_CTRL_ENABLE_Msk);

        /* find out how many ticks we have been sitting with a power switched off */
        /* this value will be needed by the underlying RTOS to update its timers */
        /* bear in mind that when the clocks are switched of the ticks are no longer received by the RTOS */
        RTC_WaitForSynchro();
        retval = RTC_GetCounter();
        RTC_WaitForLastTask();
        ENABLE_INTERRUPTS;
        wake_up_interrupt_triggered = WICED_FALSE;
        return retval;
    }
    else
    {
        ENABLE_INTERRUPTS;
        __asm("wfi");
        /*system tick is still going when wfi instruction gets executed */
        return 0;
    }


    return 0;

}
#else /* STOP_MODE_SUPPORT */

unsigned long idle_power_down_hook( unsigned long delay_ms  )
{
    UNUSED_PARAMETER( delay_ms );
    ENABLE_INTERRUPTS;
    __asm("wfi");
    return 0;
}

#endif /* STOP_MODE_SUPPORT */

unsigned long platform_power_down_hook( unsigned long delay_ms )
{
#ifdef STOP_MODE_SUPPORT
    return stop_mode_power_down_hook( delay_ms );
#else /* STOP_MODE_SUPPORT */
    return idle_power_down_hook( delay_ms );
#endif /* STOP_MODE_SUPPORT */
}

void RTCAlarm_irq( void )
{
    __set_PRIMASK( 1 );
    EXTI_ClearITPendingBit( RTC_INTERRUPT_EXTI_LINE );
    PWR_ClearFlag(PWR_FLAG_WU);
    RTC_WaitForLastTask();
    RTC_ClearFlag(RTC_FLAG_ALR);
    RTC_WaitForLastTask();
    RTC_ITConfig(RTC_IT_ALR, DISABLE);
    RTC_ClearITPendingBit(RTC_IT_ALR);

    init_clocks( );
    __set_PRIMASK( 0 );
}

void wiced_platform_mcu_enable_powersave( void )
{
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* ifdef STOP_MODE_SUPPORT  */
    return;
}

void wiced_platform_mcu_disable_powersave( void )
{
#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* ifdef STOP_MODE_SUPPORT  */
    return;
}

void platform_idle_hook( void )
{
    __asm("wfi");
}

void host_platform_get_mac_address( wiced_mac_t* mac )
{
#ifndef WICED_DISABLE_BOOTLOADER
    memcpy(mac->octet, bootloader_api->get_wifi_config_dct()->mac_address.octet, sizeof(wiced_mac_t));
#else
    UNUSED_PARAMETER( mac );
#endif
}
