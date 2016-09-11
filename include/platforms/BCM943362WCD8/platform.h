/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/*
BCM943362WCD8 platform pin definitions ...
+--------------------------------------------------------------------------------------------------------+
| Enum ID       |Pin |   Pin Name on    |    Module     | SAM4S| Peripheral  |    Board     | Peripheral  |
|               | #  |      Module      |  GPIO Alias   | Port | Available   |  Connection  |   Alias     |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_1  | 17 | MICRO_WKUP       | WICED_GPIO_1  | A 20 | GPIO        |              |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_2  | 18 | MICRO_ADC_IN1    | WICED_GPIO_2  | A 17 | GPIO        | BUTTON SW2   |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_3  | 19 | MICRO_ADC_IN2    | WICED_GPIO_3  | A 18 | GPIO        | BUTTON SW1   |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_4  | 21 | MICRO_ADC_IN3    | WICED_GPIO_4  | A 19 | GPIO        | THERMISTOR   |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_5  | 27 | MICRO_SPI_SSN    | WICED_GPIO_5  | A 11 | GPIO        |              |             |
|               |    |                  |               |      | SPI_NPCS0   |              |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_6  | 26 | MICRO_SPI_SCK    | WICED_GPIO_6  | A 14 | GPIO        |              |             |
|               |    |                  |               |      | SPI_SCK     |              |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_7  | 25 | MICRO_SPI_MISO   | WICED_GPIO_7  | A 12 | GPIO        |              |             |
|               |    |                  |               |      | SPI_MISO    |              |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_8  | 24 | MICRO_SPI_MOSI   | WICED_GPIO_8  | A 13 | GPIO        |              |             |
|               |    |                  |               |      | SPI_MOSI    |              |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_9  | 12 | MICRO_UART_TX    | WICED_GPIO_9  | A 22 | GPIO        |              |             |
|               |    |                  |               |      | USART1_TX   |              |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_10 | 11 | MICRO_UART_RX    | WICED_GPIO_10 | A 21 | GPIO        |              |             |
|               |    |                  |               |      | USART1_RX   |              |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_11 | 22 | MICRO_GPIO_0     | WICED_GPIO_11 | A 24 | GPIO        | LED D1       |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_12 | 23 | MICRO_GPIO_1     | WICED_GPIO_12 | A 25 | GPIO        | LED D2       |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_13 |  6 | MICRO_JTAG_TMS   | WICED_GPIO_13 | A  2 | GPIO        |              |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_14 |  7 | MICRO_JTAG_TCK   | WICED_GPIO_14 | A  1 | GPIO        |              |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_15 |  8 | MICRO_JTAG_TDI   | WICED_GPIO_15 | A  3 | GPIO        |              |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_16 |  9 | MICRO_JTAG_TDO   | WICED_GPIO_16 | A  4 | GPIO        |              |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_17 | 10 | MICRO_JTAG_TRSTN | WICED_GPIO_17 | A  0 | GPIO        |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
Notes
1. These mappings are defined in <WICED-SDK>/Platform/BCM943362WCD8/platform.c
2. Atmel SAM4S Datasheet -> http://www.atmel.com/Images/doc11100.pdf
*/

typedef enum
{
    WICED_GPIO_1,
    WICED_GPIO_2,
    WICED_GPIO_3,
    WICED_GPIO_4,
    WICED_GPIO_5,
    WICED_GPIO_6,
    WICED_GPIO_7,
    WICED_GPIO_8,
    WICED_GPIO_9,
    WICED_GPIO_10,
    WICED_GPIO_11,
    WICED_GPIO_12,
    WICED_GPIO_13,
    WICED_GPIO_14,
    WICED_GPIO_15,
    WICED_GPIO_16,
    WICED_GPIO_17,
    WICED_GPIO_MAX, /* Denotes the total number of GPIO port aliases. Not a valid GPIO alias */
} wiced_gpio_t;

typedef enum
{
    WICED_SPI_1,
    WICED_SPI_MAX, /* Denotes the total number of SPI port aliases. Not a valid SPI alias */
} wiced_spi_t;

typedef enum
{
    WICED_I2C_NONE = 0xFF
} wiced_i2c_t;

typedef enum
{
    WICED_PWM_1,
    WICED_PWM_2,
    WICED_PWM_3,
    WICED_PWM_4,
    WICED_PWM_5,
    WICED_PWM_6,
    WICED_PWM_7,
    WICED_PWM_8,
    WICED_PWM_9,
} wiced_pwm_t;

typedef enum
{
    WICED_ADC_1,
    WICED_ADC_2,
    WICED_ADC_3,
} wiced_adc_t;

typedef enum
{
    WICED_UART_1,
    WICED_UART_MAX, /* Denotes the total number of UART port aliases. Not a valid UART alias */
} wiced_uart_t;


/* Components connected to external I/Os*/
#define WICED_LED1       (WICED_GPIO_11)
#define WICED_LED2       (WICED_GPIO_12)
#define WICED_BUTTON1    (WICED_GPIO_3)
#define WICED_BUTTON2    (WICED_GPIO_2)
#define WICED_THERMISTOR (WICED_GPIO_4)

/* I/O connection <-> Peripheral Connections */
#define WICED_LED1_JOINS_PWM       (WICED_PWM_1)
#define WICED_LED2_JOINS_PWM       (WICED_PWM_2)
#define WICED_THERMISTOR_JOINS_ADC (WICED_ADC_3)

#define WICED_PLATFORM_INCLUDES_SPI_FLASH
#define WICED_SPI_FLASH_CS  (WICED_GPIO_5)

#ifdef __cplusplus
} /*extern "C" */
#endif
