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
#include "sam4s.h"
#include "sysclk.h"
#include "adc.h"
#include "ioport.h"
#include "usart.h"
#include "spi.h"
#include "supc.h"
#include "pdc.h"
#include "pwm.h"
#include "wdt.h"
#include "efc.h"
#include "rtt.h"
#include "supc.h"
#include "matrix.h"
#include "platform.h"
#include "platform_common_config.h"
#include "wwd_constants.h"
#include "wiced_defaults.h"
#include "wiced_utilities.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define ENABLE_INTERRUPTS   __asm("CPSIE i")
#define DISABLE_INTERRUPTS  __asm("CPSID i")

/******************************************************
 *                    Constants
 ******************************************************/

/* NVIC Interrupt Priority List */
#define SAM4S_RTT_IRQ_PRIO   ( 0x0 )
#define SAM4S_SDIO_IRQ_PRIO  ( 0x2 )
#define SAM4S_UART_IRQ_PRIO  ( 0x6 )
#define SAM4S_GPIO_IRQ_PRIO  ( 0xE )

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/* GPIO interrupt callback */
typedef void (*sam4s_gpio_irq_callback_t)( void* arg );

/* I/O pin. Use IOPORT_CREATE_PIN to define a I/O pin */
typedef ioport_pin_t sam4s_pin_t;

/* UART configuration */
typedef sam_usart_opt_t sam4s_uart_config_t;

/* Peripheral pin configuration */
typedef ioport_mode_t sam4s_peripheral_pin_config_t;

/******************************************************
 *                    Structures
 ******************************************************/

#pragma pack(1)
/* GPIO pin configuration */
typedef struct
{
    uint8_t       direction; /* direction: IOPORT_DIR_INPUT or OUTPUT */
    ioport_mode_t mode;      /* pin mode:  see ioport_pio.h           */
} sam4s_gpio_pin_config_t;

/* GPIO interrupt configuration */
typedef struct
{
    wiced_bool_t              wakeup_pin;
    uint8_t                   trigger;  /* interrupt trigger: IOPORT_SENSE_FALLING, RISING, or BOTHEDGES */
    sam4s_gpio_irq_callback_t callback; /* function called when interrupt occurs                         */
    void*                     arg;      /* argument passed into the function                             */
} sam4s_gpio_irq_config_t;

/* UART interface */
typedef struct
{
    wiced_uart_t                  uart;             /* WICED_UART index */
    void*                         peripheral;       /* Usart* or Uart*  */
    uint8_t                       peripheral_id;    /* Peripheral ID    */
    IRQn_Type                     interrupt_vector; /* Interrupt vector */
    sam4s_pin_t*                  tx_pin;           /* Tx pin           */
    sam4s_peripheral_pin_config_t tx_pin_config;    /* Tx pin config    */
    sam4s_pin_t*                  rx_pin;           /* Rx pin           */
    sam4s_peripheral_pin_config_t rx_pin_config;    /* Rx pin config    */
    sam4s_pin_t*                  cts_pin;          /* CTS pin          */
    sam4s_peripheral_pin_config_t cts_pin_config;   /* CTS pin config   */
    sam4s_pin_t*                  rts_pin;          /* RTS pin          */
    sam4s_peripheral_pin_config_t rts_pin_config;   /* RTS pin config   */
} sam4s_uart_t;

/* SPI interface */
typedef struct
{
    Spi*                          peripheral;      /* Peripheral            */
    uint8_t                       peripheral_id;   /* Peripheral ID         */
    sam4s_pin_t*                  clk_pin;         /* CLK  pin              */
    sam4s_peripheral_pin_config_t clk_pin_config;  /* CLK pin config        */
    sam4s_pin_t*                  mosi_pin;        /* MOSI pin              */
    sam4s_peripheral_pin_config_t mosi_pin_config; /* MOSI pin config       */
    sam4s_pin_t*                  miso_pin;        /* MISO pin              */
    sam4s_peripheral_pin_config_t miso_pin_config; /* MISO pin config       */
    sam4s_pin_t*                  cs_gpio_pin;     /* Chip-select GPIO pin  */
} sam4s_spi_t;

/* SPI configuration */
typedef struct
{
    uint32_t speed_hz;      /* Clock speed in Hz */
    uint8_t  phase;         /* Phase    : 0 or 1 */
    uint8_t  polarity;      /* Polarity : 0 or 1 */
} sam4s_spi_config_t;

typedef struct
{
    Adc*                     peripheral;
    uint8_t                  peripheral_id;
    sam4s_pin_t*             adc_pin;
    uint32_t                 adc_clock_hz;
    enum adc_channel_num_t   channel;
    enum adc_settling_time_t settling_time;
    enum adc_resolution_t    resolution;
    enum adc_trigger_t       trigger;
} sam4s_adc_t;

typedef struct
{
    wiced_bool_t is_wakeup_pin;
    uint8_t      wakeup_pin_number; /* wakeup pin number: 0 .. 15                     */
    uint8_t      trigger;           /* wakeup trigger: IOPORT_SENSE_FALLING or RISING */
} sam4s_wakeup_pin_config_t;
#pragma pack()

/******************************************************
 *                 Global Variables
 ******************************************************/

/* Defined in Wiced/<Platform>/platform.c */
extern const sam4s_pin_t               const platform_gpio_pin_mapping[];
extern const sam4s_uart_t              const platform_uart[];
extern const sam4s_spi_t               const platform_spi[];
extern const sam4s_adc_t               const platform_adc[];
extern const sam4s_wakeup_pin_config_t const platform_wakeup_pin_config[];

/******************************************************
 *         Powersave Function Declarations
 ******************************************************/

/* Initialise powersave */
wiced_result_t sam4s_powersave_init( void );

/* Tell the system that clock is needed */
wiced_result_t sam4s_powersave_clocks_needed( void );

/* Tell the system that clock is no longer needed */
wiced_result_t sam4s_powersave_clocks_not_needed( void );

/* Enable wake-up function on certain GPIO pin */
wiced_result_t sam4s_powersave_enable_wakeup_pin( const sam4s_wakeup_pin_config_t* config );

/* Disable wake-up function on certain GPIO pin */
wiced_result_t sam4s_powersave_disable_wakeup_pin( const sam4s_wakeup_pin_config_t* config );

/* Notify the system that it's woken up due to interrupt */
void           sam4s_powersave_wake_up_interrupt_notify( void );

/* Prepare platform-specific pin and peripheral settings before entering powersave */
wiced_result_t platform_enter_powersave( void );

/* Restore platform-specific pin and peripheral settings after exiting powersave */
wiced_result_t platform_exit_powersave( void );

/******************************************************
 *             IO Function Declarations
 ******************************************************/

/* Initialise a GPIO pin */
wiced_result_t sam4s_gpio_pin_init( const sam4s_pin_t* gpio_pin, const sam4s_gpio_pin_config_t* config );

/* Initialise a peripheral pin */
wiced_result_t sam4s_peripheral_pin_init( const sam4s_pin_t* peripheral_pin, const sam4s_peripheral_pin_config_t* config );

/* De-initialise a pin (GPIO or peripheral) */
wiced_result_t sam4s_pin_deinit( const sam4s_pin_t* pin );

/* Set output of a GPIO to 1 */
wiced_result_t sam4s_gpio_output_high( const sam4s_pin_t* gpio_pin );

/* Set output of a GPIO to 0 */
wiced_result_t sam4s_gpio_output_low( const sam4s_pin_t* gpio_pin );

/* Get logic level of a GPIO input pin */
wiced_bool_t   sam4s_gpio_get_input( const sam4s_pin_t* gpio_pin );

/* Enable interrupt on a GPIO input pin */
wiced_result_t sam4s_gpio_irq_enable( const sam4s_pin_t* gpio_pin, const sam4s_gpio_irq_config_t* config );

/* Disable interrupt on a GPIO input pin */
wiced_result_t sam4s_gpio_irq_disable( const sam4s_pin_t* gpio_pin );

/******************************************************
 *             UART Function Declarations
 ******************************************************/

/* Initialise UART interface */
wiced_result_t sam4s_uart_init( const sam4s_uart_t* uart, const sam4s_uart_config_t* config, wiced_ring_buffer_t* ring_buffer );

/* Deinitialise UART interface */
wiced_result_t sam4s_uart_deinit( const sam4s_uart_t* uart );

/* Transmit data out via UART interface */
wiced_result_t sam4s_uart_transmit( const sam4s_uart_t* uart, const uint8_t* data_out, uint32_t data_length );

/* Receive data from via UART interface */
wiced_result_t sam4s_uart_receive( const sam4s_uart_t* uart, uint8_t* data_in, uint32_t expected_data_length, uint32_t timeout_ms );

/* Initialise UART standard I/O */
void           platform_stdio_init( void );

/* Called by newlib standard I/O write */
void           platform_stdio_write( const char* str, uint32_t len );

/* Called by newlib standard I/O read */
void           platform_stdio_read( char* str, uint32_t len );

/******************************************************
 *             SPI Function Declarations
 ******************************************************/

/* Initialise SPI interface */
wiced_result_t sam4s_spi_init( const sam4s_spi_t* spi, const sam4s_spi_config_t* config );

/* Deinitialise SPI interface */
wiced_result_t sam4s_spi_deinit( const sam4s_spi_t* spi );

/* Start SPI transfer */
wiced_result_t sam4s_spi_transfer( const sam4s_spi_t* spi, const uint8_t* data_out, uint8_t* data_in, uint32_t data_length );

/* Assert chip select */
wiced_result_t sam4s_spi_assert_chip_select( const sam4s_spi_t* spi );

/* De-assert chip select */
wiced_result_t sam4s_spi_deassert_chip_select( const sam4s_spi_t* spi );

/******************************************************
 *           Watchdog Function Declarations
 ******************************************************/

/* Initialise watchdog */
wiced_result_t sam4s_watchdog_init( void );

/* Kick watchdog timer */
wiced_result_t sam4s_watchdog_kick( void );

/* Check if last reset caused by watchdog */
wiced_bool_t   sam4s_watchdog_check_last_reset( void );

/******************************************************
 *             ADC Function Declarations
 ******************************************************/

wiced_result_t sam4s_adc_init( const sam4s_adc_t* adc, uint8_t sampling_cycle );

wiced_result_t sam4s_adc_deinit( const sam4s_adc_t* adc );

wiced_result_t sam4s_adc_start_software_conversion( const sam4s_adc_t* adc, uint16_t* value );
