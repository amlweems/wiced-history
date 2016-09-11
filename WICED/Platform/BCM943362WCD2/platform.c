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
#include "stm32f1xx_platform.h"
#include "platform.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_adc.h"
#include "stdlib.h"
#include "wwd_constants.h"
#include "wwd_debug.h"
#include "wiced_platform.h"
#include "watchdog.h"
#include "platform_common_config.h"
#include "Platform/wwd_platform_interface.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

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

/* This table maps STM32 pins to GPIO definitions on the schematic
 * A full pin definition is provided in <WICED-SDK>/include/platforms/BCM943362WCD2/platform.h
 */
const platform_pin_mapping_t gpio_mapping[] =
{
    [WICED_GPIO_1]  = {GPIOA,  0,  RCC_APB2Periph_GPIOA},
    [WICED_GPIO_2]  = {GPIOA,  1,  RCC_APB2Periph_GPIOA},
    [WICED_GPIO_3]  = {GPIOA,  2,  RCC_APB2Periph_GPIOA},
    [WICED_GPIO_4]  = {GPIOA,  3,  RCC_APB2Periph_GPIOA},
    [WICED_GPIO_5]  = {GPIOA,  4,  RCC_APB2Periph_GPIOA},
    [WICED_GPIO_6]  = {GPIOA,  5,  RCC_APB2Periph_GPIOA},
    [WICED_GPIO_7]  = {GPIOA,  6,  RCC_APB2Periph_GPIOA},
    [WICED_GPIO_8]  = {GPIOA,  7,  RCC_APB2Periph_GPIOA},
    [WICED_GPIO_9]  = {GPIOA,  9,  RCC_APB2Periph_GPIOA},
    [WICED_GPIO_10] = {GPIOA, 10,  RCC_APB2Periph_GPIOA},
    [WICED_GPIO_11] = {GPIOB,  6,  RCC_APB2Periph_GPIOB},
    [WICED_GPIO_12] = {GPIOB,  7,  RCC_APB2Periph_GPIOB},
    [WICED_GPIO_13] = {GPIOA, 13,  RCC_APB2Periph_GPIOA},
    [WICED_GPIO_14] = {GPIOA, 14,  RCC_APB2Periph_GPIOA},
    [WICED_GPIO_15] = {GPIOA, 15,  RCC_APB2Periph_GPIOA},
    [WICED_GPIO_16] = {GPIOB,  3,  RCC_APB2Periph_GPIOB},
    [WICED_GPIO_17] = {GPIOB,  4,  RCC_APB2Periph_GPIOB},
    [WICED_GPIO_18] = {GPIOA,  5,  RCC_APB2Periph_GPIOA},
    [WICED_GPIO_19] = {GPIOA,  6,  RCC_APB2Periph_GPIOA},
    [WICED_GPIO_20] = {GPIOA,  7,  RCC_APB2Periph_GPIOA},
};


/*
 * Possible compile time inputs:
 * - Set which ADC peripheral to use for each ADC. All on one ADC allows sequential conversion on all inputs. All on separate ADCs allows concurrent conversion.
 */
/* TODO : These need fixing */
const platform_adc_mapping_t adc_mapping[] =
{
    [WICED_ADC_1] = {ADC1, ADC_Channel_1, RCC_APB2Periph_ADC1, 1, (platform_pin_mapping_t*)&gpio_mapping[WICED_GPIO_2]},
    [WICED_ADC_2] = {ADC1, ADC_Channel_2, RCC_APB2Periph_ADC1, 1, (platform_pin_mapping_t*)&gpio_mapping[WICED_GPIO_3]},
    [WICED_ADC_3] = {ADC1, ADC_Channel_3, RCC_APB2Periph_ADC1, 1, (platform_pin_mapping_t*)&gpio_mapping[WICED_GPIO_4]},
};


/* PWM mappings */
const platform_pwm_mapping_t pwm_mappings[] =
{
    [WICED_PWM_1]  = {TIM4, 1, RCC_APB1Periph_TIM4, (platform_pin_mapping_t*)&gpio_mapping[WICED_GPIO_11]},
    [WICED_PWM_2]  = {TIM4, 2, RCC_APB1Periph_TIM4, (platform_pin_mapping_t*)&gpio_mapping[WICED_GPIO_12]},
    [WICED_PWM_3]  = {TIM2, 2, RCC_APB1Periph_TIM2, (platform_pin_mapping_t*)&gpio_mapping[WICED_GPIO_2]},  /* or TIM5/Ch2                       */
    [WICED_PWM_4]  = {TIM2, 3, RCC_APB1Periph_TIM2, (platform_pin_mapping_t*)&gpio_mapping[WICED_GPIO_3]},  /* or TIM5/Ch3, TIM9/Ch1             */
    [WICED_PWM_5]  = {TIM2, 4, RCC_APB1Periph_TIM2, (platform_pin_mapping_t*)&gpio_mapping[WICED_GPIO_4]},  /* or TIM5/Ch4, TIM9/Ch2             */
    [WICED_PWM_6]  = {TIM2, 1, RCC_APB1Periph_TIM2, (platform_pin_mapping_t*)&gpio_mapping[WICED_GPIO_6]},  /* or TIM2_CH1_ETR, TIM8/Ch1N        */
    [WICED_PWM_7]  = {TIM3, 1, RCC_APB1Periph_TIM3, (platform_pin_mapping_t*)&gpio_mapping[WICED_GPIO_7]},  /* or TIM1_BKIN, TIM8_BKIN, TIM13/Ch1*/
    [WICED_PWM_8]  = {TIM3, 2, RCC_APB1Periph_TIM3, (platform_pin_mapping_t*)&gpio_mapping[WICED_GPIO_8]},  /* or TIM8/Ch1N, TIM14/Ch1           */
    [WICED_PWM_9]  = {TIM5, 2, RCC_APB1Periph_TIM5, (platform_pin_mapping_t*)&gpio_mapping[WICED_GPIO_2]},  /* or TIM2/Ch2                       */
    /* TODO: fill in the other options here ... */
};

const platform_spi_mapping_t spi_mapping[] =
{
    [WICED_SPI_1]  =
    {
        .spi_regs              = SPI1,
        .peripheral_clock_reg  = RCC_APB2Periph_SPI1,
        .pin_mosi              = &gpio_mapping[WICED_GPIO_20],
        .pin_miso              = &gpio_mapping[WICED_GPIO_19],
        .pin_clock             = &gpio_mapping[WICED_GPIO_18],
        .tx_dma_channel        = DMA1_Channel2,
        .rx_dma_channel        = DMA1_Channel3,
        .tx_dma                = DMA1,
        .rx_dma                = DMA1,
        .tx_dma_channel_number = 3,
        .rx_dma_channel_number = 2
    }
};

const platform_uart_mapping_t uart_mapping[] =
{
    [WICED_UART_1] =
    {
        .usart                        = USART1,
        .pin_tx                       = &gpio_mapping[WICED_GPIO_9],
        .pin_rx                       = &gpio_mapping[WICED_GPIO_10],
        .pin_cts                      = NULL,
        .pin_rts                      = NULL,
        .usart_peripheral_clock       = RCC_APB2Periph_USART1,
        .usart_peripheral_clock_func  = RCC_APB2PeriphClockCmd,
        .usart_irq                    = USART1_IRQn,
        .tx_dma                       = DMA1,
        .tx_dma_channel               = DMA1_Channel4,
        .tx_dma_channel_id            = 4,
        .tx_dma_peripheral_clock      = RCC_AHBPeriph_DMA1,
        .tx_dma_peripheral_clock_func = RCC_AHBPeriphClockCmd,
        .tx_dma_irq                   = DMA1_Channel4_IRQn,
        .rx_dma                       = DMA1,
        .rx_dma_channel               = DMA1_Channel5,
        .rx_dma_channel_id            = 5,
        .rx_dma_peripheral_clock      = RCC_AHBPeriph_DMA1,
        .rx_dma_peripheral_clock_func = RCC_AHBPeriphClockCmd,
        .rx_dma_irq                   = DMA1_Channel5_IRQn
    },
    [WICED_UART_2] =
    {
        .usart                        = USART2,
        .pin_tx                       = &gpio_mapping[WICED_GPIO_3],
        .pin_rx                       = &gpio_mapping[WICED_GPIO_4],
        .pin_cts                      = &gpio_mapping[WICED_GPIO_1],
        .pin_rts                      = &gpio_mapping[WICED_GPIO_2],
        .usart_peripheral_clock       = RCC_APB1Periph_USART2,
        .usart_peripheral_clock_func  = RCC_APB1PeriphClockCmd,
        .usart_irq                    = USART2_IRQn,
        .tx_dma                       = DMA1,
        .tx_dma_channel               = DMA1_Channel7,
        .tx_dma_channel_id            = 7,
        .tx_dma_peripheral_clock      = RCC_AHBPeriph_DMA1,
        .tx_dma_peripheral_clock_func = RCC_AHBPeriphClockCmd,
        .tx_dma_irq                   = DMA1_Channel7_IRQn,
        .rx_dma                       = DMA1,
        .rx_dma_channel               = DMA1_Channel6,
        .rx_dma_channel_id            = 6,
        .rx_dma_peripheral_clock      = RCC_AHBPeriph_DMA1,
        .rx_dma_peripheral_clock_func = RCC_AHBPeriphClockCmd,
        .rx_dma_irq                   = DMA1_Channel6_IRQn
    },
};

/******************************************************
 *               Function Definitions
 ******************************************************/
wiced_spi_device_t wiced_spi_flash =
{
	.port        = WICED_SPI_1,
	.chip_select = WICED_SPI_FLASH_CS,
	.speed       = 1000000,
	.mode        = (SPI_CLOCK_RISING_EDGE | SPI_CLOCK_IDLE_HIGH | SPI_NO_DMA | SPI_MSB_FIRST),
	.bits        = 8
};

wiced_result_t wiced_platform_init( void )
{
    WPRINT_PLATFORM_INFO( ("\r\nPlatform " PLATFORM " initialised\r\n") );

    if ( WICED_TRUE == watchdog_check_last_reset() )
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


void host_platform_reset_wifi( wiced_bool_t reset_asserted )
{
    if ( reset_asserted == WICED_TRUE )
    {
        GPIO_ResetBits( WL_RESET_BANK, WL_RESET_PIN );
    }
    else
    {
        GPIO_SetBits( WL_RESET_BANK, WL_RESET_PIN );
    }
}

void host_platform_power_wifi( wiced_bool_t power_enabled )
{
    if ( power_enabled == WICED_TRUE )
    {
        GPIO_ResetBits( WL_REG_ON_BANK, WL_REG_ON_PIN );
    }
    else
    {
        GPIO_SetBits( WL_REG_ON_BANK, WL_REG_ON_PIN );
    }
}
