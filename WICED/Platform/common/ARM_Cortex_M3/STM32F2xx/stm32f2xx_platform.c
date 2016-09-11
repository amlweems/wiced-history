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
#include "stm32f2xx_platform.h"
#include "stm32f2xx_flash.h"
#include "wwd_constants.h"
#include "wwd_assert.h"
#include "platform.h"
#include "platform_common_config.h"
#include "wiced_platform.h"
#include "gpio_irq.h"
#include "watchdog.h"
#include "platform_dct.h"
#include <string.h> // For memcmp
#include "Platform/wwd_platform_interface.h"
#include "stm32f2xx_i2c.h"
#include "crt0.h"
#include "platform_sleep.h"
#include "rtc.h"

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

#ifndef WICED_DISABLE_MCU_POWERSAVE
#define MCU_CLOCKS_NEEDED()       stm32f2xx_clocks_needed()
#define MCU_CLOCKS_NOT_NEEDED()   stm32f2xx_clocks_not_needed()
#define MCU_RTC_INIT()            RTC_Wakeup_init()
#else
#define MCU_CLOCKS_NEEDED()
#define MCU_CLOCKS_NOT_NEEDED()
#ifdef WICED_ENABLE_MCU_RTC
#define MCU_RTC_INIT() platform_rtc_init()
#else /* #ifdef WICED_ENABLE_MCU_RTC */
#define MCU_RTC_INIT()
#endif /* #ifdef WICED_ENABLE_MCU_RTC */
#endif /* ifndef WICED_DISABLE_MCU_POWERSAVE */

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

#define I2C_FLAG_CHECK_TIMEOUT      ( 1000 )
#define I2C_FLAG_CHECK_LONG_TIMEOUT ( 1000 )


#define I2C_MESSAGE_DMA_MASK_POSN 0
#define I2C_MESSAGE_NO_DMA    (0 << I2C_MESSAGE_DMA_MASK_POSN) /* No DMA is set to 0 because DMA should be enabled by */
#define I2C_MESSAGE_USE_DMA   (1 << I2C_MESSAGE_DMA_MASK_POSN) /* default, and turned off as an exception */

wiced_result_t wiced_i2c_init_tx_message(wiced_i2c_message_t* message, const void* tx_buffer, uint16_t  bufffer_length, uint16_t retries , wiced_bool_t disable_dma);
wiced_result_t wiced_i2c_init_rx_message(wiced_i2c_message_t* message, void* rx_buffer, uint16_t  bufffer_length, uint16_t retries , wiced_bool_t disable_dma);
wiced_result_t wiced_i2c_init_combined_message(wiced_i2c_message_t* message, const void* tx_buffer, void* rx_buffer, uint16_t tx_buffer_length, uint16_t rx_buffer_length, uint16_t retries , wiced_bool_t disable_dma);


#define DMA_FLAG_TC(stream_id) dma_flag_tc(stream_id)
#define DMA_TIMEOUT_LOOPS      (10000000)

#define NUMBER_OF_LSE_TICKS_PER_MILLISECOND(scale_factor) ( 32768 / 1000 / scale_factor )
#define CONVERT_FROM_TICKS_TO_MS(n,s) ( n / NUMBER_OF_LSE_TICKS_PER_MILLISECOND(s) )
#define CK_SPRE_CLOCK_SOURCE_SELECTED 0xFFFF

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
static wiced_result_t spi_dma_transfer            ( const wiced_spi_device_t* spi );
static void           spi_dma_config              ( const wiced_spi_device_t* spi, wiced_spi_message_segment_t* message );

static wiced_result_t platform_uart_init ( wiced_uart_t uart, const wiced_uart_config_t* config, wiced_ring_buffer_t* optional_rx_buffer );
static wiced_result_t platform_uart_receive_bytes( wiced_uart_t uart, void* data, uint32_t size, uint32_t timeout );

#ifndef WICED_DISABLE_MCU_POWERSAVE
void stm32f2xx_clocks_needed    ( void );
void stm32f2xx_clocks_not_needed( void );
static void RTC_Wakeup_init( void );
#endif

#if defined(WICED_ENABLE_MCU_RTC) && defined(WICED_DISABLE_MCU_POWERSAVE)
void platform_rtc_init( void );
#endif /* #if defined(WICED_ENABLE_MCU_RTC) && defined(WICED_DISABLE_MCU_POWERSAVE) */
void wake_up_interrupt_notify( void );

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

/* These come from the linker script */
extern void* dct1_start_addr_loc;
extern void* dct1_size_loc;
extern void* dct2_start_addr_loc;
extern void* dct2_size_loc;
extern void* app_hdr_start_addr_loc;
extern void* sram_start_addr_loc;
extern void* sram_size_loc;

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

static DMA_InitTypeDef  i2c_dma_init; /* Should investigate why this is global */

static wiced_spi_device_t* current_spi_device = NULL;

//#ifdef RTC_ENABLED
wiced_rtc_time_t wiced_default_time =
{
     /* set it to 12:20:30 08/04/2013 monday */
     .sec   = 30,
     .min   = 20,
     .hr    = 12,
     .weekday  = 1,
     .date  = 8,
     .month = 4,
     .year  = 13
};
//#endif /* #ifdef RTC_ENABLED */

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

#ifndef WICED_DISABLE_MCU_POWERSAVE
static wiced_bool_t wake_up_interrupt_triggered  = WICED_FALSE;
static unsigned long rtc_timeout_start_time           = 0;
#endif /* #ifndef WICED_DISABLE_MCU_POWERSAVE */

/******************************************************
 *               Function Definitions
 ******************************************************/

/* STM32F2 common clock initialisation function
 * This brings up enough clocks to allow the processor to run quickly while initialising memory.
 * Other platform specific clock init can be done in init_platform() or init_architecture()
 */
WEAK void init_clocks( void )
{
    //RCC_DeInit( ); /* if not commented then the LSE PA8 output will be disabled and never comes up again */

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
     * http://www.st.com/st-web-ui/static/active/en/st_prod_software_internet/resource/technical/software/utility/stsw-stm32090.zip
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

#ifdef INTERRUPT_VECTORS_IN_RAM
    SCB->VTOR = 0x20000000; /* Change the vector table to point to start of SRAM */
#endif /* ifdef INTERRUPT_VECTORS_IN_RAM */

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

    MCU_RTC_INIT();

    /* Disable MCU powersave at start-up. Application must explicitly enable MCU powersave if desired */
    MCU_CLOCKS_NEEDED();

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

    MCU_CLOCKS_NEEDED();

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

    MCU_CLOCKS_NOT_NEEDED();

    current_spi_device = (wiced_spi_device_t*)spi;

    return WICED_SUCCESS;
}

wiced_result_t wiced_spi_transfer( const wiced_spi_device_t* spi, wiced_spi_message_segment_t* segments, uint16_t number_of_segments )
{
    wiced_result_t result = WICED_SUCCESS;
    uint16_t       i;
    uint32_t       count = 0;

    wiced_assert("Bad args", (spi != NULL) && (segments != NULL) && (number_of_segments != 0));

    MCU_CLOCKS_NEEDED();

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
                const uint8_t* send_ptr = ( const uint8_t* )segments[i].tx_buffer;
                uint8_t*       rcv_ptr  = ( uint8_t* )segments[i].rx_buffer;
                count = segments[i].length;
                while ( count-- )
                {
                    uint16_t data;
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
                    data = SPI_I2S_ReceiveData( spi_mapping[spi->port].spi_regs );

                    if ( rcv_ptr != NULL )
                    {
                        *rcv_ptr++ = (uint8_t)data;
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
                    uint16_t data = 0xFFFF;
                    count -= 2;

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
cleanup_transfer:
    wiced_gpio_output_high(spi->chip_select);

    MCU_CLOCKS_NOT_NEEDED();

    return result;
}

wiced_result_t wiced_spi_deinit( const wiced_spi_device_t* spi )
{
    GPIO_InitTypeDef gpio_init_structure;

    MCU_CLOCKS_NEEDED();

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

    MCU_CLOCKS_NOT_NEEDED();

    return WICED_SUCCESS;
}

wiced_result_t wiced_i2c_init( wiced_i2c_device_t* device  )
{
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef  I2C_InitStructure;

    wiced_assert( "bad argument", device != NULL );

    MCU_CLOCKS_NEEDED();

    // Init I2C GPIO clocks
    RCC_APB1PeriphClockCmd( i2c_mapping[device->port].peripheral_clock_reg, ENABLE );
    RCC_AHB1PeriphClockCmd( i2c_mapping[device->port].pin_scl->peripheral_clock | i2c_mapping[device->port].pin_sda->peripheral_clock, ENABLE );
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_SYSCFG, ENABLE );

    // Reset the I2C clock
    RCC_APB1PeriphResetCmd( i2c_mapping[device->port].peripheral_clock_reg, ENABLE );
    RCC_APB1PeriphResetCmd( i2c_mapping[device->port].peripheral_clock_reg, DISABLE );

    // GPIO Configuration
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;

    GPIO_InitStructure.GPIO_Pin = (uint32_t)( 1 << i2c_mapping[device->port].pin_scl->number );
    GPIO_Init( i2c_mapping[device->port].pin_scl->bank, &GPIO_InitStructure );
    GPIO_InitStructure.GPIO_Pin = (uint32_t)( 1 << i2c_mapping[device->port].pin_sda->number );
    GPIO_Init( i2c_mapping[device->port].pin_sda->bank, &GPIO_InitStructure );

    // Configure SDA and SCL as I2C pins
    GPIO_PinAFConfig( i2c_mapping[device->port].pin_scl->bank, (uint16_t)i2c_mapping[device->port].pin_scl->number, (uint8_t)i2c_mapping[device->port].gpio_af );
    GPIO_PinAFConfig( i2c_mapping[device->port].pin_sda->bank, (uint16_t)i2c_mapping[device->port].pin_sda->number, (uint8_t)i2c_mapping[device->port].gpio_af );

    if ( device->flags & I2C_DEVICE_USE_DMA )
    {
        // Enable the DMA clock
        RCC_AHB1PeriphClockCmd( i2c_mapping[device->port].tx_dma_peripheral_clock, ENABLE );

        // Configure the DMA streams for operation with the CP
        i2c_dma_init.DMA_Channel            = i2c_mapping[device->port].tx_dma_channel;
        i2c_dma_init.DMA_PeripheralBaseAddr = (uint32_t)&i2c_mapping[device->port].i2c->DR;
        i2c_dma_init.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
        i2c_dma_init.DMA_MemoryInc          = DMA_MemoryInc_Enable;
        i2c_dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        i2c_dma_init.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
        i2c_dma_init.DMA_Mode               = DMA_Mode_Normal;
        i2c_dma_init.DMA_Priority           = DMA_Priority_VeryHigh;
        //dma_init.DMA_FIFOMode           = DMA_FIFOMode_Enable;
        //dma_init.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full;
        i2c_dma_init.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
        i2c_dma_init.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;
        i2c_dma_init.DMA_Memory0BaseAddr    = (uint32_t) 0;               // This parameter will be configured during communication
        i2c_dma_init.DMA_DIR                = DMA_DIR_MemoryToPeripheral; // This parameter will be configured during communication
        i2c_dma_init.DMA_BufferSize         = 0xFFFF;                     // This parameter will be configured during communication

        DMA_DeInit( i2c_mapping[device->port].rx_dma_stream );
        DMA_DeInit( i2c_mapping[device->port].tx_dma_stream );

        // Clear any pending flags, disable, and clear the Tx DMA channel
        //DMA_ClearFlag( i2c_mapping[device->port].tx_dma_stream, CP_TX_DMA_FLAG_FEIF | CP_TX_DMA_FLAG_DMEIF | CP_TX_DMA_FLAG_TEIF | CP_TX_DMA_FLAG_HTIF | CP_TX_DMA_FLAG_TCIF );
        DMA_Cmd( i2c_mapping[device->port].tx_dma_stream, DISABLE );
        DMA_Cmd( i2c_mapping[device->port].rx_dma_stream, DISABLE );

        // Clear any pending flags, disable, and clear the Rx DMA channel
        //DMA_ClearFlag( i2c_mapping[device->port].rx_dma_stream, CP_RX_DMA_FLAG_FEIF | CP_RX_DMA_FLAG_DMEIF | CP_RX_DMA_FLAG_TEIF | CP_RX_DMA_FLAG_HTIF | CP_RX_DMA_FLAG_TCIF );
    }

    // Initialize the InitStruct for the CP
    I2C_InitStructure.I2C_Mode                = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle           = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1         = 0xA0;
    I2C_InitStructure.I2C_Ack                 = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

    if ( device->speed_mode == I2C_LOW_SPEED_MODE )
    {
        I2C_InitStructure.I2C_ClockSpeed = 10000;
    }
    else if ( device->speed_mode == I2C_STANDARD_SPEED_MODE )
    {
        I2C_InitStructure.I2C_ClockSpeed = 100000;
    }
    else if ( device->speed_mode == I2C_HIGH_SPEED_MODE )
    {
        I2C_InitStructure.I2C_ClockSpeed = 400000;
    }

    // Enable and initialize the I2C bus
    I2C_Cmd ( i2c_mapping[device->port].i2c, ENABLE );
    I2C_Init( i2c_mapping[device->port].i2c, &I2C_InitStructure );

    if ( device->flags & I2C_DEVICE_USE_DMA )
    {
        // Enable DMA on the I2C bus
        I2C_DMACmd( i2c_mapping[device->port].i2c, ENABLE );
    }

    MCU_CLOCKS_NOT_NEEDED();

    return WICED_SUCCESS;
}

static wiced_result_t i2c_wait_for_event( I2C_TypeDef* i2c, uint32_t event_id, uint32_t number_of_waits )
{
    wiced_assert( "bad argument", i2c != NULL );

    while ( I2C_CheckEvent( i2c, event_id ) != SUCCESS )
    {
        number_of_waits--;
        if ( number_of_waits == 0 )
        {
            return WICED_TIMEOUT;
        }
    }

    return WICED_SUCCESS;
}

static uint32_t dma_flag_tc( int stream_id )
{
    const uint32_t transfer_complete_flags[]=
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

    return transfer_complete_flags[stream_id];
}

wiced_bool_t wiced_i2c_probe_device( wiced_i2c_device_t* device, int retries )
{
    int            i;
    wiced_result_t result;

    wiced_assert("Bad args", device != NULL);

    MCU_CLOCKS_NEEDED();

    for ( i = 0; i < retries; i++ )
    {
        /* generate a start condition and address a device in write mode */
        I2C_GenerateSTART( i2c_mapping[device->port].i2c, ENABLE );

        /* wait till start condition is generated and the bus becomes free */
        result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_MODE_SELECT, I2C_FLAG_CHECK_TIMEOUT );
        if ( result != WICED_SUCCESS )
        {
            MCU_CLOCKS_NOT_NEEDED();
            return WICED_FALSE;
        }

        if ( device->address_width == I2C_ADDRESS_WIDTH_7BIT )
        {
            /* send the address and R/W bit set to write of the requested device, wait for an acknowledge */
            I2C_Send7bitAddress( i2c_mapping[device->port].i2c, (uint8_t) ( device->address << 1 ), I2C_Direction_Transmitter );

            /* wait till address gets sent and the direction bit is sent and */
            result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
            if ( result != WICED_SUCCESS )
            {
                /* keep on pinging */
                continue;
            }
            else
            {
                break;
            }
        }
        else
        {
            // TODO
            /* send 10 bits of the address and wait for an acknowledge */
        }
    }

    /* generate a stop condition */
    I2C_GenerateSTOP( i2c_mapping[device->port].i2c, ENABLE );

    /* Check if the device didn't respond */
    if ( i == retries )
    {
        MCU_CLOCKS_NOT_NEEDED();
        return WICED_FALSE;
    }
    else
    {
        MCU_CLOCKS_NEEDED();
        return WICED_TRUE;
    }
}

static wiced_result_t i2c_dma_config_and_execute( wiced_i2c_device_t* device, wiced_i2c_message_t* message, wiced_bool_t tx_dma  )
{
    uint32_t counter;

    wiced_assert( "bad argument", device != NULL );
    wiced_assert( "bad argument", message != NULL );

    /* Initialize the DMA with the new parameters */
    if ( tx_dma == WICED_TRUE )
    {
        /* Enable DMA channel for I2C */
        I2C_DMACmd( i2c_mapping[device->port].i2c, ENABLE );

        /* TX DMA configuration */
        DMA_DeInit( i2c_mapping[device->port].tx_dma_stream );

        /* Configure the DMA TX Stream with the buffer address and the buffer size */
        i2c_dma_init.DMA_Memory0BaseAddr = (uint32_t)message->tx_buffer;
        i2c_dma_init.DMA_DIR             = DMA_DIR_MemoryToPeripheral;
        i2c_dma_init.DMA_BufferSize      = (uint32_t)message->tx_length;
        DMA_Init(i2c_mapping[device->port].tx_dma_stream, &i2c_dma_init);

        /* Enable DMA channel */
        DMA_Cmd( i2c_mapping[device->port].tx_dma_stream, ENABLE );

        /* wait until transfer is completed */
        /* TODO: change flag!!!!,wait on a semaphore */
        counter = DMA_TIMEOUT_LOOPS;
        while ( DMA_GetFlagStatus( i2c_mapping[device->port].tx_dma_stream, DMA_FLAG_TC(i2c_mapping[device->port].tx_dma_stream_id) ) == RESET )
        {
            --counter;
            if ( counter == 0 )
            {
                return WICED_ERROR;
            }
        }

        /* Disable DMA and channel */
        I2C_DMACmd( i2c_mapping[device->port].i2c, DISABLE );
        DMA_Cmd( i2c_mapping[device->port].tx_dma_stream, DISABLE );
    }
    else
    {
        /* Enable dma channel for I2C */
        I2C_DMACmd( i2c_mapping[device->port].i2c, ENABLE );

        /* RX DMA configuration */
        DMA_DeInit( i2c_mapping[device->port].rx_dma_stream );

        /* Configure the DMA Rx Stream with the buffer address and the buffer size */
        i2c_dma_init.DMA_Memory0BaseAddr = (uint32_t)message->rx_buffer;
        i2c_dma_init.DMA_DIR             = DMA_DIR_PeripheralToMemory;
        i2c_dma_init.DMA_BufferSize      = (uint32_t)message->rx_length;
        DMA_Init(i2c_mapping[device->port].rx_dma_stream, &i2c_dma_init);

        /* Enable DMA channel */
        DMA_Cmd( i2c_mapping[device->port].rx_dma_stream, ENABLE );

        /* wait until transfer is completed */
        counter = DMA_TIMEOUT_LOOPS;
        while ( DMA_GetFlagStatus( i2c_mapping[device->port].rx_dma_stream, DMA_FLAG_TC(i2c_mapping[device->port].rx_dma_stream_id) ) == RESET )
        {
            --counter;
            if ( counter == 0 )
            {
                return WICED_ERROR;
            }
        }

        /* disable DMA and channel */
        I2C_DMACmd( i2c_mapping[device->port].i2c, DISABLE );
        DMA_Cmd( i2c_mapping[device->port].rx_dma_stream, DISABLE );
    }

    return WICED_SUCCESS;
}

static wiced_result_t i2c_dma_transfer( wiced_i2c_device_t* device, wiced_i2c_message_t* message )
{
    wiced_result_t result;
    uint32_t       counter;
    int            i = 0;

    if ( message->combined == WICED_TRUE )
    {
        /* combined transaction case, please refer to Philips I2C document to have an understanding of a combined fragment */

        /* some chips( authentication and security related chips ) has to be addressed several times before they acknowledge their address */
        for ( i = 0; i < message->retries; i++ )
        {
            /* generate a start condition and address a device in write mode */
            I2C_GenerateSTART( i2c_mapping[device->port].i2c, ENABLE );

            /* wait till start condition is generated and the bus becomes free */
            result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_MODE_SELECT, I2C_FLAG_CHECK_TIMEOUT );
            if ( result != WICED_SUCCESS )
            {
                return WICED_TIMEOUT;
            }

            if ( device->address_width == I2C_ADDRESS_WIDTH_7BIT )
            {
                /* send the address and R/W bit set to write of the requested device, wait for an acknowledge */
                I2C_Send7bitAddress( i2c_mapping[device->port].i2c, (uint8_t) ( device->address << 1 ), I2C_Direction_Transmitter );

                /* wait till address gets sent and the direction bit is sent and */
                result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
                if ( result == WICED_SUCCESS )
                {
                    break;
                }
            }
            else
            {
                //TODO
                /* send 10 bits of the address and wait for an acknowledge */
            }
        }

        if ( i == message->retries )
        {
            return WICED_TIMEOUT;
        }

        /* configure dma tx channel for i2c */
        i2c_dma_config_and_execute( device, message, WICED_TRUE );

        /* wait till the byte is actually sent from the i2c peripheral */
        counter = 1000;
        while ( I2C_GetFlagStatus( i2c_mapping[device->port].i2c, I2C_FLAG_BTF ) == RESET )
        {
            --counter;
            if ( counter == 0 )
            {
                return WICED_ERROR;
            }
        }

        /* generate start condition again and address a device in read mode */
        /* some chips( authentication and security related chips ) has to be addressed several times before they acknowledge their address */
        for ( i = 0; i < message->retries; i++ )
        {
            /* generate a start condition */
            I2C_GenerateSTART( i2c_mapping[device->port].i2c, ENABLE );

            /* wait till start condition is generated and the bus becomes free */
            result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_MODE_SELECT, I2C_FLAG_CHECK_TIMEOUT );
            if ( result != WICED_SUCCESS )
            {
                return WICED_TIMEOUT;
            }

            if ( device->address_width == I2C_ADDRESS_WIDTH_7BIT )
            {
                /* send the address and R/W bit set to write of the requested device, wait for an acknowledge */
                I2C_Send7bitAddress( i2c_mapping[device->port].i2c, (uint8_t) ( device->address << 1 ), I2C_Direction_Receiver );

                /* wait till address gets sent and the direction bit is sent and */
                result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
                if ( result == WICED_SUCCESS )
                {
                    break;
                }
            }
            else
            {
                //TODO
                /* send 10 bits of the address and wait for an acknowledge */
            }
        }

        if ( i == message->retries )
        {
            return WICED_TIMEOUT;
        }

        /* receive data from the slave device */
        if ( message->rx_length == 1 )
        {
            /* disable acknowledgement before we start receiving bytes, this is a single byte transmission */
            I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, DISABLE );
        }
        else
        {
            /* enable acknowledgement before we start receiving bytes, this is a single byte transmission */
            I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, ENABLE );
        }

        /* start dma which will read bytes */
        i2c_dma_config_and_execute( device, message, WICED_FALSE );
        /* maybe we will have to wait on the BTF flag!!! */
    }
    else
    {

        /* read or write transaction */

        /* some chips( authentication and security related chips ) has to be addressed several times before they acknowledge their address */
        for ( i = 0; i < message->retries; i++ )
        {
            /* generate a start condition */
            I2C_GenerateSTART( i2c_mapping[device->port].i2c, ENABLE );

            /* wait till start condition is generated and the bus becomes free */
            result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_MODE_SELECT, I2C_FLAG_CHECK_TIMEOUT );
            if ( result != WICED_SUCCESS )
            {
                return WICED_TIMEOUT;
            }

            if ( device->address_width == I2C_ADDRESS_WIDTH_7BIT )
            {
                /* send the address of the requested device, wait for an acknowledge */
                I2C_Send7bitAddress( i2c_mapping[device->port].i2c, (uint8_t) ( device->address << 1 ), ( ( message->tx_buffer ) ? I2C_Direction_Transmitter : I2C_Direction_Receiver ) );

                /* wait till address gets sent and the direction bit is sent */
                if ( message->tx_buffer )
                {
                    result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
                }
                else
                {
                    result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
                }

                if ( result == WICED_SUCCESS )
                {
                    break;
                }
            }
            else
            {
                //TODO
                /* send 10 bits of the address and wait for an acknowledge */
            }
        }
        if ( i == message->retries )
        {
            return WICED_TIMEOUT;
        }

        if ( message->tx_buffer )
        {
            /* write transaction */
            /* configure dma tx channel for i2c */
            i2c_dma_config_and_execute( device, message, WICED_TRUE );

            /* wait till the byte is actually sent from the i2c peripheral */
            counter = 1000;
            while ( I2C_GetFlagStatus( i2c_mapping[device->port].i2c, I2C_FLAG_BTF ) == RESET )
            {
                --counter;
                if ( counter == 0 )
                {
                    return WICED_ERROR;
                }
            }
        }
        else
        {
            /* read transaction */
            if ( message->rx_length == 1 )
            {
                /* disable acknowledgement before we are going to receive a single byte */
                I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, DISABLE );
            }
            else
            {
                /* enable acknowledgement before we start receiving multiple bytes */
                I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, ENABLE );
            }

            /* start dma which will read bytes */
            i2c_dma_config_and_execute( device, message, WICED_FALSE );

            /* wait til the last byte is received */
            counter = 1000;
            while ( I2C_GetFlagStatus( i2c_mapping[device->port].i2c, I2C_FLAG_BTF ) == RESET )
            {
                --counter;
                if ( counter == 0 )
                {
                    return WICED_ERROR;
                }
            }
        }
    }

    /* generate a stop condition */
    I2C_GenerateSTOP( i2c_mapping[device->port].i2c, ENABLE );
    return WICED_SUCCESS;
}

static wiced_result_t i2c_transfer_message_no_dma( wiced_i2c_device_t* device, wiced_i2c_message_t* message )
{
    wiced_result_t result;
    int            i = 0;

    if ( message->combined == WICED_TRUE )
    {
        const char* tmp_ptr;
        uint8_t*    tmp_rd_ptr;

        /* combined transaction case, please refer to Philips I2C document to have an understanding of a combined fragment */

        /* some chips( authentication and security related chips ) has to be addressed several times before they acknowledge their address */
        for ( i = 0; i < message->retries; i++ )
        {
            /* generate a start condition and address a device in write mode */
            I2C_GenerateSTART( i2c_mapping[device->port].i2c, ENABLE );

            /* wait till start condition is generated and the bus becomes free */
            result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_MODE_SELECT, I2C_FLAG_CHECK_TIMEOUT );
            if ( result != WICED_SUCCESS )
            {
                return WICED_TIMEOUT;
            }

            if ( device->address_width == I2C_ADDRESS_WIDTH_7BIT )
            {
                /* send the address and R/W bit set to write of the requested device, wait for an acknowledge */
                I2C_Send7bitAddress( i2c_mapping[device->port].i2c, (uint8_t) ( device->address << 1 ), I2C_Direction_Transmitter );

                /* wait till address gets sent and the direction bit is sent and */
                result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
                if ( result == WICED_SUCCESS )
                {
                    break;
                }
            }
            else
            {
                //TODO
                /* send 10 bits of the address and wait for an acknowledge */
            }
        }

        if ( i == message->retries )
        {
            return WICED_TIMEOUT;
        }

        tmp_ptr = (const char*) message->tx_buffer;

        /* send data to the i2c device */
        for ( i = 0; i < message->tx_length; i++ )
        {
            I2C_SendData( i2c_mapping[device->port].i2c, (uint8_t) tmp_ptr[i] );

            /* wait till it actually gets transferred and acknowledged */
            result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED, I2C_FLAG_CHECK_TIMEOUT );
            if ( result != WICED_SUCCESS )
            {
                return result;
            }
        }

        /* generate start condition again and address a device in read mode */
        /* some chips( authentication and security related chips ) has to be addressed several times before they acknowledge their address */
        for ( i = 0; i < message->retries; i++ )
        {
            /* generate a start condition */
            I2C_GenerateSTART( i2c_mapping[device->port].i2c, ENABLE );

            /* wait till start condition is generated and the bus becomes free */
            result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_MODE_SELECT, I2C_FLAG_CHECK_TIMEOUT );
            if ( result != WICED_SUCCESS )
            {
                return WICED_TIMEOUT;
            }
            if ( device->address_width == I2C_ADDRESS_WIDTH_7BIT )
            {
                /* send the address and R/W bit set to write of the requested device, wait for an acknowledge */
                I2C_Send7bitAddress( i2c_mapping[device->port].i2c, (uint8_t) ( device->address << 1 ), I2C_Direction_Receiver );

                /* wait till address gets sent and the direction bit is sent and */
                result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
                if ( result != WICED_SUCCESS )
                {
                    /* keep on pinging, if a device doesnt respond */
                    continue;
                }
                else
                {
                    break;
                }
            }
            else
            {
                //TODO
                /* send 10 bits of the address and wait for an acknowledge */
            }
        }
        if ( i == message->retries )
        {
            return WICED_TIMEOUT;
        }

        /* receive data from the slave device */
        if ( message->rx_length == 1 )
        {
            /* disable acknowledgement before we start receiving bytes, this is a single byte transmission */
            I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, DISABLE );
        }
        else
        {
            /* enable acknowledgement before we start receiving bytes, this is a single byte transmission */
            I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, ENABLE );
        }
        tmp_rd_ptr = (uint8_t*) message->rx_buffer;
        /* start reading bytes */
        for ( i = 0; i < message->rx_length; i++ )
        {
            /* wait till something is in the i2c data register */
            result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_BYTE_RECEIVED, I2C_FLAG_CHECK_TIMEOUT );
            if ( result != WICED_SUCCESS )
            {
                return result;
            }

            /* get data */
            tmp_rd_ptr[i] = I2C_ReceiveData( i2c_mapping[device->port].i2c );
            if ( i == ( message->rx_length - 1 ) )
            {
                /* setup NACK for the last byte to be received */
                I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, DISABLE );
            }
            else
            {
                /* setup an acknowledgement beforehand for every byte that is to be received */
                I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, ENABLE );
            }
        }
    }
    else
    {

        /* read or write transaction */

        /* some chips( authentication and security related chips ) has to be addressed several times before they acknowledge their address */
        for ( i = 0; i < message->retries; i++ )
        {
            /* generate a start condition */
            I2C_GenerateSTART( i2c_mapping[device->port].i2c, ENABLE );

            /* wait till start condition is generated and the bus becomes free */
            result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_MODE_SELECT, I2C_FLAG_CHECK_TIMEOUT );
            if ( result != WICED_SUCCESS )
            {
                return WICED_TIMEOUT;
            }
            if ( device->address_width == I2C_ADDRESS_WIDTH_7BIT )
            {
                /* send the address of the requested device, wait for an acknowledge */
                I2C_Send7bitAddress( i2c_mapping[device->port].i2c, (uint8_t) ( device->address << 1 ), ( ( message->tx_buffer ) ? I2C_Direction_Transmitter : I2C_Direction_Receiver ) );

                /* wait till address gets sent and the direction bit is sent */
                if ( message->tx_buffer )
                {
                    result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
                }
                else
                {
                    result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, I2C_FLAG_CHECK_LONG_TIMEOUT );
                }
                if ( result != WICED_SUCCESS )
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
            else
            {
                //TODO
                /* send 10 bits of the address and wait for an acknowledge */
            }
        }
        if ( i == message->retries )
        {
            return WICED_TIMEOUT;
        }

        if ( message->tx_buffer )
        {
            /* write transaction */

            const char* temp_ptr = (const char*) message->tx_buffer;
            /* send data to the i2c device */
            for ( i = 0; i < message->tx_length; i++ )
            {
                I2C_SendData( i2c_mapping[device->port].i2c, (uint8_t) temp_ptr[i] );

                /* wait till it actually gets transferred and acknowledged */
                result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED, I2C_FLAG_CHECK_TIMEOUT );
                if ( result != WICED_SUCCESS )
                {
                    return result;
                }
            }
        }
        else
        {
            /* read transaction */

            uint8_t* tmp_ptr = (uint8_t*) message->rx_buffer;
            if ( message->rx_length == 1 )
            {
                /* disable acknowledgement before we are going to receive a single byte */
                I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, DISABLE );
            }
            else
            {
                /* enable acknowledgement before we start receiving multiple bytes */
                I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, ENABLE );
            }
            /* receive data from the i2c device */
            for ( i = 0; i < message->rx_length; i++ )
            {
                /* wait till something is in the i2c data register */
                result = i2c_wait_for_event( i2c_mapping[device->port].i2c, I2C_EVENT_MASTER_BYTE_RECEIVED, I2C_FLAG_CHECK_TIMEOUT );
                if ( result != WICED_SUCCESS )
                {
                    return result;
                }

                /* get data */
                tmp_ptr[i] = I2C_ReceiveData( i2c_mapping[device->port].i2c );
                if ( i == ( message->rx_length - 1 ) )
                {
                    /* setup NACK for the last byte to be received */
                    I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, DISABLE );
                }
                else
                {
                    /* setup an acknowledgement beforehand for every byte that is to be received */
                    I2C_AcknowledgeConfig( i2c_mapping[device->port].i2c, ENABLE );
                }
            }
        }
    }

    /* generate a stop condition */
    I2C_GenerateSTOP( i2c_mapping[device->port].i2c, ENABLE );
    return WICED_SUCCESS;

}

wiced_result_t wiced_i2c_init_tx_message(wiced_i2c_message_t* message, const void* tx_buffer, uint16_t  tx_buffer_length, uint16_t retries , wiced_bool_t disable_dma)
{
    if( ( message == 0 ) || ( tx_buffer == 0 ) || ( tx_buffer_length == 0 ) )
    {
        return WICED_BADARG;
    }
    memset(message, 0x00, sizeof(wiced_i2c_message_t));
    message->tx_buffer = tx_buffer;
    message->combined = WICED_FALSE;
    message->retries = retries;
    message->tx_length = tx_buffer_length;
    if( disable_dma )
    {
        message->flags = I2C_MESSAGE_NO_DMA;
    }
    else
    {
        message->flags = I2C_MESSAGE_USE_DMA;
    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_i2c_init_rx_message(wiced_i2c_message_t* message, void* rx_buffer, uint16_t  rx_buffer_length, uint16_t retries , wiced_bool_t disable_dma)
{
    if( ( message == 0 ) || ( rx_buffer == 0 ) || ( rx_buffer_length == 0 ) )
    {
        return WICED_BADARG;
    }
    memset(message, 0x00, sizeof(wiced_i2c_message_t));
    message->rx_buffer = rx_buffer;
    message->combined = WICED_FALSE;
    message->retries = retries;
    message->rx_length = rx_buffer_length;
    if( disable_dma )
    {
        message->flags = I2C_MESSAGE_NO_DMA;
    }
    else
    {
        message->flags = I2C_MESSAGE_USE_DMA;
    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_i2c_init_combined_message(wiced_i2c_message_t* message, const void* tx_buffer, void* rx_buffer, uint16_t tx_buffer_length, uint16_t rx_buffer_length, uint16_t retries , wiced_bool_t disable_dma)
{
    if( ( message == 0 ) || ( rx_buffer == 0 ) || ( tx_buffer == 0 ) || ( tx_buffer_length == 0 ) || ( rx_buffer_length == 0 ) )
    {
        return WICED_BADARG;
    }
    memset(message, 0x00, sizeof(wiced_i2c_message_t));
    message->rx_buffer = rx_buffer;
    message->tx_buffer = tx_buffer;
    message->combined = WICED_TRUE;
    message->retries = retries;
    message->tx_length = tx_buffer_length;
    message->rx_length = rx_buffer_length;
    if( disable_dma )
    {
        message->flags = I2C_MESSAGE_NO_DMA;
    }
    else
    {
        message->flags = I2C_MESSAGE_USE_DMA;
    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_i2c_transfer( wiced_i2c_device_t* device, wiced_i2c_message_t* messages, uint16_t number_of_messages )
{
    wiced_result_t result;
    int i = 0;

    wiced_assert( "bad argument", (device != NULL) && (messages != NULL) );

    MCU_CLOCKS_NEEDED();

    for( i=0; i < number_of_messages; i++ )
    {
        if( ( device->flags & I2C_DEVICE_USE_DMA ) && ( ( messages[i].flags & I2C_MESSAGE_USE_DMA ) == 1 ) )
        {
            result = i2c_dma_transfer(device, &messages[i]);
            if( result != WICED_SUCCESS )
            {
                MCU_CLOCKS_NOT_NEEDED();
                return WICED_ERROR;
            }
        }
        else
        {
            result = i2c_transfer_message_no_dma( device, &messages[i] );
            if( result != WICED_SUCCESS )
            {
                MCU_CLOCKS_NOT_NEEDED();
                return WICED_ERROR;
            }
        }
    }

    MCU_CLOCKS_NOT_NEEDED();
    return WICED_SUCCESS;
}

wiced_result_t wiced_i2c_deinit( wiced_i2c_device_t* device )
{
    MCU_CLOCKS_NEEDED();

    /* Disable I2C peripheral clocks */
    RCC_APB1PeriphClockCmd( i2c_mapping[device->port].peripheral_clock_reg, DISABLE );

    /* Disable sda, scl gpio clocks */
    RCC_AHB1PeriphClockCmd( i2c_mapping[device->port].pin_scl->peripheral_clock | i2c_mapping[device->port].pin_sda->peripheral_clock, DISABLE );

    /* Disable DMA */
    if( device->flags & I2C_DEVICE_USE_DMA )
    {
        DMA_DeInit( i2c_mapping[device->port].rx_dma_stream );
        DMA_DeInit( i2c_mapping[device->port].tx_dma_stream );
        RCC_AHB1PeriphClockCmd( i2c_mapping[device->port].tx_dma_peripheral_clock, DISABLE );
    }

    MCU_CLOCKS_NOT_NEEDED();

    return WICED_SUCCESS;
}

wiced_result_t wiced_adc_init( wiced_adc_t adc, uint32_t sample_cycle )
{
    GPIO_InitTypeDef      gpio_init_structure;
    ADC_InitTypeDef       adc_init_structure;
    ADC_CommonInitTypeDef adc_common_init_structure;
    uint8_t a;

    MCU_CLOCKS_NEEDED();

    /* Initialize the associated GPIO */
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
    ADC_RegularChannelConfig( adc_mapping[adc].adc, adc_mapping[adc].channel, adc_mapping[adc].rank, a );

    MCU_CLOCKS_NOT_NEEDED();

    return WICED_SUCCESS;
}

wiced_result_t wiced_adc_take_sample( wiced_adc_t adc, uint16_t* output )
{
    MCU_CLOCKS_NEEDED();

    /* Start conversion */
    ADC_SoftwareStartConv( adc_mapping[adc].adc );

    /* Wait until end of conversion */
    while ( ADC_GetFlagStatus( adc_mapping[adc].adc, ADC_FLAG_EOC ) == RESET )
    {
    }

    /* Read ADC conversion result */
    *output = ADC_GetConversionValue( adc_mapping[adc].adc );

    MCU_CLOCKS_NOT_NEEDED();

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

    MCU_CLOCKS_NEEDED();

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

    MCU_CLOCKS_NOT_NEEDED();

    return WICED_SUCCESS;
}

wiced_result_t wiced_gpio_output_high( wiced_gpio_t gpio )
{
    MCU_CLOCKS_NEEDED();

    GPIO_SetBits( gpio_mapping[gpio].bank, (uint16_t) (1 << gpio_mapping[gpio].number) );

    MCU_CLOCKS_NOT_NEEDED();

    return WICED_SUCCESS;
}

wiced_result_t wiced_gpio_output_low( wiced_gpio_t gpio )
{
    MCU_CLOCKS_NEEDED();

    GPIO_ResetBits( gpio_mapping[gpio].bank, (uint16_t) (1 << gpio_mapping[gpio].number) );

    MCU_CLOCKS_NOT_NEEDED();

    return WICED_SUCCESS;
}

wiced_bool_t wiced_gpio_input_get( wiced_gpio_t gpio )
{
    wiced_bool_t result;

    MCU_CLOCKS_NEEDED();

    result =  (GPIO_ReadInputDataBit( gpio_mapping[gpio].bank, (uint16_t) ( 1 << gpio_mapping[gpio].number ) ) == 0 )? WICED_FALSE : WICED_TRUE;

    MCU_CLOCKS_NOT_NEEDED();

    return result;
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

    MCU_CLOCKS_NEEDED();

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

    MCU_CLOCKS_NOT_NEEDED();

    return WICED_SUCCESS;
}

wiced_result_t wiced_pwm_start( wiced_pwm_t pwm )
{
    MCU_CLOCKS_NEEDED();

    TIM_Cmd(pwm_mappings[pwm].tim, ENABLE);
    TIM_CtrlPWMOutputs( pwm_mappings[pwm].tim, ENABLE );

    MCU_CLOCKS_NOT_NEEDED();

    return WICED_SUCCESS;
}

wiced_result_t wiced_pwm_stop( wiced_pwm_t pwm )
{
    MCU_CLOCKS_NEEDED();

    TIM_CtrlPWMOutputs( pwm_mappings[pwm].tim, DISABLE );
    TIM_Cmd(pwm_mappings[pwm].tim, DISABLE);

    MCU_CLOCKS_NOT_NEEDED();

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

    MCU_CLOCKS_NEEDED();

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
    DMA_ITConfig( uart_mapping[uart].tx_dma_stream, DMA_IT_TC | DMA_IT_TE, ENABLE );

    /* Enable USART's TX and RX DMA interfaces */
    USART_DMACmd( uart_mapping[uart].usart, USART_DMAReq_Rx | USART_DMAReq_Tx, ENABLE );

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
        DMA_ITConfig( uart_mapping[uart].rx_dma_stream, DMA_IT_TC | DMA_IT_TE, ENABLE );
    }

    MCU_CLOCKS_NOT_NEEDED();

    return WICED_SUCCESS;
}

wiced_result_t wiced_uart_deinit( wiced_uart_t uart )
{
    NVIC_InitTypeDef nvic_init_structure;

    MCU_CLOCKS_NEEDED();

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

    MCU_CLOCKS_NOT_NEEDED();

    return WICED_SUCCESS;
}

wiced_result_t wiced_uart_transmit_bytes( wiced_uart_t uart, const void* data, uint32_t size )
{
    uint32_t tmpvar; /* needed to ensure ordering of volatile accesses */
    uint32_t shift = uart_mapping[uart].tx_dma_stream_number % 4;

    MCU_CLOCKS_NEEDED();

    uart_mapping[uart].tx_dma_stream->CR &= ~(uint32_t) DMA_SxCR_CIRC;

    if ( uart_mapping[uart].tx_dma_stream_number > 3 )
    {
        tmpvar = uart_mapping[uart].tx_dma->HISR;
        uart_mapping[uart].tx_dma->HIFCR |= ( tmpvar & ( 0xffUL << shift ) );
    }
    else
    {
        tmpvar = uart_mapping[uart].tx_dma->LISR;
        uart_mapping[uart].tx_dma->LIFCR |= ( tmpvar & ( 0xffUL << shift ) );
    }

    uart_mapping[uart].tx_dma_stream->NDTR = size;
    uart_mapping[uart].tx_dma_stream->M0AR = (uint32_t)data;
    uart_mapping[uart].tx_dma_stream->CR  |= DMA_SxCR_EN;

    host_rtos_get_semaphore( &uart_interfaces[uart].tx_complete, WICED_NEVER_TIMEOUT, WICED_TRUE );
    while( ( uart_mapping[uart].usart->SR & USART_SR_TC )== 0 );

    MCU_CLOCKS_NOT_NEEDED();

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
    uint32_t tmpvar; /* needed to ensure ordering of volatile accesses */
    uint32_t shift = uart_mapping[uart].rx_dma_stream_number % 4;

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

    if ( uart_mapping[uart].rx_dma_stream_number > 3 )
    {
        tmpvar = uart_mapping[uart].rx_dma->HISR;
        uart_mapping[uart].rx_dma->HIFCR |= ( tmpvar & ( 0xffUL << shift ) );
    }
    else
    {
        tmpvar = uart_mapping[uart].rx_dma->LISR;
        uart_mapping[uart].rx_dma->LIFCR |= ( tmpvar & ( 0xffUL << shift ) );
    }

    uart_mapping[uart].rx_dma_stream->NDTR = size;
    uart_mapping[uart].rx_dma_stream->M0AR = (uint32_t)data;
    uart_mapping[uart].rx_dma_stream->CR  |= DMA_SxCR_EN;

    if ( timeout > 0 )
    {
        host_rtos_get_semaphore( &uart_interfaces[uart].rx_complete, timeout, WICED_TRUE );
    }

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
    platform_dct_header_t* new_dct;
    uint32_t               bytes_after_data;
    uint8_t*               new_app_data_addr;
    uint8_t*               curr_app_data_addr;
    platform_dct_header_t* curr_dct  = &platform_get_dct( )->dct_header;
    char                   zero_byte = 0;
    platform_dct_header_t  hdr =
    {
        .write_incomplete = 1,
        .is_current_dct   = 1,
        .magic_number     = BOOTLOADER_MAGIC_NUMBER
    };

    /* Check if the data is too big to write */
    if ( data_length + data_start_offset > ( PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS ) )
    {
        return -1;
    }

    /* Erase the non-current DCT */
    if ( curr_dct == ((platform_dct_header_t*)PLATFORM_DCT_COPY1_START_ADDRESS) )
    {
        new_dct = (platform_dct_header_t*)PLATFORM_DCT_COPY2_START_ADDRESS;
        ERASE_DCT_2();
    }
    else
    {
        new_dct = (platform_dct_header_t*)PLATFORM_DCT_COPY1_START_ADDRESS;
        ERASE_DCT_1();
    }

    /* Write the mfg data and initial part of app data before provided data */
    platform_write_flash_chunk( ((uint32_t) &new_dct[1]), (uint8_t*) &curr_dct[1], data_start_offset );

    /* Verify the mfg data */
    if ( memcmp( &new_dct[1], &curr_dct[1], data_start_offset ) != 0 )
    {
        return -2;
    }

    /* Write the app data */
    new_app_data_addr  =  (uint8_t*) &new_dct[1]  + data_start_offset;
    curr_app_data_addr =  (uint8_t*) &curr_dct[1] + data_start_offset;

    platform_write_flash_chunk( (uint32_t)new_app_data_addr, data, data_length );

    /* Verify the app data */
    if ( memcmp( new_app_data_addr, data, data_length ) != 0 )
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

    hdr.app_valid           = (char) (( app_valid == -1 )? curr_dct->app_valid : app_valid);
    hdr.load_app_func       = func;
    hdr.mfg_info_programmed = curr_dct->mfg_info_programmed;

    /* Write the header data */
    platform_write_flash_chunk( (uint32_t)new_dct, (uint8_t*) &hdr, sizeof(hdr) );

    /* Verify the header data */
    if ( memcmp( new_dct, &hdr, sizeof(hdr) ) != 0 )
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
    uint32_t write_address  = address;
    const uint8_t* end_of_data = data + size;
    const uint8_t* data_iter   = data;
//    flash_write_t* data_ptr = (flash_write_t*) data;
//    flash_write_t* end_ptr  = (flash_write_t*) &data[size];

    FLASH_Unlock( );

    /* Clear any error flags */
    FLASH_ClearFlag( FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR );

    /* Write data to STM32 flash memory */
    while ( data_iter <  end_of_data )
    {
        FLASH_Status status;

        if ( ( ( ((uint32_t)write_address) & 0x03 ) == 0 ) && ( end_of_data - data_iter >= FLASH_WRITE_SIZE ) )
        {
            int tries = 0;
            /* enough data available to write as the largest size allowed by supply voltage */
            while ( ( FLASH_COMPLETE != ( status = FLASH_WRITE_FUNC( write_address, *((flash_write_t*)data_iter) ) ) ) && ( tries < 10 ) )
            {
                tries++;
            }
            if ( FLASH_COMPLETE != status )
            {
                /* TODO: Handle error properly */
                wiced_assert("Error during write", 0 != 0 );
                return 1;
            }
            write_address += FLASH_WRITE_SIZE;
            data_iter     += FLASH_WRITE_SIZE;
        }
        else
        {
            int tries = 0;
            /* Limited data available - write in bytes */
            while ( ( FLASH_COMPLETE != ( status = FLASH_ProgramByte( write_address, *data_iter ) ) ) && ( tries < 10 ) )
            {
                tries++;
            }
            if ( FLASH_COMPLETE != status )
            {
                /* TODO: Handle error properly */
                wiced_assert("Error during write", 0 != 0 );
                return 1;
            }
            ++write_address;
            ++data_iter;
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
    if ( ( DMA2 ->HISR & DMA_HISR_TCIF7 ) != 0 )
    {
        DMA2 ->HIFCR |= DMA_HISR_TCIF7;
        host_rtos_set_semaphore( &uart_interfaces[0].tx_complete, WICED_TRUE );
    }

    /* TX DMA error */
    if ( ( DMA2 ->HISR & DMA_HISR_TEIF7 )!= 0 )
    {
        /* Clear interrupt */
        DMA2 ->HIFCR |= DMA_HISR_TEIF7;
    }
}

void usart2_tx_dma_irq( void )
{
    if ( (DMA1->HISR & DMA_HISR_TCIF6) != 0 )
    {
        DMA1->HIFCR |= DMA_HISR_TCIF6;
        host_rtos_set_semaphore( &uart_interfaces[1].tx_complete, WICED_TRUE );
    }

    /* TX DMA error */
    if ( (DMA1->HISR & DMA_HISR_TEIF6) != 0 )
    {
        /* Clear interrupt */
        DMA1->HIFCR |= DMA_HISR_TEIF6;
    }
}

void usart6_tx_dma_irq( void )
{
    if ( (DMA2->HISR & DMA_HISR_TCIF6) != 0 )
    {
        DMA2->HIFCR |= DMA_HISR_TCIF6;
        host_rtos_set_semaphore( &uart_interfaces[0].tx_complete, WICED_TRUE );
    }

    /* TX DMA error */
    if ( (DMA2->HISR & DMA_HISR_TEIF6) != 0 )
    {
        /* Clear interrupt */
        DMA2->HIFCR |= DMA_HISR_TEIF6;
    }
}

void usart1_rx_dma_irq( void )
{
    if ( ( DMA2->LISR & DMA_LISR_TCIF2)  != 0 )
    {
        DMA2->LIFCR |= DMA_LISR_TCIF2;
        host_rtos_set_semaphore( &uart_interfaces[0].rx_complete, WICED_TRUE );
//        DMA2_Stream2 ->NDTR = uart_interfaces[0].rx_buffer->size;
    }

    /* TX DMA error */
    if ( ( DMA2->LISR & DMA_LISR_TEIF2 ) != 0 )
    {
        /* Clear interrupt */
        DMA2->LIFCR |= DMA_LISR_TEIF2;
    }
}

void usart2_rx_dma_irq( void )
{
    if ( ( DMA1->HISR & DMA_HISR_TCIF5 ) != 0 )
    {
        DMA1->HIFCR |= DMA_HISR_TCIF5;
        host_rtos_set_semaphore( &uart_interfaces[1].rx_complete, WICED_TRUE );
//        DMA1_Stream5 ->NDTR = uart_interfaces[1].rx_buffer->size;
    }

    /* TX DMA error */
    if ( ( DMA1->HISR & DMA_HISR_TEIF5 ) != 0 )
    {
        /* Clear interrupt */
        DMA1->HIFCR |= DMA_HISR_TEIF5;
    }
}

void usart6_rx_dma_irq( void )
{
    if ( ( DMA2->LISR & DMA_LISR_TCIF1 ) != 0 )
    {
        DMA2->LIFCR |= DMA_LISR_TCIF1;
        host_rtos_set_semaphore( &uart_interfaces[0].rx_complete, WICED_TRUE );
//        DMA1_Stream5 ->NDTR = uart_interfaces[1].rx_buffer->size;
    }

    /* TX DMA error */
    if ( ( DMA2->LISR & DMA_LISR_TEIF1 ) != 0 )
    {
        /* Clear interrupt */
        DMA2->LIFCR |= DMA_LISR_TEIF1;
    }
}

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

#define RTC_INTERRUPT_EXTI_LINE EXTI_Line22
#define WUT_COUNTER_MAX  0xffff

#define ENABLE_INTERRUPTS   __asm("CPSIE i")
#define DISABLE_INTERRUPTS  __asm("CPSID i")

#if defined(WICED_DISABLE_MCU_POWERSAVE) && defined(WICED_ENABLE_MCU_RTC)
/*  */
void platform_rtc_init(void)
{
    RTC_InitTypeDef RTC_InitStruct;

    RTC_DeInit( );

    RTC_InitStruct.RTC_HourFormat = RTC_HourFormat_24;

    /* RTC ticks every second */
    RTC_InitStruct.RTC_AsynchPrediv = 0x7F;
    RTC_InitStruct.RTC_SynchPrediv = 0xFF;

    RTC_Init( &RTC_InitStruct );
    /* Enable the LSE OSC */
    RCC_LSEConfig(RCC_LSE_ON);
    /* Wait till LSE is ready */
    while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
    {
    }
    /* Select the RTC Clock Source */
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

    /* Enable the RTC Clock */
    RCC_RTCCLKCmd(ENABLE);

    /* RTC configuration -------------------------------------------------------*/
    /* Wait for RTC APB registers synchronisation */
    RTC_WaitForSynchro();

    /* write default application time inside rtc */
    platform_set_rtc_time(&wiced_default_time);
}
#endif

/**
 * This function will return the value of time read from the on board CPU real time clock. Time value must be given in the format of
 * the structure wiced_rtc_time_t
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_platform_get_rtc_time(wiced_rtc_time_t* time)
{
#ifdef WICED_ENABLE_MCU_RTC
    RTC_TimeTypeDef rtc_read_time;
    RTC_DateTypeDef rtc_read_date;

    if( time == 0 )
    {
        return WICED_BADARG;
    }

    /* save current rtc time */
    RTC_GetTime( RTC_Format_BIN, &rtc_read_time );
    RTC_GetDate( RTC_Format_BIN, &rtc_read_date );

    /* fill structure */
    time->sec = rtc_read_time.RTC_Seconds;
    time->min = rtc_read_time.RTC_Minutes;
    time->hr = rtc_read_time.RTC_Hours;
    time->weekday = rtc_read_date.RTC_WeekDay;
    time->date = rtc_read_date.RTC_Date;
    time->month= rtc_read_date.RTC_Month;
    time->year = rtc_read_date.RTC_Year;

    return WICED_SUCCESS;
#else /* #ifdef WICED_ENABLE_MCU_RTC */
    UNUSED_PARAMETER(time);
    return WICED_UNSUPPORTED;
#endif /* #ifdef WICED_ENABLE_MCU_RTC */
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
#ifdef WICED_ENABLE_MCU_RTC
    RTC_TimeTypeDef rtc_write_time;
    RTC_DateTypeDef rtc_write_date;
    wiced_bool_t    valid = WICED_FALSE;

    WICED_VERIFY_TIME(time, valid);
    if( valid == WICED_FALSE )
    {
        return WICED_BADARG;
    }
    rtc_write_time.RTC_Seconds = time->sec;
    rtc_write_time.RTC_Minutes = time->min;
    rtc_write_time.RTC_Hours   = time->hr;
    rtc_write_date.RTC_WeekDay = time->weekday;
    rtc_write_date.RTC_Date    = time->date;
    rtc_write_date.RTC_Month   = time->month;
    rtc_write_date.RTC_Year    = time->year;


    RTC_SetTime( RTC_Format_BIN, &rtc_write_time );
    RTC_SetDate( RTC_Format_BIN, &rtc_write_date );

    return WICED_SUCCESS;
#else /* #ifdef WICED_ENABLE_MCU_RTC */
    UNUSED_PARAMETER(time);
    return WICED_UNSUPPORTED;
#endif /* #ifdef WICED_ENABLE_MCU_RTC */
}

#ifndef WICED_DISABLE_MCU_POWERSAVE

static int stm32f2_clock_needed_counter = 0;

void stm32f2xx_clocks_needed( void )
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

void stm32f2xx_clocks_not_needed( void )
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


#ifndef WICED_DISABLE_MCU_POWERSAVE
void RTC_Wakeup_init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    RTC_InitTypeDef RTC_InitStruct;

    RTC_DeInit( );

    RTC_InitStruct.RTC_HourFormat = RTC_HourFormat_24;

    /* RTC ticks every second */
    RTC_InitStruct.RTC_AsynchPrediv = 0x7F;
    RTC_InitStruct.RTC_SynchPrediv = 0xFF;

    RTC_Init( &RTC_InitStruct );

    /* Enable the PWR clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

    /* RTC clock source configuration ------------------------------------------*/
    /* Allow access to BKP Domain */
    PWR_BackupAccessCmd(ENABLE);

    /* Enable the LSE OSC */
    RCC_LSEConfig(RCC_LSE_ON);
    /* Wait till LSE is ready */
    while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
    {
    }

    /* Select the RTC Clock Source */
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

    /* Enable the RTC Clock */
    RCC_RTCCLKCmd(ENABLE);

    /* RTC configuration -------------------------------------------------------*/
    /* Wait for RTC APB registers synchronisation */
    RTC_WaitForSynchro();

    RTC_WakeUpCmd( DISABLE );
    EXTI_ClearITPendingBit( RTC_INTERRUPT_EXTI_LINE );
    PWR_ClearFlag(PWR_FLAG_WU);
    RTC_ClearFlag(RTC_FLAG_WUTF);

    RTC_WakeUpClockConfig(RTC_WakeUpClock_RTCCLK_Div2);

    EXTI_ClearITPendingBit( RTC_INTERRUPT_EXTI_LINE );
    EXTI_InitStructure.EXTI_Line = RTC_INTERRUPT_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    RTC_ITConfig(RTC_IT_WUT, DISABLE);

    /* Prepare Stop-Mode but leave disabled */
//    PWR_ClearFlag(PWR_FLAG_WU);
    PWR->CR |= PWR_CR_LPDS;
    PWR->CR &= (unsigned long)(~(PWR_CR_PDDS));
    SCB->SCR |= ((unsigned long)SCB_SCR_SLEEPDEEP_Msk);

//#ifdef RTC_ENABLED
    /* application must have wiced_application_default_time structure declared somewhere, otherwise it wont compile */
    /* write default application time inside rtc */
    platform_set_rtc_time(&wiced_default_time);
//#endif /* RTC_ENABLED */
}
#endif /* #ifndef WICED_DISABLE_MCU_POWERSAVE */


static wiced_result_t select_wut_prescaler_calculate_wakeup_time( unsigned long* wakeup_time, unsigned long delay_ms, unsigned long* scale_factor )
{
    unsigned long temp;
    wiced_bool_t scale_factor_is_found = WICED_FALSE;
    int i                              = 0;

    static unsigned long int available_wut_prescalers[] =
    {
        RTC_WakeUpClock_RTCCLK_Div2,
        RTC_WakeUpClock_RTCCLK_Div4,
        RTC_WakeUpClock_RTCCLK_Div8,
        RTC_WakeUpClock_RTCCLK_Div16
    };
    static unsigned long scale_factor_values[] = { 2, 4, 8, 16 };

    if ( delay_ms == 0xFFFFFFFF )
    {
        /* wake up in a 100ms, since currently there may be no tasks to run, but after a few milliseconds */
        /* some of them can get unblocked( for example a task is blocked on mutex with unspecified ) */
        *scale_factor = 2;
        RTC_WakeUpClockConfig( RTC_WakeUpClock_RTCCLK_Div2 );
        *wakeup_time = NUMBER_OF_LSE_TICKS_PER_MILLISECOND( scale_factor_values[0] ) * 100;
    }
    else
    {
        for ( i = 0; i < 4; i++ )
        {
            temp = NUMBER_OF_LSE_TICKS_PER_MILLISECOND( scale_factor_values[i] ) * delay_ms;
            if ( temp < WUT_COUNTER_MAX )
            {
                scale_factor_is_found = WICED_TRUE;
                *wakeup_time = temp;
                *scale_factor = scale_factor_values[i];
                break;
            }
        }
        if ( scale_factor_is_found )
        {
            /* set new prescaler for wakeup timer */
            RTC_WakeUpClockConfig( available_wut_prescalers[i] );
        }
        else
        {
            /* scale factor can not be picked up for delays more that 32 seconds when RTCLK is selected as a clock source for the wakeup timer
             * for delays more than 32 seconds change from RTCCLK to 1Hz ck_spre clock source( used to update calendar registers ) */
            RTC_WakeUpClockConfig( RTC_WakeUpClock_CK_SPRE_16bits );

            /* with 1Hz ck_spre clock source the resolution changes to seconds  */
            *wakeup_time = ( delay_ms / 1000 ) + 1;
            *scale_factor = CK_SPRE_CLOCK_SOURCE_SELECTED;

            return WICED_ERROR;
        }
    }

    return WICED_SUCCESS;
}




void wake_up_interrupt_notify( void )
{
    wake_up_interrupt_triggered = WICED_TRUE;
}

static unsigned long stop_mode_power_down_hook( unsigned long delay_ms )
{
    unsigned long retval;
    unsigned long wut_ticks_passed;
    unsigned long scale_factor = 0;
    UNUSED_PARAMETER(delay_ms);
    UNUSED_PARAMETER(rtc_timeout_start_time);
    UNUSED_PARAMETER(scale_factor);

   /* pick up the appropriate prescaler for a requested delay */
    select_wut_prescaler_calculate_wakeup_time(&rtc_timeout_start_time, delay_ms, &scale_factor );

    if ( ( ( SCB->SCR & (unsigned long)SCB_SCR_SLEEPDEEP_Msk) ) != 0 )
    {
        DISABLE_INTERRUPTS;

        SysTick->CTRL &= (~(SysTick_CTRL_TICKINT_Msk|SysTick_CTRL_ENABLE_Msk)); /* systick IRQ off */
        RTC_ITConfig(RTC_IT_WUT, ENABLE);

        EXTI_ClearITPendingBit( RTC_INTERRUPT_EXTI_LINE );
        PWR_ClearFlag(PWR_FLAG_WU);
        RTC_ClearFlag(RTC_FLAG_WUTF);

        RTC_SetWakeUpCounter( rtc_timeout_start_time );
        RTC_WakeUpCmd( ENABLE );
        rtc_sleep_entry();

        DBGMCU->CR |= 0x03; /* Enable debug in stop mode */

        /* This code will be running with BASEPRI register value set to 0, the main intention behind that is that */
        /* all interrupts must be allowed to wake the CPU from the power-down mode */
        /* the PRIMASK is set to 1( see DISABLE_INTERRUPTS), thus we disable all interrupts before entering the power-down mode */
        /* This may sound contradictory, however according to the ARM CM3 documentation power-management unit */
        /* takes into account only the contents of the BASEPRI register and it is an external from the CPU core unit */
        /* PRIMASK register value doesn't affect its operation. */
        /* So, if the interrupt has been triggered just before the wfi instruction */
        /* it remains pending and wfi instruction will be treated as a nop  */
        __asm("wfi");

        /* After CPU exits powerdown mode, the processer will not execute the interrupt handler(PRIMASK is set to 1) */
        /* Disable rtc for now */
        RTC_WakeUpCmd( DISABLE );
        RTC_ITConfig(RTC_IT_WUT, DISABLE);

        /* Initialise the clocks again */
        init_clocks( );

        /* Enable CPU ticks */
        SysTick->CTRL |= (SysTick_CTRL_TICKINT_Msk|SysTick_CTRL_ENABLE_Msk);

        /* Get the time of how long the sleep lasted */
        wut_ticks_passed = rtc_timeout_start_time - RTC_GetWakeUpCounter();
        UNUSED_VARIABLE(wut_ticks_passed);
        rtc_sleep_exit( delay_ms, &retval );
        /* as soon as interrupts are enabled, we will go and execute the interrupt handler */
        /* which triggered a wake up event */
        ENABLE_INTERRUPTS;
        wake_up_interrupt_triggered = WICED_FALSE;
        return retval;
    }
    else
    {
        UNUSED_PARAMETER(wut_ticks_passed);
        ENABLE_INTERRUPTS;
        __asm("wfi");

        /* Note: We return 0 ticks passed because system tick is still going when wfi instruction gets executed */
        return 0;
    }
}

#else /* WICED_DISABLE_MCU_POWERSAVE */

static unsigned long idle_power_down_hook( unsigned long delay_ms  )
{
    UNUSED_PARAMETER( delay_ms );
    ENABLE_INTERRUPTS;
    __asm("wfi");
    return 0;
}

#endif /* WICED_DISABLE_MCU_POWERSAVE */


unsigned long platform_power_down_hook( unsigned long delay_ms )
{
#ifndef WICED_DISABLE_MCU_POWERSAVE
    return stop_mode_power_down_hook( delay_ms );
#else
    return idle_power_down_hook( delay_ms );
#endif
}

void RTC_WKUP_irq( void )
{
    EXTI_ClearITPendingBit( RTC_INTERRUPT_EXTI_LINE );
}

void wiced_platform_mcu_enable_powersave( void )
{
    MCU_CLOCKS_NOT_NEEDED();
}

void wiced_platform_mcu_disable_powersave( void )
{
    MCU_CLOCKS_NEEDED();
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
