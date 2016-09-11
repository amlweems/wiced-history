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

#include "stdio.h"
#include "string.h"
#include "MK60N512VMD100.h"
#include "core_cm4.h"
#include "platform.h"
#include "wiced_platform.h"
#include "k60_platform.h"
#include "watchdog.h"
#include "watchdog.h"
#include "k60_gpio.h"
#include "mcg.h"
#include "uart/uart.h"
#include "platform_dct.h"
#include "platform_common_config.h"
#include "Platform/wwd_platform_interface.h"
#include "wwd_assert.h"

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

/* Unimplemented*/
#define ERASE_DCT_1()
#define ERASE_DCT_2()

#define K60_CLK          1
#define REF_CLK      XTAL8   /* value isn't used, but we still need something defined */
#define CORE_CLK_MHZ PLL96   /* 96MHz is only freq tested for a clock input */

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

static void flash_identify( void );
static void set_sys_dividers( uint32_t outdiv1, uint32_t outdiv2, uint32_t outdiv3, uint32_t outdiv4 );
void vApplicationIdleSleepHook( void );

/******************************************************
 *               Variables Definitions
 ******************************************************/

static char k60_platform_inited = 0;

#ifndef WICED_DISABLE_STDIO
static host_semaphore_type_t stdio_rx_mutex;
static host_semaphore_type_t stdio_tx_mutex;
#endif

/******************************************************
 *               Function Definitions
 ******************************************************/

WEAK void init_clocks( void )
{
    watchdog_init();

    /*
     * Enable all of the port clocks. These have to be enabled to configure
     * pin muxing options, so most code will need all of these on anyway.
     */
    SIM_SCGC5 |= (SIM_SCGC5_PORTA_MASK
                  | SIM_SCGC5_PORTB_MASK
                  | SIM_SCGC5_PORTC_MASK
                  | SIM_SCGC5_PORTD_MASK
                  | SIM_SCGC5_PORTE_MASK );

    /* Ramp up the system clock */
    pll_init( CORE_CLK_MHZ, REF_CLK );
}

WEAK void init_memory( void )
{

}

void init_architecture( void )
{
    uint32_t i;

    if ( k60_platform_inited == 1 )
        return;

    /* Determine the flash revision */
    flash_identify( );

    /* Initialise the interrupt priorities to a priority lower than 0 so that the BASEPRI register can mask them */
    for ( i = 0; i < 103; i++ )
    {
        NVIC ->IP[i] = 0xff;
    }

#ifndef WICED_DISABLE_STDIO
    /* Initialise mutex for stdio thread-safeness */
    host_rtos_init_semaphore( &stdio_tx_mutex );
    host_rtos_set_semaphore ( &stdio_tx_mutex, WICED_FALSE );
    host_rtos_init_semaphore( &stdio_rx_mutex );
    host_rtos_set_semaphore ( &stdio_rx_mutex, WICED_FALSE );

    /* Configure TXD and RXD pins for UART console */
    k60_gpio_select_mux( STDIO_TX_BANK, STDIO_TX_PIN, 3 );
    k60_gpio_select_mux( STDIO_RX_BANK, STDIO_RX_PIN, 3 );

    /* UART0 and UART1 are clocked from the core clock, but all other UARTs are
     * clocked from the peripheral clock. So we have to determine which clock
     * to send to the uart_init function.
     */
    uart_init( STDIO_UART_PERIPHERAL, PERIPHERAL_CLOCK_HZ / 1000, 115200 );
#endif /* ifdef WICED_DISABLE_STDIO */

    /* Ensure 802.11 device is in reset. */
    host_platform_init( );

    k60_platform_inited = 1;
}

void vApplicationIdleSleepHook( void )
{
    __asm("wfi");
}

wiced_result_t wiced_spi_init( const wiced_spi_device_t* spi )
{
    UNUSED_PARAMETER( spi );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_spi_transfer( const wiced_spi_device_t* spi, wiced_spi_message_segment_t* segments, uint16_t number_of_segments )
{
    UNUSED_PARAMETER( spi );
    UNUSED_PARAMETER( segments );
    UNUSED_PARAMETER( number_of_segments );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_spi_deinit( const wiced_spi_device_t* spi )
{
    UNUSED_PARAMETER( spi );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_UNSUPPORTED;
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
    UNUSED_PARAMETER( adc );
    UNUSED_PARAMETER( sample_cycle );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_adc_take_sample( wiced_adc_t adc, uint16_t* output )
{
    UNUSED_PARAMETER( adc );
    UNUSED_PARAMETER( output );
    wiced_assert( "unimplemented", 0!=0 );
    *output = 0;
    return WICED_SUCCESS;
}

wiced_result_t wiced_adc_take_sample_stream( wiced_adc_t adc, void* buffer, uint16_t buffer_length )
{
    UNUSED_PARAMETER( adc );
    UNUSED_PARAMETER( buffer );
    UNUSED_PARAMETER( buffer_length );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_adc_deinit( wiced_adc_t adc )
{
    UNUSED_PARAMETER( adc );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_gpio_init( wiced_gpio_t gpio, wiced_gpio_config_t configuration )
{
    UNUSED_PARAMETER( gpio );
    UNUSED_PARAMETER( configuration );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_gpio_output_high( wiced_gpio_t gpio )
{
    UNUSED_PARAMETER( gpio );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_gpio_output_low( wiced_gpio_t gpio )
{
    UNUSED_PARAMETER( gpio );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_bool_t wiced_gpio_input_get( wiced_gpio_t gpio )
{
    UNUSED_PARAMETER( gpio );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_FALSE;
}

wiced_result_t wiced_gpio_input_irq_enable( wiced_gpio_t gpio, wiced_gpio_irq_trigger_t trigger, wiced_gpio_irq_handler_t handler, void* arg )
{
    UNUSED_PARAMETER( gpio );
    UNUSED_PARAMETER( trigger );
    UNUSED_PARAMETER( handler );
    UNUSED_PARAMETER( arg );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_gpio_input_irq_disable( wiced_gpio_t gpio )
{
    UNUSED_PARAMETER( gpio );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_pwm_init( wiced_pwm_t pwm_peripheral, uint32_t frequency, float duty_cycle )
{
    UNUSED_PARAMETER( pwm_peripheral );
    UNUSED_PARAMETER( frequency );
    UNUSED_PARAMETER( duty_cycle );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_pwm_start( wiced_pwm_t pwm )
{
    UNUSED_PARAMETER( pwm );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_pwm_stop( wiced_pwm_t pwm )
{
    UNUSED_PARAMETER( pwm );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_watchdog_kick( void )
{
    return watchdog_kick();
}

wiced_result_t wiced_uart_init( wiced_uart_t uart, const wiced_uart_config_t* config, wiced_ring_buffer_t* optional_rx_buffer )
{
    UNUSED_PARAMETER( uart );
    UNUSED_PARAMETER( config );
    UNUSED_PARAMETER( optional_rx_buffer );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_uart_deinit( wiced_uart_t uart )
{
    UNUSED_PARAMETER( uart );
    wiced_assert( "unimplemented", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_uart_transmit_bytes( wiced_uart_t uart, const void* data, uint32_t size )
{
    if ( uart == STDIO_UART )
    {
        char* ptr = (char*) data;

        while ( size > 0 )
        {
            uart_putchar( STDIO_UART_PERIPHERAL, *ptr++ );
            size--;
        }

        return WICED_SUCCESS;
    }
    else
    {
        wiced_assert( "unimplemented", 0!=0 );
        return WICED_UNSUPPORTED;
    }
}

wiced_result_t wiced_uart_receive_bytes( wiced_uart_t uart, void* data, uint32_t size, uint32_t timeout )
{
    if ( uart == STDIO_UART )
    {
        char* ptr = (char*) data;

        do
        {
            *ptr++ = uart_getchar( STDIO_UART_PERIPHERAL );
            size--;
            timeout--;
        } while ( size > 0 && timeout > 0 );

        if ( timeout == 0 )
        {
            return WICED_TIMEOUT;
        }

        return WICED_SUCCESS;
    }
    else
    {
        wiced_assert( "unimplemented", 0!=0 );
        return WICED_UNSUPPORTED;
    }
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

#ifndef WICED_DISABLE_BOOTLOADER
void platform_erase_dct( void )
{
    ERASE_DCT_1();
    ERASE_DCT_2();
}


platform_dct_data_t* platform_get_dct( void )
{
    return (platform_dct_data_t*)DCT1_START_ADDR;
}

/* TODO: Disable interrupts during function */
/* Note Function allocates a chunk of memory for the bootloader data on the stack - ensure the stack is big enough */
int platform_write_dct( uint16_t data_start_offset, const void* data, uint16_t data_length, int8_t app_valid, void (*func)(void) )
{
    UNUSED_PARAMETER( data_start_offset );
    UNUSED_PARAMETER( data );
    UNUSED_PARAMETER( data_length );
    UNUSED_PARAMETER( app_valid );
    UNUSED_PARAMETER( func );
    return 0;
}

int platform_erase_flash( uint16_t start_sector, uint16_t end_sector )
{
    UNUSED_PARAMETER( start_sector );
    UNUSED_PARAMETER( end_sector );
    return 0;
}

int platform_write_app_chunk( uint32_t offset, const uint8_t* data, uint32_t size )
{
    return platform_write_flash_chunk( APP_HDR_START_ADDR + offset, data, size );
}

int platform_write_flash_chunk( uint32_t address, const uint8_t* data, uint32_t size )
{
    UNUSED_PARAMETER( address );
    UNUSED_PARAMETER( data );
    UNUSED_PARAMETER( size );
    return 0;
}
#endif

/* These following functions are adapted from Freescale source code */

/********************************************************************/
/*!
 * \brief   flash Identify
 * \return  None
 *
 * This is primarily a reporting function that displays information
 * about the specific flash parameters and flash version ID for
 * the current device. These parameters are obtained using a special
 * flash command call "read resource." The first four bytes returned
 * are the flash parameter revision, and the second four bytes are
 * the flash version ID.
 */
static void flash_identify (void)
{
    /* Get the flash parameter version */

    /* Write the flash FCCOB registers with the values for a read resource command */
    FTFL_FCCOB0 = 0x03;
    FTFL_FCCOB1 = 0x00;
    FTFL_FCCOB2 = 0x00;
    FTFL_FCCOB3 = 0x00;
    FTFL_FCCOB8 = 0x01;

    /* All required FCCOBx registers are written, so launch the command */
    FTFL_FSTAT = FTFL_FSTAT_CCIF_MASK;

    /* Wait for the command to complete */
    while(!(FTFL_FSTAT & FTFL_FSTAT_CCIF_MASK));

    /* Get the flash version ID */

    /* Write the flash FCCOB registers with the values for a read resource command */
    FTFL_FCCOB0 = 0x03;
    FTFL_FCCOB1 = 0x00;
    FTFL_FCCOB2 = 0x00;
    FTFL_FCCOB3 = 0x04;
    FTFL_FCCOB8 = 0x01;

    /* All required FCCOBx registers are written, so launch the command */
    FTFL_FSTAT = FTFL_FSTAT_CCIF_MASK;

    /* Wait for the command to complete */
    while(!(FTFL_FSTAT & FTFL_FSTAT_CCIF_MASK));
}

unsigned char pll_init( unsigned char clk_option, unsigned char crystal_val )
{
    unsigned char pll_freq;

    if ( clk_option > 3 )
    {
        return 0;
    } //return 0 if one of the available options is not selected

    if ( crystal_val > 15 )
    {
        return 1;
    } // return 1 if one of the available crystal options is not available

//This assumes that the MCG is in default FEI mode out of reset.

// First move to FBE mode
#if (defined(K60_CLK) || defined(K53_CLK) || defined(ASB817))
    MCG_C2 = 0;
#else
// Enable external oscillator, RANGE=2, HGO=1, EREFS=1, LP=0, IRCS=0
    MCG_C2 = MCG_C2_RANGE(2) | MCG_C2_HGO_MASK | MCG_C2_EREFS_MASK;
#endif

// after initialization of oscillator release latched state of oscillator and GPIO
    SIM_SCGC4 |= SIM_SCGC4_LLWU_MASK;
    LLWU_CS |= LLWU_CS_ACKISO_MASK;

// Select external oscilator and Reference Divider and clear IREFS to start ext osc
// CLKS=2, FRDIV=3, IREFS=0, IRCLKEN=0, IREFSTEN=0
    MCG_C1 = MCG_C1_CLKS(2) | MCG_C1_FRDIV(3);

    /* if we aren't using an osc input we don't need to wait for the osc to init */
#if (!defined(K60_CLK) && !defined(K53_CLK) && !defined(ASB817))
    while ( !( MCG_S & MCG_S_OSCINIT_MASK ) )
    {
    };  // wait for oscillator to initialize
#endif

    while ( MCG_S & MCG_S_IREFST_MASK )
    {
    }; // wait for Reference clock Status bit to clear

    while ( ( ( MCG_S & MCG_S_CLKST_MASK ) >> MCG_S_CLKST_SHIFT ) != 0x2 )
    {
    }; // Wait for clock status bits to show clock source is ext ref clk

// Now in FBE

#if (defined(K60_CLK) || defined(K53_CLK))
    MCG_C5 = MCG_C5_PRDIV(0x18);
#else
// Configure PLL Ref Divider, PLLCLKEN=0, PLLSTEN=0, PRDIV=5
// The crystal frequency is used to select the PRDIV value. Only even frequency crystals are supported
// that will produce a 2MHz reference clock to the PLL.
    MCG_C5 = MCG_C5_PRDIV(crystal_val); // Set PLL ref divider to match the crystal used
#endif

    // Ensure MCG_C6 is at the reset default of 0. LOLIE disabled, PLL disabled, clk monitor disabled, PLL VCO divider is clear
    MCG_C6 = 0x0;
// Select the PLL VCO divider and system clock dividers depending on clocking option
    switch ( clk_option )
    {
        case 0:
            // Set system options dividers
            //MCG=PLL, core = MCG, bus = MCG, FlexBus = MCG, Flash clock= MCG/2
            set_sys_dividers( 0, 0, 0, 1 );
            // Set the VCO divider and enable the PLL for 50MHz, LOLIE=0, PLLS=1, CME=0, VDIV=1
            MCG_C6 = MCG_C6_PLLS_MASK | MCG_C6_VDIV(1); //VDIV = 1 (x25)
            pll_freq = 50;
            break;
        case 1:
            // Set system options dividers
            //MCG=PLL, core = MCG, bus = MCG/2, FlexBus = MCG/2, Flash clock= MCG/4
            set_sys_dividers( 0, 1, 1, 3 );
            // Set the VCO divider and enable the PLL for 100MHz, LOLIE=0, PLLS=1, CME=0, VDIV=26
            MCG_C6 = MCG_C6_PLLS_MASK | MCG_C6_VDIV(26); //VDIV = 26 (x50)
            pll_freq = 100;
            break;
        case 2:
            // Set system options dividers
            //MCG=PLL, core = MCG, bus = MCG/2, FlexBus = MCG/2, Flash clock= MCG/4
            set_sys_dividers( 0, 1, 1, 3 );
            // Set the VCO divider and enable the PLL for 96MHz, LOLIE=0, PLLS=1, CME=0, VDIV=24
            MCG_C6 = MCG_C6_PLLS_MASK | MCG_C6_VDIV(24); //VDIV = 24 (x48)
            pll_freq = 96;
            break;
        case 3:
            // Set system options dividers
            //MCG=PLL, core = MCG, bus = MCG, FlexBus = MCG, Flash clock= MCG/2
            set_sys_dividers( 0, 0, 0, 1 );
            // Set the VCO divider and enable the PLL for 48MHz, LOLIE=0, PLLS=1, CME=0, VDIV=0
            MCG_C6 = MCG_C6_PLLS_MASK; //VDIV = 0 (x24)
            pll_freq = 48;
            break;
        default: /* checked earlier */
        	break;
    }
    while ( !( MCG_S & MCG_S_PLLST_MASK ) )
    {
    }; // wait for PLL status bit to set

    while ( !( MCG_S & MCG_S_LOCK_MASK ) )
    {
    }; // Wait for LOCK bit to set

// Now running PBE Mode

// Transition into PEE by setting CLKS to 0
// CLKS=0, FRDIV=3, IREFS=0, IRCLKEN=0, IREFSTEN=0
    MCG_C1 &= (uint8_t) ~MCG_C1_CLKS_MASK;

// Wait for clock status bits to update
    while ( ( ( MCG_S & MCG_S_CLKST_MASK ) >> MCG_S_CLKST_SHIFT ) != 0x3 )
    {
    };

// Now running PEE Mode

    return pll_freq;
}

__attribute__ ((section ("text.fastcode")))
static void set_sys_dividers( uint32_t outdiv1, uint32_t outdiv2, uint32_t outdiv3, uint32_t outdiv4 )
{
    uint32_t temp_reg;
    uint8_t  i;

    temp_reg = FMC_PFAPR; // store present value of FMC_PFAPR

    // set M0PFD through M7PFD to 1 to disable prefetch
    FMC_PFAPR |= FMC_PFAPR_M7PFD_MASK | FMC_PFAPR_M6PFD_MASK | FMC_PFAPR_M5PFD_MASK | FMC_PFAPR_M4PFD_MASK | FMC_PFAPR_M3PFD_MASK | FMC_PFAPR_M2PFD_MASK | FMC_PFAPR_M1PFD_MASK | FMC_PFAPR_M0PFD_MASK;

    // set clock dividers to desired value
    SIM_CLKDIV1 = SIM_CLKDIV1_OUTDIV1(outdiv1) | SIM_CLKDIV1_OUTDIV2(outdiv2) | SIM_CLKDIV1_OUTDIV3(outdiv3) | SIM_CLKDIV1_OUTDIV4(outdiv4);

    // wait for dividers to change
    for ( i = 0; i < outdiv4; i++ )
    {
    }

    FMC_PFAPR = temp_reg; // re-store original value of FMC_PFAPR

    return;
}

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
