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
 *  Defines functions that access platform specific peripherals
 *
 */

#pragma once

#include "wiced_utilities.h"
#include "wwd_constants.h"
#include "platform.h" /* This file is unique for each platform */

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 * @cond                Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
/* SPI mode constants */
#define SPI_CLOCK_RISING_EDGE  ( 1 << 0 )
#define SPI_CLOCK_FALLING_EDGE ( 0 << 0 )
#define SPI_CLOCK_IDLE_HIGH    ( 1 << 1 )
#define SPI_CLOCK_IDLE_LOW     ( 0 << 1 )
#define SPI_USE_DMA            ( 1 << 2 )
#define SPI_NO_DMA             ( 0 << 2 )
#define SPI_MSB_FIRST          ( 1 << 3 )
#define SPI_LSB_FIRST          ( 0 << 3 )

#define I2C_DEVICE_DMA_MASK_POSN 0
#define I2C_DEVICE_NO_DMA    (0 << I2C_DEVICE_DMA_MASK_POSN)
#define I2C_DEVICE_USE_DMA   (1 << I2C_DEVICE_DMA_MASK_POSN)


/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    INPUT_PULL_UP,                  /* Input with an internal pull-up resistor - use with devices that actively drive the signal low - e.g. button connected to ground */
    INPUT_PULL_DOWN,                /* Input with an internal pull-down resistor - use with devices that actively drive the signal high - e.g. button connected to a power rail */
    INPUT_HIGH_IMPEDANCE,           /* Input - must always be driven, either actively or by an external pullup resistor */
    OUTPUT_PUSH_PULL,               /* Output actively driven high and actively driven low - must not be connected to other active outputs - e.g. LED output */
    OUTPUT_OPEN_DRAIN_NO_PULL,      /* Output actively driven low but is high-impedance when set high - can be connected to other open-drain/open-collector outputs. Needs an external pull-up resistor */
    OUTPUT_OPEN_DRAIN_PULL_UP,      /* Output actively driven low and is pulled high with an internal resistor when set high - can be connected to other open-drain/open-collector outputs. */
} wiced_gpio_config_t;

typedef enum
{
    DATA_WIDTH_5BIT,
    DATA_WIDTH_6BIT,
    DATA_WIDTH_7BIT,
    DATA_WIDTH_8BIT,
    DATA_WIDTH_9BIT
} wiced_uart_data_width_t;

typedef enum
{
    STOP_BITS_1,
    STOP_BITS_2,
} wiced_uart_stop_bits_t;

typedef enum
{
    FLOW_CONTROL_DISABLED,
    FLOW_CONTROL_CTS,
    FLOW_CONTROL_RTS,
    FLOW_CONTROL_CTS_RTS
} wiced_uart_flow_control_t;

typedef enum
{
    NO_PARITY,
    ODD_PARITY,
    EVEN_PARITY,
} wiced_uart_parity_t;

typedef enum
{
    IRQ_TRIGGER_RISING_EDGE  = 0x1, /* Interrupt triggered at input signal's rising edge  */
    IRQ_TRIGGER_FALLING_EDGE = 0x2, /* Interrupt triggered at input signal's falling edge */
    IRQ_TRIGGER_BOTH_EDGES   = IRQ_TRIGGER_RISING_EDGE | IRQ_TRIGGER_FALLING_EDGE,
} wiced_gpio_irq_trigger_t;

typedef enum
{
    I2C_ADDRESS_WIDTH_7BIT,
    I2C_ADDRESS_WIDTH_10BIT,
    I2C_ADDRESS_WIDTH_16BIT,
} wiced_i2c_bus_address_width_t;

typedef enum
{
    I2C_LOW_SPEED_MODE,         /* 10Khz devices */
    I2C_STANDARD_SPEED_MODE,    /* 100Khz devices */
    I2C_HIGH_SPEED_MODE         /* 400Khz devices */
} wiced_i2c_speed_mode_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef void (*wiced_gpio_irq_handler_t)( void* arg );

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_spi_t  port;
    wiced_gpio_t chip_select;
    uint32_t     speed;
    uint8_t      mode;
    uint8_t      bits;
} wiced_spi_device_t;

typedef struct
{
    const void* tx_buffer;
    void*       rx_buffer;
    uint32_t    length;
} wiced_spi_message_segment_t;

typedef struct
{
    wiced_i2c_t                   port;
    uint16_t                      address;       /* the address of the device on the i2c bus */
    wiced_i2c_bus_address_width_t address_width;
    uint8_t                       flags;
    wiced_i2c_speed_mode_t        speed_mode;    /* speed mode the device operates in */
} wiced_i2c_device_t;

typedef struct
{
    const void*  tx_buffer;  /* A pointer to the data to be transmitted. If NULL, the message is an RX message when 'combined' is FALSE */
    void*        rx_buffer;  /* A pointer to the data to be transmitted. If NULL, the message is an TX message when 'combined' is FALSE */
    uint16_t     tx_length;
    uint16_t     rx_length;
    uint16_t     retries;    /* Number of times to retry the message */
    wiced_bool_t combined;
    uint8_t      flags;      /* MESSAGE_DISABLE_DMA : if set, this flag disables use of DMA for the message */
} wiced_i2c_message_t;

typedef struct
{
    uint32_t                  baud_rate;
    wiced_uart_data_width_t   data_width;
    wiced_uart_parity_t       parity;
    wiced_uart_stop_bits_t    stop_bits;
    wiced_uart_flow_control_t flow_control;
} wiced_uart_config_t;

typedef struct
{
    uint8_t sec;
    uint8_t min;
    uint8_t hr;
    uint8_t weekday;/* 1-sunday... 7-saturday */
    uint8_t date;
    uint8_t month;
    uint8_t year;
}wiced_rtc_time_t;

/******************************************************
 *                     Variables
 ******************************************************/

#ifdef WICED_PLATFORM_INCLUDES_SPI_FLASH
extern wiced_spi_device_t wiced_spi_flash;
#endif

/******************************************************
 * @endcond           Function Declarations
 ******************************************************/

/*****************************************************************************/
/** @defgroup platform       Platform functions
 *
 *  WICED hardware platform functions
 */
/*****************************************************************************/

/*****************************************************************************/
/** @addtogroup uart       UART
 *  @ingroup platform
 *
 * Universal Asynchronous Receiver Transmitter (UART) Functions
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Initialises a UART interface
 *
 * Prepares an UART hardware interface for communications
 *
 * @param  uart     : the interface which should be initialised
 * @param  config   : UART configuration structure
 * @param  optional_rx_buffer : Pointer to an optional RX ring buffer
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_uart_init( wiced_uart_t uart, const wiced_uart_config_t* config, wiced_ring_buffer_t* optional_rx_buffer );


/** Deinitialises a UART interface
 *
 * @param  uart : the interface which should be deinitialised
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_uart_deinit( wiced_uart_t uart );


/** Transmit data on a UART interface
 *
 * @param  uart     : the UART interface
 * @param  data     : pointer to the start of data
 * @param  size     : number of bytes to transmit
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_uart_transmit_bytes( wiced_uart_t uart, const void* data, uint32_t size );


/** Receive data on a UART interface
 *
 * @param  uart     : the UART interface
 * @param  data     : pointer to the buffer which will store incoming data
 * @param  size     : number of bytes to receive
 * @param  timeout  : timeout in milisecond
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_uart_receive_bytes( wiced_uart_t uart, void* data, uint32_t size, uint32_t timeout );


/** @} */
/*****************************************************************************/
/** @addtogroup spi       SPI
 *  @ingroup platform
 *
 * Serial Peripheral Interface (SPI) Functions
 *
 *
 *  @{
 */
/*****************************************************************************/


/** Initialises the SPI interface for a given SPI device
 *
 * Prepares a SPI hardware interface for communication as a master
 *
 * @param  spi : the SPI device to be initialised
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if the SPI device could not be initialised
 */
wiced_result_t wiced_spi_init( const wiced_spi_device_t* spi );


/** Transmits and/or receives data from a SPI device
 *
 * @param  spi      : the SPI device to be initialised
 * @param  segments : a pointer to an array of segments
 * @param  number_of_segments : the number of segments to transfer
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_spi_transfer( const wiced_spi_device_t* spi, wiced_spi_message_segment_t* segments, uint16_t number_of_segments );


/** De-initialises a SPI interface
 *
 * Turns off a SPI hardware interface
 *
 * @param  spi : the SPI device to be de-initialised
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_spi_deinit( const wiced_spi_device_t* spi );


/** @} */
/*****************************************************************************/
/** @addtogroup i2c       I2C
 *  @ingroup platform
 *
 * Inter-IC bus (I2C) Functions
 *
 *
 *  @{
 */
/*****************************************************************************/


/** Initialises an I2C interface
 *
 * Prepares an I2C hardware interface for communication as a master
 *
 * @param  device : the device for which the i2c port should be initialised
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred during initialisation
 */
wiced_result_t wiced_i2c_init( wiced_i2c_device_t* device );


/** Checks whether the device is available on a bus or not
 *
 *
 * @param  device : the i2c device to be probed
 * @param  retries    : the number of times to attempt to probe the device
 *
 * @return    WICED_TRUE : device is found.
 * @return    WICED_FALSE: device is not found
 */
wiced_bool_t wiced_i2c_probe_device( wiced_i2c_device_t* device, int retries );


/** Initialize the wiced_i2c_message_t structure for i2c tx transaction
 *
 * @param message : pointer to a message structure, this should be a valid pointer
 * @param tx_buffer : pointer to a tx buffer that is already allocated
 * @param tx_buffer_length : number of bytes to transmit
 * @param retries    : the number of times to attempt send a message in case it can't not be sent
 * @param disable_dma : if true, disables the dma for current tx transaction. You may find it useful to switch off dma for short tx messages.
 *                     if you set this flag to 0, then you should make sure that the device flags was configured with I2C_DEVICE_USE_DMA. If the
 *                     device doesnt support DMA, the message will be transmitted with no DMA.
 *
 * @return    WICED_SUCCESS : message structure was initialised properly.
 * @return    WICED_BADARG: one of the arguments is given incorrectly
 */
wiced_result_t wiced_i2c_init_tx_message(wiced_i2c_message_t* message, const void* tx_buffer, uint16_t  tx_buffer_length, uint16_t retries , wiced_bool_t disable_dma);

/** Initialize the wiced_i2c_message_t structure for i2c rx transaction
 *
 * @param message : pointer to a message structure, this should be a valid pointer
 * @param rx_buffer : pointer to an rx buffer that is already allocated
 * @param rx_buffer_length : number of bytes to receive
 * @param retries    : the number of times to attempt receive a message in case device doesnt respond
 * @param disable_dma : if true, disables the dma for current rx transaction. You may find it useful to switch off dma for short rx messages.
 *                     if you set this flag to 0, then you should make sure that the device flags was configured with I2C_DEVICE_USE_DMA. If the
 *                     device doesnt support DMA, the message will be received not using DMA.
 *
 * @return    WICED_SUCCESS : message structure was initialised properly.
 * @return    WICED_BADARG: one of the arguments is given incorrectly
 */
wiced_result_t wiced_i2c_init_rx_message(wiced_i2c_message_t* message, void* rx_buffer, uint16_t rx_buffer_length, uint16_t retries , wiced_bool_t disable_dma);


/** Initialize the wiced_i2c_message_t structure for i2c combined transaction
 *
 * @param  message : pointer to a message structure, this should be a valid pointer
 * @param tx_buffer: pointer to a tx buffer that is already allocated
 * @param rx_buffer: pointer to an rx buffer that is already allocated
 * @param tx_buffer_length: number of bytes to transmit
 * @param rx_buffer_length: number of bytes to receive
 * @param  retries    : the number of times to attempt receive a message in case device doesnt respond
 * @param disable_dma: if true, disables the dma for current rx transaction. You may find it useful to switch off dma for short rx messages.
 *                     if you set this flag to 0, then you should make sure that the device flags was configured with I2C_DEVICE_USE_DMA. If the
 *                     device doesnt support DMA, the message will be received not using DMA.
 *
 * @return    WICED_SUCCESS : message structure was initialised properly.
 * @return    WICED_BADARG: one of the arguments is given incorrectly
 */
wiced_result_t wiced_i2c_init_combined_message(wiced_i2c_message_t* message, const void* tx_buffer, void* rx_buffer, uint16_t tx_buffer_length, uint16_t rx_buffer_length, uint16_t retries , wiced_bool_t disable_dma);


/** Transmits and/or receives data over an I2C interface
 *
 * @param  device             : the i2c device to communicate with
 * @param  message            : a pointer to a message (or an array of messages) to be transmitted/received
 * @param  number_of_messages : the number of messages to transfer. [1 .. N] messages
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred during message transfer
 */
wiced_result_t wiced_i2c_transfer( wiced_i2c_device_t* device, wiced_i2c_message_t* message, uint16_t number_of_messages );


/** Deinitialises an I2C device
 *
 * @param  device : the device for which the i2c port should be deinitialised
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred during deinitialisation
 */
wiced_result_t wiced_i2c_deinit( wiced_i2c_device_t* device );



/** @} */
/*****************************************************************************/
/** @addtogroup adc       ADC
 *  @ingroup platform
 *
 * Analog to Digital Converter (ADC) Functions
 *
 *
 *  @{
 */
/*****************************************************************************/


/** Initialises an ADC interface
 *
 * Prepares an ADC hardware interface for sampling
 *
 * @param adc            : the interface which should be initialised
 * @param sampling_cycle : sampling period in number of ADC clock cycles. If the
 *                         MCU does not support the value provided, the closest
 *                         supported value is used.
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_adc_init( wiced_adc_t adc, uint32_t sampling_cycle );



/** Takes a single sample from an ADC interface
 *
 * Takes a single sample from an ADC interface
 *
 * @param adc    : the interface which should be sampled
 * @param output : pointer to a variable which will receive the sample
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_adc_take_sample( wiced_adc_t adc, uint16_t* output );


/** Takes multiple samples from an ADC interface
 *
 * Takes multiple samples from an ADC interface and stores them in
 * a memory buffer
 *
 * @param adc           : the interface which should be sampled
 * @param buffer        : a memory buffer which will receive the samples
 *                        Each sample will be uint16_t little endian.
 * @param buffer_length : length in bytes of the memory buffer.
 *
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_adc_take_sample_stream( wiced_adc_t adc, void* buffer, uint16_t buffer_length );


/** De-initialises an ADC interface
 *
 * Turns off an ADC hardware interface
 *
 * @param  adc : the interface which should be de-initialised
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_adc_deinit( wiced_adc_t adc );

/** @} */
/*****************************************************************************/
/** @addtogroup gpio       GPIO
 *  @ingroup platform
 *
 * General Purpose Input/Output pin (GPIO) Functions
 *
 *
 *  @{
 */
/*****************************************************************************/


/** Initialises a GPIO pin
 *
 * Prepares a GPIO pin for use.
 *
 * @param gpio          : the gpio pin which should be initialised
 * @param configuration : A structure containing the required
 *                        gpio configuration
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_gpio_init( wiced_gpio_t gpio, wiced_gpio_config_t configuration );


/** Sets an output GPIO pin high
 *
 * Sets an output GPIO pin high. Using this function on a
 * gpio pin which is set to input mode is undefined.
 *
 * @param gpio          : the gpio pin which should be set high
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_gpio_output_high( wiced_gpio_t gpio );


/** Sets an output GPIO pin low
 *
 * Sets an output GPIO pin low. Using this function on a
 * gpio pin which is set to input mode is undefined.
 *
 * @param gpio          : the gpio pin which should be set low
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_gpio_output_low( wiced_gpio_t gpio );


/** Get the state of an input GPIO pin
 *
 * Get the state of an input GPIO pin. Using this function on a
 * gpio pin which is set to output mode will return an undefined value.
 *
 * @param gpio          : the gpio pin which should be read
 *
 * @return    WICED_TRUE  : if high
 * @return    WICED_FALSE : if low
 */
wiced_bool_t   wiced_gpio_input_get( wiced_gpio_t gpio );


/** Enables an interrupt trigger for an input GPIO pin
 *
 * Enables an interrupt trigger for an input GPIO pin.
 * Using this function on a gpio pin which is set to
 * output mode is undefined.
 *
 * @param gpio    : the gpio pin which will provide the interrupt trigger
 * @param trigger : the type of trigger (rising/falling edge)
 * @param handler : a function pointer to the interrupt handler
 * @param arg     : an argument that will be passed to the
 *                  interrupt handler
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_gpio_input_irq_enable( wiced_gpio_t gpio, wiced_gpio_irq_trigger_t trigger, wiced_gpio_irq_handler_t handler, void* arg );


/** Disables an interrupt trigger for an input GPIO pin
 *
 * Disables an interrupt trigger for an input GPIO pin.
 * Using this function on a gpio pin which has not been set up
 * using @ref wiced_gpio_input_irq_enable is undefined.
 *
 * @param gpio    : the gpio pin which provided the interrupt trigger
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_gpio_input_irq_disable( wiced_gpio_t gpio );

/** @} */
/*****************************************************************************/
/** @addtogroup pwm       PWM
 *  @ingroup platform
 *
 * Pulse-Width Modulation (PWM) Functions
 *
 *
 *  @{
 */
/*****************************************************************************/


/** Initialises a PWM pin
 *
 * Prepares a Pulse-Width Modulation pin for use.
 * Does not start the PWM output (use @ref wiced_pwm_start).
 *
 * @param pwm        : the PWM interface which should be initialised
 * @param frequency  : Output signal frequency in Hertz
 * @param duty_cycle : Duty cycle of signal as a floating-point percentage (0.0 to 100.0)
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_pwm_init(wiced_pwm_t pwm, uint32_t frequency, float duty_cycle);


/** Starts PWM output on a PWM interface
 *
 * Starts Pulse-Width Modulation signal output on a PWM pin
 *
 * @param pwm        : the PWM interface which should be started
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_pwm_start(wiced_pwm_t pwm);


/** Stops output on a PWM pin
 *
 * Stops Pulse-Width Modulation signal output on a PWM pin
 *
 * @param pwm        : the PWM interface which should be stopped
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_pwm_stop(wiced_pwm_t pwm);

/** @} */
/*****************************************************************************/
/** @addtogroup watchdog       Watchdog
 *  @ingroup platform
 *
 * Watchdog Timer Functions
 *
 *
 *  @{
 */
/*****************************************************************************/


/** Kick the system watchdog.
 *
 * Resets (kicks) the timer of the system watchdog.
 * This MUST be done done by the App regularly.
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_watchdog_kick( void );


/** @} */
/*****************************************************************************/
/** @addtogroup mcupowersave       Powersave
 *  @ingroup platform
 *
 * MCU Powersave Functions
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Enables the MCU to enter powersave mode.
 *
 * @warning   If the MCU drives the sleep clock input pin of the WLAN chip,   \n
 *            ensure the 32kHz clock output from the MCU is maintained while  \n
 *            the MCU is in powersave mode. The WLAN sleep clock reference is \n
 *            typically configured in the file:                               \n
 *            <WICED-SDK>/include/platforms/<PLATFORM_NAME>/platform.h
 * @return    void
 */
void wiced_platform_mcu_enable_powersave( void );


/** Stops the MCU entering powersave mode.
 *
 * @return    void
 */
void wiced_platform_mcu_disable_powersave( void );

/** @} */

/**
 * This function will return the value of time read from the on board CPU real time clock. Time value must be given in the format of
 * the structure wiced_rtc_time_t
 *
 * @param time        : pointer to a time structure
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_platform_get_rtc_time(wiced_rtc_time_t* time);

/**
 * This function will set MCU RTC time to a new value. Time value must be given in the format of
 * the structure wiced_rtc_time_t
 *
 * @param time        : pointer to a time structure
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t wiced_platform_set_rtc_time(wiced_rtc_time_t* time);


#ifdef __cplusplus
} /*extern "C" */
#endif
