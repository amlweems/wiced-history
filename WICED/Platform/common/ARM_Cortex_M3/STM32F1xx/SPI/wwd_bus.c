/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "stm32f10x.h"
#include "wwd_bus_protocol.h"
#include "Platform/wwd_platform_interface.h"
#include "Platform/wwd_bus_interface.h"
#include "Platform/wwd_spi_interface.h"
#include "internal/wifi_image/wwd_wifi_image_interface.h"
#include "wifi_nvram_image.h"
#include "Network/wwd_buffer_interface.h"
#include "string.h" /* for memcpy */
#include "wwd_assert.h"
#include "gpio_irq.h"
#include "platform_common_config.h"

/******************************************************
 *             Constants
 ******************************************************/

#define DMA_TIMEOUT_LOOPS      (10000000)
#define EXTI9_5_IRQChannel     ((u8)0x17)  /* External Line[9:5] Interrupts */

/******************************************************
 *             Structures
 ******************************************************/

/******************************************************
 *             Variables
 ******************************************************/

/******************************************************
 *             Function declarations
 ******************************************************/

void dma_irq( void );

/******************************************************
 *             Function definitions
 ******************************************************/

#ifndef WICED_DISABLE_MCU_POWERSAVE
#define STOP_MODE_SUPPORT
#endif /* ifndef WICED_DISABLE_MCU_POWERSAVE */

extern void stm32f1xx_clocks_needed( void );
extern void stm32f1xx_clocks_not_needed( void );
extern void wake_up_interrupt_notify( void );

static void spi_irq_handler( void* arg )
{
    UNUSED_PARAMETER(arg);
#ifdef STOP_MODE_SUPPORT
    wake_up_interrupt_notify( );
#endif
    wiced_platform_notify_irq( );
}

void dma_irq( void )
{
    /* TODO: Not implemented yet */
}

wiced_result_t host_platform_bus_init( void )
{

    SPI_InitTypeDef spi_init;
    DMA_InitTypeDef dma_init_structure;
    GPIO_InitTypeDef gpio_init_structure;

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* STOP_MODE_SUPPORT */

    RCC_PCLK1Config( RCC_HCLK_Div2 ); /* Set clock to 18MHz (assuming 72MHz STM32 system clock) */

    /* Enable SPI_SLAVE DMA clock */
    RCC_AHBPeriphClockCmd( RCC_AHBPeriph_DMA1, ENABLE );

    /* Enable SPI_SLAVE Periph clock */
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2, ENABLE );

    /* Enable GPIO Bank B & C */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE );
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_AFIO, ENABLE);

    /* Setup the interrupt input for WLAN_IRQ */
    gpio_init_structure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init_structure.GPIO_Pin = ( 1 << SPI_IRQ_PIN);
    GPIO_Init( SPI_IRQ_BANK, &gpio_init_structure );
    gpio_irq_enable(SPI_IRQ_BANK, SPI_IRQ_PIN, IRQ_TRIGGER_RISING_EDGE, spi_irq_handler, 0);

    /* Setup the SPI lines */
    gpio_init_structure.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init_structure.GPIO_Pin = ( 1 << SPI_BUS_CLOCK_PIN ) | ( 1 << SPI_BUS_MISO_PIN ) | ( 1 << SPI_BUS_MOSI_PIN );
    GPIO_Init( SPI_BUS_CLOCK_BANK, &gpio_init_structure );

    /* Setup SPI slave select GPIOs */
    gpio_init_structure.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init_structure.GPIO_Pin =  ( 1 << SPI_BUS_CS_PIN );
    GPIO_Init( SPI_BUS_CS_BANK, &gpio_init_structure );
#if 0
    gpio_init_structure.GPIO_Pin = SPI_CS_EXT_GPIO_PIN;
#endif
    GPIO_Init( SPI_BUS_CS_BANK, &gpio_init_structure );
    GPIO_SetBits( SPI_BUS_CS_BANK, ( 1 << SPI_BUS_CS_PIN ) );   /* Set CS high (disabled) */
#if 0
    GPIO_SetBits( SPI_BUS_CS_BANK, SPI_CS_EXT_GPIO_PIN ); /* Set CS high (disabled) */
#endif

    /* Set GPIO_B[1:0] to 01 to put WLAN module into gSPI mode */
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init_structure.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init_structure.GPIO_Pin = ( 1 << WL_GPIO0_PIN );
    GPIO_Init( WL_GPIO0_BANK, &gpio_init_structure );
    gpio_init_structure.GPIO_Pin = ( 1 << WL_GPIO1_PIN );
    GPIO_Init( WL_GPIO1_BANK, &gpio_init_structure );
    GPIO_SetBits( WL_GPIO0_BANK, ( 1 << WL_GPIO0_PIN ) );
    GPIO_ResetBits( WL_GPIO1_BANK, ( 1 << WL_GPIO1_PIN ) );

    /* Setup DMA for SPI2 RX */
    DMA_DeInit( DMA1_Channel4 );
    dma_init_structure.DMA_PeripheralBaseAddr = (u32) &SPI2->DR;
    dma_init_structure.DMA_DIR = DMA_DIR_PeripheralSRC;
    dma_init_structure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma_init_structure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma_init_structure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_init_structure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma_init_structure.DMA_Mode = DMA_Mode_Normal;
    dma_init_structure.DMA_Priority = DMA_Priority_VeryHigh;
    dma_init_structure.DMA_M2M = DMA_M2M_Disable;
    dma_init_structure.DMA_MemoryBaseAddr = 0;
    dma_init_structure.DMA_BufferSize = 0;
    DMA_Init( DMA1_Channel4, &dma_init_structure );

    /* Setup DMA for SPI2 TX */
    DMA_DeInit( DMA1_Channel5 );
    dma_init_structure.DMA_PeripheralBaseAddr = (u32) &SPI2->DR;
    dma_init_structure.DMA_DIR = DMA_DIR_PeripheralDST;
    dma_init_structure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma_init_structure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma_init_structure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_init_structure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma_init_structure.DMA_Mode = DMA_Mode_Normal;
    dma_init_structure.DMA_Priority = DMA_Priority_VeryHigh;
    dma_init_structure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init( DMA1_Channel5, &dma_init_structure );

    /* Enable DMA for TX */
    SPI_I2S_DMACmd( SPI2, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE );


    /* Setup SPI */
    spi_init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi_init.SPI_Mode = SPI_Mode_Master;
    spi_init.SPI_DataSize = SPI_DataSize_8b;
    spi_init.SPI_CPOL = SPI_CPOL_High;
    spi_init.SPI_CPHA = SPI_CPHA_2Edge;
    spi_init.SPI_NSS = SPI_NSS_Soft;
    spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    spi_init.SPI_FirstBit = SPI_FirstBit_MSB;
    spi_init.SPI_CRCPolynomial = (uint16_t) 7;

    /* Init SPI and enable it */
    SPI_Init( SPI2, &spi_init );
    SPI_Cmd( SPI2, ENABLE );

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* STOP_MODE_SUPPORT */

    return WICED_SUCCESS;
}

wiced_result_t host_platform_bus_deinit( void )
{
    GPIO_InitTypeDef gpio_init_structure;

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed();
#endif /* STOP_MODE_SUPPORT */

    /* Disable SPI and SPI DMA */
    SPI_Cmd( SPI2, DISABLE );
    SPI_I2S_DeInit( SPI2 );
    SPI_I2S_DMACmd( SPI2, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, DISABLE );
    DMA_DeInit( DMA1_Channel5 );
    DMA_DeInit( DMA1_Channel4 );

    /* Clear GPIO_B[1:0] */
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init_structure.GPIO_Mode = GPIO_Mode_AIN;
    gpio_init_structure.GPIO_Pin = WL_GPIO0_PIN;
    GPIO_Init( WL_GPIO0_BANK, &gpio_init_structure );
    gpio_init_structure.GPIO_Pin = WL_GPIO1_PIN;
    GPIO_Init( WL_GPIO1_BANK, &gpio_init_structure );

    /* Clear SPI slave select GPIOs */
    gpio_init_structure.GPIO_Mode = GPIO_Mode_AIN;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init_structure.GPIO_Pin = ( 1 << SPI_BUS_CS_PIN );
    GPIO_Init( SPI_BUS_CS_BANK, &gpio_init_structure );
#if 0
    gpio_init_structure.GPIO_Pin = SPI_CS_EXT_GPIO_PIN;
    GPIO_Init( SPI_CS_EXT_GPIO_BANK, &gpio_init_structure );
#endif

    /* Clear the SPI lines */
    gpio_init_structure.GPIO_Mode = GPIO_Mode_AIN;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init_structure.GPIO_Pin = ( 1 << SPI_BUS_CLOCK_PIN ) | ( 1 << SPI_BUS_MISO_PIN ) | ( 1 << SPI_BUS_MOSI_PIN);
    GPIO_Init( SPI_BUS_CLOCK_BANK, &gpio_init_structure );

    gpio_irq_disable( SPI_IRQ_BANK, SPI_IRQ_PIN );

    /* Disable SPI_SLAVE Periph clock and DMA1 clock */
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2, DISABLE );
    RCC_AHBPeriphClockCmd( RCC_AHBPeriph_DMA1, DISABLE );

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed();
#endif /* STOP_MODE_SUPPORT */

    return WICED_SUCCESS;
}

wiced_result_t host_platform_spi_transfer( bus_transfer_direction_t dir, uint8_t* buffer, uint16_t buffer_length )
{
    uint32_t loop_count;
    uint32_t junk;

#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_needed( );
#endif /* ifdef STOP_MODE_SUPPORT */

    DMA1_Channel5->CNDTR = buffer_length;
    DMA1_Channel5->CMAR = (uint32_t) buffer;
    if ( dir == BUS_READ )
    {
        DMA1_Channel4->CNDTR = buffer_length;
        DMA1_Channel4->CMAR = (uint32_t) buffer;
        DMA1_Channel4->CCR |= DMA_MemoryInc_Enable;
    }
    else
    {
        DMA1_Channel4->CNDTR = buffer_length;
        DMA1_Channel4->CMAR = (uint32_t) &junk;
        DMA1_Channel4->CCR &= ( ~DMA_MemoryInc_Enable );
    }

    GPIO_ResetBits( SPI_BUS_CS_BANK, ( 1 << SPI_BUS_CS_PIN ) ); /* CS low (to select) */
    DMA_Cmd( DMA1_Channel4, ENABLE );
    DMA_Cmd( DMA1_Channel5, ENABLE );

    /* Wait for DMA TX to complete */
    loop_count = 0;
    while ( ( DMA_GetFlagStatus( DMA1_FLAG_TC4 ) == RESET ) &&
            ( loop_count < (uint32_t) DMA_TIMEOUT_LOOPS ) )
    {
        loop_count++;
    }

    if ( loop_count >= (uint32_t) DMA_TIMEOUT_LOOPS )
    {
#ifdef STOP_MODE_SUPPORT
        stm32f1xx_clocks_not_needed( );
#endif /* ifdef STOP_MODE_SUPPORT */
        return WICED_TIMEOUT;
    }

    DMA_Cmd( DMA1_Channel4, DISABLE );
    DMA_Cmd( DMA1_Channel5, DISABLE );

    /* Clear the CS pin and the DMA status flag */
    GPIO_SetBits( SPI_BUS_CS_BANK, ( 1 << SPI_BUS_CS_PIN ) ); /* CS low (to select) */
	DMA_ClearFlag( DMA1_FLAG_TC4 );


#ifdef STOP_MODE_SUPPORT
    stm32f1xx_clocks_not_needed( );
#endif /* ifdef STOP_MODE_SUPPORT */

    return WICED_SUCCESS;
}

