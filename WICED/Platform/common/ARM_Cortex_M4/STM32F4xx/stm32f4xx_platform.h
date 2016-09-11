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

#include "stm32f4xx.h"

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

typedef USART_TypeDef      usart_registers_t;
typedef DMA_TypeDef        dma_registers_t;
typedef DMA_Stream_TypeDef dma_stream_registers_t;
typedef uint32_t           dma_channel_t;
typedef IRQn_Type          irq_vector_t;
typedef FunctionalState    functional_state_t;
typedef uint32_t           peripheral_clock_t;
typedef void (*peripheral_clock_func_t)(peripheral_clock_t clock, functional_state_t state );

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    GPIO_TypeDef* bank;
    uint8_t       number;
    uint32_t      peripheral_clock;
} platform_pin_mapping_t;

typedef struct
{
    ADC_TypeDef*            adc;
    uint8_t                 channel;
    uint32_t                adc_peripheral_clock;
    uint8_t                 rank;
    platform_pin_mapping_t* pin;
} platform_adc_mapping_t;

typedef struct
{
    TIM_TypeDef*            tim;
    uint8_t                 channel;
    uint32_t                tim_peripheral_clock;
    uint8_t                 gpio_af;
    platform_pin_mapping_t* pin;
} platform_pwm_mapping_t;


/* DMA can be enabled by setting SPI_USE_DMA */
typedef struct
{
    SPI_TypeDef*                  spi_regs;
    uint8_t                       gpio_af;
    uint32_t                      peripheral_clock_reg;
    peripheral_clock_func_t       peripheral_clock_func;
    const platform_pin_mapping_t* pin_mosi;
    const platform_pin_mapping_t* pin_miso;
    const platform_pin_mapping_t* pin_clock;
    DMA_Stream_TypeDef*           tx_dma_stream;
    DMA_Stream_TypeDef*           rx_dma_stream;
    uint32_t                      tx_dma_channel;
    uint32_t                      rx_dma_channel;
    uint8_t                       tx_dma_stream_number;
    uint8_t                       rx_dma_stream_number;
} platform_spi_mapping_t;

typedef struct
{
    USART_TypeDef*                usart;
    uint8_t                       gpio_af;
    const platform_pin_mapping_t* pin_tx;
    const platform_pin_mapping_t* pin_rx;
    const platform_pin_mapping_t* pin_cts;
    const platform_pin_mapping_t* pin_rts;
    peripheral_clock_t            usart_peripheral_clock;
    peripheral_clock_func_t       usart_peripheral_clock_func;
    irq_vector_t                  usart_irq;
    dma_registers_t*              tx_dma;
    dma_stream_registers_t*       tx_dma_stream;
    uint8_t                       tx_dma_stream_number;
    dma_channel_t                 tx_dma_channel;
    peripheral_clock_t            tx_dma_peripheral_clock;
    peripheral_clock_func_t       tx_dma_peripheral_clock_func;
    irq_vector_t                  tx_dma_irq;
    dma_registers_t*              rx_dma;
    dma_stream_registers_t*       rx_dma_stream;
    uint8_t                       rx_dma_stream_number;
    dma_channel_t                 rx_dma_channel;
    peripheral_clock_t            rx_dma_peripheral_clock;
    peripheral_clock_func_t       rx_dma_peripheral_clock_func;
    irq_vector_t                  rx_dma_irq;
} platform_uart_mapping_t;


/******************************************************
 *                 Global Variables
 ******************************************************/

extern const platform_pin_mapping_t  gpio_mapping[];
extern const platform_adc_mapping_t  adc_mapping[];
extern const platform_pwm_mapping_t  pwm_mappings[];
extern const platform_spi_mapping_t  spi_mapping[];
extern const platform_uart_mapping_t uart_mapping[];

/******************************************************
 *               Function Declarations
 ******************************************************/
